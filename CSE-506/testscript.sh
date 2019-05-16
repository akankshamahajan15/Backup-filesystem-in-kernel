#!/bin/sh

#This testscript runs multiple scripts from test01.sh to test12.sh
#stores results in results file and running logs in log file

/bin/rm -f log
/bin/rm -f results

#compile bkptcl.c and cbkptcl.c
make 

touch log
touch results

for i in {01..13}
do
    echo running script test$i.sh
    ./test$i.sh
    echo completed script test$i.sh
    echo ""
done


echo "Check logs and results file in this folder to the details"
