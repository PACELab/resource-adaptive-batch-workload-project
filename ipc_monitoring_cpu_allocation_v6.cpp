#include<iostream>
#include<string>
#include<algorithm>
#include<sstream>
#include<map>
#include<vector>
#include<numeric>
#include<fstream>
#include<thread>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<chrono>
#include<thread>
#include<ctime>                                // time

using namespace std;

//global var
char* scavenger_home;


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

    double currentIPC;
    double mean;
    double stdeviation;
    double sum; 
    double sum2;
    vector<double> window; 
    int windowSize; 
};

std::string runCommand(const char *s)
{
    const int max_buffer = 2048;
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

pair<double,double> findMeanAndSTD(std::vector<double>& a)
{
    if(a.size() == 0)
        return make_pair(0.0,0.0);
    double sum = 0;
    double sq_sum = 0;
    for(int i = 0; i < a.size(); ++i) {
       sum += a[i];
       sq_sum += a[i] * a[i];
    }
    double mean = sum / a.size();
    double variance = (sq_sum / a.size()) - (mean * mean);
    return make_pair(mean,sqrt(variance));
}

void monitorIPC(long int pid, bool shallWait)
{
    char command[1000]="sudo sh -c \"";
    strcat(command,scavenger_home);
    strcat(command,"/call_perf_for_given_pid_with_wait_option.sh ");
    strcat(command,to_string(pid).c_str());
    if(shallWait)
    {
      strcat(command," 1");
    }
    strcat(command,"\"");
    std::string res = runCommand(command);

}


double readLastData(long int pid, std::string itemName)
{
    char command[1000]="";
    strcat(command,scavenger_home);
    strcat(command,"/get_");
    strcat(command,itemName.c_str());
    strcat(command,"_from_pid_out.sh ");
    strcat(command,to_string(pid).c_str());
    std::string res = runCommand(command);
    if(res == "") return 0;
    double retVal=0;
    try
    {
        retVal=stof(res);
    }
    catch(...)
    {
       	retVal=0;
    }

    return retVal;
}

double readAndComputAvg(vector<long int> pids,std::string itemName)
{
    int nonZeroCount=0;
    double nonZeroSum=0.0;

    //cout<<itemName<<": ";
    for(int i=0;i<pids.size();i++)
    {
        double item=readLastData(pids[i],itemName);
        if(item!=0)
        {
              
	 	nonZeroCount++;
		//cout<<item<<" ";
                nonZeroSum+=item; 
        }
    }
    //cout<<endl;

    if(nonZeroCount==0)
        return 0.0;
    else 
        return nonZeroSum/nonZeroCount;

}


double getMeanIPC(int pidCount, string pidsString, vector<long int> pids, double intervalLength)
{
    char command[1000]="sudo sh -c \"";
    strcat(command,scavenger_home);
    strcat(command,"/ipc_scripts/monitor_");
    strcat(command,to_string(pidCount).c_str());
    strcat(command,"_pid_v2.sh ");
    strcat(command,pidsString.c_str());
    strcat(command," ");
    strcat(command,to_string(intervalLength).c_str());
    strcat(command,"\"");
    //cout << command << endl; 

    std::string res = runCommand(command);
    if(res == "") return 0;
    double retVal=0;
    try
    {
        retVal=stof(res);
    }
    catch(...)
    {
        retVal=0;
    }
    return retVal;
    //return readAndComputAvg(pids,"ipc");
}



double getMeanIPC(vector<long int> pids)
{ 
    int len=pids.size();
    if(len==0)
	return 0;
    
    for(int i=0;i<len-1;i++){
    	monitorIPC(pids[i],false);
    }
    monitorIPC(pids[len-1],true);
    return readAndComputAvg(pids,"ipc"); 
}


double getMeanLLCLoadMisses(vector<long int> pids)
{
	return readAndComputAvg(pids,"llc_load_misses");
}
double getMeanCacheMisses(vector<long int> pids)
{
	return readAndComputAvg(pids,"cache_misses");
}


int getVcpuCount(std::string vmName)
{

    char command[1000]="";
    strcat(command,scavenger_home);
    strcat(command, "/get_vcpu_count_for_given_vm.sh ");
    strcat(command,vmName.c_str()); 
    std::string res = runCommand(command);
    //cout << res << endl; 
    if(res == "") return 0;
    int vcpuCount=0;
    try
    {
        vcpuCount=stoi(res);
    }
    catch(...)
    {
        vcpuCount=0;
    }

     return vcpuCount;


}

long int getPidForGivenVcpu(std::string vmName, int vcpuNum)
{
    char command[1000]="";
    strcat(command,scavenger_home);
    strcat(command,"/get_pid_for_given_vcpu.sh ");
    strcat(command,vmName.c_str());
    strcat(command," ");
    strcat(command,to_string(vcpuNum).c_str());

    std::string res = runCommand(command);
    //cout << res << endl; 
    if(res == "")
    {
	throw "error in getting PID!";
	return 0;  
    }
    long int pid=0;
    try
    {
        pid=stoi(res);
    }
    catch(...)
    {
        throw "error in getting PID!"; 
        return 0;
    }

    return pid;
}

vector<long int> getPIDs(std::string vmName)
{
	vector<long int> res;
	int vcpuCount=getVcpuCount(vmName);
	for(int i=0;i<vcpuCount;i++)
	{
		res.push_back(getPidForGivenVcpu(vmName,i));  
	}

	return res; 
}


void setCpuQuotaForDocker(int cpuQuota, int cpuQuotaFlag)
{
	if(cpuQuotaFlag!=0)
	{
		char command[1000]="sudo sh -c \"";
		strcat(command,scavenger_home);
		strcat(command, "/set_cpu_quota_for_docker.sh ");
		std::string quota = std::to_string(cpuQuota);
		strcat(command,quota.c_str());
		strcat(command, "\"");
		//cout << command << endl;
		runCommand(command);
	}

}


void computeMainMetrics(double mean, double std, double stdFactor, double &phaseChangeBound, double &quotaIncreaingBound, double &quotaIncreasingUpperBound, double &stableBound, double &lowerBound)
{
    double stdFactorized=stdFactor*std; 
    
    phaseChangeBound=mean + (2*stdFactorized);
    quotaIncreasingUpperBound=mean + stdFactorized; 
    //quotaIncreaingBound=mean;
    stableBound=mean-stdFactorized;
    quotaIncreaingBound=stableBound;
    lowerBound= mean - (2*stdFactorized);
    /*phaseChangeBound=mean + stdFactorized;
    quotaIncreasingUpperBound=phaseChangeBound; 
    //quotaIncreaingBound=mean;
    stableBound=mean-stdFactorized;
    quotaIncreaingBound=stableBound; 
    lowerBound= stableBound;*/
}

std::chrono::system_clock::rep time_since_epoch(){ 
    static_assert(
        std::is_integral<std::chrono::system_clock::rep>::value,
        "Representation of ticks isn't an integral value."
    );
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(now).count();
}

void resetVmInfo(vm_info* vmInfo)
{

	vmInfo->window.clear();
	vmInfo->sum=0;
	vmInfo->sum2=0;
	vmInfo->mean=0;
    	vmInfo->stdeviation=0;

}

int main(int argc,char** argv)

{
    scavenger_home = getenv ("Scavenger_Home");
    cout<<scavenger_home<<endl; 
    if(!scavenger_home)
    {
	cout<<"please define Scavenger_Home Env"<<endl;
	return -1; 
    }
    struct vm_info* vmInfo = new vm_info;
    struct container * conInfo = new container;

    if(argc<9)
    {
	cout <<"Please provide windowSize, quotaChaneFactor, minCpuQuota, maxCpuQuota, stdFactor, timeToRun, VM name, cpuQuotaFlag, intervalLength (default: 1s) in second; in order"<<endl;
	exit(0);     
    }

    int windowSize;
    double quotaChaneFactor; 
    int minCpuQuota;
    int maxCpuQuota;
    double stdFactor;
    int timeToRun;    
    std::string vmName; 
    int cpuQuotaFlag; 
    double intervalLength=1; 

    try{
    	windowSize = stoi(argv[1]);
    	quotaChaneFactor=stof(argv[2]);
    	minCpuQuota=stoi(argv[3]);
    	maxCpuQuota=stoi(argv[4]);
    	stdFactor=stof(argv[5]); 
    	timeToRun = stoi(argv[6]);
	vmName=argv[7];
	cpuQuotaFlag=stoi(argv[8]);
	if(argc==10)
	{
		intervalLength=stof(argv[9]);
	}
    } 
    catch(...)
    {
	cout<<"Error in reading/converting inputs!" <<endl;
	exit(0); 
    }

	cout<<time_since_epoch()<<"-"<<"input_info:"<<windowSize<<","<<quotaChaneFactor<<","<<minCpuQuota<<","<<maxCpuQuota<<","<<stdFactor<<","<<timeToRun<<","<<vmName<<","<<cpuQuotaFlag<<","<<intervalLength<<endl; 
    
    timeToRun=(int)(timeToRun/intervalLength); 
    double cpuQuotaIncreaseRate=1+(quotaChaneFactor/100);
    double cpuQuotaDecreasingRate=0.5; 

    cout<<time_since_epoch()<<"-"<<"action_info:"<<cpuQuotaIncreaseRate<< " " << cpuQuotaDecreasingRate << endl; 
 
    vector<long int> pids=getPIDs(vmName);
    string pidsString;
    cout<<"pids:" <<pids.size()<<" ";
    for(int i=0; i<pids.size();i++)
    {
	cout <<pids[i]<<" "; 
	pidsString+=to_string(pids[i]);
	pidsString+=" "; 
    }
    cout<<endl; 

    double phaseChangeBound=0;
    double quotaIncreaingBound=0;
    double stableBound=0;
    double lowerBound=0;
    double quotaIncreasingUpperBound=0;
    bool reFillMovingWindows=false;
    
    
    if(cpuQuotaFlag!=0)
    {
    	conInfo->cpuQuota = minCpuQuota;
    	setCpuQuotaForDocker(conInfo->cpuQuota,1);
    	cout << time_since_epoch() <<"-"<<"setting CPU quota to min and sleep 10 seconds to make sure it got affected!" << endl; 
    	std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }
    else
    {
	
	conInfo->cpuQuota = maxCpuQuota;
        setCpuQuotaForDocker(conInfo->cpuQuota,1);
        cout << time_since_epoch() <<"-"<<"setting CPU quota to max and sleep 10 seconds to make sure it got affected!" << endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }
    cout <<time_since_epoch()<<"-"<<"monitoring IPC for first window"<<endl; 

    

    for (int i=0;i<windowSize;i++)
    {
	
	//double ipc=getMeanIPC(pids);
	double ipc=getMeanIPC(pids.size(),pidsString,pids,intervalLength);
	//double llcMisses=getMeanLLCLoadMisses(pids);
	//double cacheMisses=getMeanCacheMisses(pids);
	if(ipc!=0)
	{
		vmInfo->window.push_back(ipc);
		vmInfo->sum += ipc;
		vmInfo->sum2 += (ipc*ipc);
		vmInfo->windowSize++; 
		vmInfo->mean = (vmInfo->sum / vmInfo->windowSize); 
        	vmInfo->stdeviation= sqrt((vmInfo->sum2/vmInfo->windowSize)-(vmInfo->mean*vmInfo->mean));	
        
        	//cout <<time_since_epoch()<<"-"<<"ipc_info: "<< ipc << "," << vmInfo->mean <<","<<vmInfo->stdeviation<<"," << lowerBound <<"," <<stableBound << ","<<quotaIncreaingBound<<","<<quotaIncreasingUpperBound<<","<<phaseChangeBound <<"," <<reFillMovingWindows<<"," <<llcMisses <<"," << cacheMisses << ","<<conInfo->cpuQuota<<endl;
	  	cout <<time_since_epoch()<<"-"<<"ipc_info: "<< ipc << "," << vmInfo->mean <<","<<vmInfo->stdeviation<<"," << lowerBound <<"," <<stableBound << ","<<quotaIncreaingBound<<","<<quotaIncreasingUpperBound <<","<<phaseChangeBound <<"," <<reFillMovingWindows<<","<<conInfo->cpuQuota<<endl;

	}
	else
	{
		i--; 	
	}

    	timeToRun--;
	if(timeToRun<0)
	{
		cout<<time_since_epoch()<<"-"<<"time to run is done!" << endl; 
		return 0; 
	}
    } 


    cout << time_since_epoch() <<"-"<< vmInfo->windowSize << " IPCs are collected-moving to main loop" <<endl;
 
    bool quotaIncreasing=true; 
   
    
    computeMainMetrics(vmInfo->mean,vmInfo->stdeviation,stdFactor,phaseChangeBound, quotaIncreaingBound, quotaIncreasingUpperBound, stableBound, lowerBound);
   

     while(1)
    {

	//double ipc=getMeanIPC(pids); 
	double ipc=getMeanIPC(pids.size(),pidsString,pids,intervalLength);

	if(ipc!=0)
	{
		cout <<time_since_epoch()<<"-"<<"ipc_info: "<< ipc << "," << vmInfo->mean <<","<<vmInfo->stdeviation<<"," << lowerBound <<"," <<stableBound << ","<<quotaIncreaingBound<<","<<quotaIncreasingUpperBound<<","<<phaseChangeBound <<"," <<reFillMovingWindows<<","<<conInfo->cpuQuota<< endl;


		if(!reFillMovingWindows)
		{
			if(ipc>phaseChangeBound)
			{
				conInfo->cpuQuota = minCpuQuota;
                		cout<<time_since_epoch()<<"-"<<"passing_uperbound-quota_info: deacresing quota to " << conInfo->cpuQuota << endl;
				setCpuQuotaForDocker(conInfo->cpuQuota,cpuQuotaFlag);
				reFillMovingWindows=true;
				resetVmInfo(vmInfo); 
			}
			else if(ipc>=quotaIncreaingBound && ipc<=quotaIncreasingUpperBound)
			{
				//conInfo->cpuQuota = min((int)(conInfo->cpuQuota*cpuQuotaIncreasingRate),maxCpuQuota);
				conInfo->cpuQuota = min((int)(conInfo->cpuQuota+quotaChaneFactor),maxCpuQuota);
				cout<<time_since_epoch()<<"-"<<"in_safe_area-quota_info: increasing quota to, " << conInfo->cpuQuota << endl;
				setCpuQuotaForDocker(conInfo->cpuQuota,cpuQuotaFlag);
 
			}
			else if(ipc>=lowerBound && ipc < stableBound)
			{
				conInfo->cpuQuota = max((int)(conInfo->cpuQuota*cpuQuotaDecreasingRate),minCpuQuota);
                        	cout<<time_since_epoch()<<"-"<<"in_unsafe_area-quota_info: deacreasing quota to, " << conInfo->cpuQuota<< endl;
				setCpuQuotaForDocker(conInfo->cpuQuota,cpuQuotaFlag);
			}
			else if(ipc<lowerBound)
			{
				conInfo->cpuQuota = minCpuQuota;
				cout<<time_since_epoch()<<"-"<<"passing_lower_bound-quota_info: deacresing quota to " << conInfo->cpuQuota << endl;
				setCpuQuotaForDocker(conInfo->cpuQuota,cpuQuotaFlag);
				reFillMovingWindows=true;
				resetVmInfo(vmInfo); 
			}
		}
		/*
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
    		}*/
		else
		{
			vmInfo->window.push_back(ipc);
                	vmInfo->sum += ipc; 
                	vmInfo->sum2 += (ipc*ipc);
			int wSize=vmInfo->window.size();
               		vmInfo->mean = (vmInfo->sum / wSize);
               		double variance=(vmInfo->sum2/wSize)-(vmInfo->mean*vmInfo->mean); 
			if(variance>0)			
				vmInfo->stdeviation = sqrt(variance);
			else
				vmInfo->stdeviation = 0;
			if(wSize>=vmInfo->windowSize)
			{	
				reFillMovingWindows=false;
				computeMainMetrics(vmInfo->mean,vmInfo->stdeviation,stdFactor,phaseChangeBound,quotaIncreaingBound,quotaIncreasingUpperBound,stableBound,lowerBound);
			}
	
		}

	}
	timeToRun--;
    	if(timeToRun<0)
        {
                cout<<time_since_epoch()<<"-"<<"time to run is done!" << endl;

                runCommand("sudo sh -c \"echo \"-1\" > /sys/fs/cgroup/cpu,cpuacct/docker/cpu.cfs_quota_us\"");
		return 0;
        }
	
    } 

    return 0; 
}

