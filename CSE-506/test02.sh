#!/bin/sh


# This test maxversion functionality of backup files system and 
# runs the user (file creation) program 11 times.
# Since by default maxver is 5 so version 6 to 10 will be created.
# Maximum number of backups will be 5. 

echo  "" >> results
echo  "" >> log
echo test02.sh:  >> results
echo test02.sh:  >> log

echo mount -t bkpfs /test/src/ /mnt/bkpfs >> results
echo mount -t bkpfs /test/src/ /mnt/bkpfs>>  log

mount -t bkpfs /test/src/ /mnt/bkpfs >> log

retval=$?
if test $retval != 0 ; then
	echo mount failed with error $retval >> results
else
	echo mount succeded >> results
fi

echo  "" >> results
echo  "" >> log

/bin/rm -f /mnt/bkpfs/prog1.txt

for i in {1..11}
do
	echo $i ./cbkptcl >> results
	echo $i ./cbkptcl >> log
	./cbkptcl
done

echo  "" >> results
echo  "" >> log

echo umount /mnt/bkpfs >> results
echo umount /mnt/bkpfs>>  log

umount /mnt/bkpfs >> log

retval=$?
if test $retval != 0 ; then
        echo umount failed with error $retval >> results
else
        echo umount succeded >> results
fi

echo "------------------------------------------------------------------------------" >> results
echo "------------------------------------------------------------------------------" >> log
