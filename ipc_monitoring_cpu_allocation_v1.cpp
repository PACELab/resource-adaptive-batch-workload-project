#include<iostream>
#include<string>
#include<algorithm>
#include<sstream>
#include<map>
#include<vector>
#include<numeric>
#include<fstream>
#include<thread>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <thread>


using namespace std;

struct container
{
    int cpuQuota;
    int cpuShare;

    container()
    {
        cpuQuota = -1;
        cpuShare = 2;
    }
};


//typedef unsigned long int uint64_t;

struct vm_info
{

    float currentIPC;
    float mean;
    float stdeviation;
    float sum; 
    float sum2;
    vector<float> window; 
    int windowSize; 
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

pair<float,float> findMeanAndSTD(std::vector<float>& a)
{
    if(a.size() == 0)
        return make_pair(0.0,0.0);
    float sum = 0;
    float sq_sum = 0;
    for(int i = 0; i < a.size(); ++i) {
       sum += a[i];
       sq_sum += a[i] * a[i];
    }
    float mean = sum / a.size();
    float variance = (sq_sum / a.size()) - (mean * mean);
    return make_pair(mean,sqrt(variance));
}

float getMeanIPC()
{
    string res = runCommand("sudo sh -c \"/home/ahmad/spark-intereference-project/call_perf_for_two_vcpu_vm_v2.sh server3-vm1 \"");
    //cout << res << endl; 

    if(res == "") return 0; 
    float ipc=0;
    try
    {
	ipc=stod(res);
    }
    catch(...)
    {
        ipc=0; 
    }

     return ipc; 
}

void setCpuQuotaForDocker(int cpuQuota)
{

char command[]="sudo sh -c \"/home/ahmad/spark-intereference-project/set_cpu_quota_for_docker.sh ";
std::string quota = std::to_string(cpuQuota);
strcat(command,quota.c_str());
strcat(command, "\"");
//cout << command << endl;
runCommand(command);

}



int main(int argc,char** argv)
{
    struct vm_info* vmInfo = new vm_info;
    struct container * conInfo = new container;

    if(argc<=2)
    {
	cout <<"winodw size, time to run, is needed"<<endl;
	exit(0);     
    }

    conInfo->cpuQuota = 5000;
    setCpuQuotaForDocker(conInfo->cpuQuota);
    cout << "sleep 10 seconds" << endl; 

    std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    cout <<"monitoring IPC for first window"<<endl; 
    int windowSize = stoi(argv[1]);
    int timeToRun = stoi(argv[2]); 

    float guardStep=1; 
    float cpuQuotaIncreasingRate=1.1; 
    int minCpuQuota=5000; 
    int maxCpuQuota=400000; 
    float cpuQuotaDecreasingRate=0.75;
    int minDistanceBetweenIncrease=0;

    for (int i=0;i<windowSize;i++)
    {
	float ipc=getMeanIPC();
	vmInfo->window.push_back(ipc);
	vmInfo->sum += ipc;
	vmInfo->sum2 += (ipc*ipc);
	vmInfo->windowSize++; 
	vmInfo->mean = (vmInfo->sum / vmInfo->windowSize); 
        vmInfo->stdeviation= sqrt((vmInfo->sum2/vmInfo->windowSize)-(vmInfo->mean*vmInfo->mean));	
	cout <<"ipc_info: "<<ipc << "," << vmInfo->mean <<","<<vmInfo->stdeviation<<endl; 

    	timeToRun--;
	if(timeToRun<0)
	{
		cout<<"time to run is done!" << endl; 
		return 0; 
	}
    }
    cout << vmInfo->windowSize << " IPCs are collected" <<endl;

    bool quotaIncreasing=true; 
    float lowerBound= vmInfo->mean - (guardStep*vmInfo->stdeviation);

    while(1)
    {
	float ipc=getMeanIPC(); 
	//float lowerBound= vmInfo->mean - (guardStep*vmInfo->stdeviation);
        float newLowerBound = vmInfo->mean - (guardStep*vmInfo->stdeviation);
	lowerBound = (0.95 * lowerBound) + (0.05 * newLowerBound); 
 
	cout <<"ipc_info: "<< ipc << "," << vmInfo->mean <<","<<vmInfo->stdeviation<<","<< lowerBound <<endl;
	 	
	//if(ipc>=lowerBound && quotaIncreasing)
	//if(ipc>=lowerBound && minDistanceBetweenIncrease==0)
	if(ipc>=lowerBound)
	{
		conInfo->cpuQuota = min((int)(conInfo->cpuQuota*cpuQuotaIncreasingRate),maxCpuQuota);
		cout<<"quota_info: increasing quota to, " << conInfo->cpuQuota << endl;
		setCpuQuotaForDocker(conInfo->cpuQuota);
		minDistanceBetweenIncrease=5; 
 
	}
	if(ipc<lowerBound)
	{
		//cout<<"stop increasing quota" << endl;
		quotaIncreasing=false; 
		if(ipc < lowerBound)
		{
			conInfo->cpuQuota = max(minCpuQuota,(int)(conInfo->cpuQuota * cpuQuotaDecreasingRate));
			cout<<"quota_info: deacresing quota to " << conInfo->cpuQuota << endl;
                	setCpuQuotaForDocker(conInfo->cpuQuota);
 
		}
	}
	
	vmInfo->sum -= vmInfo->window[0];
        vmInfo->sum2 -= (vmInfo->window[0]*vmInfo->window[0]);
        vmInfo->window.erase(vmInfo->window.begin());

        vmInfo->window.push_back(ipc);
        vmInfo->sum += ipc;
        vmInfo->sum2 += (ipc*ipc);
        vmInfo->mean = (vmInfo->sum / vmInfo->windowSize);
        vmInfo->stdeviation= sqrt((vmInfo->sum2/vmInfo->windowSize)-(vmInfo->mean*vmInfo->mean));
    	
	minDistanceBetweenIncrease=max(0,minDistanceBetweenIncrease - 1);
	
	timeToRun--;
    	if(timeToRun<0)
        {
                cout<<"time to run is done!" << endl;
                return 0;
        }

    } 
    return 0; 
}



