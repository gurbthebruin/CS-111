#!/bin/bash

# Gurbir Arora 
# gurbthebruin@g.ucla.edu
# 105178554




echo | ./lab4b --period=1 --fail &> /dev/null
if [[ $? -eq 1 ]]
then
	echo "Error: Invalid args"
else
	echo "Failed to detect period bound error!!!"
fi

echo | ./lab4b --scale=eflkrmf &> /dev/null
if [[ $? -ne 1 ]]
then	
	echo "Failed test #2"
else
	echo "Passed test #2"
fi

