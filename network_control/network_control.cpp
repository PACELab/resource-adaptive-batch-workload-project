#include<iostream>
#include<string>
#include<algorithm>
#include<sstream>
#include<map>
#include<vector>
#include<numeric>
#include<fstream>
#include<thread>

#define RATE 55
#define BURST 32
#define LATENCY 400
#define INTERFACE "eth0"

using namespace std;

string runCommand(const char *s)
{
    const int max_buffer = 1024;
    char buffer[max_buffer];
    std::string data;
    FILE* stream = popen(s,"r");
    if (stream) {
        while (!feof(stream))
            if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
        pclose(stream);
    }
    return data;

}

void update_container(string container_name, string interface, int rate, int burst, int latency)
{
    // Whenever actual value of bandwidth reserved changes, update the container
    // tc qdisc del dev eth0 root
    runCommand(("/usr/bin/docker exec -u root -t -i "+container_name+" tc qdisc del dev "+interface+" root ").c_str());
    // tc qdisc add dev eth0 root tbf rate 50mbit burst 32kbit latency 400ms
    string updateCommand = ("/usr/bin/docker exec -u root -t -i "+container_name+" tc qdisc add dev "+interface+" root tbf rate "+std::to_string(rate)+"mbit burst "+std::to_string(burst)+"kbit latency "+std::to_string(latency)+"ms ").c_str();
    runCommand(("/usr/bin/docker exec -u root -t -i "+container_name+" tc qdisc add dev "+interface+" root tbf rate "+std::to_string(rate)+"mbit burst "+std::to_string(burst)+"kbit latency "+std::to_string(latency)+"ms ").c_str());
    // cout<< "Updated docker network limits! Rate: " << rate <<endl;
    // cout<< "Command: " << updateCommand << "\n" <<endl;
}

struct vm_info
{
    double curBW;
    double mean;
    double xSquare; //E[X^2]
    double stdeviation;
    double sum;
    double sum2;
    vector<double> updata;
    vector<double> downdata;
    double  vm_bwReserved;
    double  original_limit;
    double couldBeContBW;
    int window_size;
    int initWindowSize;
    double origContainerBW;
    double curContBW;
    double contMaxBW;
    double contMinBW;
    string container_name;

    vm_info()
    {
        curBW = sum = sum2 = vm_bwReserved = original_limit = window_size = initWindowSize = 0;
    }

    void to_string()
    {

        printf("mean : %.3f \t", mean);
        printf("std : %.3f \n", stdeviation);
        return ;
        // Helper method to print out all the values
        printf("original_limit : %f \t", original_limit);
        printf("curBW : %f \t", curBW);
        printf("vm_bwReserved : %f \n", vm_bwReserved);
        printf("mean : %.3f \t", mean);
        printf("std : %.3f \t", stdeviation);
        printf("sum : %.3f \t", sum);
        printf("sum2 : %.3f \t", sum2);
        printf("window_size : %d \n", window_size);
        if(updata.size() > 0)
        {
            printf("updata : ");
            for(int i = 0; i < updata.size(); i++) printf("%f ", updata[i]);
            printf("\t");
        }
        else
            printf("updata : <empty>\t");
        if(downdata.size() > 0)
        {
            printf("downdata : ");
            for(int i = 0; i < downdata.size(); i++) printf("%f ",downdata[i]);
            printf("\n");
        }
        else
            printf("downdata : <empty>\n");
    }

    void calcRate(int call_loc){
        double setBW = original_limit - vm_bwReserved;
        // WARNING: the VMs require most of BW, so we are providing the least possible BW to container, for now!
        setBW = (vm_bwReserved > original_limit) ? origContainerBW : setBW; 
        // std::cout << "\t " << call_loc << ". curBW " << curBW << " vm_bwReserved " << (vm_bwReserved) << "\t setBW " << (setBW) << "\n";
        // Send rate in mbits
        update_container(container_name, INTERFACE, setBW*8, BURST, LATENCY);
        curContBW = setBW;    
    }
};

//code ref: https://www.strchr.com/standard_deviation_in_one_pass
pair<double,double> getSumnSumSq(std::vector<double>& a)
{
    if(a.size() == 0)
        return make_pair(0.0,0.0);
    double sum = 0;
    double sq_sum = 0;
    for(int i = 0; i < a.size(); ++i) {
       sum += a[i];
       sq_sum += a[i] * a[i];
    }
    
    return make_pair(sum, sq_sum);
    //double mean = sum / a.size();
    //double variance = (sq_sum / a.size()) - (mean * mean);
    //return make_pair(mean,sqrt(variance));
}

