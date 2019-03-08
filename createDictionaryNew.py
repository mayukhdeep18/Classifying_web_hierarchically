#read all files in "featureVector/AllVectorNew2StopStemSnowball"
#create "datafiles/DictionaryNew2.txt" and "datafiles/DictionaryFrequencyNew2.txt"
#also create copy of each file in "featureVector/AllVectorNew2StopStemSnowballFreq" with frequency of each word

# -*- coding: utf-8 -*-
import codecs
import re
from collections import Counter
import os
import datetime
from nltk import word_tokenize

now = datetime.datetime.now()
logFile = "datafiles/log_createDictionaryNew.log"
logOut=codecs.open(logFile,'w','utf-8')
print>>logOut,"Starting at :",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)

cnt  = Counter()
idfCnt = Counter()
count = 0

defPath = "featureVector/AllVectorNew2StopStemSnowball"
WritePath2 = "featureVector/AllVectorNew2StopStemSnowballFreq"
WriteFile1="datafiles/DictionaryNew.txt"
WriteFile2="datafiles/DictionaryFrequencyNew.txt"

for root,folder,files in os.walk(defPath):
	if files:
		for f in files:
			cntLocal  = Counter()
			flagLocal  = Counter()
			fin = codecs.open(root+"/"+f,'r','utf-8').read()
			tokens = [w for w in word_tokenize(fin)]
			lenf = len(tokens)
			ulenf = len(set(tokens))
			for word in tokens:
				cnt[word] +=1
				cntLocal[word] +=1
				if flagLocal[word]!=1:
					idfCnt[word]+=1
					flagLocal[word]=1

			gtemp = codecs.open(WritePath2+"/"+f,'w','utf-8')
			for w in sorted(cntLocal,key=cntLocal.get,reverse=True):
				if(cntLocal[w]>0):
					print>>gtemp, w+'\t'+str(cntLocal[w])

			count+=lenf
			
			#maintaining log
			now = datetime.datetime.now()
			print>>logOut,"Processed file:",root+"/"+f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)
			print>>logOut,"\t\t"+"Total Words = "+str(lenf)+"\t"+"Unique Words = "+str(ulenf)
			print>>logOut,"-"*60
			print "Processed file:",root+"/"+f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)

g1 = codecs.open(WriteFile1,'w','utf-8')
g2 = codecs.open(WriteFile2,'w','utf-8')

for w in sorted(cnt,key=cnt.get,reverse=True):
	if(cnt[w]>0):
		print>>g2, w+'\t'+str(cnt[w])+'\t'+str(idfCnt[w])
		#g.write(''+cnt[w]+' '+w);

for w in sorted(cnt):
	if(cnt[w]>0):
		print>>g1, w

g1.close()
g2.close()

print>>logOut,'\n\n',"*"*101
print>>logOut,"Files created: "+WriteFile1+", "+WriteFile2
print>>logOut,"*"*101

print>>logOut,'\n\n',"*"*101
print>>logOut,"Total words are " , count
print>>logOut,"Total unique words are ", len(cnt)
print>>logOut,"*"*101
print "Total words are " , count
print "Total unique words are ", len(cnt)
