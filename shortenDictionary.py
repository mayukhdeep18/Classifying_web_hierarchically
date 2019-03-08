# -*- coding: utf-8 -*-
import codecs
import re
from collections import Counter
import os

cnt  = Counter()
count = 0

WriteFile="datafiles/DictionaryFrequencyNewShort.txt"
ReadFile="datafiles/DictionaryFrequencyNew.txt"

g1 = codecs.open(ReadFile,'r','utf-8').readlines();
g2 = codecs.open(WriteFile,'w','utf-8')

for l in g1:
	word,freq,nDoc = l.split('\t')
	if(float(freq) >= 5.0):
		count+=1
		print>>g2,l.strip()

print "Total unique words left are ", count