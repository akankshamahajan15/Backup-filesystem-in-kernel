#!/bin/sh


# This test listing  functionality of backup files system and
# it prints the version numbers of a file. It checks following cases
# case1 : when no backup exist (file created for the first time)
# case2 : when only one backup exist (when file goes for update)
# case3 : when more than one exist (when files goes for update multiple times)

echo  "" >> results
echo  "" >> log
echo test03.sh:  >> results
echo test03.sh:  >> log

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

echo 1. ./cbkptcl >> results
echo 1. ./cbkptcl >> log
./cbkptcl >> log

#case 1: when no backup exist
echo ./bkptcl -l /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -l /mnt/bkpfs/prog1.txt >> log
./bkptcl -l /mnt/bkpfs/prog1.txt >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl list failed with error $retval >> results
else
        echo bkptcl list succeded >> results
fi



echo 2. ./cbkptcl >> results
echo 2. ./cbkptcl >> log
./cbkptcl

#case 2: when only one backup exist
echo ./bkptcl -l /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -l /mnt/bkpfs/prog1.txt >> log
./bkptcl -l /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl list failed with error $retval >> results
else
        echo bkptcl list succeded >> results
fi

for i in {1..2}
do
	val=$((i+2));
	echo $val ./cbkptcl >> results
	echo $val ./cbkptcl >> log
	./cbkptcl  >> log

done

#case 3: when more than one backup exist
echo ./bkptcl -l /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -l /mnt/bkpfs/prog1.txt >> log
./bkptcl -l /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl list failed with error $retval >> results
else
        echo bkptcl list succeded >> results
fi

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
