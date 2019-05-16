#!/bin/sh


# This test reading functionality of backup files system and
# it prints the contents of a file. It checks following cases
# case1 : when OLD is given
# case2 : when NEW is given
# case3 : when any version is provided

echo  "" >> results
echo  "" >> log
echo test05.sh:  >> results
echo test05.sh:  >> log

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

#case 1: print contents of oldest backup 
echo ./bkptcl -v OLD /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -v OLD /mnt/bkpfs/prog1.txt >> log
./bkptcl -v OLD /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl view contents failed with error $retval >> results
else
        echo bkptcl view contents succeded >> results
fi

echo  "" >> results
echo  "" >> log


#case 2: print contents of newest backup
echo ./bkptcl -v NEW /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -v NEW /mnt/bkpfs/prog1.txt >> log
./bkptcl -v NEW /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl view contents failed with error $retval >> results
else
        echo bkptcl view contents succeded >> results
fi

echo  "" >> results
echo  "" >> log

#case 1: print contents of 3rd  backup
echo ./bkptcl -v 3 /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -v 3 /mnt/bkpfs/prog1.txt >> log
./bkptcl -v 3 /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl view contents failed with error $retval >> results
else
        echo bkptcl view contents succeded >> results
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
