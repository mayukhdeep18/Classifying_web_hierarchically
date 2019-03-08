UGP3
=====

Folders


datafiles/ 	: contains data and dictionary
featureVector/	: contains various files relaing to feature vector of a link or web page
library/ 	: contains R libraries
R-3.0.2/ 	: R setup
tm/ 		: R library
tutorial/ 	: Tutorials and documents regarding project
UGP Report/	: UGP2 report


Procedure New
------------------------------
1) run parseWebNew2.py: Read file structure from "datafiles/dmoz/TopNew2" and write english web pages in "featureVector/AllVectorNew2"

2) run stopAndStemData.py: Read folder "featureVector/AllVectorNew2" and read file "datafiles/englishStopList" for stop list and output stem files in "featureVector/AllVectorNew2StopStemPorter","featureVector/AllVectorNew2StopStemSnowball" depending upon the stemming algorithm.

3) run createDictionaryNew.py: Read folder "featureVector/AllVectorNew2StopStemSnowball" and creates dictionary of words present in files in mentioned folder. "datafiles/DictionaryNew2.txt","datafiles/DictionaryFrequencyNew2.txt". Also create folder "featureVector/AllVectorNew2StopStemSnowballFreq" with web pages having word frequencies.

4) run shortenDictionary.py: Reduce dictionary size by taking words with frequency above some threshold one. Input File: "datafiles/DictionaryFrequencyNew.txt" Output file:"datafiles/DictionaryFrequencyNewShort.txt"

5) run shortenDatasetFreq.py: Read the files from folder  "featureVector/AllVectorNew2StopStemSnowballFreq" and dictionary "datafiles/DictionaryFrequencyNewShort.txt" and write corresponding files in new folder "featureVector/AllVectorNew2StopStemSnowballFreqShort" pruning words only in this new dictionary. This also prunes files that after removing all non-dictionary words have too few words left.

6) run "wordIndex.py" to create numerical map of valid words. Input is shorten dictionary i.e. "datafiles/DictionaryFrequencyNewShort.txt" and output is "datafiles/wordIndexMap.txt".

7) run "labelIndex.py" to create numerical map of valid labels. Input is file structure from "datafiles/dmoz/TopNew2" and writefile is "datafiles/labelIndexMap.txt".

8) run "createLibsvmBasic.py" Create the libsvm format. Input is folder structure "featureVector/AllVectorNew2StopStemSnowballFreqShort" and mapping files "datafiles/wordIndexMap.txt" and "datafiles/labelIndexMap.txt". Output is corresponding files in libSVM format in folder "featureVector/AllVectorNew2StopStemSnowballFreqShort_LibSVM/"

9) run CreateFeatureNew.py: Forms the tf and standardized-tf feature vectors 
	Read dicionary  	: "datafiles/DictionaryFrequencyNewShort.txt"
	Read Folder     	: "featureVector/AllVectorNew2StopStemSnowballFreq"
	Write Folder tf 	: "featureVector/tf_AllVectorNew2StopStemSnowballFreq"
	Write File tf 		: "featureVector/CombineTf_AllVectorNew2StopStemSnowballFreq.csv"
	Write Folder st-tf		:"featureVector/sttf_AllVectorNew2StopStemSnowballFreq"
	Write File st-tf 	: "featureVector/CombineStTf_AllVectorNew2StopStemSnowballFreq.csv"

10) run CreateValidationSet.py

11) run trainFastXML.java
	- javac trainFastXML.java treeFastXML.java wordMapIndex.java TestModel.java
	- java trainFastXML

	This creates the training models.

''''''''''''''  ignore this
12) run lda file:
javac -cp \* tree.java trainModel.java
java -cp .:\* trainModel > output 
''''''''''''''

13) Once done with all the steps, use runAgain.sh to experiment further.

-------------------------------------------------------------------------------