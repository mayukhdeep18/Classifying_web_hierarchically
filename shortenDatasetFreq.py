#Read the files from folder  "featureVector/AllVectorNew2StopStemSnowballFreq" and 
#dictionary "datafiles/DictionaryFrequencyNewShort.txt" and write corresponding files in 
#new folder "featureVector/AllVectorNew2StopStemSnowballFreqShort" pruning words only in this new dictionary. 
#This also prunes files that after removing all non-dictionary words have too few words left.

# -*- coding: utf-8 -*-
import codecs
import re
from collections import Counter
import os
import datetime
from nltk import word_tokenize

now = datetime.datetime.now()
logFile = "datafiles/log_create_AllVectorNew2StopStemSnowballFreqShort.log"
logOut=codecs.open(logFile,'w','utf-8')
print>>logOut,"Starting at :",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)

ReadFile="datafiles/DictionaryFrequencyNewShort.txt"
defPath = "featureVector/AllVectorNew2StopStemSnowballFreq"
WritePath1="featureVector/AllVectorNew2StopStemSnowballFreqShort/"

thresholdCount = 50

#reading words from dictionary
dicWords = Counter()

g1 = codecs.open(ReadFile,'r','utf-8').readlines()
for l in g1:
	word,freq,nDoc = l.split('\t')
	dicWords[word] = int(freq)

dicWordList = list(set(dicWords.elements()))

#creating directory if not present
if not os.path.exists(os.path.dirname(WritePath1)):
    os.makedirs(os.path.dirname(WritePath1))

#traversing defpath
for root,folder,files in os.walk(defPath):
	if files:
		totalFiles = len(files)
		filesRemaining = totalFiles
		for f in files:
			print "*"*20, filesRemaining, "*"*20
			filesRemaining-=1
			cntLocal  = Counter()
			totalLocalCount = 0

			fin = codecs.open(root+"/"+f,'r','utf-8').readlines()
			initialTotalUniqueWords = len(fin)
			finalTotalUniqueWords = 0
			for l in fin:
				word,freq = l.split('\t')
				if word in dicWordList:
					cntLocal[word] = int(freq)
					totalLocalCount += int(freq)
					finalTotalUniqueWords += 1

			if totalLocalCount > thresholdCount:
				gtemp = codecs.open(WritePath1+f,'w','utf-8')
				for w in sorted(cntLocal,key=cntLocal.get,reverse=True):
					if(cntLocal[w]>0):
						print>>gtemp, w+'\t'+str(cntLocal[w])
			
			#maintaining log
			now = datetime.datetime.now()
			print>>logOut,"Processed file:",root+"/"+f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)
			print>>logOut,"\t\t"+"Total Unique Words Before = "+str(initialTotalUniqueWords)+"\t"+"Unique Words After= "+str(finalTotalUniqueWords)
			print>>logOut,"-"*60
			print "Processed file:",root+"/"+f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)
			print "Total Unique Words Before = "+str(initialTotalUniqueWords)+"\t"+"Unique Words After= "+str(finalTotalUniqueWords)
