import sys
import libvirt
import subprocess
import os
import time
import docker
import json
#import __future__ import print_function
conn  = libvirt.open('qemu:///system')
domainIDs = conn.listDomainsID()
ps = subprocess.check_output(["docker","ps"])
line = ps.splitlines()[1]
id_list = line.split()
container_id = id_list[0]
for ids in  domainIDs:
    command = 'virsh schedinfo  --set cpu_shares=20048 '+str(ids)+' --live'
    os.system(command)
    print ids
os.system('docker update --cpu-shares 1000 '+ container_id)
f=open('output_diff_share.txt','w')
vm=0
timer=0
paused = False
while(timer < 800):
    vm=subprocess.check_output(["virsh","nodecpustats --percent"])
    docker_stats = subprocess.check_output(["docker","stats","--no-stream"])
    dock_cpu = docker_stats.splitlines()[1]
    cpu_stats = dock_cpu.split( )[1]
    #f.write(str(cpu_stats) + ' ')
    calc_cpu = float(cpu_stats[:-1])
    #print calc_cpu
    f.write(str(calc_cpu)+' ')
    vm_cpu  = vm.splitlines()[0]
    cpu_usage = vm_cpu.split(':')
    usage = cpu_usage[1]
    usage_calc = float(usage[:-1])
    print usage_calc
    if(usage_calc*16 > 1500):
	#print "yes"
        if(paused==False):
                #os.system('docker pause ' + container_id)
                os.system('docker update --cpu-quota=500000 ' + container_id)
                pause_time = timer
                #print "haa"
                paused=True
    else:
        if(paused==True and timer-pause_time > 60):
                os.system('docker update --cpu-quota=-1 ' + container_id)
                paused=False
    vm_cpu_usage = (1600*int(usage_calc)/100) - calc_cpu
    f.write(str(vm_cpu_usage))
    f.write("\n")
    time.sleep(1)
    timer = timer +1
f.close()

