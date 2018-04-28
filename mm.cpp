#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <regex>
#include<unordered_map>
#include <vector>
#include <string.h>
#include <algorithm>
#include <mutex>
#include <thread>
#include <chrono>

struct dataValues
{
    int currentMemory;
    int maxPeak;
};

void setVMCurrentMemoryUsage(std::pair<std::string,struct dataValues*>& vm);

std::vector<std::thread> monitorThreads;
//swpd   free   buff  cache
// 2       3      4     5
std::vector<int> hostStats;
static std::vector<std::pair<std::string,struct dataValues*>> vm;
static std::vector<int> max_peak;



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

void startMonitoring()
{
    for(int i=0;i<vm.size();i++) {

        monitorThreads.push_back(std::thread([=] {
            int timer  = 1, maxMem =0;
            while(1) {
                std::cout<<vm[i].first<<std::endl;
                setVMCurrentMemoryUsage(vm[i]);
                std::this_thread::sleep_for (std::chrono::seconds(1));
                std::cout<<vm[i].second->currentMemory<<" ";
                maxMem = std::max(vm[i].second-> currentMemory,maxMem);
                timer++;
                if(timer==30)
                {
                    timer=0;
                    vm[i].second->maxPeak = maxMem;
                    std::cout<<"30-------------"<<vm[i].second->maxPeak<<std::endl;
                    maxMem=0;
                }
            }
        }));

    }



}

void  MemoryMonitor() {

    std::string data = runCommand("/usr/bin/virsh list| /bin/grep running");

    int j=0,i=0;

    std::stringstream ss(data);
    std::string to;


    while(!ss.eof())
    {
        std::getline(ss,to,'\n');

        std::stringstream ss1(to);

        while(!ss1.eof())
        {
            std::getline(ss1,to,' ');

            if(to.length()>0)
            {
                if(j%2==1)
                {
                    struct dataValues* dv = new struct dataValues();
                    dv->currentMemory=dv->maxPeak=0;
                    vm.push_back(make_pair(to,dv));
                }
                j++;
            }
        }

        j=0;

    }

}

void setVMCurrentMemoryUsage(std::pair<std::string,struct dataValues*>& vm)
{
     std::string data = runCommand(("/usr/bin/virsh dommemstat " + vm.first).c_str());

     std::regex r2("unused (.*)");
     std::regex r1("available (.*)");
     std::smatch s;

     regex_search(data,s,r1);
     vm.second->currentMemory = stod(s.str(1));

     regex_search(data,s,r2);

    vm.second->currentMemory -= stod(s.str(1));
}

void startProcessing()
{

    monitorThreads.push_back(std::thread([=] {

        int timer  = 1;

        while(1)
        {
            if (timer == 30)
            {
                for(int i=0;i<vm.size();i++)
                {
                    std::cout <<"30 Sec Read: "<<vm[i].second->maxPeak << std::endl;
                }
                timer=0;
            }
            std::this_thread::sleep_for (std::chrono::seconds(1));
            timer+=1;
        }
    }));


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

    std::cout<<hostStats[2]<<" ";
    std::cout<<hostStats[3]<<" ";
    std::cout<<hostStats[4]<<" ";
    std::cout<<hostStats[5]<<" ";


}


//    void operator()(std::string vm)
//    {
//        while(1)
//        {
//            setVMCurrentMemoryUsage(vm);
//            setHostCurrentMemoryUsage();
//            std::cout<<std::endl;
//            std::this_thread::sleep_for (std::chrono::seconds(1));
//        }
//
//    }


int main(int argc,char* argv[]) {

    MemoryMonitor();
    startMonitoring();
    std::this_thread::sleep_for (std::chrono::seconds(5));
    startProcessing();

    for(auto& t : monitorThreads)
    {
        t.join();
    }

    return 0;
}
