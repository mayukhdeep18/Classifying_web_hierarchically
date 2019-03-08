import java.io.*;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Scanner;
import java.util.Collections;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import javax.xml.crypto.Data;



public class TestModel {
	static wordMapIndex wordIndex; 
	static int hardCorrect = 0;
	static int hardWrong = 0;
	static int kmodel = 4;

	public class featureVector{
		float[] data;
		String label;

		public String fToString()
		{
			return "label:" + label + " data:" + data;	
		}
	}
	public static ArrayList<File> walk(File dir, ArrayList<File> result){
		File listFile[] = dir.listFiles();
		//System.out.println(dir.getAbsolutePath());
	        if (listFile != null) {
	        	//System.out.println("W1");
	            for (int i=0; i<listFile.length; i++) {
	                if (listFile[i].isDirectory()) {
	                	
	                    walk(listFile[i],result);
	                    
	                } else {
	                	result.add(listFile[i]);
	                }
	            }
	        }
	        return result;
	}
	float[] getDataFromFile(File currentFile) {
		Scanner in;
		String[] vals=null;
		//System.out.println("gdff");
		try {
			in = new Scanner(currentFile);
			in.nextLine();
			vals=in.nextLine().split(",");
			//System.out.println(in.nextLine());
			//System.out.println(in.nextLine());
			//String[] tempVal=in.toString().split("\n");
			
			//vals=tempVal[1].split(",");
		} catch (FileNotFoundException e) {
			//System.out.println("ERROR!!");
			e.printStackTrace();
			
		}
		//System.out.println("gdff");
		float[] result=new float[vals.length-1] ;
		for (int i=1;i<vals.length;i++){
			//System.out.println(vals[i]);
			result[i-1]=(Float.parseFloat(vals[i]));
		}
		return result;
		
	}
	
	private void ClassifyTestPages(ArrayList<featureVector> testPages, treeFastXML dataTree) {
		Iterator<featureVector> pages=testPages.iterator();
		int totalPages = testPages.size();
		int  currPage = totalPages;
		while(pages.hasNext()){
			System.err.println(""+currPage+" to go");
			ClassifyPage(pages.next(),dataTree);
			currPage--;
		}
	}
	
	private void ClassifyPage(featureVector feature, treeFastXML dataTree) {
		/** Things to do here!
		 * classify over all levels+
		 * store accuracy and failure
		 * ? compile result in copied treeFastXML?**/
		System.err.println("Testing for > "+feature.label);
		System.out.println("Testing for > "+feature.label);
		String[] treePath=feature.label.split("-");
		int level=0;
		String path="";
		treeFastXML.node resultNode=classifyAtLevel(dataTree.head,path,feature.data,feature.label,treePath[level]);
		treeFastXML.node parentNode=dataTree.head;
 		
 		System.out.println("treepath length = "+treePath.length);
 		level = 0;
 		while(level < treePath.length-2)
 		{
 			if(treePath[level].equals(resultNode.name)){
 				System.out.print("at level-->" + level);
 				System.out.println(" :yolo4 " + treePath[level]+ "--::--"+resultNode.name);

				path+="/"+resultNode.name;
				resultNode.fscore.tp+=1;
				parentNode.accuracy.correct+=1;
				parentNode=resultNode;
				resultNode=classifyAtLevel(resultNode, path, feature.data,feature.label,treePath[level+1]);
				level++;
				if(resultNode==null)
				{
					System.out.print("at level-->" + level + ":" + "yolo4 null returned");
					//Never reach here
					break;
				}
			}
			else
			{
				System.out.print("at level-->" + level);
				System.out.println(" :yolo4 " + treePath[level]+ "--wrong--"+resultNode.name);
				parentNode.accuracy.wrong+=1;
				resultNode.fscore.fp+=1;
				Iterator<treeFastXML.node> childrenIter=parentNode.children.iterator();
				while(childrenIter.hasNext()){
					treeFastXML.node child=childrenIter.next();
					if (child.name.equals(treePath[level])){
						child.fscore.fn+=1;
					}
				}		
				break;
			}
 		}

 		if(level == treePath.length-2 && treePath[level].equals(resultNode.name))
 		{
 			System.out.print("at level-->" + level);
 			System.out.println(" :yolo4 " + treePath[level]+ "--::--"+resultNode.name);
 			System.out.println("----->>"+ feature.label + "<<------ correctly classified\n");
 			parentNode.accuracy.correct+=1;
			resultNode.fscore.tp+=1;
 			hardCorrect +=1;
 		}
 		else
 		{
 			System.out.print("failed at level-->" + level);
 			System.out.println(" :yolo4 " + treePath[level]+ "--++--"+resultNode.name);
 			System.out.println("-----++"+ feature.label + "++------ wrongly classified\n");
 			parentNode.accuracy.wrong+=1;
			resultNode.fscore.fp+=1;
			Iterator<treeFastXML.node> childrenIter=parentNode.children.iterator();
			while(childrenIter.hasNext()){
				treeFastXML.node child=childrenIter.next();
				if (child.name.equals(treePath[level])){
					child.fscore.fn+=1;
				}
			}	
 			hardWrong +=1;
 		}

		System.out.print("yolo Done\n");
		/**NOTE: not giving more wrongs down the line. This will be the only wrong given!!**/
	}

