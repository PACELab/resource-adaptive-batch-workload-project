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

using namespace std;

struct container
{
    double bgReserved;
    double bgunused;

    container()
    {
        bgReserved = 0;
        bgunused = 0;
    }
};


//typedef unsigned long int uint64_t;

struct vm_info
{

    double currentMemory;
    double mean;
    double stdeviation;
    double sum;
    double fgReserved;
    double sum2; 
    vector<double> downdata;
    deque<double> windowData;
    double  original_limit;
    int window_size;

    vm_info()
    {
        currentMemory=sum=sum2=original_limit=window_size=fgReserved=0;

    }

    // void to_string()
    // {
    //    // printf("\033c");
    //     printf("original_limit : %f \n",original_limit);
    //     printf("currentMemory : %f \n",currentMemory);
    //     printf("fgReserved : %f \n",fgReserved);
    //     printf("mean : %f \n",mean);
    //     printf("std : %f \n",stdeviation);
    //     printf("sum : %f \n",sum);
    //     printf("sum2 : %f \n",sum2);
    //     printf("window_size : %d \n",window_size);
    //     if(updata.size()>0)
    //     {
    //         printf("updata : ");
    //         for(int i=0;i<updata.size();i++) printf("%f ",updata[i]);
    //         printf("\n");
    //     }
    //     else
    //         printf("updata : <empty>\n");
    //     if(downdata.size()>0)
    //     {
    //         printf("downdata : ");
    //         for(int i=0;i<downdata.size();i++) printf("%f ",downdata[i]);
    //         printf("\n");
    //     }
    //     else
    //         printf("downdata : <empty>\n");


