/**CFile***********************************************************************

   FileName    [install.c]

   PackageName [Aircrack-CLI]

   Synopsis    [Aircrack Command Line Interface - Installer handler]

   Description [Command Line Interface for Aircrack-ng 
   (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

   Author      [ynad, stethewwolf]

   License     [GPLv2, see LICENSE.md]
  
   Revision    [2014-06-10]

******************************************************************************/


#ifndef __linux__
#error Compatible with Linux only!
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

//installer handler header
#include "install.h"

#define BUFF 255
#define FALSE 0
#define TRUE 1

#define TMPDIR "/tmp/ngtmp/"
#define AIRNAME "aircrack-ng-1.2-beta3.tar.gz"
#define AIRCODE "1.2-beta3"
#define AIRVERS "https://raw.github.com/aircrack-ng/aircrack-ng/master/VERSION"
#define DISTRO "/proc/version"
#define UBUNTU "Ubuntu"
#define DEBIAN "Debian"
#define FEDORA "fedora"
#define SUSE "SUSE"
#define ARCH "ARCH"

//error variable
//extern int errno;


/* Dependencies installer */
int depInstall()
{
	char c='0', id='0', buff[BUFF];

	fprintf(stdout, "\nThe following dependencies are requested:\n"
			"\t* build-essential\n"
			"\t* libssl-dev/openssl-devel\n"
			"\t* libnl-3-dev, libnl-genl-3-dev\n"
			"\t* xterm\n"
			"\t* wget\n"
			"\t* macchanger\n"
			"Continue?  [Y-N]\n"
			);

	//OS check: quit if not supported
	if ((id = checkDistro()) == '0')
		return (EXIT_FAILURE);

	do {
		fgets(buff, BUFF-1, stdin);
		sscanf(buff, "%c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && fprintf(stdout, "Type only [Y-N]\n"));
	if (c == 'Y') {
		fprintf(stdout, "\nInstalling dependencies...\n\n");
		//Ubuntu
		if (id == 'u')
			system("apt-get install build-essential libssl-dev libnl-3-dev libnl-genl-3-dev xterm wget macchanger --install-suggests -y --force-yes");
		//yum-based
		else if (id == 'y')
			system("yum install make automake gcc gcc-c++ kernel-devel openssl-devel libnl3-devel libnl-genl3-devel xterm wget macchanger -y");
		else if (id == 'a')
			system("pacman -S base-devel openssl libnl xterm wget macchanger --noconfirm");
		else {
			fprintf(stderr, "ERROR\n");
			return (EXIT_FAILURE);
		}
	}
	else {
		fprintf(stdout, "Execution aborted.\n");
		return (EXIT_FAILURE);
	}
	fprintf(stdout, "\n");
	return (EXIT_SUCCESS);
}


/* OS version checker */
char checkDistro()
{
	FILE *fp;
	char name[BUFF], id='0';

	//clear error value
	errno = 0;

	//reading info file
	if ((fp = fopen(DISTRO, "r")) == NULL) {
		fprintf(stderr, "Error opening file \"%s\", there may be misbehavior: %s.\n", DISTRO, strerror(errno));
		return id;
	}

	if (fgets(name, BUFF-1, fp) == NULL) {
		fprintf(stderr, "File \"%s\" corrupted!\n", DISTRO);
		return id;
	}
	fclose(fp);

	if (strstr(name, UBUNTU) != NULL || strstr(name, DEBIAN) != NULL) {
		id = 'u';
	}
	else if (strstr(name, FEDORA) != NULL || strstr(name, SUSE) != NULL) {
		id = 'y';
	} 
	else if (strstr(name, ARCH) != NULL) {
		id = 'a';
	}
	else {
		fprintf(stderr, "\nDistribution not supported:\n%s\n", name);
	}

	return id;
}


/* Aircrack-ng downloader and installer */
int akngInstall()
{
	int go=TRUE;
	char c='0', buff[BUFF], cgdir[BUFF], vers[BUFF], command[BUFF]={"wget http://download.aircrack-ng.org/"};
	DIR *dp;
	FILE *fp;
	struct dirent *dirp;
	struct stat statbuf;

	//clear error value
	errno = 0;

	if (getcwd(cgdir, BUFF) == NULL) {
		fprintf(stderr, "Error reading current directory: %s.\n", strerror(errno));
		return (EXIT_FAILURE);
	}
	if (mkdir(TMPDIR, S_IRWXU) == -1) {
		fprintf(stderr, "Error creating temporary directory \"%s\": %s\n", TMPDIR, strerror(errno));
		return (EXIT_FAILURE);
	}
	if (chdir(TMPDIR) == -1) {
		fprintf(stderr, "Error moving to directory \"%s\": %s.\n", TMPDIR, strerror(errno));
		return (EXIT_FAILURE);
	}

	fprintf(stdout, "\n\tDownloading and extracting source code...\n\n");

	//version check - errors muted
	sprintf(vers, "wget %s -O /tmp/AIRVERSION -q", AIRVERS);
	system(vers);
	if ((fp = fopen("/tmp/AIRVERSION", "r")) != NULL) {
		if (fscanf(fp, "%s", vers) != 1);
			//fprintf(stderr, "No data collected or no internet connection.\n\n");
		else {
			if (strcmp(vers, AIRCODE) > 0) {
				fprintf(stdout, "\nA newer version of \"Aircrack-ng\" is available (%s), do you want to use it? (default is %s)  [Y-N]\n", vers, AIRCODE);
				do {
					fgets(buff, BUFF-1, stdin);
					sscanf(buff, "%c", &c);
					c = toupper(c);
				} while (c != 'Y' && c != 'N' && fprintf(stdout, "Type only [Y-N]\n"));
			}
		}
		fclose(fp);
	}
	//else
		//fprintf(stderr, "Error reading from file \"%s\": %s\n", "/tmp/AIRVERSION", strerror(errno));
	system("rm -f /tmp/AIRVERSION");

	//download chosen version
	if (c == 'Y')
		sprintf(command, "%saircrack-ng-%s.tar.gz && tar -zxvf aircrack-ng-%s.tar.gz 1> tar.log", command, vers, vers);
	else
		sprintf(command, "%s%s && tar -zxvf %s 1> tar.log", command, AIRNAME, AIRNAME);
	system(command);

	fprintf(stdout, "\nConfirm installation?  [Y-N]\n");
	do {
		fgets(buff, BUFF-1, stdin);
		sscanf(buff, "%c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && fprintf(stdout, "Type only [Y-N]\n"));

	if( c=='Y' ) {
		fprintf(stdout, "\tCompiling and installing... (in case of errors check dependencies)\n\n");

		if ( (dp = opendir(TMPDIR)) == NULL ) {
			fprintf(stderr, "Error opening directory \"%s\": %s.\n", TMPDIR, strerror(errno));
			return (EXIT_FAILURE);
		}
		while ((dirp = readdir(dp)) != NULL && go == TRUE) {    //search extracted dir and move to it
			sprintf(command, "%s%s", TMPDIR, dirp->d_name);
			if (lstat(command, &statbuf) < 0) {
				fprintf(stderr, "Error reading stat (%s): %s.\n", command, strerror(errno));
				return (EXIT_FAILURE);
			}
			if (S_ISDIR(statbuf.st_mode) != 0 && strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, "..")) {
				if (chdir(command) == -1) {
					fprintf(stderr, "Error moving to directory \"%s\": %s.\n", command, strerror(errno));
					return (EXIT_FAILURE);
				}
				go = FALSE;
			}
		}

		//complide and install
		system("make && make install");

		if (closedir(dp) < 0) {
			fprintf(stderr, "Error closing directory \"%s\": %s\n", TMPDIR, strerror(errno));
			//return (EXIT_FAILURE);
		}
		go = FALSE;
	}

	if (chdir(cgdir) == -1) {       //go back to original path
		fprintf(stderr, "Error moving to directory \"%s\": %s.\n", cgdir, strerror(errno));
		return (EXIT_FAILURE);
	}
	fprintf(stdout, "\n\tRemoving temporary directory...\n");
	sprintf(command, "rm -fr %s", TMPDIR);
	system(command);

	if (go == TRUE) {
		fprintf(stdout, "\n INSTALLATION ABORTED!\n");
		return (EXIT_FAILURE);
	}
	fprintf(stdout, "\n INSTALLATION COMPLETED!\n\n");
	return (EXIT_SUCCESS);
}

