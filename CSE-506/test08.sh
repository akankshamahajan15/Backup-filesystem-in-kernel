#!/bin/sh

# This test basic functionality of backup files system and 
# runs the user (file creation) program by passing maxver=7
# It is to check if backup files are created properly
# or not. I am calling ./cbkptcl (user file append function)  10 times
# and each time a file is going for update, its backup file is created (total 7 in this case)

echo  "" >> results
echo  "" >> log
echo test08.sh:  >> results
echo test08.sh:  >> log

echo mount -t bkpfs -o maxver=7  /test/src/ /mnt/bkpfs >> results
echo mount -t bkpfs -o maxver=7 /test/src/ /mnt/bkpfs>>  log

mount -t bkpfs -o maxver=7 /test/src/ /mnt/bkpfs >> log

retval=$?
if test $retval != 0 ; then
	echo mount failed with error $retval >> results
else
	echo mount succeded >> results
fi

echo  "" >> results
echo  "" >> log

/bin/rm -f /mnt/bkpfs/prog1.txt

for i in {1..10}
do
	echo $i ./cbkptcl >> results
	echo $i ./cbkptcl >> log
	./cbkptcl
done

echo  "" >> results
echo  "" >> log

echo ./bkptcl -l /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -l /mnt/bkpfs/prog1.txt >> log
./bkptcl -l /mnt/bkpfs/prog1.txt >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl list failed with error $retval >> results
else
        echo bkptcl list succeded >> results
fi

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
