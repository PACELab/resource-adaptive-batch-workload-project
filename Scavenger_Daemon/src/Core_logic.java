import java.util.*;
import java.util.concurrent.TimeUnit;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.lang.*;
import java.util.concurrent.TimeUnit;
import static java.lang.System.out;


public class Core_logic extends Mem_Analytics{
	
	public static void setNodeResources(double bgMem, Mem_Analytics vm) {
		String cm1 = "/usr/local/hadoop/bin/yarn rmadmin  -updateNodeResource bay7:40208 memory-mb=";
                cm1 = cm1.concat(Double.toString(bgMem));
                cm1 = cm1.concat("Mi,vcores=4");
                vm.runCommand(cm1);
        }
 	public static void main(String[] args) {
		 if (args.length <= 1) {
			 System.out.println("./a.out <container_reserved_memory_in_gb> <init_window_size> <PHASE_CHANGE_SIZE> <GUARD_STEP_SIZE> <RECLAM_PCT> <CONTAINER RECLAIM SIZE> <FG_RESERVED_CLOSENESS_PCT" );
			 System.exit(0);
		 }
		 
		 Mem_Analytics vms = new Mem_Analytics();
		 Container container =  new Container();
		 
		 vms.window_size = Integer.parseInt(args[2]);
		 double reclaim_pct = Double.parseDouble(args[5]);
		 double fg_reserved_pct = Double.parseDouble(args[7]);
		 double container_reclaim_size =  Double.parseDouble(args[6]);
		 int PHASE_CHANGE_SIZE = Integer.parseInt(args[3]);
		 double GUARD_STEP_SIZE = Double.parseDouble(args[4]);
		 double violations,phasechanges,guardMem;

	         String val = vms.runCommand("cat /proc/meminfo | grep MemTotal | awk '{print $2}'");
		 vms.original_limit = Integer.parseInt(val)/1048576;
		 //Reserving some memory for Bg processes
		 vms.original_limit-= Integer.parseInt(args[1]);
		 vms.fgReserved = vms.getTotalActualMemory();
		 container.bgReserved = vms.original_limit - vms.fgReserved;
		 container.bgunused = container.bgReserved % container_reclaim_size;
		 //Reserving in the ratio of memory used per worker thread
		 container.bgReserved -=container.bgunused;
                 setNodeResources(container.bgReserved,vms);
		 
		 //Gathering initial data for the running window.
		 for(int i = 0;i < vms.window_size;i++) {
			 double currentMemory = vms.getTotalCurrentMemory();
			 vms.sum+=currentMemory;
			 vms.sum2+=(currentMemory*currentMemory);
			 vms.currentMemory = currentMemory;
			 vms.windowData.addLast(currentMemory);

		     try {
				TimeUnit.SECONDS.sleep(1);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		 }
                 //Setting some guardMemory for the VM to not run out of space
		 double minGuardMem = reclaim_pct*vms.original_limit;
		 violations=phasechanges=0;

		 vms.mean = vms.sum/(double)vms.windowData.size();
		 vms.stdeviation = Math.sqrt((vms.sum2/(double)vms.windowData.size())  - (vms.mean * vms.mean));

		 if (minGuardMem > GUARD_STEP_SIZE * vms.stdeviation){
		 			guardMem = minGuardMem;
		 }
		 else{
		 	guardMem = GUARD_STEP_SIZE*vms.stdeviation;
		 }

		 double predictedPeakbelow = vms.mean - guardMem;
		 vms.fgReserved = vms.mean + guardMem;
		 container.bgReserved = (vms.original_limit - vms.fgReserved);
		 container.bgunused = container.bgReserved % container_reclaim_size;
		 container.bgReserved -= container.bgunused;
                 setNodeResources(container.bgReserved,vms);  
                                
		 while(true) {
		 	double currentMemory = vms.getTotalCurrentMemory();
                        System.out.println(currentMemory);
		 	vms.currentMemory = currentMemory;

		 	vms.mean = vms.sum/(double)vms.windowData.size();
		 	vms.stdeviation = Math.sqrt((vms.sum2 / (double)vms.windowData.size()) - (vms.mean * vms.mean));
		 	if (minGuardMem > GUARD_STEP_SIZE * vms.stdeviation){
				 guardMem = minGuardMem;
		 	}
		 	else{
				 guardMem = GUARD_STEP_SIZE*vms.stdeviation;
		 	}
                        
                        //Appending the memory usages to the window
		 	vms.sum+=currentMemory;
		 	vms.sum2+= currentMemory*currentMemory;
		 	vms.windowData.addLast(vms.currentMemory);

                        //Pop the first guy from the window to ensure that we have exatly window_size readings
		 	if(vms.windowData.size() > vms.window_size) {
		 		vms.sum-=vms.windowData.get(0);
		 		vms.sum2-= vms.windowData.get(0) * vms.windowData.get(0);
		 		vms.windowData.removeFirst();

			}
              
                        //If we cross the upper limit, we first ingest the container unused memory 
		 	if (currentMemory >= fg_reserved_pct * (vms.fgReserved)){
		 		currentMemory-=container.bgunused;
		 		container.bgunused = 0;
			}

                        //Still not enough?Have to get it from bg reserved and re calculate the fgReserved and PPbelow
		 	if(currentMemory >= fg_reserved_pct*(vms.fgReserved)){
		 		vms.fgReserved = currentMemory + guardMem;
		 		predictedPeakbelow = currentMemory  -  guardMem;

		 		container.bgReserved = (vms.original_limit - vms.fgReserved);
		 		container.bgunused = container.bgReserved % container_reclaim_size;
		 		container.bgReserved -= container.bgunused;
		 		violations+=1;
                                setNodeResources(container.bgReserved,vms);
			}
			
			//If memory is not in use , we can give it to the bg but we need to see a pattern of 'Phase changes'
		 	if(currentMemory < predictedPeakbelow) {
		 		vms.downData.add(currentMemory);
			} else {
		 		vms.downData.clear();
			}

                        //If we have enough phase changes we recalculate the predicted limits
		 	if(vms.downData.size() >= PHASE_CHANGE_SIZE) {
		 		vms.windowData.clear();
			        //recalculate the mean std for the new Phase change window
		 		Stats stat = vms.findMeanAndSTD(vms.downData,vms.windowData);

		 		vms.mean = stat.mean;
		 		vms.stdeviation = stat.std;
		 		vms.sum = stat.sum;
		 		vms.sum2 = stat.sq_sum;

		 		double guard = GUARD_STEP_SIZE * vms.stdeviation;

		 		if(minGuardMem > guard) {
		 			guardMem = minGuardMem;
				}
		 		else {
		 			guardMem = guard;
				}
		 		vms.fgReserved = vms.mean + guardMem;
		 		predictedPeakbelow = vms.mean - guardMem;

		 		container.bgReserved = (vms.original_limit - vms.fgReserved);
		 		container.bgunused = container.bgReserved % container_reclaim_size;
		 		container.bgReserved -= container.bgunused;
                                setNodeResources(container.bgReserved,vms);
		 		vms.downData.clear();
		 		phasechanges+=1;
			}
			try {
				 TimeUnit.SECONDS.sleep(1);
			} catch (InterruptedException e) {
				 e.printStackTrace();
			}

		 }
		 
	}
}
