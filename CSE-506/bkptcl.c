// SPDX-License-Identifier: GPL-2.0

#include <asm/unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <asm/unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/bkpfs_common.h>

void help(void)
{
	printf("\nUsage: ./bkptcl {-l | -d <version> | -r <version> | ");
	printf("-v <version>} <input file>\n");
	printf("\nOptions :\n");
	printf("-l  : to list all the versions of a file\n");
	printf("-d  : to delete specified backup file\n");
	printf("-v  : to view contents of a backup file\n");
	printf("-r  : to restore the contents of backup file\n");
	printf("\tWith -d arguments should be ALL/NEW/OLD\n");
	printf("\tWith -r arguments should be NEW/OLD/version_num\n");
	printf("\tWith -v arguments should be NEW/OLD/version_num\n");
	printf("\tOnly one option should be at a time\n");
	printf("\tAtleast one option should be specified\n");
	printf("\nArguments :\n");
	printf("<input file> %1s: original input file\n\n", " ");
}

int main(int argc, char *argv[])
{
	int rc = 0, err_num, opt;
	int len, flag = 0, fd, bk_fd;
	struct xstr *arg = NULL;
	char *infile = NULL, *buf = NULL;

	while ((opt = getopt(argc, argv, "hld:v:r:")) != -1) {
		switch (opt) {
			case'l':
				flag |= 1;
				len = 20;
				arg = (struct xstr *)malloc(sizeof(struct xstr)
							    + len);
				arg->len = len;

				if (!arg) {
					printf("Could not allocate memory.\n");
					rc = -1;
					goto out;
				}
				break;
			case'd':
				flag |= 2;
				if (!optarg) {
					printf("Argument not specified with ");
					printf("-d. Try 'bkptcl -h' for more");
					printf("information\n");
					rc = -1;
					goto out_free_arg;
				}
				if (!strlen(optarg)) {
					printf("Argument specified with -d ");
					printf("is empty\n");
					rc = -1;
					goto out_free_arg;
				}
				len = strlen(optarg);
				arg = (struct xstr *)malloc(sizeof(struct xstr)
							    + len);
				strcpy(arg->str, optarg);
				arg->len = len;

				if (!arg) {
					printf("Could not allocate memory.\n");
					rc = -1;
					goto out;
				}
				break;
			case'v':
				flag |= 4;
				if (!optarg) {
					printf("Argument not specified with ");
					printf("-v. Try 'bkptcl -h' for more");
					printf("information\n");
					rc = -1;
					goto out_free_arg;
				}
				if (!strlen(optarg)) {
					printf("Argument specified with -d ");
					printf("is empty\n");
					rc = -1;
					goto out_free_arg;
				}
				len = strlen(optarg);
				arg = (struct xstr *)malloc(sizeof(struct xstr)
							    + len);
				strcpy(arg->str, optarg);
				arg->len = len;
				break;
			case'r':
				flag |= 8;
				if (!optarg) {
					printf("Argument not specified with ");
					printf("-d. Try 'bkptcl -h' for more");
					printf("information\n");
					rc = -1;
					goto out_free_arg;
				}
				if (!strlen(optarg)) {
					printf("Argument specified with -d ");
					printf("is empty\n");
					rc = -1;
					goto out_free_arg;
				}

				len = strlen(optarg);
				arg = (struct xstr *)malloc(sizeof(struct xstr)
							    + len);
				strcpy(arg->str, optarg);
				arg->len = len;

				if (!arg) {
					printf("Could not allocate memory.\n");
					rc = -1;
					goto out;
				}
				break;
			case'h':
				help();
				return 0;
			case':':
				printf("Missing arguments for %c. ", optopt);
				printf("Try 'bkptcl -h' for more ");
				printf("information.\n");
				rc = -1;
				goto out_free_arg;
			case'?':
				printf("Unknown option: %c. ", optopt);
				printf("Try 'bkptcl -h' for more ");
				printf("information.\n");
				rc = -1;
				goto out_free_arg;
		}
	}

	if (!flag) {
		printf("No option specified to list/view/delete/restore ");
		printf("the backup files. Try './bkptcl -h' for ");
		printf("more information.\n");
		rc = -1;
		goto out_free_arg;
	}

	if (flag & (flag - 1)) {
		printf("-l/-v/-d/-r option specified together. ");
		printf("Try '.bkptcl -h' for more information.\n");
		rc = -1;
		goto out_free_arg;
	}

	/* Get all of the non-option arguments */
	if (optind < argc) {
		if (argv[optind][0] == '\0') {
			printf("File name is empty. Use a valid name\n");
			rc = -1;
			goto out_free_arg;
		}
		infile = (char *)malloc(strlen(argv[optind]) + 1);
		if (!infile) {
			printf("Could not allocate memory to input ");
			printf("file name\n");
			rc = -1;
			goto out_free_arg;
		}
		strcpy(infile, argv[optind++]);
	} else {
		printf("Input file not specified. ");
		printf("Please check the arguments\n");
		rc = -1;
		goto out_free_arg;
	}

	if (optind < argc) {
		printf("Extra arguments are passed\n");
		printf("Try 'bkptcl -h' for more information.\n");
		rc = -1;
		goto out_free_arg;
	}

	/* Now use ioctl call to actually do the work */
	if (flag & 0x1) { /* list the versions */
		fd = open(infile, O_RDONLY);
		if (fd < 0) {
			rc = fd;
			err_num = errno;
			printf("Error in opening file %s\n", infile);
			printf("Errno= %d, %s\n", err_num, strerror(err_num));
			goto out_free_arg;
		}
		rc = ioctl(fd, FS_BKP_VERSION_LIST, (int *)arg);
		err_num = errno;
		if (rc < 0) {
			printf("Errno= %d, %s\n", err_num, strerror(err_num));
			goto out_free_arg;
		}
		rc = 0;
		printf("\nListing for file %s:\n%s\n\n", infile, arg->str);
		close(fd);

	} else if (flag & 0x2) { /* delete the version */
		fd = open(infile, O_RDONLY);
		if (fd < 0) {
			rc = fd;
			err_num = errno;
			printf("Error in opening file %s\n", infile);
			printf("Errno= %d, %s\n", err_num, strerror(err_num));
			goto out_free_arg;
		}
		rc = ioctl(fd, FS_BKP_VERSION_DELETE, (int *)arg);
		err_num = errno;
		if (rc < 0) {
			printf("Errno= %d, %s\n", err_num, strerror(err_num));
			goto out_free_arg;
		}
		rc = 0;
		printf("\nVersion %s deleted successfully\n\n", arg->str);
		close(fd);
	} else if (flag & 0x4) { /*view the contents of a version */
		buf = (char *)malloc(1024);

		/* get file descriptor of the backup file to read */
		fd = open(infile, O_RDONLY);
		if (fd < 0) {
			rc = fd;
			err_num = errno;
			printf("Error in opening file %s\n", infile);
			printf("Errno= %d, %s\n", err_num, strerror(err_num));
			goto out_free_arg;
		}

		bk_fd = ioctl(fd, FS_BKP_VERSION_OPEN_FILE, (int *)arg);
		if (bk_fd < 0) {
			err_num = errno;
			printf("Errno= %d, %s\n", err_num, strerror(err_num));
			goto out_free_arg;
		}

		/* read the contents of the backup file */
		printf("\nContents:\n");
		while ((rc = read(bk_fd, buf, 1024)) > 0)
			printf("%s\n", buf);
		if (rc < 0) {
			err_num = errno;
			printf("Errno= %d, %s\n", err_num, strerror(err_num));
			goto out_free_arg;
		}
		printf("\n");

		/* now close the backup file */
		rc = ioctl(fd, FS_BKP_VERSION_CLOSE_FILE, (int *)&bk_fd);
		if (rc < 0) {
			err_num = errno;
			printf("Errno= %d, %s\n", err_num, strerror(err_num));
			goto out_free_arg;
		}
	} else if (flag & 0x8) { /*restore the version */
		fd = open(infile, O_RDONLY);
		if (fd < 0) {
			rc = fd;
			err_num = errno;
			printf("Error in opening file %s\n", infile);
			printf("Errno= %d, %s\n", err_num, strerror(err_num));
			goto out_free_arg;
		}
		rc = ioctl(fd, FS_BKP_VERSION_RESTORE, (int *)arg);
		err_num = errno;
		if (rc < 0) {
			printf("Errno= %d, %s\n", err_num, strerror(err_num));
			goto out_free_arg;
		}
		rc = 0;
		printf("\nVersion %s restored successfully\n\n", arg->str);
		close(fd);
	}

	/* free all memory assigned in userland */
out_free_arg:
	if (buf)
		free(buf);
	if (infile)
		free(infile);
	if (arg)
		free(arg);
out:
	exit(rc);
}
