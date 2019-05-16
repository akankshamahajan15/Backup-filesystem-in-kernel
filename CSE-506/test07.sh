#!/bin/sh


# This test restore functionality of backup files system and
# it restores the backup file. This script checks the following cases
# case1 : when OLD is given
# case2 : when NEW is given
# case3 : when version number is given

echo  "" >> results
echo  "" >> log
echo test07.sh:  >> results
echo test07.sh:  >> log

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

#create 5 backups
for i in {1..6}
do
	echo $i ./cbkptcl >> results
	echo $i ./cbkptcl >> log
	./cbkptcl  >> log
done

echo  "" >> results
echo  "" >> log

echo cat /mnt/bkpfs/prog1.txt >> log
cat /mnt/bkpfs/prog1.txt >> log

echo  "" >> log

#case 1: restore the oldest backup 
echo ./bkptcl -r OLD /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -r OLD /mnt/bkpfs/prog1.txt >> log
./bkptcl -r OLD /mnt/bkpfs/prog1.txt  >> log

echo cat /mnt/bkpfs/prog1.txt >> log
cat /mnt/bkpfs/prog1.txt >> log

echo  "" >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl restore failed with error $retval >> results
else
        echo bkptcl restore succeded >> results
fi

echo  "" >> results
echo  "" >> log


#case 2: restore the newest backup
echo ./bkptcl -r NEW /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -r NEW /mnt/bkpfs/prog1.txt >> log
./bkptcl -r NEW /mnt/bkpfs/prog1.txt  >> log

echo "cat /mnt/bkpfs/prog1.txt" >> log
cat /mnt/bkpfs/prog1.txt >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl restore failed with error $retval >> results
else
        echo bkptcl restore succeded >> results
fi

echo  "" >> results
echo  "" >> log


#case 3: restore 3rd  backup
echo ./bkptcl -r 3 /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -r 3 /mnt/bkpfs/prog1.txt >> log
./bkptcl -r 3 /mnt/bkpfs/prog1.txt  >> log

echo cat /mnt/bkpfs/prog1.txt >> log
cat /mnt/bkpfs/prog1.txt >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl restore failed with error $retval >> results
else
        echo bkptcl restore succeded >> results
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
