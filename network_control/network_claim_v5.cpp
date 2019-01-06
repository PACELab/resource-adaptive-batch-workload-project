#include<iostream>
#include<string>
#include<algorithm>
#include<sstream>
#include<map>
#include<vector>
#include<numeric>
#include<fstream>
#include<deque>
#include<thread>

// avs
#define RATE 55
#define BURST 32
#define LATENCY 400
#define INTERFACE "eth0"
// avs

using namespace std;
// Copying Jan-2nd's version of mm claim algorithm v5.
struct container{
    double bgReserved;
    double bgunused;

    container()
    {
        bgReserved = 0;
        bgunused = 0;
    }
};


//typedef unsigned long int uint64_t;

struct vm_info{

    double currentBW;
    double mean;
    double stdeviation;
    double sum;
    double fgReserved;
    double sum2; 
    vector<double> downdata;
    deque<double> windowData;
    double  original_limit;
    int window_size;

    vm_info(){
        currentBW=sum=sum2=original_limit=window_size=fgReserved=0;

    }
};

string runCommand(const char *s){
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

//code ref: https://www.strchr.com/standard_deviation_in_one_pass
pair< pair<double,double>,pair<double,double> > findMeanAndSTD(std::vector<double>& a,std::deque<double>& b){
    if(a.size() == 0)
        return make_pair(make_pair(0,0),make_pair(0.0,0.0));
    double sum = 0;
    double sq_sum = 0;
    for(int i = 0; i < a.size(); ++i) {
       b.push_back(a[i]);
       sum += a[i];
       sq_sum += a[i] * a[i];
    }
    double mean = sum / a.size();
    double variance = (sq_sum / a.size()) - (mean * mean);
    return make_pair(make_pair(sum,sq_sum),make_pair(mean,sqrt(variance)));
}

void updateBgBW(string container_name, string interface, int rate, int burst, int latency){
    // Whenever actual value of bandwidth reserved changes, update the container
    // tc qdisc del dev eth0 root
    runCommand(("/usr/bin/docker exec -u root -t -i "+container_name+" tc qdisc del dev "+interface+" root ").c_str());
    // tc qdisc add dev eth0 root tbf rate 50mbit burst 32kbit latency 400ms
    string updateCommand = ("/usr/bin/docker exec -u root -t -i "+container_name+" tc qdisc add dev "+interface+" root tbf rate "+std::to_string(rate)+"mbit burst "+std::to_string(burst)+"kbit latency "+std::to_string(latency)+"ms ").c_str();
    runCommand(("/usr/bin/docker exec -u root -t -i "+container_name+" tc qdisc add dev "+interface+" root tbf rate "+std::to_string(rate)+"mbit burst "+std::to_string(burst)+"kbit latency "+std::to_string(latency)+"ms ").c_str());
    // cout<< "Updated docker network limits! Rate: " << rate <<endl;
    // cout<< "Command: " << updateCommand << "\n" <<endl;
}

double getFgBW(){
    // Get list of running VMs
    string data = runCommand("/usr/bin/virsh list | /bin/grep running | /usr/bin/awk '{print $2}'");
    int vm_count = count(data.begin(),data.end(),'\n');

    string vm_name;
    double curr_net_sum = 0;

    // Calculate total sum of network bandwidth (ingress + egress) for all fg VMs
    istringstream iss(data);
    double prev_net_usage=0,curr_net_usage=0;

    for(int i=0;i<vm_count;i++){
        getline(iss,vm_name,'\n');
        // Get interface name for each VM
        string interface = runCommand(("/usr/bin/virsh domiflist "+vm_name+" | awk '{ if(NR==3) print $1 }' ").c_str());
        
        // domifstat command outputs cumulative values of received & transmitted bytes
        prev_net_usage = (stod(runCommand(("/usr/bin/virsh domifstat "+vm_name+" "+interface.substr(0, interface.size() - 1)+" | grep tx_bytes | awk '{print $3}'").c_str()))/1024)/1024;
        //prev_net_usage += (stod(runCommand(("/usr/bin/virsh domifstat "+vm_name+" "+interface.substr(0, interface.size() - 1)+" | grep rx_bytes | awk '{print $3}'").c_str()))/1024)/1024;

    }
    std::this_thread::sleep_for (std::chrono::seconds(1));
    for(int i=0;i<vm_count;i++){
        string interface = runCommand(("/usr/bin/virsh domiflist "+vm_name+" | awk '{ if(NR==3) print $1 }' ").c_str());
        curr_net_usage = (stod(runCommand(("/usr/bin/virsh domifstat "+vm_name+" "+interface.substr(0, interface.size() - 1)+" | grep tx_bytes | awk '{print $3}'").c_str()))/1024)/1024;
        //curr_net_usage+= (stod(runCommand(("/usr/bin/virsh domifstat "+vm_name+" "+interface.substr(0, interface.size() - 1)+" | grep rx_bytes | awk '{print $3}'").c_str()))/1024)/1024;
        // Assuming rx_bytes & tx_bytes are cumulative values (always increasing values)
    }
    curr_net_sum = (curr_net_usage - prev_net_usage)*8; // Units - MegaBits/sec  
    return curr_net_sum; // returns the sum of received & transmitted bits by all running VMs in MegaBits/sec
}

void initWindow(struct vm_info *vm,struct container *con ){
    //gathering initial data for the defined running window
    vm->sum = 0; vm->sum2 = 0;vm->windowData.clear();
    for(int i=0;i<vm->window_size;i++){
        double currentBW = getFgBW();
        vm->sum+=currentBW;
        vm->sum2+=currentBW*currentBW;
        vm->currentBW = currentBW;
        vm->windowData.push_back(currentBW);
        //vm->to_string();
        cout<<vm->currentBW<<","<<0<<","<<0<<","<<vm->fgReserved<<","<<con->bgReserved<<","<<con->bgunused<<","<<0<<","<<0<<endl;
        std::this_thread::sleep_for (std::chrono::seconds(1));
    }
}

int main(int argc,char** argv){
    struct vm_info *vm = new vm_info;
    struct container *con = new container;

    if(argc<=1){
        cout<<"./network_claim_v5 <container_min_bw_in_mbps> <init_window_size> <PHASE_CHANGE_SIZE> <GUARD_STEP_SIZE> <RECLAM_PCT> <CONTAINER RECLAIM SIZE> <FG_RESERVED_CLOSENESS_PCT>"<<endl;
        exit(0);
    }

    double reclaim_pct = 0.05;//stod(argv[5]);
    double fg_reserved_pct = 0.95;//stod(argv[7]);
    double guardBand = 0.25;
    //get the initial window size
    vm->window_size = stoi(argv[2]);

    double container_reclaim_size = 10;//stod(argv[6]);

    //cout<<"Num of Sockets :"<num_of_sockets;

    int PHASE_CHANGE_SIZE = stoi(argv[3]);
    double GUARD_STEP_SIZE = stof(argv[4]);
    //todo-1
    vm->original_limit = 1000;// Mbps;//stod(runCommand("cat /proc/meminfo | grep MemTotal | awk '{print $2}'"))/1048576;
    double minPeakBelow = 40.0;
    //subtract container mmemory from the total allocated memory
    int contOrigLimit = stod(argv[1]);
    
    vm->fgReserved = getFgBW();
    con->bgReserved = (vm->original_limit - vm->fgReserved);
    //todo-2
    con->bgunused = fmod(con->bgReserved,container_reclaim_size);
    con->bgReserved = con->bgReserved - con->bgunused;

    int updateContBW = 0;
    contOrigLimit = (contOrigLimit<100) ? contOrigLimit : 100;
    updateBgBW("bay12-cont1","br0",contOrigLimit,BURST,LATENCY);
    initWindow(vm,con);

    double minguardBW = reclaim_pct*vm->original_limit;

    double violations,phasechanges,quotient;
    violations=phasechanges=0;

    vm->mean = vm->sum/(double)vm->windowData.size();
    vm->stdeviation = sqrt((vm->sum2 / (double)vm->windowData.size()) - (vm->mean * vm->mean));

    double guardBW = (minguardBW > GUARD_STEP_SIZE*vm->stdeviation)?minguardBW:(GUARD_STEP_SIZE)*vm->stdeviation;
    //double predictedPeakabove = vm->mean+guardBW;
    double predictedPeakbelow = vm->mean-guardBW;

    cout<<"\t vm->mean "<<vm->mean<<" guardBW "<<guardBW<<" minguardBW "<<minguardBW<<" vm->stdeviation "<<(vm->stdeviation)<<"\n";
    vm->fgReserved = vm->mean+guardBW;
    con->bgReserved = (vm->original_limit - (1+guardBand)*vm->fgReserved);
    con->bgunused = fmod(con->bgReserved,container_reclaim_size);
    con->bgReserved = con->bgReserved - con->bgunused;

    cout<<"\t fgReserved "<<vm->fgReserved<<"\n";
    cout<<"\t con->bgunused "<<con->bgunused<<"\n";
    cout<<"\t con->bgReserved "<<con->bgReserved<<"\n";
    
    if(con->bgReserved!=contOrigLimit){
        double toSetLimit = ((con->bgReserved<contOrigLimit) || (con->bgReserved<0) )? contOrigLimit : con->bgReserved;
        toSetLimit = (toSetLimit<800) ? toSetLimit : 800; 
        cout<<"\t toSetLimit "<<toSetLimit<<" contOrigLimit "<<contOrigLimit<<" con->bgReserved "<<con->bgReserved<<"\n";
            //updateBgBW(string container_name, string interface, int rate, int burst, int latency);
        updateBgBW("bay12-cont1","br0",toSetLimit,BURST,LATENCY);
        con->bgReserved = toSetLimit;
    }

    //int timeAfterViolation=-1;     
    while(1){

        double currentBW = getFgBW();
        vm->currentBW = currentBW;

        vm->mean = vm->sum/(double)vm->windowData.size();
        vm->stdeviation = sqrt((vm->sum2 / (double)vm->windowData.size()) - (vm->mean * vm->mean));
        guardBW = (minguardBW > GUARD_STEP_SIZE*vm->stdeviation)?minguardBW:GUARD_STEP_SIZE*vm->stdeviation;

        vm->sum += currentBW;
        vm->sum2 += currentBW*currentBW;
        vm->windowData.push_back(vm->currentBW);

        if(vm->windowData.size()>vm->window_size){
            vm->sum -= vm->windowData[0];
            vm->sum2 -= vm->windowData[0]*vm->windowData[0];
            vm->windowData.pop_front();
        }

        if(currentBW >= fg_reserved_pct*(vm->fgReserved)){
            currentBW-=con->bgunused;
            con->bgunused=0;
        }

        quotient = (predictedPeakbelow>0) ? (currentBW/predictedPeakbelow) : currentBW;
        if(quotient<1){
            //cout<<"\t currentBW "<<currentBW<<"\t predictedPeakbelow "<<predictedPeakbelow<<" currentBW < predictedPeakbelow "<<(currentBW < predictedPeakbelow)<<" qut "<<qut<<"\n";
            vm->downdata.push_back(currentBW);
        }
        else{
            vm->downdata.clear();
        }

        updateContBW = 0;
        if(currentBW >= fg_reserved_pct*(vm->fgReserved)){

            vm->fgReserved = currentBW+guardBW;
            predictedPeakbelow = currentBW-guardBW;   
            //predictedPeakbelow = vm->mean-guardBW;
            con->bgReserved = (vm->original_limit - (1+guardBand)*vm->fgReserved);
            con->bgunused = fmod(con->bgReserved,container_reclaim_size);
            con->bgReserved = con->bgReserved - con->bgunused;

            con->bgReserved = contOrigLimit;
            violations+=1;//timeAfterViolation=vm->window_size;
            cout<<" 1. currentBW "<<currentBW<<" predictedPeakbelow "<<predictedPeakbelow
                <<"\t vm->mean "<<vm->mean<<" guardBW "<<guardBW<<" minguardBW "<<minguardBW<<" vm->stdeviation "<<(vm->stdeviation)
                <<"\t fgReserved "<<vm->fgReserved<<"\t con->bgunused "<<con->bgunused<<"\t con->bgReserved "<<con->bgReserved<<"\t predictedPeakbelow "<<(predictedPeakbelow)<<"\n";
            
            updateContBW = 2;
        }

        if(vm->downdata.size()>=PHASE_CHANGE_SIZE){
            vm->windowData.clear();

            pair<pair<double,double>,pair<double,double>> p = findMeanAndSTD(vm->downdata,vm->windowData);
           
            vm->mean = p.second.first;
            vm->stdeviation = p.second.second;
            vm->sum = p.first.first;
            vm->sum2 = p.first.second;
            double guard=GUARD_STEP_SIZE*vm->stdeviation;

            guardBW = (minguardBW > guard)?minguardBW:guard;
            vm->fgReserved  = vm->mean+guardBW;
            predictedPeakbelow = vm->mean-guardBW;

            //vm->fgReserved = *std::max_element(vm->downdata.begin(),vm->downdata.end())+guardBW;
            con->bgReserved = (vm->original_limit - (1+guardBand)*vm->fgReserved);
            con->bgunused = fmod(con->bgReserved,container_reclaim_size);
            con->bgReserved = con->bgReserved - con->bgunused;
            cout<<" 2. currentBW "<<currentBW<<" predictedPeakbelow "<<predictedPeakbelow
                <<"\t vm->mean "<<vm->mean<<" guardBW "<<guardBW<<" minguardBW "<<minguardBW<<" vm->stdeviation "<<(vm->stdeviation)
                <<"\t fgReserved "<<vm->fgReserved<<"\t con->bgunused "<<con->bgunused<<"\t con->bgReserved "<<con->bgReserved<<"\t predictedPeakbelow "<<(predictedPeakbelow)<<"\n";
            vm->downdata.clear(); 
            phasechanges+=1;
            updateContBW = 1;
        }

        if(updateContBW){
            double toSetLimit = ((con->bgReserved<contOrigLimit) || (con->bgReserved<0) )? contOrigLimit : con->bgReserved;
            toSetLimit = (toSetLimit<800) ? toSetLimit : 800; 
            cout<<"\t toSetLimit "<<toSetLimit<<" contOrigLimit "<<contOrigLimit<<" con->bgReserved "<<con->bgReserved<<"\n";
            //updateBgBW(string container_name, string interface, int rate, int burst, int latency);
            updateBgBW("bay12-cont1","br0",toSetLimit,BURST,LATENCY);
            //updateBgBW("bay12-cont1","br0",contOrigLimit,BURST,LATENCY);

            if(updateContBW==2){
            initWindow(vm,con); // now to set the values if necessary.//
            vm->mean = vm->sum/(double)vm->windowData.size();
            vm->stdeviation = sqrt((vm->sum2 / (double)vm->windowData.size()) - (vm->mean * vm->mean));

            guardBW = (minguardBW > GUARD_STEP_SIZE*vm->stdeviation)?minguardBW:GUARD_STEP_SIZE*vm->stdeviation;
            //double predictedPeakabove = vm->mean+guardBW;
            predictedPeakbelow = vm->mean-guardBW;
            //
            double guard=GUARD_STEP_SIZE*vm->stdeviation;
            guardBW = (minguardBW > guard)?minguardBW:guard;
            vm->fgReserved  = vm->mean+guardBW;
            predictedPeakbelow = vm->mean-guardBW;
            //vm->fgReserved = *std::max_element(vm->downdata.begin(),vm->downdata.end())+guardBW;
            con->bgReserved = (vm->original_limit - (1+guardBand)*vm->fgReserved);
            con->bgunused = fmod(con->bgReserved,container_reclaim_size);
            con->bgReserved = con->bgReserved - con->bgunused;

            cout<<"\t vm->mean "<<vm->mean<<" guardBW "<<guardBW<<" minguardBW "<<minguardBW<<" vm->stdeviation "<<(vm->stdeviation)<<"\n";
            cout<<"\t fgReserved "<<vm->fgReserved<<"\n";
            cout<<"\t con->bgunused "<<con->bgunused<<"\n";
            cout<<"\t con->bgReserved "<<con->bgReserved<<"\n";
            cout<<"\t predictedPeakbelow "<<(predictedPeakbelow)<<"\n";

            cout<<"vm->original_limit "<<vm->original_limit <<" vm->fgReserved "<<(vm->fgReserved)<<"\t con->bgReserved "<<(con->bgReserved)<<" con->bgunused "<<(con->bgunused)<<"\n";
            vm->downdata.clear(); 

            toSetLimit = ((con->bgReserved<contOrigLimit) || (con->bgReserved<0) )? contOrigLimit : con->bgReserved;
            toSetLimit = (toSetLimit<800) ? toSetLimit : 800; 
            cout<<"\t toSetLimit "<<toSetLimit<<" contOrigLimit "<<contOrigLimit<<" con->bgReserved "<<con->bgReserved<<"\n";
            //updateBgBW(string container_name, string interface, int rate, int burst, int latency);
            updateBgBW("bay12-cont1","br0",toSetLimit,BURST,LATENCY);
            con->bgReserved = toSetLimit;       
            }                 

        }
        predictedPeakbelow = (predictedPeakbelow<minPeakBelow) ? minPeakBelow : predictedPeakbelow; 
        vm->fgReserved = (vm->fgReserved>1000) ? 1000 : vm->fgReserved;
        vm->fgReserved = (vm->fgReserved<minPeakBelow) ? minPeakBelow : vm->fgReserved;
        cout<<vm->currentBW<<","<<predictedPeakbelow<<","<<vm->fgReserved<<","<<con->bgReserved<<","<<con->bgunused<<","<<violations<<","<<phasechanges<<","<<vm->mean<<","<<vm->stdeviation<<","<<vm->downdata.size()<<","<<quotient<<endl;
        //std::this_thread::sleep_for (std::chrono::seconds(1));

    }

    return 0;

}
