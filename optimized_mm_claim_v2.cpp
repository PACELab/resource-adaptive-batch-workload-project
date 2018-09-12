#include<iostream>
#include<string>
#include<algorithm>
#include<sstream>
#include<map>
#include<thread>
#include<fstream>

#define WAIT_INTERVAL_IN_SEC 10
#define CONTAINER_ID c3293aefbfdf
#define MIN_CLAIM_VALUE 1436549 // minimum value that should be claimed from the container 
#define VM_IGNORE_INTERVAL 3 // 2*WAIT_INTERVAL_IN_SEC is the time in seconds
#define GUARD_MEM_PCT 25 // total claimable = maxReserved - ( maxPeak+0.25*maxPeak )
#define MAX_PEAK_THRESHOLD_PCT 90 // reclaim from container when the currentMemory >= 0.9*maxPeak
#define THREAD_SLEEP_TIME 1 // This is the thread sleep time 


using namespace std;

typedef unsigned long int uint64_t;

ofstream logfile;

struct vm_info
{
    string name;
    uint64_t currentMemory;
    uint64_t  maxPeak;
    uint64_t  maxPeakHelper;
    uint64_t  memoryReserved;
    ofstream outfile;
    uint64_t  total_claimed;
    uint64_t  original_limit;

    vm_info()
    {
        currentMemory=maxPeak=memoryReserved=total_claimed=original_limit=maxPeakHelper=0;
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

    while(1)
    {
        //find mem usage per vm
        for(int i=0;i<vm_count;i++)
        {
            vm[i].currentMemory = stoi(runCommand(("/usr/bin/virsh dommemstat "+vm[i].name+" | grep available | awk '{print $2}'").c_str()));
            vm[i].currentMemory -= stoi(runCommand(("/usr/bin/virsh dommemstat "+vm[i].name+" | grep unused | awk '{print $2}'").c_str()));

            time_t t = std::time(0);
        long int now = static_cast<long int> (t);
            vm[i].outfile.open(vm[i].name,std::ios_base::app);
            vm[i].outfile<<to_string(now)<<","<<to_string(vm->memoryReserved)<<","<<to_string(vm[i].currentMemory)<<"\n";
            vm[i].outfile.close();

            vm[i].maxPeakHelper = max(vm[i].maxPeakHelper,vm[i].currentMemory);


            if(vm[i].currentMemory >= (MAX_PEAK_THRESHOLD_PCT/100)*(vm[i].memoryReserved))
            {
                //talk to ahmad and fill his code here
            runCommand("sh killStress.sh "+CONTAINER_ID);
                        cout<<"reached here......"<<endl;
                vm_not_in_use[i] = maxPeakTimer>=5?VM_IGNORE_INTERVAL+1:VM_IGNORE_INTERVAL;

                vm[i].memoryReserved = vm[i].original_limit;

                uint64_t toBeClaimed = vm[i].memoryReserved - (vm[i].maxPeak+((GUARD_MEM_PCT/100)*vm[i].maxPeak));

                logfile<<to_string(now)<<" claiming memory from vm : "<<totalClaimed<<"\n";
                logfile.close();

                if(toBeClaimed > MIN_CLAIM_VALUE)
                {
                   runCommand(("sh runStress.sh "+CONTAINER_ID+"  "+to_string(vm[i].original_limit-vm[i].memoryReserved)).c_str());
                }

                cout<<vm[i].memoryReserved<<" "<<vm[i].name<<endl;


            }

            

        }

        if(maxPeakTimer==0)
        {
        uint64_t totalClaimed = 0;
            for(int i=0;i<vm_count;i++)
            {
                if(vm_not_in_use[i]>0)
                {
                    vm_not_in_use[i]--;
                }

                vm[i].maxPeak = max(vm[i].maxPeakHelper,vm[i].original_limit/10);
                //reset max peak helper
                vm[i].maxPeakHelper=0;

                uint64_t toBeClaimed = vm[i].memoryReserved - (vm[i].maxPeak+((GUARD_MEM_PCT/100)*vm[i].maxPeak));

                if(toBeClaimed > MIN_CLAIM_VALUE and vm_not_in_use[i] == 0)
                {
                    vm_not_in_use[i] = VM_IGNORE_INTERVAL;
                    vm[i].memoryReserved -= toBeClaimed;
                    //Claim what is to be claimed here
                    totalClaimed+=toBeClaimed;

                    cout<<vm[i].memoryReserved<<" "<<vm[i].name<<endl;
                    //cout<<"Claiming Memory from the VM to be given to the container"<<endl;
            }
            }
        time_t t = std::time(0);
        long int now = static_cast<long int> (t);
        logfile.open("pace_log",std::ios_base::app);

        totalClaimed /= 1024;
        logfile<<to_string(now)<<" claiming   memory from vms : "<<totalClaimed<<"\n";
        logfile.close();
        runCommand(("sh runStress.sh "+CONTAINER_ID+"  "+to_string(totalClaimed)).c_str());
        }

        maxPeakTimer++;
        maxPeakTimer%=WAIT_INTERVAL_IN_SEC;
        std::this_thread::sleep_for (std::chrono::seconds(THREAD_SLEEP_TIME));
    }

    return 0;

}
