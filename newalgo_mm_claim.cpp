#include<iostream>
#include<string>
#include<algorithm>
#include<sstream>
#include<map>
#include<thread>
#include<numeric>
#include<fstream>

#define WAIT_INTERVAL_IN_SEC 10
#define CONTAINER_ID c3293aefbfdf
#define MIN_CLAIM_VALUE 1436549 // minimum value that should be claimed from the container 
#define VM_IGNORE_INTERVAL 3 // 2*WAIT_INTERVAL_IN_SEC is the time in seconds
#define GUARD_STEP_SIZE 2 // total claimable = maxReserved - ( maxPeak+0.25*maxPeak )
#define MAX_PEAK_THRESHOLD_PCT 90 // reclaim from container when the currentMemory >= 0.9*maxPeak
#define THREAD_SLEEP_TIME 1 // This is the thread sleep time 
#define PHASE_CHANGE_SIZE 3


using namespace std;


typedef unsigned long int uint64_t;

struct vm_info
{
    string name;
    uint64_t currentMemory;
    uint64_t  maxPeak;
    double window[10]; //should be reconfigured
    int activeWindowSize;
    double mean;
    double stdeviation;
    uint64_t  memoryReserved;
    uint64_t  total_claimed;
    uint64_t  original_limit;
    int up_limit_violations;
    int low_limit_violations;

    vm_info()
    {
        currentMemory=maxPeak=memoryReserved=total_claimed=original_limit=up_limit_violations=low_limit_violations=activeWindowSize=0;

    }
};

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

void printVMData(struct vm_info* vm,int vm_count)
{
    for(int i=0;i<vm_count;i++)
    {
        cout<<"vm_name: "<<vm[i].name<<endl;
        cout<<"vm_mem_reserved: "<<vm[i].memoryReserved<<endl;
    }

}

void updateRunningWindow(struct vm_info* vm)
{
    for(int i=0;i<PHASE_CHANGE_SIZE;i++) vm->window[i]=vm->window[10 - PHASE_CHANGE_SIZE +i];
    for(int i=PHASE_CHANGE_SIZE;i<10;i++) vm->window[i]=0;
    vm->activeWindowSize=PHASE_CHANGE_SIZE;
    
}

//code ref: https://www.strchr.com/standard_deviation_in_one_pass
pair<double,double> findMeanAndSTD(double a[], int n)
{
    if(n == 0)
        return make_pair(0.0,0.0);
    double sum = 0;
    double sq_sum = 0;
    for(int i = 0; i < n; ++i) {
       sum += a[i];
       sq_sum += a[i] * a[i];
    }
    double mean = sum / n;
    double variance = (sq_sum / n) - (mean * mean);
    return make_pair(mean,sqrt(variance));
}

void add(struct vm_info* vm)
{
    if(vm->activeWindowSize==10)
    {
        for(int i=1;i<10;i++) vm->window[i-1]=vm->window[i];
        vm->window[9]=vm->currentMemory/1024;
    }
    else
    {
        vm->window[vm->activeWindowSize]=vm->currentMemory/1024;
        vm->activeWindowSize++;
    }
}

void printWindow(struct vm_info* vm)
{
    for(int i=0;i<vm->activeWindowSize;i++)
    {
        cout<<vm->window[i]<<" ";
    }
}

int main()
{
    //list all the running VMs
    string data = runCommand("/usr/bin/virsh list | /bin/grep running | /usr/bin/awk '{print $2}'");
    int vm_count = count(data.begin(),data.end(),'\n');
    struct vm_info* vm = new vm_info[vm_count];
    map<int,int> vm_not_in_use;

    istringstream iss(data);
    for(int i=0;i<vm_count;i++)
    {
        getline(iss,vm[i].name,'\n');
        vm[i].original_limit = vm[i].memoryReserved = stoi(runCommand(("/usr/bin/virsh dommemstat "+vm[i].name+" | grep actual | awk '{print $2}'").c_str()));
        vm_not_in_use[i]=VM_IGNORE_INTERVAL;
    }

    //print VM data
    printVMData(vm,vm_count);

    int maxPeakTimer=1;

    //gathering initial data for the defined running window

    for(int i=0;i<10;i++)
    {
       for(int j=0;j<vm_count;j++)
        {
            vm[j].currentMemory = stoi(runCommand(("/usr/bin/virsh dommemstat "+vm[j].name+" | grep available | awk '{print $2}'").c_str()));
            vm[j].currentMemory -= stoi(runCommand(("/usr/bin/virsh dommemstat "+vm[j].name+" | grep unused | awk '{print $2}'").c_str()));
            vm[j].window[i]=(vm[j].currentMemory/1024);
            cout<<vm[j].currentMemory<<endl;
        }   

        std::this_thread::sleep_for (std::chrono::seconds(1));

    }

    for(int i=0;i<vm_count;i++)
    {
        pair<double,double> mean_std = findMeanAndSTD(vm[i].window,10);
        vm[i].activeWindowSize=10;
        vm[i].mean = mean_std.first;
        vm[i].stdeviation = mean_std.second;
    }

    while(1)
    {
        //find mem usage per vm
        for(int i=0;i<vm_count;i++)
        {
            vm[i].currentMemory = stoi(runCommand(("/usr/bin/virsh dommemstat "+vm[i].name+" | grep available | awk '{print $2}'").c_str()));
            vm[i].currentMemory -= stoi(runCommand(("/usr/bin/virsh dommemstat "+vm[i].name+" | grep unused | awk '{print $2}'").c_str()));

            if(vm[i].currentMemory >= 0.9*(vm[i].memoryReserved))
            {
                //talk to ahmad and fill his code here
                cout<<"reached here......"<<endl;
                vm[i].memoryReserved = vm[i].currentMemory+(GUARD_STEP_SIZE*vm[i].stdeviation);
                //give "vm[i].original_limit - vm[i].memoryReserved"
                
            }

            add(&vm[i]);
            double predictedPeakabove = vm[i].mean + GUARD_STEP_SIZE*vm[i].stdeviation;
            double predictedPeakbelow = vm[i].mean - GUARD_STEP_SIZE*vm[i].stdeviation;

            if(vm[i].currentMemory > predictedPeakabove)
            {
                vm[i].up_limit_violations+=1;
                vm[i].low_limit_violations=0;
            }
            if(vm[i].currentMemory < predictedPeakabove)
            {
                vm[i].up_limit_violations=0;
                vm[i].low_limit_violations+=1;
            }
            else
            {
                vm[i].up_limit_violations = vm[i].low_limit_violations = 0;
            }

            vm[i].up_limit_violations=5;

            if(vm[i].up_limit_violations>=PHASE_CHANGE_SIZE || vm[i].low_limit_violations>=PHASE_CHANGE_SIZE)
            {
                updateRunningWindow(&vm[i]);
                pair<double,double> mean_std = findMeanAndSTD(vm[i].window,PHASE_CHANGE_SIZE);
                vm[i].mean = mean_std.first;
                vm[i].stdeviation = mean_std.second;
                vm[i].memoryReserved = vm[i].currentMemory+(GUARD_STEP_SIZE*vm[i].stdeviation);
                //give "vm[i].original_limit - vm[i].memoryReserved"
            }

            printWindow(&vm[i]);
            cout<<endl;

        }    
    }

    return 0;

}
