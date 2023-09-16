#!/bin/sh

FILEDIR=$1
SEARCHSTR=$2

if [ $# -eq 2 ]
then
	if [ -d $FILEDIR ]
	then
		echo "$FILEDIR dir exists"

	else
		echo "$FILEDIR dir doesnot exists"
		exit 1
	fi
else
	echo "Number of argument is correct"
	exit 1
fi

FILECOUNT=$(ls "$FILEDIR"|wc -l)
MATCHCOUNT=$(grep -R $SEARCHSTR "$FILEDIR"|wc -l)

echo "The number of files are $FILECOUNT and the number of matching lines are $MATCHCOUNT"

