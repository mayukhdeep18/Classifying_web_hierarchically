# -*- coding: utf-8 -*-
import codecs
import re
import os
from collections import Counter
import datetime
import random
import shutil

now = datetime.datetime.now()
logFile = "datafiles/log_CreateValidationSet.log"
logOut=codecs.open(logFile,'w','utf-8')
print>>logOut,"Starting at :",str(now.hour),":",str(now.minute),":",str(now.second),", ",str(now.day),"/",str(now.month),"/",str(now.year)

defPath = "featureVector/tfIDF_AllVectorNew2StopStemSnowballFreq"	#all features
WriteFolderTest ="testF/"
WriteFolderTrain ="trainF/"

# CombineFileTest ="test.csv"
# CombineFileTrain ="train.csv"

# cFTest = codecs.open(CombineFileTest,'w','utf-8')
# cFTrain = codecs.open(CombineFileTrain,'w','utf-8')

filelist = [ f for f in os.listdir(WriteFolderTest)]
for f in filelist:
    os.remove(WriteFolderTest+f)

filelist = [ f for f in os.listdir(WriteFolderTrain)]
for f in filelist:
    os.remove(WriteFolderTrain+f)

leaf = {}
testLeaf = {}
trainLeaf = {}
trainsetRatio = .80	#i.e. 80% train set and 20% test set

for root,folder,files in os.walk(defPath):
	if files:
		totalFiles = len(files)
		filesRemaining = totalFiles
		for f in files:
			print "*"*10, filesRemaining, "*"*10
			filesRemaining-=1
			trail,n = f.rsplit('-',1)
			if(leaf.has_key(trail)):
				leaf[trail].append(n)
			else:
				leaf[trail] = [n]

			#maintaining logs
			now = datetime.datetime.now()
			print>>logOut,"Processed file:",f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)
		
			print "Processed file:",f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)

#for key in leaf:
#	lines=open(defPath+"/"+key+"-"+leaf[key][0],'r').readlines()
#	print>>cFTrain,lines[0].strip('\n')
#	print>>cFTest,lines[0].strip('\n')
#	break

for key in leaf:
	numList = leaf[key]
	trainLeaf[key] = random.sample( numList,int(trainsetRatio*len(numList)) )
	testLeaf[key] = list( set(numList) - set(trainLeaf[key]) )

	for i in trainLeaf[key]:
		shutil.copy2(defPath+"/"+key+"-"+i,WriteFolderTrain)
		#lines=open(defPath+"/"+key+"-"+i,'r').readlines()
		#print>>cFTrain,lines[1].strip('\n')

	for i in testLeaf[key]:
		shutil.copy2(defPath+"/"+key+"-"+i,WriteFolderTest)
		#lines=open(defPath+"/"+key+"-"+i,'r').readlines()
		#print>>cFTest,lines[1].strip('\n')
