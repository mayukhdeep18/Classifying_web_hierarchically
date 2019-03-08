import java.io.File;
import java.io.FileNotFoundException;
import java.util.Scanner;
import java.util.ArrayList;
import java.util.Map;
import java.util.HashMap;

public class wordMapIndex {
	
	String fileName;
	Map<String, String> wordMap = new HashMap<String, String>();

	public wordMapIndex(String fileN)
	{
		fileName = fileN;
		readFromFile();
	}

	private void readFromFile()
	{
		File file = new File(fileName);
		String[] vals=null;
		try
		{
			Scanner in = new Scanner(file);
			while (in.hasNextLine()) {
				vals=in.nextLine().split("\t");
				wordMap.put(vals[1], vals[0]);
			}
			in.close();
    	} 
    	catch (FileNotFoundException e)
    	{
        	e.printStackTrace();
    	}
	}
}