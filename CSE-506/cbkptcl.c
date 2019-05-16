#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int main()
{
	int err_num;
	FILE *fp  = fopen("/mnt/bkpfs/prog1.txt", "a");
	if (fp == NULL) {
		err_num = errno;
		printf("Error in opening file:\n");
		printf("Errno= %d, %s\n", err_num, strerror(err_num));
		return -1;
	}
	fprintf(fp, "OS CSE506 : Assignment 2 by Akanksha Mahajan\n");
	fclose(fp);
	return 0;
}
