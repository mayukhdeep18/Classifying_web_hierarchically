import java.io.*;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.regex.*;
import java.util.Formatter;
import java.util.Locale;
import java.util.TreeSet;

import cc.mallet.pipe.*;
import cc.mallet.pipe.iterator.*;
import cc.mallet.types.*;
import cc.mallet.topics.*;

public class trainModel {

	public static Pipe pipe;

	public trainModel() {
        pipe = buildPipe();
    }

	public Pipe buildPipe() {
		ArrayList pipeList = new ArrayList();

		// Read data from File objects
		pipeList.add(new Input2CharSequence("UTF-8"));

		// Regular expression for what constitutes a token.
		//  This pattern includes Unicode letters, Unicode numbers, 
		//   and the underscore character. Alternatives:
		//    "\\S+"   (anything not whitespace)
		//    "\\w+"    ( A-Z, a-z, 0-9, _ )
		//    "[\\p{L}\\p{N}_]+|[\\p{P}]+"   (a group of only letters and numbers OR
		//                                    a group of only punctuation marks)
		Pattern tokenPattern = Pattern.compile("[\\p{L}\\p{N}_]+");

		// Tokenize raw strings
		pipeList.add(new CharSequence2TokenSequence(tokenPattern));

		// Normalize all tokens to all lowercase
		//pipeList.add(new TokenSequenceLowercase());

		// Remove stopwords from a standard English stoplist.
		//  options: [case sensitive] [mark deletions]
		//pipeList.add(new TokenSequenceRemoveStopwords(false, false));

		// Rather than storing tokens as strings, convert 
		//  them to integers by looking them up in an alphabet.
		pipeList.add(new TokenSequence2FeatureSequence());

		// Do the same thing for the "target" field: 
		//  convert a class label string to a Label object,
		//  which has an index in a Label alphabet.
		pipeList.add(new Target2Label());

		// Now convert the sequence of features to a sparse vector,
		//  mapping feature IDs to counts.
		//pipeList.add(new FeatureSequence2FeatureVector());

		// Print out the features and the label
		//pipeList.add(new PrintInputAndTarget());

		return new SerialPipes(pipeList);
    }


	public static void AddTrainData(String location,tree DataTree){
		ArrayList<File> TrainSet=walk(new File(location),new ArrayList<File>());
		File currentFile;
		Iterator<File> FileIter=TrainSet.iterator();
		int i=0;int j=0;
		while(FileIter.hasNext()){
			currentFile=FileIter.next();
			String[] TreePath=currentFile.getName().split("-");
			if (!DataTree.checkIfPath2NodeExists(TreePath)){
				i++;
				DataTree.addPathNode(TreePath,currentFile);
			}
			else{
				j++;
			}
		}
		DataTree.printGui();
	}
	public static void TestData(String location,tree DataTree){
		//do some stuff
		tree.node thisNode=DataTree.getHead();//get head of the tree
		//do some loopy stuff!
		ArrayList<tree.featureVector> allData=DataTree.getSubtreeData(thisNode); // gets all data in subtree with labels as children of thisNode.
	}
	
