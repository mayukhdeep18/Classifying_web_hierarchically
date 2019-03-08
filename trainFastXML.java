import java.io.*;
import java.util.ArrayList;
import java.util.Iterator;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.Scanner;
import java.util.Map;
import java.util.HashMap;

public class trainFastXML {

	static wordMapIndex wordIndex; 

	public static void AddTrainData(String location,treeFastXML DataTree){
		ArrayList<File> TrainSet=walk(new File(location),new ArrayList<File>());
		File currentFile;
		int totalFiles = TrainSet.size();
		Iterator<File> FileIter=TrainSet.iterator();
		int i=0;int j=0;
		while(FileIter.hasNext()){
			System.out.println("Files to go: "+totalFiles);
			totalFiles--;
			currentFile=FileIter.next();
			String[] TreePath=currentFile.getName().split("-");
			if (!DataTree.checkIfPath2NodeExists(TreePath)){
				i++;
				System.out.print("*************** Adding *******************"+currentFile+"\n");
				DataTree.addPathNode(TreePath,currentFile);
			}
			else{
				j++;
			}
		}
	}
	public static void TestData(String location,treeFastXML DataTree){
		//do some stuff
		treeFastXML.node thisNode=DataTree.getHead();//get head of the tree
		//do some loopy stuff!
	}
	
	public static ArrayList<File> walk(File dir, ArrayList<File> result){
		File listFile[] = dir.listFiles();
	        if (listFile != null) {
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
	
	private static void TrainAllNodes(treeFastXML DataTree) {
		System.out.println("W12");
		treeFastXML.node thisNode=DataTree.head;
		Iterator<treeFastXML.node> childrenIter=thisNode.children.iterator();
		while(childrenIter.hasNext()){
			TrainSubtree(childrenIter.next(), "",DataTree);
		}
	}
	
	private static void TrainSubtree(treeFastXML.node thisNode, String path,treeFastXML DataTree ){
		Iterator<treeFastXML.node> childrenIter=thisNode.children.iterator();
		TrainNode(thisNode,path,DataTree);
		while(childrenIter.hasNext()){
			TrainSubtree(childrenIter.next(), path+"/"+thisNode.name,DataTree);
		}
	}
	
	private static void TrainNode(treeFastXML.node thisNode,String path,treeFastXML DataTree) {
		if(thisNode.children==null){
			return ;
		}
		ArrayList<treeFastXML.featureVector> features=DataTree.getSubtreeData(thisNode); 
		//now i have feature and path!
		if(features.size()>1)
			CallFastXML(features,path,thisNode.name);
	}


	private static void CallFastXML(ArrayList<treeFastXML.featureVector> features,	String path, String nodeName) {
		int n = features.size();

		System.out.println("New Classifier for this node->"+nodeName+"->"+Integer.toString(n));

		//open temp file to write this features in libsvm format and label mapping
		String tempFileName = "FastXMLData/feat/tempLibSVMFeature_"+nodeName+".txt"; 
		File fout = new File(tempFileName);
		String labelMapFileName = "FastXMLData/label/labelLibSVM_"+nodeName+".txt"; 
		File fout2 = new File(labelMapFileName);

		Map<String, String> labelMap = new HashMap<String, String>();
		int labelIndex = 0;
		try
		{
			PrintWriter printer = new PrintWriter(fout);
			PrintWriter printer2 = new PrintWriter(fout2);
			for(int i = 0; i < n; i++)
			{
				treeFastXML.featureVector tempF;
				tempF = features.get(i);
				
				//writing label
				if(labelMap.get(tempF.label)==null)
				{
					labelMap.put(tempF.label,Integer.toString(labelIndex));
					labelIndex++;
				}
				printer.write(labelMap.get(tempF.label));

				//writing data
				System.out.println(wordIndex.wordMap.size());
				System.out.println(tempF.data.length);
				if(tempF.data.length != wordIndex.wordMap.size())
					System.out.println("Mismatch in word length");
				else
				{
					for(int j=0;j<tempF.data.length;j++)
					{
						if(tempF.data[j]!=0)
						{
							printer.write(" ");
							printer.write(Integer.toString(j)+":"+Float.toString(tempF.data[j]));
						}
					}
				}

				//go to new line
				printer.write("\n");
				printer.flush();
			}

			//write label mapping to a file
			for ( Map.Entry<String, String> entry : labelMap.entrySet() ) {
			    String key = entry.getKey();
			    String value = entry.getValue();

			    printer2.write(key+":"+value+"\n");
			}
			printer2.flush();
		}
    	catch(FileNotFoundException e) {
        	System.err.println("File not found. Please scan in new file."+fout);
    	}

		CallUsingTermi(tempFileName,nodeName);
	}
	
	
	private static void CallUsingTermi(String fileName, String nodeName) {
		// ./train -s 0 -t 50 -m 10 -l 10 -c 1.0 -b 1.0 "Data/BibTeX/trn_data.txt" "model"

		String ss = "./train -l 1 -t 45 \""+fileName+"\""+" \"FastXMLData/Models/model_"+nodeName+"\"";
		System.out.print(ss+"\n");
		String[] command = {
				"/bin/sh",
				"-c",
				ss
				};

				try {
					Process p = Runtime.getRuntime().exec(command);
					try{
						p.waitFor();	
					}
					catch (InterruptedException e) {
					e.printStackTrace();
					}	
				} catch (IOException e) {
					System.out.print(",,,,,,,,,,,,,,,,,yayayaya");
					e.printStackTrace();
				}
		
	}

	public static void main(String[] args){
		
		//files
		String wordMapFile = "datafiles/wordIndexMap.txt";
		String folderTrain="trainF";
		String folderTest = "testF";

		//reading word indexing
		wordIndex = new wordMapIndex(wordMapFile);		
		System.out.print(wordIndex.wordMap.get("comput"));

		//create training tree data
		treeFastXML DataTree=new treeFastXML();

		System.out.print("*************** Adding Training Data *******************\n");
		AddTrainData(folderTrain,DataTree);
		DataTree.printGui();
		
		//partition train and test
		//run CreateValidationSet.py

		//create training models for each node
		//TrainAllNodes(DataTree);

		System.out.print("*************** Training Done *******************\n");
		TestModel testModel = new TestModel();
		testModel.TestData(folderTest,DataTree);
	}
}
