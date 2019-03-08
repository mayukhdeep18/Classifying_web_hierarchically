"""
It takes a set of feature vector files and partition them in test and training data with threshold ratio
"""

import os
path="featureVector/AllVectorNew2StopStemSnowballFreq"

categories=[]
numFiles={}
for root,folder,files in os.walk(path):
	if files:
		for f in files:
			print f
			LINES=open(path+f,'r').readlines()[1:]
			for line in LINES:
				line=line.replace('	',',')
				category,rem=line.split(',',1)
				newCat=category.split('-')[1:-1]
				newpath='/'.join(newCat)
				if newpath not in numFiles:
					numFiles[newpath]=1
					if not os.path.exists("Top_tf/"+newpath):
						os.makedirs('Top_tf/'+newpath)
					print "created >> ",newpath
				else:
					numFiles[newpath]+=1
				num = numFiles[newpath]
				fin=open('Top_tf/'+newpath+'/'+str(num),'w')
				print>>fin,line