double getTotalcurrentBandwidth()
{
    // Get list of running VMs
    string data = runCommand("/usr/bin/virsh list | /bin/grep running | /usr/bin/awk '{print $2}'");
    int vm_count = count(data.begin(),data.end(),'\n');

    string vm_name;
    double curr_net_sum = 0;

    // Calculate total sum of network bandwidth (ingress + egress) for all fg VMs
    istringstream iss(data);
    for(int i=0;i<vm_count;i++)
    {
        getline(iss,vm_name,'\n');
        // Get interface name for each VM
        string interface = runCommand(("/usr/bin/virsh domiflist "+vm_name+" | awk '{ if(NR==3) print $1 }' ").c_str());
        
        // domifstat command outputs cumulative values of received & transmitted bytes
        double prev_net_usage = (stod(runCommand(("/usr/bin/virsh domifstat "+vm_name+" "+interface.substr(0, interface.size() - 1)+" | grep tx_bytes | awk '{print $3}'").c_str()))/1024)/1024;
        prev_net_usage += (stod(runCommand(("/usr/bin/virsh domifstat "+vm_name+" "+interface.substr(0, interface.size() - 1)+" | grep rx_bytes | awk '{print $3}'").c_str()))/1024)/1024;

        std::this_thread::sleep_for (std::chrono::seconds(1));

        double curr_net_usage = (stod(runCommand(("/usr/bin/virsh domifstat "+vm_name+" "+interface.substr(0, interface.size() - 1)+" | grep tx_bytes | awk '{print $3}'").c_str()))/1024)/1024;
        curr_net_usage += (stod(runCommand(("/usr/bin/virsh domifstat "+vm_name+" "+interface.substr(0, interface.size() - 1)+" | grep rx_bytes | awk '{print $3}'").c_str()))/1024)/1024;
        
        // Assuming rx_bytes & tx_bytes are cumulative values (always increasing values)
        curr_net_sum += curr_net_usage - prev_net_usage; // Units - MegaBytes/sec        
    }

    return curr_net_sum; // returns the sum of received & transmitted bytes by all running VMs in MegaBytes/sec
}

// Fetch values from trace. The format is '<ingress + outgress value>''
bool fetchBandwidthVal(ifstream& infile, long *val)
{
    std::string line;
    if(!std::getline(infile, line))
    {
        // End of file
        return false;
    }
    std::istringstream iss(line);
    iss >> *val;
    return true;
}

int printTrace = 0; double guardUpperLimit =0, guardLowerLimit=0;
double couldBeContBW, temp,temp1,temp2, curBW;
int phaseChangeSamples, uPhaseChangeSamples,phaseChangeState;
double stdDevThreshold, netBWGuardBand, containerChangeRatio, temp_stddev, reduceContBW=0.0f;
int resetWindow = 0, resetIters=0, iterCount=0, numContainers=0, i=0, violationFlag = 0;
int numViolations=0, numChanges=0; 
double accumFgBw = 0.0f, accumBgBw = 0.0f; int numSamples = 0.0; 

int calculateNewWindow(struct vm_info* vm,ifstream& infile,ofstream& simtracefile){
    long val1;
    vm->sum = 0, vm->sum2 = 0, vm->curBW = 0;
    //std::cout<<"\t Enter <calculateNewWindow> vm->sum "<<vm->sum<<" vm->sum2 "<<vm->sum2<<" vm->initWindowSize "<<vm->initWindowSize<<"\n";
    for(int i = 0; i < vm->initWindowSize; i++){
        // double curBW = getTotalcurrentBandwidth();
        if(!fetchBandwidthVal(infile, &val1)){
            return 0;
        }
        // Just to make the trace value usable
        double curBW = ((double)val1)/(1024*1024);
        curBW*=8;
        vm->sum += curBW;
        vm->sum2 += (curBW * curBW);
        vm->curBW = curBW;
        // vm->to_string();
        // printf("\n");
        if(printTrace) simtracefile<< curBW << "," << vm->curContBW<<","<< guardUpperLimit<<","<<guardLowerLimit<< endl;
    }
    //std::cout<<"\t Exit <calculateNewWindow> vm->sum "<<vm->sum<<" vm->sum2 "<<vm->sum2<<" vm->initWindowSize "<<vm->initWindowSize<<"\n";
    return 1;
}

