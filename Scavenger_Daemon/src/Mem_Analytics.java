import java.util.*;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.lang.*;

public class Mem_Analytics {

    double currentMemory;//current memory used by the Vm
    double mean;//Mean of the window of memory usages
    double stdeviation;//Sd of the window of memory usages
    double sum;//Sum of window of memory usages
    double fgReserved;//The  'total' amount of memory reserved for the foreground vms
    double sum2;
    double original_limit;
    int window_size;//Size of the window before we start recording mean and SD
    
    ArrayList <Double> downData;
    LinkedList <Double> windowData;
  
    Mem_Analytics(){
	currentMemory = mean = stdeviation = sum = fgReserved = sum2 = original_limit = 0;
	window_size =0;
        downData = new ArrayList();
        windowData = new LinkedList();
    }
    
    //Run a command on the terminal
    String runCommand(String command) {
        String[] cmd = {
		"/bin/sh",
		"-c",
		command
	};
        //System.out.println(cmd);
    	String line = "";
    	Process proc = null;
		try {
			proc = Runtime.getRuntime().exec(cmd);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    	BufferedReader reader =  
                new BufferedReader(new InputStreamReader(proc.getInputStream()));
    	StringBuilder sb1 = new 
                StringBuilder(""); 
    	
    	
        try {
			while((line = reader.readLine()) != null) {
			    sb1.append(line);
			    sb1.append("\n");
			}
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

        try {
			proc.waitFor();
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
        
        if(sb1.toString().length() > 0)
        	return sb1.toString().substring(0,sb1.toString().length() - 1);
        else
		return "";
    }

    
    Stats findMeanAndSTD(ArrayList<Double> a, LinkedList<Double> b) {
    	Stats stat = new Stats();
    	if (a.size() == 0)
    		return stat;
    	double sum = 0;
    	double sq_sum = 0;
    	for(int i = 0; i < a.size(); ++i) {
    	       b.addLast(a.get(i));
    	       sum += a.get(i);
    	       sq_sum += a.get(i) * a.get(i);
    	}
    	double mean = sum / a.size();
    	double variance = (sq_sum / a.size()) - (mean * mean);
    	stat.mean =  mean;
    	stat.sq_sum = sq_sum;
    	stat.sum = sum;
    	stat.std = Math.sqrt(variance);
    	return stat;
    }
    
    //Get the total memory currently used by the VMs
    Double getTotalCurrentMemory() {
    	String data  = runCommand("/usr/bin/virsh list | /bin/grep running | /usr/bin/awk '{print $2}'");
    	String[] lines = data.split("\r\n|\r|\n");
    	int vm_count = lines.length;
    	
    	String vm_name;
        double curr_mem_sum = 0;
        
        for(int i =0;i<vm_count;i++) {
                String cm1 = "/usr/bin/virsh dommemstat ";
                cm1 = cm1.concat(lines[i]);
                cm1 = cm1.concat(" | grep available | awk '{print $2}'");
                System.out.println(cm1);
        	String available = runCommand(cm1);
        	if(available == "") continue;
                System.out.println("Are we even here?");
        	double curr_mem = Double.parseDouble(available)/1048576;
                String cm2 = "/usr/bin/virsh dommemstat ";
                cm2 = cm2.concat(lines[i]);
                cm2 = cm2.concat(" | grep unused | awk '{print $2}'");
        	String unused = runCommand(cm2);
        	curr_mem -= Double.parseDouble(unused)/1048576;
        	curr_mem_sum+=curr_mem;
        }
        
        return curr_mem_sum;

    	
    }
    
    //Get the actual memory reserved for all VMs
    Double getTotalActualMemory() {
    	String data  = runCommand("/usr/bin/virsh list | /bin/grep running | /usr/bin/awk '{print $2}'");
    	String[] lines = data.split("\r\n|\r|\n");
    	int vm_count = lines.length;
    	
    	String vm_name;
        double curr_mem_sum = 0;
        
        for(int i =0;i<vm_count;i++) {
        	//String actual = runCommand("/usr/bin/virsh dommemstat "+lines[i]+" | grep actual | awk '{print $2}'");
        	String cm1 = "/usr/bin/virsh dommemstat ";
                cm1 = cm1.concat(lines[i]);
                cm1 = cm1.concat(" | grep actual | awk '{print $2}'");
                String actual = runCommand(cm1);
		if(actual == "") continue;
        	double curr_mem = Double.parseDouble(actual)/1048576;
        	curr_mem_sum+=curr_mem;
        }
        
        return curr_mem_sum;	
    }
}
