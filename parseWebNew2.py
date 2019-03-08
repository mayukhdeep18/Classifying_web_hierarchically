#to check for english take random 50 words and check

from bs4 import BeautifulSoup
import urllib2,urllib
import os
import re
import Queue
from nltk.corpus import words
from nltk import word_tokenize
from nltk import tokenize
from nltk.tag import pos_tag
import socket
import codecs
import datetime
import random

now = datetime.datetime.now()
defPath = "datafiles/dmoz/TopNew2"
WriteFolder="featureVector/AllVectorNew2"

logFile = "datafiles/log_parseWebNew2.log"
logOut=codecs.open(logFile,'w','utf-8')
print>>logOut,"Starting at :",str(now.hour),":",str(now.minute),":",str(now.second),", ",str(now.day),"/",str(now.month),"/",str(now.year)

Dict=[]
thresholdNonEnglish = 0.8	#threshhold value( non-english-words/total-words )
thresholdTextLength = 80	#minimum length of text for a page to be a valid feature vector

#setting proxy connection
proxy = urllib2.ProxyHandler({'https': 'http://cseguest:natraj25@ironport1.iitk.ac.in:3128',
			      'http': 'http://cseguest:natraj25@ironport1.iitk.ac.in:3128'})
auth = urllib2.HTTPBasicAuthHandler()
opener = urllib2.build_opener(proxy, auth, urllib2.HTTPHandler,urllib2.HTTPSHandler,urllib2.HTTPRedirectHandler)
urllib2.install_opener(opener)
#proxy connection made

hdr = {'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.64 Safari/537.11',
       'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
       'Accept-Charset': 'ISO-8859-1,utf-8;q=0.7,*;q=0.3',
       'Accept-Encoding': 'none',
       'Accept-Language': 'en-US,en;q=0.8',
       'Connection': 'keep-alive'}


def getHTMLData(root):
	print root
	print>>logOut, root
	fin = open(root+"/content","r").read()
	soup = BeautifulSoup(fin)
	links = soup.find_all("link")
	count = 0
	for l in links:
		if count==20:
			break
		
		url = l["r:resource"]
		print url
		print>>logOut, url
		prog = re.compile('.PDF$|.pdf$|.jpg$|.JPG$|.png$|.PNG$')
		if not prog.search(url):
			req = urllib2.Request(url, headers=hdr)
			try:
				f=urllib2.urlopen(req, timeout=10)
				status = getTextData(f,root,count)	#status = 0 if error
				if status:
					count=count+1
			except urllib2.HTTPError, e:
				print "http error -",type(e)
				print>>logOut, "http error -",type(e)
			except urllib2.URLError, e:
				print "url error -",type(e)
				print>>logOut, "url error -",type(e)
			except socket.timeout, e:
				print "timeout error -",type(e)
				print>>logOut, "timeout error -",type(e)
			except:
				print "some unrecognized error"
				print>>logOut, "some unrecognized error"
		else:
			print "page type error"

def getTextData(html,path,i):
	soup = BeautifulSoup(html)
	texts = soup.findAll(text=True)
	ignoreWords = ['.', ',', ':', '~', '`', '?', ';', '"', '[', ']', '{', '}', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '-', '\\', '/', '+', '=', '<', '>', "'", '|', "'s", "url", "http"]
	stopList=['<!.*>','<?.*?>']

	st = ""
	for element in texts:
		if element.parent.name in ['style', 'script', '[document]', 'head', 'title', 'noframes']:
			continue
		else:
			stopElement = 0
			textElement = element.string.lower()	#if not convertable then goes to except
			#validate for stopList
			for stop in stopList:
				if re.match(stop,textElement):
					stopElement = 1
					break
			if stopElement == 0:
				st = st + textElement + " "

	#st now contains all relevant data. Still have to do english validation on it.

	nonEnglish = 0.0	#keep track of non english words
	
	tokens = word_tokenize(st)
	nTokens = len(tokens)
	eTokens = 50 if (nTokens>50) else nTokens

	randomSample = random.sample(range(0, nTokens), eTokens)

	for ran in randomSample:
		if tokens[ran] not in words.words():
			nonEnglish += 1

	print nonEnglish," ",eTokens
	print>>logOut, nonEnglish," ",eTokens
	
	#check for length of net text
	if (nTokens) > thresholdTextLength:
		percNonEnglish = nonEnglish/(eTokens+0.1)
		#check for non english content
		if percNonEnglish >= thresholdNonEnglish:
			print "validation error -- thresholdNonEnglish"
			print>>logOut, "validation error -- thresholdNonEnglish"
			return 0
		else:
			stEnglish = ' '.join(tokens)
			pathNew = re.sub('/','-',path)
			pathNew=pathNew.split('datafiles-dmoz-')[1]
			print str(pathNew)+str(i)
			print>>logOut, str(pathNew)+str(i)
			print "Accepted"
			print>>logOut, "Accepted"
			fout=codecs.open(WriteFolder+"/"+pathNew+'-'+str(i),'w','utf-8')
			fout2=codecs.open(path+"/"+pathNew+"-"+str(i),'w','utf-8')
			print>>fout,stEnglish
			print>>fout2,stEnglish
			return 1
	else:
		print "validation error -- thresholdTextLength"
		print>>logOut, "validation error -- thresholdTextLength"
		return 0


for root,folder,files in os.walk(defPath):
	if files:
		for f in files:
			print f
			print>>logOut, f
			if f == "content":
				print "="*20 + "Getting Content" + "="*21
				print>>logOut, "="*20 + "Getting Content" + "="*21
				getHTMLData(root)
				print "="*26 + "Done" + "="*26
				print>>logOut, "="*26 + "Done" + "="*26
			elif f=="web1":
				print "*"*20 + "deleting web1" + "*"*20
				print>>logOut, "*"*20 + "deleting web1" + "*"*20
				os.system("rm "+root+'/'+f)
			else:
				print "*"*20 + "Unknown File >> "+ f + "*"*20
				print>>logOut, "*"*20 + "Unknown File >> "+ f + "*"*20
