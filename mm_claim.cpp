using namespace std;
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <regex>
#include <unordered_map>
#include <vector>
#include <string.h>
#include <algorithm>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include<fstream>
#include <unistd.h>

#include <sys/eventfd.h>

#define MEMORY_TO_GIVE 1
#define ON_COOLDOWN 0

struct dataValues
{
    std::string name;
    int currentMemory;
    int  maxPeak;
    int  memoryReserved;
    double stats[100] = {0};
    ofstream outfile;
    ofstream logfile;
    int  total_claimed;
    int  original_limit;
    int state;//state which tells if we can give more memory or not.
    int cycles_left; // cycles left before next check
};

struct containerValues
{
    std::string containerID;
    double currMemory;
    ofstream con_output;
};

void setVMCurrentMemoryUsage(struct dataValues* & vm);
void monitorMemPressure(char* memoryPressureNotificationFile);
void monitorMemoryStatsByVM(char* memStatsfile,struct dataValues* dv);

std::vector<std::thread> monitorThreads;
//swpd   free   buff  cache
// 2       3      4     5
std::vector<int> hostStats;
static std::vector<struct dataValues*> vm;
static std::vector<struct containerValues*> container;


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

mutex m4;
int setToZero;

void startMonitoring()
{
    for(int i=0;i<vm.size();i++) {

        monitorThreads.push_back(std::thread([=] {
            int timer  = 1;
            int  maxMem =0;
            while(1) {
                setVMCurrentMemoryUsage(vm[i]);
                std::this_thread::sleep_for (std::chrono::milliseconds(100));
                maxMem = std::max(vm[i]-> currentMemory,maxMem);
                timer++;
                if(timer==100)
                {
                    timer=0;
                    vm[i]->maxPeak = std::max(maxMem,((vm[i]->original_limit)/10));
                    time_t t = std::time(0);
                    long int now = static_cast<long int> (t);
                    maxMem=0;
                } else if(setToZero>0)
                {
                    unique_lock<mutex> m(m4);
                    timer=0;
                    cout<<timer<<" ZERO ZERO ZZERO "<<endl;
                    setToZero--;

          	}
            }
        }));
    }
}


void  MemoryMonitor() {


    //Monitor VM

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
                    dv->original_limit = 0;
                    dv->name=to;
		    dv->state = MEMORY_TO_GIVE;
		    dv->cycles_left  = 0;

                      std::string process = "/sys/fs/cgroup/memory/machine/"+to+".libvirt-qemu/memory.pressure_level";
    //                char *cstr = new char[process.length() + 1];
      //              strcpy(cstr, process.c_str());
//                    monitorMemPressure(cstr);

                    process = "/sys/fs/cgroup/memory/machine/"+to+".libvirt-qemu/memory.stat";
                    char *cstr1 = new char[process.length() + 1];
                    strcpy(cstr1, process.c_str());
                    monitorMemoryStatsByVM(cstr1,dv);

                    vm.push_back(dv);
                }
                j++;
            }
        }
        j=0;
    }
}

void setVMCurrentMemoryUsage(struct dataValues*& vm)
{
    std::string data = runCommand(("/usr/bin/virsh dommemstat " + vm->name).c_str());

    std::regex r2("unused (.*)");
    std::regex r1("available (.*)");
    std::smatch s;

    regex_search(data,s,r1);
    if(vm->original_limit==0)
    {
        vm->original_limit = stod(s.str(1));
        vm->memoryReserved = vm->original_limit;
    }
    vm->currentMemory = stod(s.str(1));

    regex_search(data,s,r2);

    time_t t = std::time(0);
    long int now = static_cast<long int> (t);
    // cout<<"Total claimed from "<<vm->name<<" "<<vm->total_claimed<<"\n";
    vm->currentMemory -= stod(s.str(1));
    if(vm->currentMemory >= 0.9*(vm->memoryReserved))
    {
        //AHMAD'S SCRIPT RUNS HERE
        runCommand("sh killStress.sh 9ba76a756b8a ");
        vm->total_claimed=0;
        setToZero=2;
        //update the vm->memoryReserved and vm->total_claimed here.
        vm->memoryReserved = vm->original_limit;
    }

    string containerVal = runCommand("cat /sys/fs/cgroup/memory/docker/9ba76a756b8a4c6df6266a2f9f994510838cb486e3a48030cd7ee02b2bbd8c6a/memory.usage_in_bytes");
    vm->outfile.open(vm->name,std::ios_base::app);
    vm->outfile<<to_string(now)<<","<<to_string(vm->memoryReserved)<<","<<to_string(vm->currentMemory)<<","<<to_string(vm->total_claimed)<<","<<containerVal;
//    vm->outfile<<"\n";
    vm->outfile.close();
}

