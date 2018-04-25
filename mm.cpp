#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <regex>
#include <vector>
#include <string.h>
#include <thread>
#include <chrono>
#include <future>
using namespace std;


class Memory
{
    unsigned long long currentMemory;
    //swpd   free   buff  cache 
    // 2       3      4     5
    std::vector<int> hostStats;
    std::string vm;

public:

    Memory(int currentMemory, const std::string &vm) : currentMemory(currentMemory), vm(vm), hostStats(17,0) {}

    Memory() {}

    int getCurrentMemory() const {
        return currentMemory;
    }

    void setCurrentMemory(int currentMemory) {
        Memory::currentMemory = currentMemory;
    }

    void setVMCurrentMemoryUsage()
    {
        std::string data = runCommand(("/usr/bin/virsh dommemstat " + vm).c_str());

        std::regex r2("unused (.*)");
        std::regex r1("available (.*)");
        std::smatch s;
        
        regex_search(data,s,r1);
        currentMemory = stoi(s.str(1));
        
        regex_search(data,s,r2);
        currentMemory-= stoi(s.str(1));
        
        std::cout<<currentMemory<<" ";
        
    }

    void setHostCurrentMemoryUsage()
    {

        std::string data = runCommand("vmstat");

        std::stringstream ss(data);
        std::string to;

        int j=0;

        std::getline(ss,to,'\n');
        std::getline(ss,to,'\n');
        std::getline(ss,to,'\n');

        std::stringstream ss1(to);
        
        for(int i=0;i<30;i++)
        {
            std::getline(ss1,to,' ');
            try
            {
                int temp = stoi(to);
                hostStats[j++] = temp;
            }
            catch(...){}
        }

        /*std::cout<<hostStats[2]<<" ";
        std::cout<<hostStats[3]<<" ";
        std::cout<<hostStats[4]<<" ";
        std::cout<<hostStats[5]<<" ";

        std::cout<<hostStats[2]+hostStats[3]+hostStats[4]+hostStats[5]+currentMemory<<" ";
        */
    }

    std::string runCommand(const char *s)
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
    void change_yarn_limits(long curr_av,long prev_safe_state)
    {
       if(curr_av > prev_safe_state)
	   cout<<curr_av-prev_safe_state<<" "<<"extra memory available\n";
       if(prev_safe_state > curr_av)
	   cout<<prev_safe_state-curr_av<<" "<<"prev free memory now under use\n";
    }
    void operator()(int a)
    {
	if(a==1)
	{
        	while(1)
        	{
                        cout<<"hello";
            		setVMCurrentMemoryUsage();
            		setHostCurrentMemoryUsage();
          		std::cout<<std::endl;
            		std::this_thread::sleep_for (std::chrono::seconds(1));
        	}
	}
	else
	{
                std::this_thread::sleep_for (std::chrono::seconds(1));
		unsigned long long prev_safe_state = 0;
                unsigned long long curr_unused =0;
                int time=0;
		int cycles_count=0;
		while(1)
		{
                        //cout<<hostStats[3]<<" ";
		        setVMCurrentMemoryUsage();
                        setHostCurrentMemoryUsage();
			if(time==120)
			{
			   unsigned int curr_av = curr_unused/120;
                           cout<<"curr_av: "<<curr_av<<"\n";
			   if(abs(curr_av - prev_safe_state) > 2000000)
			   {
                               if(cycles_count==1)
			       {
				   change_yarn_limits(curr_av,prev_safe_state);
                                   cycles_count=0;
                                   prev_safe_state = curr_av;
                               }
			       else
				   cycles_count++;
			       time=0;
                 
                           }
		           curr_unused=0;
			}
		        time++;
                        std::this_thread::sleep_for (std::chrono::seconds(1));
                        unsigned long long unused = 28000000 - currentMemory;
                        cout<<"time: "<<time<<" "<<"host unused: "<<hostStats[3]<<" "<<currentMemory<<"\n";
			curr_unused+=unused;
		}
		/*string container_id = runCommand("docker ps -q");
		while(1)
		{
			if(hostStats[3] < 700000)
			{
				runCommand(("docker pause " + container_id).c_str());
                                runCommand(("docker update -m 1G "+ container_id).c_str());
                                std::this_thread::sleep_for (std::chrono::seconds(2));                          
      				runCommand(("docker unpause " + container_id).c_str());
                        }
                        else if(16000000-currentMemory  > 3000000)
			{
			        runCommand(("docker pause " + container_id).c_str());
                                runCommand(("docker update -m 2G "+ container_id).c_str());
				std::this_thread::sleep_for (std::chrono::seconds(2));
                                runCommand(("docker unpause " + container_id).c_str());
			}
		}*/
	}	

    }	

    virtual ~Memory() {}

};

int main(int argc,char* argv[]) {

    if(argc<2)
    {
        fprintf(stderr, "Input_Required : domain_name \n");
        std::exit(EXIT_FAILURE);
    }

    Memory m(0,argv[1]);
    //std::async (m,1);    
    //std::thread t(m,1);
    std::thread t1(m,2);

    //t.join();
    t1.join();

    return 0;
}
