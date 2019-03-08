#read datafiles/DictionaryFrequencyNewShort.txt
#read pages from featureVector/AllVectorNew2StopStemPorter or featureVector/AllVectorNew2StopStemSnowball
#make tf, standardTf and tf-idf vector for each

# -*- coding: utf-8 -*-
import codecs
import re
from collections import Counter
import os
import datetime
import sys
import math

now = datetime.datetime.now()
logFile = "datafiles/log_createFeatureNew.log"
logOut=codecs.open(logFile,'w','utf-8')
print>>logOut,"Starting at :",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)

acceptThreshold = 0.7
dicGlobalWord = []
dicGlobalFreq = []
dicGlobalnDoc = []
count = 0

Dictionary = "datafiles/DictionaryFrequencyNewShort.txt"
defPath = "featureVector/AllVectorNew2StopStemSnowballFreqShort"
defPath_tf = "featureVector/tf_AllVectorNew2StopStemSnowballFreq"
WriteFile_tf = "featureVector/CombineTf_AllVectorNew2StopStemSnowballFreq.csv"
defPath_st_tf = "featureVector/sttf_AllVectorNew2StopStemSnowballFreq"
WriteFile_st_tf = "featureVector/CombineStTf_AllVectorNew2StopStemSnowballFreq.csv"

defPath_tfIDF = "featureVector/tfIDF_AllVectorNew2StopStemSnowballFreq"

dicFile = codecs.open(Dictionary,'r','utf-8').readlines()
tfFile = codecs.open(WriteFile_tf,'w','utf-8')
stTfFile = codecs.open(WriteFile_st_tf,'w','utf-8')

wr = "AllWords"

for l in dicFile:
	word,freq,nDoc = l.split('\t')
	dicGlobalWord.append(word)
	dicGlobalFreq.append(float(freq))
	dicGlobalnDoc.append(float(nDoc))
	wr = wr+","+word

# print>>tfFile,wr
# print>>stTfFile,wr

dicGlobalSize = len(dicGlobalWord)
if dicGlobalSize != len(dicGlobalFreq):
	print "what has happened!!!!"
	sys.exit()


for root,folder,files in os.walk(defPath):
	if files:
		totalFiles = len(files)
		filesRemaining = totalFiles
		for f in files:
			print "*"*20, filesRemaining, "*"*20
			print f
			filesRemaining-=1
			vectorCount_tf_idf = [0]*dicGlobalSize
			vectorCount_st_tf = [0]*dicGlobalSize
			vectorCount_tf = [0]*dicGlobalSize
			
			fin = codecs.open(root+"/"+f,'r','utf-8').readlines()
			wordLocalMap = map(lambda x:x.split('\t'), fin)
			wordLocalMapDic = [x for x in wordLocalMap if x[0] in dicGlobalWord]
			
			lenLocalAllWord = len(wordLocalMap)
			lenLocalDicWord = len(wordLocalMapDic)
			ratioLocalDicWord = float(lenLocalDicWord)/float(lenLocalAllWord)
			print ratioLocalDicWord, acceptThreshold, "total words = " ,lenLocalDicWord

			if ratioLocalDicWord >= acceptThreshold:
				for w in wordLocalMapDic:
					ind = dicGlobalWord.index(w[0])
					vectorCount_tf[ind] = int(w[1])	#tf = x[1]
					vectorCount_st_tf[ind] = float(w[1])/lenLocalDicWord	#st_tf = x[1]/lenLocalDicWord
					vectorCount_tf_idf[ind]	= vectorCount_st_tf[ind]*math.log(totalFiles/dicGlobalnDoc[ind])					#tf-idf = 

				#create entry in featureFiles
				# wr=f
				# for i in range(0,dicGlobalSize):
				# 	wr=wr+","+str(vectorCount_tf[i])
				# print>>tfFile,wr

				# #
				# wr=f
				# for i in range(0,dicGlobalSize):
				# 	wr=wr+","+str(vectorCount_st_tf[i])
				# print>>stTfFile,wr

				localTfFile = codecs.open(defPath_tf+"/"+f,'w','utf-8')
				localStTfFile = codecs.open(defPath_st_tf+"/"+f,'w','utf-8')
				localTfIDFFile = codecs.open(defPath_tfIDF+"/"+f,'w','utf-8')

				wr="AllWords"
				for w in dicGlobalWord:
					wr=wr+","+w
				print>>localStTfFile,wr
				print>>localTfFile,wr
				print>>localTfIDFFile,wr
				#
				wr=f
				for i in range(0,dicGlobalSize):
					wr=wr+","+str(vectorCount_st_tf[i])
				print>>localStTfFile,wr

				wr=f
				for i in range(0,dicGlobalSize):
					wr=wr+","+str(vectorCount_tf[i])
				print>>localTfFile,wr

				wr=f
				for i in range(0,dicGlobalSize):
					wr=wr+","+str(vectorCount_tf_idf[i])
				print>>localTfIDFFile,wr


				#maintaining logs
				now = datetime.datetime.now()
				print>>logOut,"Processed file:",root+"/"+f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)
				print>>logOut,"\t\t"+"Total Words = "+str(lenLocalAllWord)+"\t"+"Dictionary Words = "+str(lenLocalDicWord)
				print>>logOut,"-"*60
				print "Processed file:",root+"/"+f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)
			else:
				print "Foreign file: "+f
				now = datetime.datetime.now()
				print>>logOut, "Foreign file: "+f
				print>>logOut,"\t\t"+"Total Words = "+str(lenLocalAllWord)+"\t"+"Dictionary Words = "+str(lenLocalDicWord)
				print>>logOut,"-"*60