void claim_memory_vm(vector<dataValues *> claim_list)
{
    int total_claimed = 0;
    cout<<"original_memory " << vm[0]->original_limit<<"\n";
    for(int i=0;i<claim_list.size();i++)
    {
        int claim_val = claim_list[i]->memoryReserved  - (claim_list[i]->maxPeak + 0.25*claim_list[i]->maxPeak);
        claim_list[i]->memoryReserved = claim_list[i]->memoryReserved - claim_val;
        claim_list[i]->total_claimed+=claim_val;

        total_claimed+=claim_val;
        cout<<" current reserved "<<claim_list[i]->memoryReserved<<"\n";
        //cout<<"Total claimed from " <<claim_list[i]->name<<" "<<claim_list[i]->total_claimed<<"\n"
    }
    vm[0]->logfile.open(vm[0]->name,std::ios_base::app);
    time_t t = std::time(0);
    long int now = static_cast<long int> (t);
    vm[0]->logfile<<to_string(now)<<"total claimable by yarn Node Manager : "<<total_claimed<<"\n"; 
    vm[0]->logfile.close();

    int threads = total_claimed/32;
    //cout<<"runnable memory/16: "<<threads<<"\n";
    if(threads > 0)
    {
        runCommand(("sh runStress.sh 9ba76a756b8a  "+to_string(threads)).c_str());
        cout<<"sh runStress.sh 9ba76a756b8a  "+to_string(threads)<<endl;
    }
    //runCommand(("sh runStress.sh 898246e93082 2048m"))
}

