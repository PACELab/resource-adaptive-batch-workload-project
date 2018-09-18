#include<iostream>
#include<string>
#include<algorithm>
#include<sstream>
#include<map>
#include<vector>
#include<numeric>
#include<fstream>
#include<thread>

#define WAIT_INTERVAL_IN_SEC 10
#define CONTAINER_ID c3293aefbfdf
#define MIN_CLAIM_VALUE 1436549 // minimum value that should be claimed from the container 
#define VM_IGNORE_INTERVAL 3 // 2*WAIT_INTERVAL_IN_SEC is the time in seconds
#define GUARD_STEP_SIZE 2 // total claimable = maxReserved - ( maxPeak+0.25*maxPeak )
#define MAX_PEAK_THRESHOLD_PCT 90 // reclaim from container when the currentMemory >= 0.9*maxPeak
#define THREAD_SLEEP_TIME 1 // This is the thread sleep time 
#define PHASE_CHANGE_SIZE 3


using namespace std;


//typedef unsigned long int uint64_t;

struct vm_info
{
    string name;
    double currentMemory;
    double mean;
    double stdeviation;
    double sum;
    double sum2;
    vector<double> updata;
    vector<double> downdata;
    double  memoryReserved;
    double  original_limit;

    vm_info()
    {
        currentMemory=sum=sum2=memoryReserved=original_limit=0;

    }

