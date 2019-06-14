					CSE 506 OPERATING SYSTEM SPRING 2019
					HOMEWORK ASSIGNMENT 2
					AKANKSHA MAAHAJAN
					112074564

How to Run:

    1. cd fs/bkpfs from hw2
    2. run install_modules.sh
    3. run mount -t bkpfs /test/src /mnt/bkpfs
        or mount -t bkpfs -o maxver=value /test/src /mnt/bkpfs
    4. cd CSE-506 from hw2
    5. run make                    	        => to compile user program bkptcl.c
    6. run ./cbkptcl (2 times)         		=> user program to create a file /mnt/bkpfs/prog1.txt
    7. run ./bkptcl -l | -r | -d | -v filename[/mnt/bkpfs/prog1.txt] 

    Assumptions to run test scripts: under TESTSCRIPTS section
-------------------------------------------------------------------------------------------------------------------------------------------             

Design:

    CREATION OF BACKUP FILES:

     Assumptions :   * Backup file name:    .bkp+ counter + _ + original_file_name
           	                             Foreg : .bkp1_prog1.txt

    		     * Backup file place:    Its created in the directory where original file is present.

		     * What is backup   :    Regular files are backedup as of now.

	             * BY DEFAULT, maxver = 5, if not passed through mount options.

                     * On creation of file for the first time, backup file is not created.
		       Its created from the second time for write/update mode.

    1. On first time user creates a file, its extended attributes are set : count = 0, max = provided on mount, first = 0, last = 0.
		count : Number of backups exist right now
		max   : Maximum number of backups allowed for a file
		first : oldest backup number of a file
		last  : newest backup number of a file
		All these values stored in struct version in bkpfs.h

    2. When next time user writes or appends to the file, first create a backup file in bkpfs_open and
       then updates the original file.

   ----------------------------------------------------------------------------

    VERISONING OF BACKUP FILE:

    1. Backups are created starting from 1 and incremented so on.
    2. For example, if file has versions 5, 6, 7, 8 then 5 is the oldest and 8 is the newest/recent one.
    3. When version exceeds maxver,
	3.1 oldest one is deleted first. (only one is deleted when required)
	3.2 version is incremented from latest one (last), then that version is created.

	Assumption/ Efficiencies :	* No renaming is done when oldest is deleted as its costly and complicated.
					* By incrementing version, without deletion, gives user the idea how much versions he has updated as of now.
					* If version is 4 to 7, user knows that he has updated 7 times since creation of file.

   ----------------------------------------------------------------------------

    Struct xstr, IOCTL CALLS:

    1. I have made a structure for efficiency purpose (cpu cache efficiency)
	struct xstr { 
		int len;
		char str[1];
	};
	object_xstr = malloc(sizeof (struct xstr) + len_of_string_passed);

    2. For ioctl calls, i pass this structure as third argument.
       Ioctl calls either :
			2.1 populate xstr->str for listing, viewed contents of a file.
       			2.2 or read this string to get the argument (OLD/ ALL/ NEW/ VERSION_NUM).

    ---------------------------------------------------------------------------

    LISTING OF BACKUP FILES:

    Command : ./bkptcl -l /mnt/bkpfs/program.txt

    1. Through ioctl call, request is sent to list the backup versions of a file.
    2. It reads the extened attributes to get the versions and populate xstr->str with
	case1 : "No backup exist"
	case2 : "Version : 1"
	case3 : "Version: 7 to 12".

    3. User program bkptcl.c print this string passed (xstr->str) on cmd.

	Assumptions :	* Maximum length of string can be in case 3 with digits version number.
			  So, max len passed for this string is 20. [space is saved].

    ---------------------------------------------------------------------------
    
    DELETION OF BACKUP FILES:

    command : ./bkptcl -d ALL/NEW/OLD /mnt/bkpfs/program.txt
	NOTE: * only ALL, NEW, OLD in capital letters is supported. 

    1. Through ioctl call, request is sent to list the backup versions of a file.
    2. Through version, name is created and then that backup file is deleted from upper and lower fs.
    3. Checks : 
	3.1. If no backup exist.
	3.2. If backup file version is out of range [first, last].
	3.3. Wrong option passed instead of NEW/OLD/ALL.
    4. Renaming of backup files is not done as mentioned in previous section.

   ----------------------------------------------------------------------------

   RESTORE BACKUP FILES:
   
    command : ./bkptcl -r NEW/OLD/version_num /mnt/bkpfs/program.txt
        NOTE: * NEW, OLD in capital letters is supported.
	      * version_num is 1 or 2 or 3...

    1. Through ioctl call, request is sent to list the backup versions of a file.
    2. Through version, name is created and open file and then copy the contents of that version to original file.
    3. Inode attributes and size is updated in both upper and lower fs.
    4. Checks :
        4.1. If no backup exist.
        4.2. If backup file version is out of range [first, last].
	4.3 Wrong option passed instead of NEW/OLD/NUM.

    ---------------------------------------------------------------------------

    VIEW CONTENTS OF BACKUP FILES:

    command :  ./bkptcl -v NEW/OLD/version_num /mnt/bkpfs/program.txt
        NOTE: * NEW, OLD in capital letters is supported.
              * version_num is 1 or 2 or 3...

    1. Through READ ioctl call, version file is opened and its fd is returned to user as return value.
    2. Once fd is returned, use genereal read function in while loop to read the contents.
    3. Print those contents on cmd
    4. Once all content is read, call CLOSE ioctl call to close the file. fd is passed as argument to the close ioctl.
    5. Checks :
	5.1. If no backup exist.
        5.2. If backup file version is out of range [first, last].
        5.3 Wrong option passed instead of NEW/OLD/NUM.    

	Assumptions :	* File is opened one time to be efficient.

    ---------------------------------------------------------------------------

    VISIBILITY:

    1. Backup files created are hidden from user on doing "ls" on upper fs (bkpfs) [Using filldir]
    2. Backup files are hidden and can't do "cat"/ "stat" on it on upper fs

    ---------------------------------------------------------------------------

    DELETION OF ORIGINAL FILE:

    1. When original file is deleted, its backup files are not deleted.
    2. But since I am storing all the info in extended attributes, so those backup files cannot be recovered through upper fs (bkpfs).
    3. When user creates a file with same original file then those backup files are overwritten.

