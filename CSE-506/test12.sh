#!/bin/sh

#This is negative testcase for all possible combinations in general.

#case1 : No input file passed while running bkptcl
#case2 : Multiple options passed -d/ -r/-l all together
#case3 : No option passed

echo  "" >> results
echo  "" >> log
echo test11.sh:  >> results
echo test11.sh:  >> log

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

echo ./cbkptcl >> results
echo ./cbkptcl >> log
./cbkptcl  >> log

#no input file passed
echo ./bkptcl -l  >> results
echo ./bkptcl -l  >> log
./bkptcl -l   >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl list failed with error $retval >> results
else
        echo bkptcl list succeded >> results
fi

echo  "" >> results
echo  "" >> log


#when multiple options passed
echo ./bkptcl -r -d -l OLD /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -r -d -l OLD /mnt/bkpfs/prog1.txt >> log
./bkptcl -r -d -l OLD /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl failed with error $retval >> results
else
        echo bkptcl succeded >> results
fi

echo  "" >> results
echo  "" >> log

# when no option passed
echo ./bkptcl  /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl  /mnt/bkpfs/prog1.txt >> log
./bkptcl  /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl failed with error $retval >> results
else
        echo bkptcl succeded >> results
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