	public static ArrayList<File> walk(File dir, ArrayList<File> result){
		File listFile[] = dir.listFiles();
	        if (listFile != null) {;
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

	public static void ApplyLDA(tree DataTree)
	{
		//get data in correct format
		ArrayList<ArrayList<ArrayList<String>>> trainingSample = new ArrayList<ArrayList<ArrayList<String>>>();
		trainingSample = GetDataLDA(DataTree,DataTree.head.children.get(0));
		//System.out.println(trainingSample);
		
		//prepare instance
		InstanceList instances = new InstanceList(pipe);
		int size = trainingSample.size();
		//System.out.println(size);
		for (int i = 0; i < size; i++)
        {
            try {
            	instances.addThruPipe (new ArrayIterator (trainingSample.get(i).get(0),trainingSample.get(i).get(1).get(0)));
            } catch (Exception e) { System.out.println(e);}
        }

		// Create a model with 100 topics, alpha_t = 0.01, beta_w = 0.01
		//  Note that the first parameter is passed as the sum over topics, while
		//  the second is the parameter for a single dimension of the Dirichlet prior.
		int numTopics = 20;
		ParallelTopicModel model = new ParallelTopicModel(numTopics, 1.0, 0.01);

		model.addInstances(instances);

		// Use two parallel samplers, which each look at one half the corpus and combine
		//  statistics after every iteration.
		model.setNumThreads(2);

		// Run the model for 50 iterations and stop (this is for testing only, 
		//  for real applications, use 1000 to 2000 iterations)
		model.setNumIterations(2000);
		try {
            	model.estimate();
            } catch (IOException e) { System.out.println(e);}

		// Show the words and topics in the first instance

		// The data alphabet maps word IDs to strings
		Alphabet dataAlphabet = instances.getDataAlphabet();

		FeatureSequence tokens = (FeatureSequence) model.getData().get(0).instance.getData();
		LabelSequence topics = model.getData().get(0).topicSequence;

		Formatter out = new Formatter(new StringBuilder(), Locale.US);
		for (int position = 0; position < tokens.getLength(); position++) {
		    out.format("%s-%d ", dataAlphabet.lookupObject(tokens.getIndexAtPosition(position)), topics.getIndexAtPosition(position));
		}
		System.out.println(tokens);
		System.out.println(out);

		// Estimate the topic distribution of the first instance, given the current Gibbs state.
        double[] topicDistribution = model.getTopicProbabilities(1);

        // Get an array of sorted sets of word ID/count pairs
        ArrayList<TreeSet<IDSorter>> topicSortedWords = model.getSortedWords();
        
        // Show top 5 words in topics with proportions for the first document
        for (int topic = 0; topic < numTopics; topic++) {
            Iterator<IDSorter> iterator = topicSortedWords.get(topic).iterator();
            
            out = new Formatter(new StringBuilder(), Locale.US);
            out.format("%d\t%.3f\t", topic, topicDistribution[topic]);
            int rank = 0;
            while (iterator.hasNext() && rank < 5) {
                IDSorter idCountPair = iterator.next();
                out.format("%s (%.0f) ", dataAlphabet.lookupObject(idCountPair.getID()), idCountPair.getWeight());
                rank++;
            }
            System.out.println(out);
        }
	}

	public static ArrayList<ArrayList<ArrayList<String>>> GetDataLDA(tree DataTree, tree.node Parent)
	{
		ArrayList<tree.node> children = Parent.children;
		ArrayList<ArrayList<ArrayList<String>>> result = new ArrayList<ArrayList<ArrayList<String>>>();

		for(tree.node c : children)
		{
			ArrayList<tree.featureVector> test = DataTree.getSubtreeData(c);
			
			String labelTemp = test.get(0).label;
			ArrayList<String> labelTempArray = new ArrayList<String>();
			labelTempArray.add(labelTemp); 
			
			ArrayList<String> dataTemp = new ArrayList<String>();

			for (tree.featureVector vec : test)
	    	{
	    		//System.out.println(vec.fToString());
	    		if(!labelTemp.equals(vec.label))
	    		{
	    			System.out.println("Error: labels not matching: labelTemp="+labelTemp+" vec.label="+vec.label);
	    			System.exit(0);
	    		}
	    		dataTemp.add(vec.data);
	    	}

			ArrayList<ArrayList<String>> pair = new ArrayList<ArrayList<String>>();
			pair.add(dataTemp);
			pair.add(labelTempArray);
			result.add(pair);
		}
		return result;
	}
	
	
	public static void main(String[] args){
		trainModel ob1=new trainModel();
		String folderTrain="classificationTest/Test2/train";
		String folderTest="test";
		tree DataTree=new tree();
		AddTrainData(folderTrain,DataTree);

		//data modelling
		ApplyLDA(DataTree);
		
		//insert classification here!!
		//TestData(folderTest,DataTree);
		
	}
}
