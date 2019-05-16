INC=/lib/modules/$(shell uname -r)/build

all: bkptcl cbkptcl

cbkptcl: cbkptcl.c
	gcc -Wall cbkptcl.c -o cbkptcl
bkptcl: bkptcl.c
	gcc -Wall -I$(INC)/include/   bkptcl.c -o bkptcl