	private treeFastXML.node classifyAtLevel(treeFastXML.node thisNode,String path, float[] data, String fileLabel, String actName) {

		if (thisNode.children.size()==1 && thisNode.pages.size()==0)	return thisNode.children.get(0);
		treeFastXML.node dummy = thisNode;

		Map<String, String> testResult=TestFastXML(thisNode,data,fileLabel);
		System.out.println("testResult: "+testResult);
		String kk = "";
		for (Map.Entry<String, String> entry : testResult.entrySet())
		{
		    String key = entry.getKey();
		    String value = entry.getValue();
		    kk = key;
		    break;
		}

		Iterator<treeFastXML.node> childrenIter=thisNode.children.iterator();
		while(childrenIter.hasNext()){
			treeFastXML.node child=childrenIter.next();
			String childName = child.name;
			try {
			    int foo = Integer.parseInt(childName);
			    childName = "NO_CLASS";
			} catch (NumberFormatException e) {
				childName = child.name;
			}

			child.name = childName;

			for (Map.Entry<String, String> entry : testResult.entrySet())
			{
			    String key = entry.getKey();
			    String value = entry.getValue();

			    //System.out.println("child: "+child.name+" :"+key+" :"+actName);
			    if (child.name.equals(key)){
			    	if(child.name.equals(actName))
			    	{
			    		//System.out.println("hurray");
						return child;
			    	}
				}
			}

			if (child.name.equals(kk)){
		    	if(child.name.equals(actName))
					dummy = child;
			}
		}
		return dummy;
	}

	private Map<String, String> TestFastXML(treeFastXML.node thisNode, float[] data, String fileLabel) {
		String fileLabel2 = fileLabel.replaceAll("/","-" );
		String tempFileName = "FastXMLData/tempTestLibSVMFeature_"+fileLabel2+"_"+thisNode.name+".txt"; 
		File fout = new File(tempFileName);
		String maxLabel = "";
		Map<String, String> resultProbSort = new HashMap<String, String>();

		try
		{
			PrintWriter printer = new PrintWriter(fout);

			//writing data
			if(data.length != wordIndex.wordMap.size())
				System.out.println("Mismatch in word length");
			else
			{
				for(int j=0;j<data.length;j++)
				{
					if(data[j]!=0)
					{
						printer.write(" ");
						printer.write(Integer.toString(j)+":"+Float.toString(data[j]));
					}
				}
			}

			//go to new line
			printer.write("\n");
			printer.flush();

			String testFile = tempFileName;
			String modelName = "FastXMLData/Models/model_"+ thisNode.name;
			String tempScore = "FastXMLData/Scores/scores_"+fileLabel2+"_"+thisNode.name+".txt";
			String cmmd = "./predict -p 5 \""+testFile+"\" \""+modelName+"\" \""+tempScore + "\"";
			
			//System.out.println(cmmd+"\n");

			String[] command = {
				"/bin/sh",
				"-c",
				cmmd
				};

			CallUsingTermi(command);
			
			/** Read output that is written in a file!
			 * 	Using this decide the solution class, return the string!**/
			Map<String, String> resultProb = new HashMap<String, String>();
			resultProb = ReadScoreFile(tempScore,thisNode.name);
			resultProbSort = sortByValues(resultProb);

			// float max = -1f;
			// for (Map.Entry<String, String> entry : resultProb.entrySet())
			// {
			//     String key = entry.getKey();
			//     String value = entry.getValue();

			//     float v = Float.valueOf(value);
			//     for (Map.Entry<String, String> entry2 : resultProb2.entrySet())
			// 	{
			// 	    String key2 = entry2.getKey();
			// 	    String value2 = entry2.getValue();

			// 	    float v2 = Float.valueOf(value2);

			// 	    if (v1 < v2)
			// 	    {

			// 	    }
			// 	}
			// }
		}
    	catch(FileNotFoundException e) {
        	System.err.println("File not found."+fout);
    	}
    	return resultProbSort;
	}

