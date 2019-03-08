import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Scanner;
import java.util.logging.Logger;
import java.util.Arrays;

public class tree {
	
	public class node{
		ArrayList<node> children=new ArrayList<node>();
		String name="";
		ArrayList<String> pages=new ArrayList<String>();

		public node()
		{
			name="MotherFucker";
		}
	}

	public class featureVector{
		String data;
		String label;

		public String fToString()
		{
			return "label:" + label + " data:" + data;	
		}
	}

	node head=new node();
	
	public node getHead(){
		return head;
	}

	public boolean checkIfPath2NodeExists(String[] treePath) {
		node treeNode=head;
		node childWithName=null;
		for (int i=0; i<treePath.length;i++){
			childWithName=getChildIfExist(treeNode, treePath[i]);
			if (childWithName==null){
				return false;
			}
			treeNode=childWithName;
		}
		return true;
	}

	public void addPathNode(String[] treePath, File currentFile) {
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
			treeNode.children.add(childWithName);
			treeNode=childWithName;
		}
		treeNode.pages.add(getDataFromFile(currentFile));	
	}

	private String getDataFromFile(File currentFile) {
		String result = "";
		Scanner in;
		try {
			in = new Scanner(currentFile);
			while (in.hasNextLine()) {
                result += in.nextLine();
            }
			//System.out.println(result);
		} catch (FileNotFoundException e) {
			e.printStackTrace();
			
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
		ArrayList<featureVector> result=new ArrayList<featureVector>();
		getChildrenData(thisNode,null,result);
		return result;
	}

	private void getChildrenData(node thisNode,String label, ArrayList<featureVector> result) 
	{
		if(thisNode==null){
			return ;
		}
		Iterator<node> childrenIter=thisNode.children.iterator();
		while(childrenIter.hasNext()){
			if (label!=null){
				getChildrenData(childrenIter.next(), label, result);
			}
			else{
				getChildrenData(childrenIter.next(), thisNode.name, result);
			}
		}
				
		Iterator<String> dataIter=thisNode.pages.iterator();
		while(dataIter.hasNext()){
			featureVector f=new featureVector();
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

	public void printTree() {
		if (head.children==null) {
			System.out.println("NULL");return;
		}
		Iterator<node> childrenIter=head.children.iterator();
		while(childrenIter.hasNext()){
			drawNodeTree(childrenIter.next());
		}
		
	}

	private void drawNodeTree(node thisNode) {
		// TODO Auto-generated method stub
		if (thisNode.children.isEmpty()) {
			System.out.println(" "+thisNode.name+"/"+thisNode.pages.size());
		}
		else{
			System.out.print(" "+thisNode.name);
			Iterator<node> childrenIter=thisNode.children.iterator();
			while(childrenIter.hasNext()){
				drawNodeTree(childrenIter.next());
			}	
		}
	}

	public void printGui() {
        printGui("", true,head);
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
}