int updateLimits(struct vm_info* vm,int updateChangeFlag){
    //if( ((vm->curContBW/couldBeContBW)>(1-containerChangeRatio)) && ((vm->curContBW/couldBeContBW)<(1+containerChangeRatio)) ) continue;
        
        // cout << "Current Container limit: " << vm->curContBW << " Could be container limit: " << couldBeContBW << endl;
    if( vm->curContBW < couldBeContBW ) {      
        couldBeContBW =  vm->original_limit - ((1+netBWGuardBand) * vm->vm_bwReserved); 
        if(couldBeContBW < vm->contMinBW) couldBeContBW = vm->contMinBW; // dont drop below the minimum limit
        if(couldBeContBW > vm->contMaxBW) couldBeContBW = vm->contMaxBW; // max limit of bw for container

        vm->curContBW = couldBeContBW;  
        // cout <<  "Increasing container BW limit to: " << couldBeContBW << endl;
        vm->downdata.clear();vm->updata.clear();
    }else if(vm->curContBW > couldBeContBW){
            //couldBeContBW = vm->origContainerBW;
        double reduceBWto = reduceContBW*couldBeContBW;
        couldBeContBW = (reduceBWto < vm->contMinBW) ? vm->contMinBW : reduceBWto;

        vm->curContBW = couldBeContBW;  
        vm->downdata.clear();vm->updata.clear();
    } 
    if(updateChangeFlag)
        numChanges+=1;
}

enum {
    phase_change_nil = 0,
    phase_change_up,
    phase_change_down
} possible_phaseChangeVals;