void startProcessing()
{
    monitorThreads.push_back(std::thread([=] {
        int timer  = 1;
        vector <dataValues *> claim_list;
        while(1)
        {
            if (timer == 10)
            {
                for(int i=0;i<vm.size();i++)
                {
		    if (vm[i]->state == MEMORY_TO_GIVE ) {  
                    	int claim_val = vm[i]->memoryReserved  - (vm[i]->maxPeak + 0.25*vm[i]->maxPeak);
                    	cout<<"to be claimed "<<claim_val<<"\n";
                    	if(claim_val > 1436549){
				vm[i]->state = ON_COOLDOWN;
				vm[i]->cycles_left = 2;
                        	claim_list.push_back(vm[i]);
			}
		    }
		    else {
				if(vm[i]->cycles_left == 0) {
					vm[i]->state = MEMORY_TO_GIVE;
				} else{
					vm[i]->cycles_left--;
				}
		    }
                }
                claim_memory_vm(claim_list);
                claim_list.resize(0);
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

}
double getdValue(std::string s)
{
    int j=0;
    double ret=0;

    for(int i=0;i<s.length();i++)
    {
        if(s[i]=='.') j=i;
        else
        {
            ret= (ret*10)+ s[i]-48;
        }
    }

    return ret/(pow(10,s.length()-j));
}

std::mutex m1;

void collectContainerStats()
{
    /*std::lock_guard<std::mutex> locker(m1);
    container.clear();

    std::string data = runCommand("docker stats --format \"{{.Container}} {{.MemUsage}}\" --no-stream");

    std::stringstream ss(data);
    std::string to;

    while(!ss.eof())
    {
        std::getline(ss,to,'\n');
        std::stringstream ss1(to);
        struct containerValues* cv = new struct containerValues();
        std::getline(ss1,to,' ');
        cv->containerID = to;
        std::getline(ss1,to,' ');
        
        time_t t = std::time(0);
        long int now = static_cast<long int> (t);
        
        cv->currMemory = getdValue(to);
        std::getline(ss1,to,' ');
        if(to=="GiB")
             cv->currMemory*=1024;
        cv->con_output.open(cv->containerID,std::ios_base::app);
        cv->con_output<<to_string(now)<<","<<to_string(cv->currMemory);
        cv->con_output<<"\n";
        cv->con_output.close();
        
        if(cv->currMemory!=0)
            container.push_back(cv);
        else
            free(cv);

    }*/

    runCommand(("echo \""+to_string(vm[0]->total_claimed)+"\" >> cbc0788f0dc4.txt").c_str());

    //for(int i=0;i<container.size();i++)
    //  std::cout<<container[i]->currMemory<<" "<<container[i]->containerID<<std::endl;

}

void readContainerStats() {
    std::lock_guard<std::mutex> locker(m1);
    for(int i=0;i<container.size();i++)
        std::cout<<container[i]->currMemory<<" "<<container[i]->containerID<<std::endl;
}


void monitorMemoryStatsByVM(char* memStatsfile,struct dataValues* dv)
{

    //If using this logic :: use a mutex


    monitorThreads.push_back(std::thread([=] {

        while(1) {
            //std::cout << memStatsfile << std::endl;

            std::ifstream in(memStatsfile);


            int i = 0;


            if (!in) {
                std::cout << "Cannot open input file.\n";
                return;
            }

            std::string str;

            while (std::getline(in, str)) {


                std::stringstream ss(str);
                std::getline(ss, str, ' ');
                std::getline(ss, str, ' ');

                try {
                    dv->stats[i++] = stod(str);
                }
                catch (...) {

                }

            }

            in.close();

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    }));

}

void monitorMemPressure(char* memoryPressureNotificationFile)
{

    monitorThreads.push_back(std::thread([=] {

        //std::cout<<memoryPressureNotificationFile<<std::endl;
        int efd = -1;
        int cfd = -1;
        int event_control = -1;
        char event_control_path[PATH_MAX];
        char line[LINE_MAX];
        int ret;

        cfd = open(memoryPressureNotificationFile, O_RDONLY);
        if (cfd == -1)
            err(1, "Cannot open %s", memoryPressureNotificationFile);

        ret = snprintf(event_control_path, PATH_MAX, "%s/cgroup.event_control",
                       dirname(memoryPressureNotificationFile));
        if (ret >= PATH_MAX)
            errx(1, "Path to cgroup.event_control is too long");

        event_control = open(event_control_path, O_WRONLY);
        if (event_control == -1)
            err(1, "Cannot open %s", event_control_path);

        efd = eventfd(0, 0);
        if (efd == -1)
            err(1, "eventfd() failed");

        ret = snprintf(line, LINE_MAX, "%d %d low", efd, cfd);
        if (ret >= LINE_MAX)
            errx(1, "Arguments string is too long");



        ret = write(event_control, line, strlen(line) + 1);
        if (ret == -1)
            err(1, "Cannot write to cgroup.event_control");

        while (1) {

            uint64_t result;

            ret = read(efd, &result, sizeof(result));
            if (ret == -1) {
                if (errno == EINTR)
                    continue;
                err(1, "Cannot read from eventfd");
            }
            assert(ret == sizeof(result));

            ret = access(event_control_path, W_OK);
            if ((ret == -1) && (errno == ENOENT)) {
                puts("The cgroup seems to have removed.");
                break;
            }

            if (ret == -1)
                err(1, "cgroup.event_control is not accessible any more");

            std::this_thread::sleep_for(std::chrono::seconds(1));

            printf("\n\n\nLOW PRESSURE LEVEL REACHED\n\n\n");
            /*runCommand(("sh killStress.sh 898246e93082"));
                //update the vm->memoryReserved and vm->total_claimed here.
            vm->memoryReserved = vm->original_limit;*/
        }

    }));
}



void MonitorContainerMemory()
{
    monitorThreads.push_back(std::thread([=] {

        while (1) {
            collectContainerStats();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

    }));
}
int main(int argc,char* argv[]) {

    MemoryMonitor();
    startMonitoring();
    //MonitorContainerMemory();
    std::this_thread::sleep_for (std::chrono::seconds(5));
    startProcessing();
    //std::this_thread::sleep_for (std::chrono::seconds(5));
    //readContainerStats();

    for (auto &t : monitorThreads) {
        t.join();
    }
    //system("sh runStress.sh 898246e93082 2048m"); 
    return 0;
}
