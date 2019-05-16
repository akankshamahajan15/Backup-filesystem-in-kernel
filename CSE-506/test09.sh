#!/bin/sh


# This is a negative test case to view contents of backup files.
# I call user programa one time for creation and for first time no backup is created.
# then i do lisitng of version which says no backup exist
# Then i do negative testing fo viewing contents.
# case 1: Valid argument passed but no backup exist
# case 2: Illgeal argument passed with -dv(negative value/0/ out of range
# case 3: No argument passed with -v

echo  "" >> results
echo  "" >> log
echo test09.sh:  >> results
echo test09.sh:  >> log

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

echo Negative testing when no backup exist for view contents >> log
echo Negative testing when no backup exist for view contents>> results

echo "" >> log
echo "" >> results
#when no backup exist and you tries to read one
echo ./bkptcl -v NEW /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -v NEW /mnt/bkpfs/prog1.txt >> log
./bkptcl -v NEW /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl view contents failed with error $retval >> results
else
        echo bkptcl view ocntents succeded >> results
fi

echo  "" >> results
echo  "" >> log

# when you try to pass illegal /out of range argument
echo ./bkptcl -v -5 /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -v -5 /mnt/bkpfs/prog1.txt >> log
./bkptcl -v -5 /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl view contents failed with error $retval >> results
else
        echo bkptcl view contents succeded >> results
fi

echo  "" >> results
echo  "" >> log

#case 3: no argument passed
echo ./bkptcl -v /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -v /mnt/bkpfs/prog1.txt >> log
./bkptcl -v  /mnt/bkpfs/prog1.txt  >> log

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
