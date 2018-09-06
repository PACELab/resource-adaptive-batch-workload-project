#include<iostream>
#include<string>
#include<algorithm>
#include<sstream>
#include<map>
#include<thread>

using namespace std;

typedef unsigned long int uint64_t;

struct vm_info
{
	string name;
	uint64_t currentMemory;
    uint64_t  maxPeak;
    uint64_t  maxPeakHelper;
    uint64_t  memoryReserved;
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
        vm_not_in_use[i]=0;
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
    	
    	    if(vm[i].currentMemory >= 0.9*(vm[i].memoryReserved))
		    {
		        //talk to ahmad and fill his code here
                cout<<"reached here......"<<endl;
		    	vm_not_in_use[i] = maxPeakTimer>=5?3:2;
		        vm[i].memoryReserved = vm[i].original_limit;
		        cout<<vm[i].memoryReserved<<" "<<vm[i].name<<endl;
		    }

		    vm[i].maxPeakHelper = max(vm[i].maxPeakHelper,vm[i].currentMemory);

    	}

    	if(maxPeakTimer==0)
    	{
    		for(int i=0;i<vm_count;i++)
    		{
    			if(vm_not_in_use[i]>0)
    			{
    				vm_not_in_use[i]--;
    			}

	    		vm[i].maxPeak = max(vm[i].maxPeakHelper,vm[i].original_limit/10);
	    		//reset max peak helper 
	    		vm[i].maxPeakHelper=0;

	    		uint64_t toBeClaimed = vm[i].memoryReserved - (1.25*vm[i].maxPeak);
	    		
	    		if(toBeClaimed > 1436549 and vm_not_in_use[i] == 0)
	    		{
	    			vm_not_in_use[i] = 2;
	    			vm[i].memoryReserved -= toBeClaimed;
	    			//Claim what is to be claimed here

	    			cout<<vm[i].memoryReserved<<" "<<vm[i].name<<endl;
	    			cout<<"Claiming Memory from the VM to be given to the container"<<endl;
	    		}
    		}
    	}

    	maxPeakTimer++;
    	maxPeakTimer%=10;
        std::this_thread::sleep_for (std::chrono::seconds(1));    	
    }

	return 0;

}
