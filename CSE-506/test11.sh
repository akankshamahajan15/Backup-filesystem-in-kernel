#!/bin/sh

# This is a negative test case of restoration of backup files.
# I call user programa one time for creation and for first time no backup is created.
# then i do lisitng of version which says no backup exist
# Then i do negative testing to restore
# case 1: Valid argument passed but no backup exist
# case 2: Illgeal argument passed with -r (negative value/0/ out of range
# case 3: No argument passed with -r

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

echo Negative testing when no backup exist and invalid args passed to restore backup files>> log
echo Negative testing when no backup exist and invalid args passed to restore backup files>> results

echo "" >> log
echo "" >> results

#when no backup exist and you tries to restore
echo ./bkptcl -r OLD /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -r OLD /mnt/bkpfs/prog1.txt >> log
./bkptcl -r OLD /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl restore failed with error $retval >> results
else
        echo bkptcl restore succeded >> results
fi

echo  "" >> results
echo  "" >> log

# when you try to pass illegal /out of range argument
echo ./bkptcl -r -5 /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -r -5 /mnt/bkpfs/prog1.txt >> log
./bkptcl -r -5 /mnt/bkpfs/prog1.txt  >> log

retval=$?
if test $retval != 0 ; then
        echo bkptcl restore failed with error $retval >> results
else
        echo bkptcl restore succeded >> results
fi

echo  "" >> results
echo  "" >> log

#case 3: no argument passed
echo ./bkptcl -r /mnt/bkpfs/prog1.txt >> results
echo ./bkptcl -r /mnt/bkpfs/prog1.txt >> log
./bkptcl -r  /mnt/bkpfs/prog1.txt  >> log

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
