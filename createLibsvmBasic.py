# -*- coding: utf-8 -*-
import codecs
import re
from collections import Counter
import os
import datetime
import sys

now = datetime.datetime.now()
logFile = "datafiles/log_createLibsvmBasic.log"
logOut=codecs.open(logFile,'w','utf-8')
print>>logOut,"Starting at :",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)

defPath = "featureVector/AllVectorNew2StopStemSnowballFreqShort"
ReadFile1 = "datafiles/wordIndexMap.txt"
ReadFile2 = "datafiles/labelIndexMap.txt"
writePath = "featureVector/AllVectorNew2StopStemSnowballFreqShort_LibSVM/"

#creating directory if not present
if not os.path.exists(os.path.dirname(writePath)):
    os.makedirs(os.path.dirname(writePath))

#wordIndexMap
wordMap = Counter()
wordList = []
lines = codecs.open(ReadFile1,'r','utf-8').readlines();
for line in lines:
	index,word,freq = line.split('\t')
	wordMap[word] = int(index)
wordList = sorted(wordMap)

#labelIndexMap
labelMap = Counter()
lines = codecs.open(ReadFile2,'r','utf-8').readlines();
for line in lines:
	index,label = line.split('\t')
	labelMap[label.strip()] = int(index)
labelList = sorted(labelMap)

#traversing defpath
for root,folder,files in os.walk(defPath):
	if files:
		for f in files:
			cntLocal = Counter()
			fin = codecs.open(root+"/"+f,'r','utf-8').readlines()
			labelLocal = -1
			f2 = re.sub('-[0-9]*$', '',f)
			f2 = re.sub('/','-',f2)
			if f2 in labelList:
				labelLocal = labelMap[f2]
			else:
				print "Error label->",f2

			for l in fin:
				word,freq = l.split('\t')
				if word in wordList:
					cntLocal[wordMap[word]] = int(freq)
				else:
					print "Error word->",word

			gtemp = codecs.open(writePath+f,'w','utf-8')
			wr = str(labelLocal)+" "
			for w in sorted(cntLocal):
				wr = wr+str(int(w))+":"+str(cntLocal[w])+" "

			wr = wr.strip()
			print>>gtemp,wr

			#maintaining log
			now = datetime.datetime.now()
			print>>logOut,"Processed file:",root+"/"+f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)
			print "Processed file:",root+"/"+f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)