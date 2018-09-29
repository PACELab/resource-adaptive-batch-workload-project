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
    string res = runCommand("sudo sh -c \"/home/ahmad/spark-intereference-project/call_perf_for_two_vcpu_vm_v2.sh server3-vm1\"");
    //cout << res << endl; 
    if(res == "") return 0; 
    float ipc=0;
    try
    {
	ipc=stof(res);
    }
    catch(...)
    {
        cout<<"ipc script reutrn null" << endl; 
	ipc=0; 
    }

     return ipc; 
}


float getMeanLLCLoadMisses()
{
    string res = runCommand("/home/ahmad/spark-intereference-project/get_llc_miss_for_two_vcpus.sh server3-vm1");
    //cout << res << endl; 
    if(res == "") return 0;
    float llcMiss=0;
    try
    {
        llcMiss=stof(res);
    }
    catch(...)
    {
        llcMiss=0;
    }

     return llcMiss;
}


float getMeanCacheMisses()
{
    string res = runCommand("/home/ahmad/spark-intereference-project/get_cache_miss_for_two_vcpus.sh server3-vm1");
    //cout << res << endl; 
    if(res == "") return 0;
    float cacheMiss=0;
    try
    {
        cacheMiss=stof(res);
    }
    catch(...)
    {
        cacheMiss=0;
    }

     return cacheMiss;
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


void computeMainMetrics(float mean, float std, float stdFactor, float &phaseChangeBound, float &quotaIncreaingBound, float &quotaIncreasingUpperBound, float &stableBound, float &lowerBound)
{
    float stdFactorized=stdFactor*std; 
    phaseChangeBound=mean + (2*stdFactorized);
    quotaIncreasingUpperBound=mean + stdFactorized; 
    quotaIncreaingBound=mean;
    stableBound=mean-stdFactorized;
    lowerBound= mean - (2*stdFactorized);

}
int main(int argc,char** argv)
{
    struct vm_info* vmInfo = new vm_info;
    struct container * conInfo = new container;

    if(argc<=7)
    {
	cout <<"windowSize, cpuIncreaseValue, cpuQuotaDecreasingRate, minCpuQuota, maxCpuQuota, stdFactor, timeToRun in order are needed"<<endl;
	exit(0);     
    }

    int windowSize;
    int cpuIncreaseValue;
    float cpuQuotaDecreasingRate;
    int minCpuQuota;
    int maxCpuQuota;
    float stdFactor;
    int timeToRun;    
    try{
    	windowSize = stoi(argv[1]);
    	cpuIncreaseValue=stoi(argv[2]);
    	cpuQuotaDecreasingRate=stof(argv[3]);
    	minCpuQuota=stoi(argv[4]);
    	maxCpuQuota=stoi(argv[5]);
    	stdFactor=stof(argv[6]); 
    	timeToRun = stoi(argv[7]);
    } 
    catch(...)
    {
	cout<<"Error in reading/converting inputs!" <<endl;
	exit(0); 
    }

    std::time_t currTime = std::time(nullptr);

    cout<<currTime<<"-"<<"input_info:"<<windowSize<<","<<cpuIncreaseValue<<","<<cpuQuotaDecreasingRate<<","<<minCpuQuota<<","<<maxCpuQuota<<","<<stdFactor<<","<<timeToRun<<endl;

  
    float phaseChangeBound=0;
    float quotaIncreaingBound=0;
    float stableBound=0;
    float lowerBound=0;
    float quotaIncreasingUpperBound=0;
    bool reFillMovingWindows=false;


    conInfo->cpuQuota = minCpuQuota;
    setCpuQuotaForDocker(conInfo->cpuQuota);
    cout << currTime <<"-"<<"setting CPU quota to min and sleep 10 seconds to make sure it got affected!" << endl; 
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    cout <<currTime <<"-"<<"monitoring IPC for first window"<<endl; 

    

    for (int i=0;i<windowSize;i++)
    {
	
	float ipc=getMeanIPC();

	float llcMisses=getMeanLLCLoadMisses();
	float cacheMisses=getMeanCacheMisses();

	vmInfo->window.push_back(ipc);
	vmInfo->sum += ipc;
	vmInfo->sum2 += (ipc*ipc);
	vmInfo->windowSize++; 
	vmInfo->mean = (vmInfo->sum / vmInfo->windowSize); 
        vmInfo->stdeviation= sqrt((vmInfo->sum2/vmInfo->windowSize)-(vmInfo->mean*vmInfo->mean));	
        
        cout <<currTime <<"-"<<"ipc_info: "<< ipc << "," << vmInfo->mean <<","<<vmInfo->stdeviation<<"," << lowerBound <<"," <<stableBound << ","<<quotaIncreaingBound<<","<<quotaIncreasingUpperBound<<","<<phaseChangeBound <<"," <<reFillMovingWindows<<"," <<llcMisses <<"," << cacheMisses << ","<<conInfo->cpuQuota<<endl;
	

    	timeToRun--;
	if(timeToRun<0)
	{
		cout<<currTime <<"-"<<"time to run is done!" << endl; 
		return 0; 
	}
	currTime = std::time(nullptr);
    }
    cout << currTime <<"-"<< vmInfo->windowSize << " IPCs are collected-moving to main loop" <<endl;

    bool quotaIncreasing=true; 
   
    
    computeMainMetrics(vmInfo->mean,vmInfo->stdeviation,stdFactor,phaseChangeBound, quotaIncreaingBound, quotaIncreasingUpperBound, stableBound, lowerBound);
    while(1)
    {

	float ipc=getMeanIPC(); 
	float llcMisses=getMeanLLCLoadMisses();
        float cacheMisses=getMeanCacheMisses();

	cout <<currTime <<"-"<<"ipc_info: "<< ipc << "," << vmInfo->mean <<","<<vmInfo->stdeviation<<"," << lowerBound <<"," <<stableBound << ","<<quotaIncreaingBound<<","<<quotaIncreasingUpperBound<<","<<phaseChangeBound <<"," <<reFillMovingWindows<<"," <<llcMisses <<"," << cacheMisses <<","<<conInfo->cpuQuota<< endl;
	 	
	if(!reFillMovingWindows)
	{
		if(ipc>phaseChangeBound)
		{
			conInfo->cpuQuota = minCpuQuota;
                	cout<<currTime <<"-"<<"passing_uperbound-quota_info: deacresing quota to " << conInfo->cpuQuota << endl;
                	setCpuQuotaForDocker(conInfo->cpuQuota);
			reFillMovingWindows=true; 
			vmInfo->window.clear();
			vmInfo->sum=0;
			vmInfo->sum2=0; 
		
		}
		else if(ipc>=quotaIncreaingBound && ipc<=quotaIncreasingUpperBound)
		{
			//conInfo->cpuQuota = min((int)(conInfo->cpuQuota*cpuQuotaIncreasingRate),maxCpuQuota);
			conInfo->cpuQuota = min((int)(conInfo->cpuQuota+cpuIncreaseValue),maxCpuQuota);
			cout<<currTime <<"-"<<"in_safe_area-quota_info: increasing quota to, " << conInfo->cpuQuota << endl;
			setCpuQuotaForDocker(conInfo->cpuQuota);
 
		}
		else if(ipc>lowerBound && ipc <= stableBound)
		{
			conInfo->cpuQuota = max((int)(conInfo->cpuQuota*cpuQuotaDecreasingRate),minCpuQuota);
                        cout<<currTime <<"-"<<"in_unsafe_area-quota_info: deacreasing quota to, " << conInfo->cpuQuota << endl;
                        setCpuQuotaForDocker(conInfo->cpuQuota);
		}
		else if(ipc<lowerBound)
		{
			conInfo->cpuQuota = minCpuQuota;
			cout<<currTime <<"-"<<"passing_lower_bound-quota_info: deacresing quota to " << conInfo->cpuQuota << endl;
			setCpuQuotaForDocker(conInfo->cpuQuota);
			reFillMovingWindows=true;
			vmInfo->window.clear();  	
			vmInfo->sum=0;
                        vmInfo->sum2=0;
		}
	}
	if(!reFillMovingWindows)
	{
		vmInfo->sum -= vmInfo->window[0];
        	vmInfo->sum2 -= (vmInfo->window[0]*vmInfo->window[0]);
        	vmInfo->window.erase(vmInfo->window.begin());

        	vmInfo->window.push_back(ipc);
        	vmInfo->sum += ipc;
        	vmInfo->sum2 += (ipc*ipc);
        	vmInfo->mean = (vmInfo->sum / vmInfo->windowSize);
        	vmInfo->stdeviation= sqrt((vmInfo->sum2/vmInfo->windowSize)-(vmInfo->mean*vmInfo->mean));
    	}
	else
	{
		vmInfo->window.push_back(ipc);
                vmInfo->sum += ipc; 
                vmInfo->sum2 += (ipc*ipc);
		int wSize=vmInfo->window.size();
                vmInfo->mean = (vmInfo->sum / wSize);
                vmInfo->stdeviation= sqrt((vmInfo->sum2/wSize)-(vmInfo->mean*vmInfo->mean));
		if(wSize>=vmInfo->windowSize)
		{	
			reFillMovingWindows=false;
			computeMainMetrics(vmInfo->mean,vmInfo->stdeviation,stdFactor,phaseChangeBound,quotaIncreaingBound,quotaIncreasingUpperBound,stableBound,lowerBound);
		}
	
	}

	timeToRun--;
    	if(timeToRun<0)
        {
                cout<<currTime <<"-"<<"time to run is done!" << endl;

                runCommand("sudo sh -c \"echo \"-1\" > /sys/fs/cgroup/cpu,cpuacct/docker/cpu.cfs_quota_us\"");
		return 0;
        }

    } 

    return 0; 
}

