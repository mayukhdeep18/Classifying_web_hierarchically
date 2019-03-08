# -*- coding: utf-8 -*-
import codecs
import re
import os
import datetime
from nltk import word_tokenize
from nltk.stem.porter import *
from nltk.stem.snowball import SnowballStemmer
import snowballstemmer as sbs

now = datetime.datetime.now()
logFile = "datafiles/log_stopAndStemData.log"
logOut=codecs.open(logFile,'w','utf-8')
print>>logOut,"Starting at :",str(now.hour),":",str(now.minute),":",str(now.second),", ",str(now.day),"/",str(now.month),"/",str(now.year)

defPath = "featureVector/AllVectorNew2"
WriteFolderP="featureVector/AllVectorNew2StopStemPorter"
WriteFolderS="featureVector/AllVectorNew2StopStemSnowball"
stopWordFile = "datafiles/englishStopList"

prog = re.compile('\n|\t|[ ]+[ ]|[ ]')
stopWordf = [w.rstrip() for w in codecs.open(stopWordFile,'r','utf-8').readlines() if not prog.match(w)]

#stemmerP = PorterStemmer()
stemmerS = stemmer = sbs.stemmer('english');

nonEnglish = re.compile('^([a-zA-Z][-]*){3,}$')
for root,folder,files in os.walk(defPath):
	if files:
		totalFiles = len(files)
		filesRemaining = totalFiles
		for f in files:
			print "*"*20, filesRemaining, "*"*20
			filesRemaining-=1
			fin = codecs.open(root+"/"+f,"r",'utf-8').read()
			tokens = [re.sub(r'\.','',w) for w in word_tokenize(fin) if not prog.match(w)]
			
			#remove stop words
			#keep only alphanumeric words and words like post-man etc
			#length of word >= 3
			filtered_tokens = [w for w in tokens if (w not in stopWordf and nonEnglish.match(w))]
			
			#stemmedP_token = [stemmerP.stem(w) for w in filtered_tokens]
			stemmedS_token = [stemmerS.stemWord(w) for w in filtered_tokens]
			#print stemmed_token[0:10]
			#print " "
			#print "="*90
			#stEnglishP = ' '.join(stemmedP_token)
			stEnglishS = ' '.join(stemmedS_token)
			#foutP=codecs.open(WriteFolderP+"/"+f,'w','utf-8')
			foutS=codecs.open(WriteFolderS+"/"+f,'w','utf-8')
			#print>>foutP,stEnglishP
			print>>foutS,stEnglishS

			#maintaining logs
			now = datetime.datetime.now()
			#print>>logOut,"Generated file:",WriteFolderP+"/"+f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)
			print>>logOut,"Generated file:",WriteFolderS+"/"+f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)
			print "Generated file:",WriteFolderS+"/"+f," at:",str(now.hour)+":"+str(now.minute)+":"+str(now.second)+" "+str(now.day)+"/"+str(now.month)+"/"+str(now.year)

