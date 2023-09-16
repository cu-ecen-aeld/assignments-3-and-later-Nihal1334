#!/bin/bash

WRITEFILE=$1
WRITESTR=$2

if [ $# -eq 2 ]
then
	if [[ ! -d ${WRITEFILE%/*} ]]
	then
		##echo "File doesnot exists. Creating"
		mkdir -p "${WRITEFILE%/*}" && touch "$WRITEFILE"	
		
		if [[ ! -d ${WRITEFILE%/*} ]]
	        then
			echo "${WRITEFILE%/*} dir creation failed"
                	exit 1
        	fi
	fi
else
	echo "Number of argument is incorrect"
        exit 1
fi

echo "$WRITESTR" > "$WRITEFILE"
