#!/bin/sh


# This test deletion functionality of backup files system and
# it deletes the backup file. This script checks the following cases
# case1 : when OLD is given (deletes the oldest version)
# case2 : when NEW is given (deletes the newest version)
# case3 : when ALL is given (deletes ALL the versions)

echo  "" >> results
echo  "" >> log
echo test06.sh:  >> results
echo test06.sh:  >> log

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

#case 1: deletes the oldest backup 
echo ./bkptcl -d OLD /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -d OLD /mnt/bkpfs/prog1.txt >> log
./bkptcl -d OLD /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl deletion failed with error $retval >> results
else
        echo bkptcl deletion succeded >> results
fi

echo  "" >> results
echo  "" >> log


#case 2: delete newest backup
echo ./bkptcl -d NEW /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -d NEW /mnt/bkpfs/prog1.txt >> log
./bkptcl -d NEW /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl deletion failed with error $retval >> results
else
        echo bkptcl deletion succeded >> results
fi

echo  "" >> results
echo  "" >> log

#case 1: print contents of 3rd  backup
echo ./bkptcl -d ALL /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -d ALL /mnt/bkpfs/prog1.txt >> log
./bkptcl -d ALL /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl deletion failed with error $retval >> results
else
        echo bkptcl deletion succeded >> results
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
