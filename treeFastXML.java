import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Scanner;
import java.util.logging.Logger;
import java.util.Arrays;

public class treeFastXML {
	//-s 0 -t 50 -m 10 -l 10 -c 1.0 -b 1.0
	String[] fastXMLArgs = new String[6];
	public class node{
		ArrayList<node> children=new ArrayList<node>();
		String name="";
		ArrayList<float[]> pages=new ArrayList<float[]>();
		nodeAccuracy accuracy=new nodeAccuracy();
		nodeF fscore=new nodeF();
		public node()
		{
			name="LOL";
		}
	}
	
	node head=new node();
	public class nodeAccuracy{
		int wrong=0;
		int correct=0;
	}
	public class nodeF{
		int tp=0;
		int fp=0;
		int fn=0;
		//int wrong=0;
	}

	public class featureVector{
		float[] data;
		String label;
		int type; //0-> test, 1->train

		public String fToString()
		{
			return "label:" + label + " data:" + Arrays.toString(data) + '\n';	
		}
	}
	
	public node getHead(){
		return head;
	}

	public boolean checkIfPath2NodeExists(String[] treePath) {
		/*for (int kk=0;kk<treePath.length;kk++){
			System.out.print(" "+treePath[kk]);
		}
		System.out.println("/*");
		*/// TODO Auto-generated method stub
		node treeNode=head;
		node childWithName=null;
		for (int i=0; i<treePath.length;i++){
			//System.out.println(treePath[i]+treeNode.name);
			childWithName=getChildIfExist(treeNode, treePath[i]);
			if (childWithName==null){
				//System.out.print(i+"aaa/");
				return false;
			}
			treeNode=childWithName;
		}
		System.out.println("FUCCCCCCCCCAAAAAAA");
		return true;
	}

	public void addPathNode(String[] treePath, File currentFile) {
		//for (int kk=0;kk<treePath.length;kk++){
		//	System.out.print(" "+treePath[kk]);
		//}
		//System.out.println("/");
		// TODO Auto-generated method stub
		node treeNode=head;
		node childWithName=null;
		int i;
		for (i=0; i<treePath.length;i++){
			childWithName=getChildIfExist(treeNode, treePath[i]);
			if (childWithName==null){
				break; //no more path exists
			}
			else{
				treeNode=childWithName;
			}
		}
		
		//create path till leaf
		for (; i<treePath.length;i++){
			childWithName=new node();
			childWithName.name=treePath[i];
			//System.out.println(childWithName.name+"/");
			treeNode.children.add(childWithName);
			treeNode=childWithName;
		}
		//System.out.println(treeNode.name);
		//add data to last node 
		//System.out.println("Addpnf");
		treeNode.pages.add(getDataFromFile(currentFile));
		//System.out.println("Addpnfr");
		
		
	}
	
	private float[] getDataFromFile(File currentFile) {
		Scanner in;
		String[] vals=null;
		try {
			in = new Scanner(currentFile);
			in.nextLine();
			vals=in.nextLine().split(",");
		} catch (FileNotFoundException e) {
			e.printStackTrace();
			
		}
		float[] result=new float[vals.length-1] ;
		for (int i=1;i<vals.length;i++){
			result[i-1]=(Float.parseFloat(vals[i]));
		}
		return result;
	}

	private node getChildIfExist(node parent,String childName){
		if (parent!=null){
			Iterator<node> childIter=parent.children.iterator();
			while(childIter.hasNext()) {
				node child=childIter.next();
				if(child.name.equals(childName)){					
					return child;
				}
			}
		}
		return null;
	}


	public ArrayList<featureVector> getSubtreeData(node thisNode) {
		// TODO Auto-generated method stub
		ArrayList<featureVector> result=new ArrayList<featureVector>();
		getChildrenData(thisNode,null,result);
		return result;
	}

	private void getChildrenData(node thisNode,String label,
			ArrayList<featureVector> result) {
		// TODO Auto-generated method stub
		if(thisNode==null){
			return ;
		}
		Iterator<node> childrenIter=thisNode.children.iterator();
		while(childrenIter.hasNext()){
			if (label!=null){
				getChildrenData(childrenIter.next(), label, result);
			}
			else{
				node child=childrenIter.next();
				if (child.children.size()==0){
					getChildrenData(child, "NO_CLASS", result);
				}
				else{
					getChildrenData(child, child.name, result);
				}		
			}
		}
				
		Iterator<float[]> dataIter=thisNode.pages.iterator();
		while(dataIter.hasNext()){
			featureVector f=new treeFastXML.featureVector();
			f.data=dataIter.next();
			if (label!=null){
				f.label=label;
			}
			else{
				f.label=thisNode.name;
			}
			result.add(f);
		}
		return;
	}

	
	/*
	 * Prints tree for viewing! 
	 */	
	public void printGui() {
        printGui("", true,head);
        return;
    }

    private void printGui(String prefix, boolean isTail,node thisNode) {
        System.out.println(prefix + (isTail ? "└── " : "├── ") + thisNode.name);
        for (int i = 0; i < thisNode.children.size() - 1; i++) {
            printGui(prefix + (isTail ? "    " : "│   "), false,thisNode.children.get(i));
        }
        if (thisNode.children.size() >= 1) {
            printGui(prefix + (isTail ?"    " : "│   "), true,thisNode.children.get(thisNode.children.size() - 1));
        }
    }

