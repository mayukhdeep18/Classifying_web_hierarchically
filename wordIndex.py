# -*- coding: utf-8 -*-
import codecs
import re
from collections import Counter
import os

cnt  = Counter()
count = 0

ReadFile = "datafiles/DictionaryFrequencyNewShort.txt"
WriteFile = "datafiles/wordIndexMap.txt"

g1 = codecs.open(ReadFile,'r','utf-8').readlines();
g2 = codecs.open(WriteFile,'w','utf-8')

index = 0

for l in g1:
	word,freq,nDoc = l.split('\t')
	s = str(index)+'\t'+word+'\t'+freq.strip()
	print>>g2,s
	index += 1