    void to_string()
    {
        printf("name : %s \n",name.c_str());
        printf("original_limit : %f \n",original_limit);
        printf("currentMemory : %f \n",currentMemory);
        printf("memoryReserved : %f \n",memoryReserved);
        printf("mean : %f \n",mean);
        printf("std : %f \n",stdeviation);
        printf("sum : %f \n",sum);
        printf("sum2 : %f \n",sum2);
        
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

// void printVMData(struct vm_info* vm,int vm_count)
// {
//     for(int i=0;i<vm_count;i++)
//     {
//         cout<<"vm_name: "<<vm[i].name<<endl;
//         cout<<"vm_mem_reserved: "<<vm[i].memoryReserved<<endl;
//     }

// }

// void updateRunningWindow(struct vm_info* vm)
// {
//     for(int i=0;i<PHASE_CHANGE_SIZE;i++) vm->window[i]=vm->window[10 - PHASE_CHANGE_SIZE +i];
//     for(int i=PHASE_CHANGE_SIZE;i<10;i++) vm->window[i]=0;
//     vm->activeWindowSize=PHASE_CHANGE_SIZE;
    
// }

//code ref: https://www.strchr.com/standard_deviation_in_one_pass
pair<double,double> findMeanAndSTD(std::vector<double>& a, int n)
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

// void add(struct vm_info* vm)
// {
//     if(vm->activeWindowSize==10)
//     {
//         for(int i=1;i<10;i++) vm->window[i-1]=vm->window[i];
//         vm->window[9]=vm->currentMemory/1024;
//     }
//     else
//     {
//         vm->window[vm->activeWindowSize]=vm->currentMemory/1024;
//         vm->activeWindowSize++;
//     }
// }

// void printWindow(struct vm_info* vm)
// {
//     for(int i=0;i<vm->activeWindowSize;i++)
//     {
//         cout<<vm->window[i]<<" ";
//     }
// }

int main()
{
    //list all the running VMs
    string data = runCommand("/usr/bin/virsh list | /bin/grep running | /usr/bin/awk '{print $2}'");
    int vm_count = count(data.begin(),data.end(),'\n');
    struct vm_info* vm = new vm_info[vm_count];

    

    istringstream iss(data);
    for(int i=0;i<vm_count;i++)
    {
        getline(iss,vm[i].name,'\n');
        cout<<"Found Machines "<<i+1<<" "<<vm[i].name<<endl;
        vm[i].original_limit = vm[i].memoryReserved = (stod(runCommand(("/usr/bin/virsh dommemstat "+vm[i].name+" | grep actual | awk '{print $2}'").c_str()))/1024)/1024;
    }

    //gathering initial data for the defined running window
    for(int i=0;i<10;i++)
    {
       for(int j=0;j<vm_count;j++)
        {
            vm[j].currentMemory = (stod(runCommand(("/usr/bin/virsh dommemstat "+vm[j].name+" | grep available | awk '{print $2}'").c_str()))/1024)/1024;
            vm[j].currentMemory -= (stod(runCommand(("/usr/bin/virsh dommemstat "+vm[j].name+" | grep unused | awk '{print $2}'").c_str()))/1024)/1024;
            vm[j].sum+=vm[j].currentMemory;
            vm[j].sum2+=vm[j].currentMemory*vm[j].currentMemory;//Mbs
            vm[j].to_string();
        }   

        std::this_thread::sleep_for (std::chrono::seconds(1));

    }

    int count = 10;

    while(1)
    {
        //find mem usage per vm
        for(int i=0;i<vm_count;i++)
        {
            vm[i].mean = vm[i].sum/count;
            vm[i].stdeviation = sqrt((vm[i].sum2 / count) - (vm[i].mean * vm[i].mean));

            vm[i].to_string();

            vm[i].currentMemory = (stod(runCommand(("/usr/bin/virsh dommemstat "+vm[i].name+" | grep available | awk '{print $2}'").c_str()))/1024)/1024;
            vm[i].currentMemory -= (stod(runCommand(("/usr/bin/virsh dommemstat "+vm[i].name+" | grep unused | awk '{print $2}'").c_str()))/1024)/1024;
            
            
            vm[i].sum+=vm[i].currentMemory; //Mbs
            vm[i].sum2 += vm[i].currentMemory*vm[i].currentMemory; //Mbs
            

            if(vm[i].currentMemory >= 1*(vm[i].memoryReserved))
            {
                //talk to ahmad and fill his code here
                cout<<"reached here......"<<endl;
                vm[i].memoryReserved = vm[i].currentMemory+(GUARD_STEP_SIZE*vm[i].stdeviation);
                //give "vm[i].original_limit - vm[i].memoryReserved"
                
            }

            double temp = vm[i].mean + GUARD_STEP_SIZE*vm[i].stdeviation;
            double predictedPeakabove = temp>vm[i].original_limit?vm[i].original_limit:temp;
            temp = (vm[i].mean - GUARD_STEP_SIZE*vm[i].stdeviation);
            double predictedPeakbelow = temp<0?0:temp;

            cout<<"predictedPeakabove : "<<predictedPeakabove<<endl;
            cout<<"curr_mem : "<<vm[i].currentMemory<<endl;
            cout<<"predictedPeakbelow : "<<predictedPeakbelow<<endl;

            if(vm[i].currentMemory > predictedPeakabove)
            {
                vm[i].downdata.clear();
                vm[i].updata.push_back(vm[i].currentMemory);
               
            }
            if(vm[i].currentMemory < predictedPeakabove)
            {
                vm[i].updata.clear();
                vm[i].downdata.push_back(vm[i].currentMemory);
                
            }
            else
            {
                vm[i].downdata.clear();
                vm[i].updata.clear();
            }

            if(vm[i].updata.size()>=PHASE_CHANGE_SIZE)
            {
                pair<double,double> p = findMeanAndSTD(vm[i].updata,vm[i].updata.size());
                vm[i].mean = p.first;
                vm[i].stdeviation = p.second;
                vm[i].memoryReserved = *std::max_element(vm[i].updata.begin(),vm[i].updata.end())+(GUARD_STEP_SIZE*vm[i].stdeviation);
                vm[i].downdata.clear();
                vm[i].updata.clear();

           
            }
            else if(vm[i].downdata.size()>=PHASE_CHANGE_SIZE)
            {
                pair<double,double> p = findMeanAndSTD(vm[i].downdata,vm[i].downdata.size());
                vm[i].mean = p.first;
                vm[i].stdeviation = p.second;
                vm[i].memoryReserved = *std::max_element(vm[i].updata.begin(),vm[i].updata.end())+(GUARD_STEP_SIZE*vm[i].stdeviation);
                vm[i].downdata.clear();
                vm[i].updata.clear();

            }

            //give "vm[i].original_limit - vm[i].memoryReserved"

        }  

        std::this_thread::sleep_for (std::chrono::seconds(1));
        count++;  
    }

    return 0;

}