--------------------------------------------------------------------------------------------------------------------------------------------

    NEW FILES INCLUDED :

    1.  hw2-amahajan/CSE-506/
    	1.1 Makefile        : make the user program bkpctcl.c and cbkptcl.c
    	1.2 cbkptcl.c       : user program to append the contents in /mnt/bkpfs/prog1.txt
    	1.3 bkptcl.c        : user program to view, list, delete and restore versions of a file
   	1.4 testscript.sh   : Master testscript that runs 13 sub test scripts starting from test01.sh ... test13.sh
   	1.5 test01.sh to    : Sub scripts to check basic functionality  and negative testcases
	    test13.sh

    2. hw2-amahajan/
	2.1 kernel.config

    3. hw2-amahajan/fs/bkpfs/
	3.1 copied the files from wrapfs and changed everything from wrapfs to bkpfs
        3.2 bkpfs_file.c   : contains all functions related to backup creation, listing, deletion, restore and view contents

    4. hw2-amahajan/include/linux/
        4.1 bkpfs_common.h : header file that contains new ioctl calls definition and struct xstr (mentioned in previous section)

-------------------------------------------------------------------------------------------------------------------------------------------

    USER PROGRAM "bkptcl.c" :

    1. It reads arguments using getopt().

    2. Arguments checking : This user level program does all the arguments checking. Following arguments checks are handled:
	1.1 No arguments specified.
	1.2 Multiple options are specified together => -d, -l, -r, -v. I am using bitwise operation to set only 1 bit and using
			bitwise operation to check how many bits are set using => flag & (flag-1). If it value is 0 then only
			bit is set else error out. 
	1.3 No argument passed with -d/ -v/ -r/ or no input file name specified.
	1.4 No option specified.
	1.5 Unknon options specified

-------------------------------------------------------------------------------------------------------------------------------------------

    USER PROGRAM "cblptcl.c"

    This user program is for test scripts purpose in order to append the data to the file /mnt/bkpfs/prog1.txt for backup creation.

-------------------------------------------------------------------------------------------------------------------------------------------

    TESTSCRIPTS :

	RUN TESTSCRIPTS :

	Important  : * vim resuts, log to view the logs and results of all testscripts
                     * create /mnt/bkpfs as mount point

        1. cd hw2-amahajan/fs/bkpfs
        2. install_modules.sh
        3. create a folder /mnt/bkpfs => mount point
        4. cd hw2-amahajan/CSE-506
        5. ./testscript.sh    => Master testscript
	6. vim log            => for logs (screen logs) of all testscripts in one file [for better readability]
	7. vim results        => for resuls of all tescripts in one place which have passed and which not along with error.

	
        
	testscript.sh : 1. It does the make of bkptcl.c and cbkptcl.c
                        2. Then it run 13 sub testscripts  [positive plus negative testcase]
        
	test01.sh to
        test12.sh    : 1. It contains the positive testcases of
		          1.1 mounting with option maxver and without option.
                          1.2 creation of backup files when count reaches max allowed.
			  1.3 listing, deletion, restore and view contents.

 		      2. Negative testcase:
                          2.1 Version passed is wrong.
                          2.2 Wrong mount option passed.
                          2.3 No argument passed with option -d/ -r/ -v or no file name mentioned.
                          2.4 No option specified or multiple option specified.

-------------------------------------------------------------------------------------------------------------------------------------------
Help function :

	./bkptcl -h for help function.

-------------------------------------------------------------------------------------------------------------------------------------------

Checkpatch.pl :
	Ran checkpatch.pl on bkpfs folder and user program bkptl.c

-------------------------------------------------------------------------------------------------------------------------------------------
References :
     1. kernel code.
     2. ecryptfs code.

-------------------------------------------------------------------------------------------------------------------------------------------
