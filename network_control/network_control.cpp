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
    runCommand(("/usr/bin/docker exec "+container_name+" tc qdisc del dev "+interface+" root ").c_str());
    // tc qdisc add dev eth0 root tbf rate 50mbit burst 32kbit latency 400ms
    string updateCommand = ("/usr/bin/docker exec "+container_name+" tc qdisc add dev "+interface+" root tbf rate "+std::to_string(rate)+"mbit burst "+std::to_string(burst)+"kbit latency "+std::to_string(latency)+"ms ").c_str();
    runCommand(("/usr/bin/docker exec "+container_name+" tc qdisc add dev "+interface+" root tbf rate "+std::to_string(rate)+"mbit burst "+std::to_string(burst)+"kbit latency "+std::to_string(latency)+"ms ").c_str());
    cout<<"Updated docker network limits! rate "<<rate<<endl;
    cout<<"Command: "<<updateCommand<<"\n"<<endl;
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
    double origContainerBW;
    double curContBW;
    double contMaxBW;
    string container_name;

    vm_info()
    {
        curBW = sum = sum2 = vm_bwReserved = original_limit = window_size = 0;
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
        if(downdata.size()>0)
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
        if(setBW<0) setBW = origContainerBW;
        setBW = (vm_bwReserved > original_limit) ? origContainerBW : setBW; // WARNING: the VMs require most of BW, so we are providing the least possible BW to container, fow now!
        std::cout<<"\t "<<call_loc<<". curBW "<<curBW<<" vm_bwReserved "<<(vm_bwReserved)<<"\t setBW "<<(setBW)<<"\n";
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
    
    return make_pair(sum,sq_sum);
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
        
        // TODO: precision of this?
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

enum {
    phase_change_nil = 0,
    phase_change_up,
    phase_change_down
} possible_phaseCangeVals;

int main(int argc, char** argv)
{
    struct vm_info* vm = new vm_info;
    double couldBeContBW,temp,guardUpperLimit,guardLowerLimit,curBW;
    int phaseChangeSamples,phaseChangeState;
    double stdDevThreshold,netBWGuardBand,containerChangeRatio,temp_stddev,reduceContBW=0.0f,vmLowerBW=5;
    int resetWindow = 0, resetIters=0,iterCount=0,numContainers=0,i=0;
    // TODO: Fetch original network bandwidth of VM - Hardcoding it to 944 Mbits/sec
    vm->original_limit = 125; // Units = Mbytes/sec (944/8)
    vm->contMaxBW = 100;
    vmLowerBW = 5;
    if(argc<=1){
        cout<< "./network_control <container_name> <container_reserved_bandwidth_in_Mbytes_per_sec> <phaseChangeSamples> <NET-BW-GUARD-BAND> <THRSLD_STEP_SIZE>" <<endl;
        exit(0);
    }

    // Get the initial window size
    numContainers = stoi(argv[1]);
    vm->origContainerBW =  stod(argv[2]);
    phaseChangeSamples = stoi(argv[3]);
    vm->window_size = phaseChangeSamples;
    netBWGuardBand = stof(argv[4]);
    stdDevThreshold = stof(argv[5]);
    reduceContBW = stof(argv[6]);

    printf("\t  numContainers: %d phaseChangeSamples: %d netBWGuardBand: %.3f stdDevThreshold: %.2f reduceContBW: %.2f \n",numContainers,phaseChangeSamples,netBWGuardBand,stdDevThreshold,reduceContBW);
    string container_name[numContainers];
    for(i=0;i<numContainers;i++){
      container_name[i] = argv[7+i];  
      printf("\t Cont-#: %d name: %s \n",i,container_name[i].c_str());
    } 
    printf("\t cont[0]: %s \n",container_name[0].c_str());
    
    containerChangeRatio = 0.2;
    phaseChangeState = phase_change_nil;
    
    vm->curContBW = 0;//vm->origContainerBW;
    vm->container_name = argv[1];
    for(i=0;i<numContainers;i++) update_container(container_name[i], INTERFACE, vm->origContainerBW*8, BURST, LATENCY);       
    vm->curContBW = vm->origContainerBW;

    // Subtract container bandwidth from the total bandwidth
    vm->vm_bwReserved = vm->original_limit;

    // Gathering initial data for the defined running window
    for(int i = 0; i < vm->window_size; i++){
        double curBW = getTotalcurrentBandwidth();
        vm->sum += curBW;
        vm->sum2 += (curBW * curBW);
        vm->curBW = curBW;
        vm->to_string();
        printf("\n");
        std::this_thread::sleep_for (std::chrono::seconds(1));
    }

    while(1){

        //printf("\t vm->mean: %.2f vm->xSquare: %.2f vm->stdeviation: %.6f vm->xSquare - (vm->mean * vm->mean): %.6f \n",vm->mean,vm->xSquare,vm->stdeviation,(vm->xSquare - (vm->mean * vm->mean)));
        // Get current values
        iterCount++;
        curBW = getTotalcurrentBandwidth();
        vm->curBW = curBW;
        vm->sum += curBW; 
        vm->sum2 += (curBW * curBW); 
        vm->window_size++;

        resetIters++;
        if((resetWindow) && (resetIters<phaseChangeSamples)){
            //printf("**iterCount: %d resetIters: %d vmCurBW: %f vm_bwReserved: %.2f upper: %f lower: %f couldBeContBW: %.3f mean: %.2f std_dev: %.2f \n",iterCount,resetIters,vm->curBW,vm->vm_bwReserved,guardUpperLimit,guardLowerLimit,couldBeContBW,vm->mean,vm->stdeviation);
            continue;
        }
        // Calculate mean and standard deviation for the window period
        vm->mean = (vm->sum/vm->window_size);
        temp_stddev = ((vm->sum2/vm->window_size) - (vm->mean * vm->mean));

        if(temp_stddev<0){
            vm->sum = curBW;
            vm->sum2 = (curBW*curBW);
            resetWindow = 1;
            resetIters = 1;
            continue;
        }else{
            resetIters = 0;
            resetWindow = 0;
        }

        vm->stdeviation = (temp_stddev > 0) ? sqrt(temp_stddev) : vm->stdeviation;/// vm->window_size);

        // Self computed boundaries for bandwidth usage: Peak usage
        temp = vm->mean + (stdDevThreshold * vm->stdeviation);
        guardUpperLimit = temp > vm->original_limit ? vm->original_limit : temp;
        temp = (vm->mean - (stdDevThreshold * vm->stdeviation));
        guardLowerLimit = temp < 0 ? vmLowerBW : temp;

        //vm->vm_bwReserved = vm->mean + (stdDevThreshold * vm->stdeviation);

// Phase change stuff        
        // To remove outliers, we wait for 3 such consecutive changes to peak values
        if(curBW > guardUpperLimit){
            phaseChangeState = phase_change_up;
            vm->downdata.clear();
            vm->updata.push_back(curBW);
        }
        else if(curBW < guardLowerLimit){
            phaseChangeState = phase_change_down;
            vm->updata.clear();
            vm->downdata.push_back(curBW);
        }
        else{
            phaseChangeState = phase_change_nil;
            vm->downdata.clear();
            vm->updata.clear();
        }

        //printf("iterCount: %d vmCurBW: %f vm_bwReserved: %.2f upper: %f lower: %f couldBeContBW: %.3f mean: %.2f std_dev: %.2f \n",iterCount,vm->curBW,vm->vm_bwReserved,guardUpperLimit,guardLowerLimit,couldBeContBW,vm->mean,vm->stdeviation);
        //printf("pcstate: %d curContBW: %.3f  updata.size(): %ld downdata.size(): %ld \n",phaseChangeState,vm->curContBW,vm->updata.size(),vm->downdata.size());

        // On a phase change, only the anomalies are considered for mean & std deviation
        if(vm->updata.size() >= phaseChangeSamples){
            pair<double,double> p = getSumnSumSq(vm->updata);
            //vm->mean = p.first; vm->stdeviation = p.second;            
            vm->sum = p.first;vm->sum2 = p.second; vm->window_size = phaseChangeSamples;
            vm->mean = vm->sum/vm->window_size;
            temp_stddev = ((vm->sum2/vm->window_size) - (vm->mean * vm->mean));
            vm->stdeviation = (temp_stddev > 0) ? sqrt(temp_stddev) : vm->stdeviation;/// vm->window_size);
            vm->vm_bwReserved = *std::max_element(vm->updata.begin(), vm->updata.end()) + (stdDevThreshold * vm->stdeviation);// TODO: Handle case where stdev ~ 0
        }
        else if(vm->downdata.size() >= phaseChangeSamples){
            pair<double,double> p = getSumnSumSq(vm->downdata);
            //vm->mean = p.first; vm->stdeviation = p.second;            
            vm->sum = p.first;vm->sum2 = p.second; vm->window_size = phaseChangeSamples;
            temp_stddev = ((vm->sum2/vm->window_size) - (vm->mean * vm->mean));
            vm->stdeviation = (temp_stddev > 0) ? sqrt(temp_stddev) : vm->stdeviation;/// vm->window_size);
            // TODO: Handle case where stdev ~ 0
            vm->vm_bwReserved = *std::max_element(vm->downdata.begin(), vm->downdata.end()) + (stdDevThreshold * vm->stdeviation);
        }else{
    continue;
    }

        //std::this_thread::sleep_for (std::chrono::seconds(1));

// Phase change ends..
        couldBeContBW =  vm->original_limit - ((1+netBWGuardBand) * vm->vm_bwReserved); 

        if(couldBeContBW<0) couldBeContBW = vm->origContainerBW;
        if(couldBeContBW<vm->origContainerBW) couldBeContBW = vm->origContainerBW; // dont drop below the minimum limit
        if(couldBeContBW>vm->contMaxBW) couldBeContBW = vm->contMaxBW;
        //if( ((vm->curContBW/couldBeContBW)>(1-containerChangeRatio)) && ((vm->curContBW/couldBeContBW)<(1+containerChangeRatio)) ) continue;

        if( vm->curContBW<couldBeContBW ) {
            //vm->calcRate(2);     
            printf("\t 1. couldBeContBW: %.3f vm->curContBW: %.3f ratio: %.3f \n",couldBeContBW,vm->curContBW,(couldBeContBW/vm->curContBW));
            //update_container(container_name[0], INTERFACE, couldBeContBW*8, BURST, LATENCY);    
            for(i=0;i<numContainers;i++) update_container(container_name[i], INTERFACE, couldBeContBW*8/numContainers, BURST, LATENCY);       
            vm->curContBW = couldBeContBW;   
        }else if(vm->curContBW>couldBeContBW){
            //couldBeContBW = vm->origContainerBW;
            printf("\t 2. couldBeContBW: %.3f vm->curContBW: %.3f ratio: %.3f \n",couldBeContBW,vm->curContBW,(couldBeContBW/vm->curContBW));
            double reduceBWto = reduceContBW*couldBeContBW;
            couldBeContBW = (reduceBWto<vm->origContainerBW) ? vm->origContainerBW : reduceBWto;
            //update_container(container_name[0], INTERFACE, couldBeContBW*8, BURST, LATENCY);       
            for(i=0;i<numContainers;i++) update_container(container_name[i], INTERFACE, couldBeContBW*8/numContainers, BURST, LATENCY);       
            vm->curContBW = couldBeContBW;  
            //vm->calcRate(3);     
        } 
    }

    return 0;

}