int main(int argc, char** argv){

    struct vm_info* vm = new vm_info;    
    // TODO: Fetch original network bandwidth of VM - Hardcoding it to 944 Mbits/sec
    vm->original_limit = 945; // Units = Mbytes/sec (944/8) = 125
    vm->contMaxBW = 800;
    vm->contMinBW = 1*8; // 8Mbps 

    if(argc <= 1){
        // Sample: ./a.out 1 10 5 0.25 1.5 0.01 netstat > res_values.txt
        cout<< "./network_control <numContainers> <container_reserved_bandwidth_in_Mbytes_per_sec> <phaseChangeSamples> <NET-BW-GUARD-BAND> <THRSLD_STEP_SIZE> <percentage-reduce-container-BW> <container1_name> <container2_name>" <<endl;
        exit(0);
    }

    numContainers = 1; //stoi(argv[1]);
    // Minimum a container can go to
    vm->origContainerBW = 10*8;// stod(argv[2]);
    // Number of samples required to re-calculate bands
    phaseChangeSamples = stoi(argv[1]);
    uPhaseChangeSamples = (phaseChangeSamples>1) ? phaseChangeSamples : phaseChangeSamples;
    // Get the initial window size

    vm->initWindowSize = 3*phaseChangeSamples;
    vm->window_size = phaseChangeSamples;
    // set container bw guard percentage 
    netBWGuardBand = 0.25;//
    stdDevThreshold = stof(argv[2]);
    reduceContBW = stof(argv[3]);
    printTrace = stoi(argv[4]);
    char inFilename[128];
    sprintf(inFilename,"%s",argv[5]);

    // Initialise output file
    ofstream simtracefile,summaryfile;
    char out_filename[128],out_filename1[128];
    sprintf(out_filename,"simtrace/%s_phCh%d_stdDev%.2f.txt",inFilename,phaseChangeSamples,stdDevThreshold);
    sprintf(out_filename1,"summary/%s_phCh%d_stdDev%.2f.txt",inFilename,phaseChangeSamples,stdDevThreshold);
    sprintf(inFilename,"processed_traces/%s.txt",argv[5]);
    printf("\t inFilename: %s phaseChangeSamples: %d stdDevThreshold: %.2f printTrace: %d out_filename: %s out_filename1: %s \n",inFilename,phaseChangeSamples,stdDevThreshold,printTrace,out_filename,out_filename1);
    simtracefile.open(out_filename,ios::out); 
    summaryfile.open(out_filename1,ios::out); 

    //simtracefile << "Running tests for phase change of " << phaseChangeSamples << " and std dev of " << stdDevThreshold << endl;
    // printf("\t  numContainers: %d phaseChangeSamples: %d netBWGuardBand: %.3f stdDevThreshold: %.2f reduceContBW: %.2f \n", 
    //     numContainers, phaseChangeSamples, netBWGuardBand, stdDevThreshold, reduceContBW);
    
    string container_name[numContainers];
    //for(i=0; i < numContainers; i++){
      //container_name[i] = argv[7+i];  
      // printf("\t Cont-#: %d name: %s \n", i, container_name[i].c_str());
    //} 
    // Not used for nw
    containerChangeRatio = 0.2;

    phaseChangeState = phase_change_nil;
    // vm->curContBW = 0; //vm->origContainerBW;
    // vm->container_name = argv[1];
    //for(i=0; i<numContainers; i++) update_container(container_name[i], INTERFACE, vm->origContainerBW*8, BURST, LATENCY);       
    vm->curContBW = vm->origContainerBW;

    // Subtract container bandwidth from the total bandwidth
    vm->vm_bwReserved = vm->original_limit;

    // Gathering initial data for the defined running window
    // TODO: Take this file as input
    std::ifstream infile(inFilename);
    long val1;
    calculateNewWindow(vm,infile,simtracefile);

    vm->window_size = vm->initWindowSize;
    vm->mean = (vm->sum/vm->window_size);
    temp_stddev = ((vm->sum2/vm->window_size) - (vm->mean * vm->mean));
    vm->stdeviation = (temp_stddev > 0) ? sqrt(temp_stddev) : vm->stdeviation;  

    //if(printTrace) summaryfile << "X *** vm->sum "<<vm->sum<<" vm->sum2 "<<(vm->sum2)<<" vmCurBW "<< curBW <<" vm->mean "<<vm->mean<<" stdDevThreshold "<<stdDevThreshold<<" vm->stdeviation "<<vm->stdeviation<<","<< guardUpperLimit<<","<<guardLowerLimit<<"****"<< endl;
    //vm->vm_bwReserved = *std::max_element(vm->updata.begin(), vm->updata.end()) + (stdDevThreshold * vm->stdeviation);

    while(1){
        //printf("\t vm->mean: %.2f vm->xSquare: %.2f vm->stdeviation: %.6f vm->xSquare - (vm->mean * vm->mean): %.6f \n",vm->mean,vm->xSquare,vm->stdeviation,(vm->xSquare - (vm->mean * vm->mean)));
        // Get current values
        iterCount++;
        // curBW = getTotalcurrentBandwidth();
        if(!fetchBandwidthVal(infile, &val1)){
            break;
        }
        // Just to make the trace value usable
        double curBW = ((double)val1)/(1024*1024);
        curBW*=8;
        vm->curBW = curBW;
        vm->sum += curBW; 
        vm->sum2 += (curBW * curBW); 
        vm->window_size++;

        // BEGIN should-move-after-resetIters-loop
        // Calculate mean and standard deviation for the window period
        vm->mean = (vm->sum/vm->window_size);
        temp_stddev = ((vm->sum2/vm->window_size) - (vm->mean * vm->mean));

        numSamples+=1;
        accumFgBw+=curBW;
        accumBgBw+=(vm->curContBW);

        if(printTrace==2) summaryfile << "0 *** numSamples "<<numSamples<<" vm->sum "<<vm->sum <<" vm->sum2 "<<vm->sum2<<" vmCurBW "<< curBW 
            <<" vm->mean "<<vm->mean<<" stdDevThreshold "<<stdDevThreshold<<" vm->stdeviation "<<vm->stdeviation
            <<","<< guardUpperLimit<<","<<guardLowerLimit<<"****"<< endl;
        // END should-move-after-resetIters-loop
        resetIters++;
        if((resetWindow) && (resetIters < phaseChangeSamples)){
            //printf("**iterCount: %d resetIters: %d vmCurBW: %f vm_bwReserved: %.2f upper: %f lower: %f couldBeContBW: %.3f mean: %.2f std_dev: %.2f \n",iterCount,resetIters,vm->curBW,vm->vm_bwReserved,guardUpperLimit,guardLowerLimit,couldBeContBW,vm->mean,vm->stdeviation);
            continue;
        }

        if(temp_stddev < 0){
            vm->sum = curBW;
            vm->sum2 = (curBW * curBW);
            resetWindow = 1;
            resetIters = 1;
            continue;
        }else{
            resetIters = 0;
            resetWindow = 0;
        }

        vm->stdeviation = (temp_stddev > 0) ? sqrt(temp_stddev) : vm->stdeviation; // vm->window_size);

        // Self computed boundaries for bandwidth usage: Peak usage
        temp1 = vm->mean + (stdDevThreshold * vm->stdeviation);
        guardUpperLimit = temp1 > vm->original_limit ? vm->original_limit : temp1;
        temp2 = (vm->mean - (stdDevThreshold * vm->stdeviation));
        guardLowerLimit = temp2 < 0 ? vm->contMinBW : temp2;
        if(printTrace==2) summaryfile << "1 *** numSamples "<<numSamples<<"temp1 "<<temp1<<" temp2 "<<temp2<<" vmCurBW "<< curBW 
            <<" vm->mean "<<vm->mean<<" stdDevThreshold "<<stdDevThreshold<<" vm->stdeviation "<<vm->stdeviation
            <<","<< guardUpperLimit<<","<<guardLowerLimit<<"****"<< endl;
        vm->vm_bwReserved = vm->mean + (stdDevThreshold * vm->stdeviation);

        // Phase change stuff        
        // To remove outliers, we wait for 3 such consecutive changes to peak values
        if(curBW > guardUpperLimit){
            numViolations += 1;
            // cout << "Current bandwidth: " << curBW << " Upper limit: " << guardUpperLimit << ". curBW of VM greater than upper limit, adding to updata" << endl;
            phaseChangeState = phase_change_up;
            vm->downdata.clear();
            vm->updata.push_back(curBW);
        }
        else if(curBW < guardLowerLimit){
            // cout << "Current bandwidth: " << curBW << " Lower limit: " << guardLowerLimit << ". curBW of VM lesser than lower limit, adding to downdata" << endl;
            phaseChangeState = phase_change_down;
            vm->updata.clear();
            vm->downdata.push_back(curBW);
        }
        else{
            phaseChangeState = phase_change_nil;
            vm->downdata.clear();
            vm->updata.clear();
        }

        // cout << vm->mean << " " << vm->mean + vm->stdeviation << " " << vm->mean - vm->stdeviation << " "<< curBW << " " << guardUpperLimit << " " << guardLowerLimit << endl;
        
        //printf("iterCount: %d vmCurBW: %f vm_bwReserved: %.2f upper: %f lower: %f couldBeContBW: %.3f mean: %.2f std_dev: %.2f \n",iterCount,vm->curBW,vm->vm_bwReserved,guardUpperLimit,guardLowerLimit,couldBeContBW,vm->mean,vm->stdeviation);
        //printf("pcstate: %d curContBW: %.3f  updata.size(): %ld downdata.size(): %ld \n",phaseChangeState,vm->curContBW,vm->updata.size(),vm->downdata.size());
        // time,fg_BW,fg_reserved,fg_upperLimit,fg_lowerLimit,fg_mean,fg_stddev,couldBe_bg_BW,cur_bg_limit,num_up_samples,num_down_samples
        // printf("netcontrol_info,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%lf,%lf\n",
        //     iterCount,vm->curBW,vm->vm_bwReserved,guardUpperLimit,
        //     guardLowerLimit,vm->mean,vm->stdeviation,couldBeContBW,
        //     vm->curContBW,vm->updata.size(),vm->downdata.size());        
        if(printTrace) simtracefile << ""<< curBW << "," << vm->curContBW<<","<< guardUpperLimit<<","<<guardLowerLimit<< endl;
        // On a phase change, only the anomalies are considered for mean & std deviation
        if(vm->updata.size() >= uPhaseChangeSamples){
            pair<double,double> p = getSumnSumSq(vm->updata);
            //vm->mean = p.first; vm->stdeviation = p.second;            
            /*
            vm->sum = p.first;
            vm->sum2 = p.second; 
            vm->window_size = phaseChangeSamples;
            vm->mean = vm->sum/vm->window_size;
            temp_stddev = ((vm->sum2/vm->window_size) - (vm->mean * vm->mean));
            vm->stdeviation = (temp_stddev > 0) ? sqrt(temp_stddev) : vm->stdeviation;  
            // TODO: Handle case where stdev ~ 0
            vm->vm_bwReserved = *std::max_element(vm->updata.begin(), vm->updata.end()) + (stdDevThreshold * vm->stdeviation);
            */
            violationFlag = 1;
            couldBeContBW = vm->contMinBW;
            if(printTrace==2) summaryfile << "*** vm->updata.size() "<< vm->updata.size()<<" curBW "<< curBW<< " curContBW "<< vm->curContBW<<" UL "<< guardUpperLimit<<" LL "<<guardLowerLimit<<"****"<< endl;
        }
        else if(vm->downdata.size() >= phaseChangeSamples){
            pair<double,double> p = getSumnSumSq(vm->downdata);
            //vm->mean = p.first; vm->stdeviation = p.second;            
            vm->sum = p.first;
            vm->sum2 = p.second; 
            vm->window_size = phaseChangeSamples;
            temp_stddev = ((vm->sum2/vm->window_size) - (vm->mean * vm->mean));
            vm->stdeviation = (temp_stddev > 0) ? sqrt(temp_stddev) : vm->stdeviation;/// vm->window_size);
            // TODO: Handle case where stdev ~ 0
            vm->vm_bwReserved = *std::max_element(vm->downdata.begin(), vm->downdata.end()) + (stdDevThreshold * vm->stdeviation);

            couldBeContBW =  vm->original_limit - ((1+netBWGuardBand) * vm->vm_bwReserved); 
            violationFlag = 0;
        }
        else{
            continue;
        }

        //std::this_thread::sleep_for (std::chrono::seconds(1));
        updateLimits(vm,0);

        if(violationFlag==1){

            if(calculateNewWindow(vm,infile,simtracefile)==0){
                //simtracefile << "Violations: " << numViolations << " Phase Changes: " << numChanges << endl;

                break;
                //return 0;

            }
            vm->window_size = vm->initWindowSize;
            vm->mean = (vm->sum/vm->window_size);
            temp_stddev = ((vm->sum2/vm->window_size) - (vm->mean * vm->mean));
            vm->stdeviation = (temp_stddev > 0) ? sqrt(temp_stddev) : vm->stdeviation;  
            // TODO: Handle case where stdev ~ 0
            vm->vm_bwReserved = *std::max_element(vm->updata.begin(), vm->updata.end()) + (stdDevThreshold * vm->stdeviation);

            accumFgBw+=(vm->mean* vm->initWindowSize);
            accumFgBw+=(vm->curContBW * vm->initWindowSize);
            numSamples+=(vm->initWindowSize);
            violationFlag = 0;
            if(printTrace==2) summaryfile << "*** vm->mean "<< vm->mean<<" vm->stdeviation "<<vm->stdeviation<<" vm->window_size "<< vm->window_size<< " vm->sum "<<vm->sum<<" vm->sum2 "<<vm->sum2<<"****"<< endl;
            updateLimits(vm,1);
        }
        //if(numSamples%2000==0) std::cout<<"\t numSamples "<<numSamples<<" fgBW "<<(accumFgBw/numSamples)<<" bgBW "<<(accumBgBw/numSamples)<<"\n";
    }

    std::cout<<"\t numSamples: "<<numSamples<<"\n";
    if(numSamples!=0) 
        std::cout << "Violations:\t" << numViolations << "\nPhaseChanges:\t" << numChanges<<"\nFGBW:\t"<<(accumFgBw/numSamples)<<"\nBGBW:\t"<<(accumBgBw/numSamples)<<"\nnumSamples:\t"<<(numSamples)<< endl;
    else
        std::cout << "Violations:\t" << numViolations << "\nPhaseChanges:\t" << numChanges<<"\nnumSamples:\t"<<(numSamples)<< endl;

    if(numSamples!=0) 
        summaryfile << "Violations:\t" << numViolations << "\nPhaseChanges:\t" << numChanges<<"\nFGBW:\t"<<(accumFgBw/numSamples)<<"\nBGBW:\t"<<(accumBgBw/numSamples)<<"\nnumSamples:\t"<<(numSamples)<< endl;
    else
        summaryfile << "Violations:\t" << numViolations << "\nPhaseChanges:\t" << numChanges<<"\nnumSamples:\t"<<(numSamples)<< endl;


    
    simtracefile.close();
    summaryfile.close();
    return 0;

}