    public void printfscores(){
	System.out.println("Note: Each node in this tree shows the data for classification for itself (NOT its children!).");
	printGuiWithFScore("",true,head);
    }

     private String getNodeFScore(node thisNode){
	int tp=thisNode.fscore.tp;
	int fn=thisNode.fscore.fn;
	int fp=thisNode.fscore.fp;
	double precision=tp/((double)(tp+fp));
	double recall=tp/((double)(tp+fn));
	double f=2*precision*recall/(recall+precision);
	return " p,r,f: "+String.valueOf(precision)+", "+String.valueOf(recall)+", "+String.valueOf(f)+
		" tp,fp,fn:"+String.valueOf(tp)+", "+String.valueOf(fp)+", "+String.valueOf(fn);
    }

    private void printGuiWithFScore(String prefix, boolean isTail,node thisNode) {
	try {
		int foo = Integer.parseInt(thisNode.name);
	    return;
	} catch (NumberFormatException e) {
				;
	}
	System.out.println(prefix + (isTail ? "└── " : "├── ") + thisNode.name+getNodeFScore(thisNode));
        for (int i = 0; i < thisNode.children.size() - 1; i++) {
            printGuiWithFScore(prefix + (isTail ? "    " : "│   "), false,thisNode.children.get(i));
        }
        if (thisNode.children.size() >= 1) {
            printGuiWithFScore(prefix + (isTail ?"    " : "│   "), true,thisNode.children.get(thisNode.children.size() - 1));
        }
    }

    public void printSoftAccuracy(){
	System.out.println("Note: Each node in this tree shows the data for classification for its children.");
	printGuiWithSA("",true,head);
    }
    private String getNodeSAScore(node thisNode){
	int c=thisNode.accuracy.correct;
	int w=thisNode.accuracy.wrong;
	double precision=c/((double)(c+w));
	return " correct,wrong, accuracy: "+String.valueOf(c)+", "+String.valueOf(w)+", "+String.valueOf(precision);
    }
	
    private void printGuiWithSA(String prefix, boolean isTail,node thisNode) {
	try {
		int foo = Integer.parseInt(thisNode.name);
	    return;
	} catch (NumberFormatException e) {
				;
	}
	System.out.println(prefix + (isTail ? "└── " : "├── ") + thisNode.name+getNodeSAScore(thisNode));
        for (int i = 0; i < thisNode.children.size() - 1; i++) {
            printGuiWithSA(prefix + (isTail ? "    " : "│   "), false,thisNode.children.get(i));
        }
        if (thisNode.children.size() >= 1) {
            printGuiWithSA(prefix + (isTail ?"    " : "│   "), true,thisNode.children.get(thisNode.children.size() - 1));
        }
    }

    Double[] fscoreArray=new Double[100];
	Double[] softArray=new Double[100];
	
	Double[] totf=new Double[100];
	Double[] tots=new Double[100];

	public void getLevelResult(){
		for (int i=0;i<100;i++){
			fscoreArray[i]=0.0;softArray[i]=0.0;tots[i]=0.0;totf[i]=0.0;
		}
		getfscore(head,0);
		getsoft(head,0);

		for (int i=0;i<100;i++){
			if(totf[i]!=0)
			{
				fscoreArray[i]/=totf[i];
				System.out.println("fscore at level "+Integer.toString(i)+" = " + String.valueOf(fscoreArray[i]) );
			}
			if(tots[i]!=0)
			{
				softArray[i]/=tots[i];
				System.out.println("softScore at level "+Integer.toString(i)+" = " + String.valueOf(softArray[i]));
			}
		}
	}
	public void getfscore(node t,int level){
		Iterator<node> childrenIter=t.children.iterator();
		while(childrenIter.hasNext()){
			getfscore(childrenIter.next(),level+1);
		}
		double ttt = getNodeFScore2(t);
		if(ttt > 0)
		{
			fscoreArray[level]+=ttt;
			totf[level]+=1;
		}
	}


	public void getsoft(node t,int level){
		Iterator<node> childrenIter=t.children.iterator();
		while(childrenIter.hasNext()){
			getsoft(childrenIter.next(),level+1);
		}
		double ttt = getNodeSAScore2(t);
		if(ttt > 0)
		{
			softArray[level]+=ttt;
			tots[level]+=1;
		}
	}

	private double getNodeFScore2(node thisNode){
		int tp=thisNode.fscore.tp;
		int fn=thisNode.fscore.fn;
		int fp=thisNode.fscore.fp;
		double precision=tp/((double)(tp+fp));
		double recall=tp/((double)(tp+fn));
		double f=2*precision*recall/(recall+precision);
		if(f==Double.NaN) return -1.0;
		return f;
    }

    private double getNodeSAScore2(node thisNode){
		int c=thisNode.accuracy.correct;
		int w=thisNode.accuracy.wrong;
		double precision=c/((double)(c+w));
		if(precision==Double.NaN) return -1.0;
		return precision;
    }
}
