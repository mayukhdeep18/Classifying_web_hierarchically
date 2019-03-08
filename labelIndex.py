# -*- coding: utf-8 -*-
import codecs
import re
from collections import Counter
import os

cnt  = Counter()
count = 0

defPath = "datafiles/dmoz/TopNew2"
WriteFile = "datafiles/labelIndexMap.txt"
prefixPathLabel = "datafiles-dmoz-"


g1 = codecs.open(WriteFile,'w','utf-8')

index = 0

for root,folder,files in os.walk(defPath):
	labelNew = re.sub('/','-',root)
	labelNew = labelNew.split(prefixPathLabel)[1]
	s = str(index)+'\t'+labelNew
	print>>g1,s
	index += 1