    // }
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
pair< pair<double,double>,pair<double,double> > findMeanAndSTD(std::vector<double>& a,std::deque<double>& b)
{
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

double getTotalCurrentMemory()
{
    string data = runCommand("/usr/bin/virsh list | /bin/grep running | /usr/bin/awk '{print $2}'");

    int vm_count = count(data.begin(),data.end(),'\n');

    string vm_name;

    double curr_mem_sum = 0;

    istringstream iss(data);
    for(int i=0;i<vm_count;i++)
    {
        getline(iss,vm_name,'\n');
        string available = runCommand(("/usr/bin/virsh dommemstat "+vm_name+" | grep available | awk '{print $2}'").c_str());
        if(available == "") continue;
        double curr_mem = stod(available)/1048576;
        curr_mem -= (stod(runCommand(("/usr/bin/virsh dommemstat "+vm_name+" | grep unused | awk '{print $2}'").c_str())))/1048576;
        curr_mem_sum += curr_mem;
    }

    return curr_mem_sum;
}


double getTotalActualMemory()
{
    string data = runCommand("/usr/bin/virsh list | /bin/grep running | /usr/bin/awk '{print $2}'");

    int vm_count = count(data.begin(),data.end(),'\n');

    string vm_name;

    double curr_mem_sum = 0;

    istringstream iss(data);
    for(int i=0;i<vm_count;i++)
    {
        getline(iss,vm_name,'\n');

        string available = runCommand(("/usr/bin/virsh dommemstat "+vm_name+" | grep actual | awk '{print $2}'").c_str());
        if(available == "") continue;

        curr_mem_sum += stod(available)/1048576;
    }

    return curr_mem_sum;
}

int main(int argc,char** argv)
{
    struct vm_info* vm = new vm_info;
    struct container * con = new container;

    if(argc<=1)
    {
        cout<<"./a.out <container_reserved_memory_in_gb> <init_window_size> <PHASE_CHANGE_SIZE> <GUARD_STEP_SIZE> <RECLAM_PCT> <CONTAINER RECLAIM SIZE> <FG_RESERVED_CLOSENESS_PCT>"<<endl;
        exit(0);
    }

    double reclaim_pct = stod(argv[5]);
    double fg_reserved_pct = stod(argv[7]);

    //double num_of_sockets=vm->original_limit = stod(runCommand("cat /proc/meminfo | grep MemTotal | awk '{print $2}'");

    //get the initial window size
    vm->window_size = stoi(argv[2]);

    double container_reclaim_size = stod(argv[6]);

    //cout<<"Num of Sockets :"<num_of_sockets;

    int PHASE_CHANGE_SIZE = stoi(argv[3]);
    double GUARD_STEP_SIZE = stof(argv[4]);

    vm->original_limit = stod(runCommand("cat /proc/meminfo | grep MemTotal | awk '{print $2}'"))/1048576;
    //subtract container mmemory from the total allocated memory
    vm->original_limit -= stod(argv[1]);   // argv[1] == <container_reserved_memory_in_gb>
    
    vm->fgReserved = getTotalActualMemory();
    con->bgReserved = (vm->original_limit - vm->fgReserved);
cout<<(int)con->bgReserved*32768<<endl;
    cout<<runCommand("sh /home/ahmad/spark-intereference-project/killStress.sh 01160ff14a31");
    cout<<endl<<runCommand(("sh /home/ahmad/spark-intereference-project/runStress.sh 01160ff14a31 32 "+ to_string((int)con->bgReserved*32768)).c_str());
    con->bgunused = fmod(con->bgReserved,container_reclaim_size);
    con->bgReserved = con->bgReserved - con->bgunused;


    
    //gathering initial data for the defined running window
    for(int i=0;i<vm->window_size;i++)
    {
        double currentMemory = getTotalCurrentMemory();
        vm->sum+=currentMemory;
        vm->sum2+=currentMemory*currentMemory;
        vm->currentMemory = currentMemory;
        vm->windowData.push_back(currentMemory);
        //vm->to_string();
       cout<<vm->currentMemory<<","<<0<<","<<vm->fgReserved<<","<<con->bgReserved<<","<<con->bgunused<<","<<0<<","<<0<<","<<0<<","<<0<<","<<runCommand("docker stats --format \"{{.MemUsage}}\" --no-stream |  awk '{ print $1 }'")<<endl;
       std::this_thread::sleep_for (std::chrono::seconds(1));

    }

    double minGaurdMem = reclaim_pct*vm->original_limit;

    double violations,phasechanges;
    violations=phasechanges=0;

    vm->mean = vm->sum/(double)vm->windowData.size();
    vm->stdeviation = sqrt((vm->sum2 / (double)vm->windowData.size()) - (vm->mean * vm->mean));

    double gaurdMem = (minGaurdMem > GUARD_STEP_SIZE*vm->stdeviation)?minGaurdMem:GUARD_STEP_SIZE*vm->stdeviation;
    //double predictedPeakabove = vm->mean+gaurdMem;
    double predictedPeakbelow = vm->mean-gaurdMem;

    vm->fgReserved = vm->mean+gaurdMem;
    con->bgReserved = (vm->original_limit - vm->fgReserved);
    cout<<runCommand("sh /home/ahmad/spark-intereference-project/killStress.sh 01160ff14a31");
    cout<<endl<<runCommand(("sh /home/ahmad/spark-intereference-project/runStress.sh 01160ff14a31 32 "+ to_string((int)con->bgReserved*32768)).c_str());
    con->bgunused = fmod(con->bgReserved,container_reclaim_size);
    con->bgReserved = con->bgReserved - con->bgunused;

    int timeAfterViolation=-1;     
    while(1)
    {

        cout<<vm->currentMemory<<","<<predictedPeakbelow<<","<<vm->fgReserved<<","<<con->bgReserved<<","<<con->bgunused<<","<<violations<<","<<phasechanges<<","<<vm->mean<<","<<vm->stdeviation<<","<<runCommand("docker stats --format \"{{.MemUsage}}\" --no-stream |  awk '{ print $1 }'")<<endl;

        double currentMemory = getTotalCurrentMemory();
        vm->currentMemory = currentMemory;

        vm->mean = vm->sum/(double)vm->windowData.size();
        vm->stdeviation = sqrt((vm->sum2 / (double)vm->windowData.size()) - (vm->mean * vm->mean));
        gaurdMem = (minGaurdMem > GUARD_STEP_SIZE*vm->stdeviation)?minGaurdMem:GUARD_STEP_SIZE*vm->stdeviation;

        vm->sum += currentMemory;
        vm->sum2 += currentMemory*currentMemory;
        vm->windowData.push_back(vm->currentMemory);

        if(vm->windowData.size()>vm->window_size)
        {
            vm->sum -= vm->windowData[0];
            vm->sum2 -= vm->windowData[0]*vm->windowData[0];
            vm->windowData.pop_front();
        }
	/*
        if(vm->stdeviation<0.5)
        {
            gaurdMem = (minGaurdMem > GUARD_STEP_SIZE*vm->stdeviation)?minGaurdMem:GUARD_STEP_SIZE*vm->stdeviation;
            predictedPeakabove = vm->mean+gaurdMem;
            predictedPeakbelow = vm->mean-gaurdMem;
        }*/

        //cout<<vm->windowData.size()<<endl;
	if(timeAfterViolation>0) 
        {
	       timeAfterViolation=timeAfterViolation-1;
	}          
        if(timeAfterViolation==0)
	{
                predictedPeakbelow = vm->mean-gaurdMem; 
                timeAfterViolation=-1;
        }


        if(currentMemory >= fg_reserved_pct*(vm->fgReserved)) 
        {
            currentMemory-=con->bgunused;
            con->bgunused=0;
        }

        if(currentMemory >= fg_reserved_pct*(vm->fgReserved))
        {

            vm->fgReserved = currentMemory+gaurdMem;
	    predictedPeakbelow = currentMemory-gaurdMem;
            
	    //predictedPeakbelow = vm->mean-gaurdMem;

	    con->bgReserved = (vm->original_limit - vm->fgReserved);
      con->bgunused = fmod(con->bgReserved,container_reclaim_size);
      con->bgReserved = con->bgReserved - con->bgunused;
cout<<runCommand("sh /home/ahmad/spark-intereference-project/killStress.sh 01160ff14a31");
    cout<<endl<<runCommand(("sh /home/ahmad/spark-intereference-project/runStress.sh 01160ff14a31 32 "+ to_string((int)con->bgReserved*32768)).c_str());
              violations+=1;
	   
	    timeAfterViolation=vm->window_size;

        }

        if(currentMemory < predictedPeakbelow)
        {
            vm->downdata.push_back(currentMemory);
        }
        else
        {
            vm->downdata.clear();
        }

        if(vm->downdata.size()>=PHASE_CHANGE_SIZE)
        {
            vm->windowData.clear();

            pair<pair<double,double>,pair<double,double>> p = findMeanAndSTD(vm->downdata,vm->windowData);
           
            vm->mean = p.second.first;
            vm->stdeviation = p.second.second;
            vm->sum = p.first.first;
            vm->sum2 = p.first.second;
	    double guard=GUARD_STEP_SIZE*vm->stdeviation;

            gaurdMem = (minGaurdMem > guard)?minGaurdMem:guard;
            vm->fgReserved  = vm->mean+gaurdMem;
            predictedPeakbelow = vm->mean-gaurdMem;

            //vm->fgReserved = *std::max_element(vm->downdata.begin(),vm->downdata.end())+gaurdMem;
            con->bgReserved = (vm->original_limit - vm->fgReserved);
            con->bgunused = fmod(con->bgReserved,container_reclaim_size);
            con->bgReserved = con->bgReserved - con->bgunused;
cout<<runCommand("sh /home/ahmad/spark-intereference-project/killStress.sh 01160ff14a31");
    cout<<endl<<runCommand(("sh /home/ahmad/spark-intereference-project/runStress.sh 01160ff14a31 32 "+ to_string((int)con->bgReserved*32768)).c_str());
                   vm->downdata.clear(); 
            phasechanges+=1;

        }

        std::this_thread::sleep_for (std::chrono::seconds(1));

    }

    return 0;

}