	private static HashMap sortByValues(Map<String, String> map)
	{ 
       List list = new LinkedList(map.entrySet());
       Collections.sort(list, new Comparator() {
            public int compare(Object o1, Object o2) {
               return ( (Comparable) ((Map.Entry) (o2)).getValue() )
                  .compareTo( ((Map.Entry) (o1)).getValue() );
            }
       });

       HashMap sortedHashMap = new LinkedHashMap();
       int kCount = 0;
       for (Iterator it = list.iterator(); it.hasNext();) {
       		
       	//code for top k model
       		if(kCount > kmodel-1)
       			break;

            Map.Entry entry = (Map.Entry) it.next();
            sortedHashMap.put(entry.getKey(), entry.getValue());
            kCount++;
       } 
       System.out.println("scores: "+sortedHashMap);
       return sortedHashMap;
  	}
	
	private static void CallUsingTermi(String[] command) {
				try {
					Process p = Runtime.getRuntime().exec(command);
					try{
						p.waitFor();	
					}
					catch (InterruptedException e) {
						e.printStackTrace();
					}	
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
	}
	
	private static Map<String,String> ReadScoreFile(String scoresFile, String nodeName)
	{
		Map<String, String> result = new HashMap<String, String>();
		Map<String, String> labelMap = new HashMap<String, String>();

		//reading labels
		String labelMapFileName = "FastXMLData/label/labelLibSVM_"+nodeName+".txt"; 
		
		Scanner in;
		String[] vals=null;
		String[] vals2=null;
		try{
			File ff = new File(labelMapFileName);
			in = new Scanner(ff);
			while(in.hasNextLine())
			{
				vals=in.nextLine().split(":");
				if(vals[0]!="\n")
					labelMap.put(vals[1], vals[0]);
			}
			in.close();

			//reading scores.txt
			ff = new File(scoresFile);
			in = new Scanner(ff);
			vals=in.nextLine().split(" ");
			for(int i=0;i<vals.length;i++)
			{
				vals2 = vals[i].split(":");
				result.put(labelMap.get(vals2[0]), vals2[1]);	
			}
			in.close();
		}catch (FileNotFoundException e) {
			e.printStackTrace();
		}

		return result;
	}
	public  void TestData(String location,treeFastXML dataTree){
		String wordMapFile = "datafiles/wordIndexMap.txt";
		//reading word indexing
		wordIndex = new wordMapIndex(wordMapFile);

		treeFastXML.node thisNode=dataTree.getHead();//get head of the treeFastXML
		ArrayList<File> TestSet=walk(new File(location),new ArrayList<File>());
		File currentFile;
		ArrayList<featureVector>  testPages=new ArrayList<featureVector>();
		Iterator<File> FileIter=TestSet.iterator();
		while(FileIter.hasNext()){
			currentFile=FileIter.next();
			featureVector f=new featureVector();
			f.data=getDataFromFile(currentFile);
			f.label=currentFile.getName();
			testPages.add(f);
		}
		
		ClassifyTestPages(testPages,dataTree);
		
		System.out.println("Total Correct = "+ hardCorrect);
		System.out.println("Total Wrong = "+ hardWrong);

		System.err.println("Total Correct = "+ hardCorrect);
		System.err.println("Total Wrong = "+ hardWrong);

		if(hardWrong!=0)
		{
			double hardPercAccuracy = ( (float)hardCorrect/(hardCorrect+hardWrong) )*100;
			System.out.println("Perc Correct = "+ hardPercAccuracy);
			System.err.println("Perc Correct = "+ hardPercAccuracy);
		}
		dataTree.printfscores();
		dataTree.printSoftAccuracy();
		dataTree.getLevelResult();
	}	
}