#!/bin/sh

# This test the functionality when wrong option is passed during mount

echo  "" >> results
echo  "" >> log
echo test13.sh:  >> results
echo test13.sh:  >> log

echo mount -t bkpfs -o max=7  /test/src/ /mnt/bkpfs >> results
echo mount -t bkpfs -o max=7 /test/src/ /mnt/bkpfs>>  log

mount -t bkpfs -o max=7 /test/src/ /mnt/bkpfs >> log

retval=$?
if test $retval != 0 ; then
	echo mount failed with error $retval >> results
else
	echo mount succeded >> results
fi


echo "------------------------------------------------------------------------------" >> results
echo "------------------------------------------------------------------------------" >> log
