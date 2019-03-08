#!/bin/bash
javac -Xlint trainFastXML.java treeFastXML.java wordMapIndex.java TestModel.java
for i in `seq 1 1`;
do
	#python CreateValidationSet.py
	rm FastXMLData/feat/*
	rm FastXMLData/label/*
	rm -rf FastXMLData/Models/*
	rm -rf FastXMLData/Scores/*
	rm FastXMLData/tempTestLibSVMFeature*
	java trainFastXML > output
done
