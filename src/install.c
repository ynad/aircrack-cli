/**CFile***********************************************************************

  FileName    [install.c]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - Installer handler]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad, stethewwolf]

  License     [GPLv2, see LICENSE.md]
  
  Revision    [beta-04, 2013-12-23]

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
#define AIRNAME "aircrack-ng-1.2-beta1.tar.gz"
#define DISTRO "/proc/version"
#define UBUNTU "Ubuntu"
#define FEDORA "fedora"
#define SUSE "SUSE"
#define ARCH "ARCH"

/*
    NOTE:
        - supported only OS debian-based e RedHad-based (YUM/rpm) [Fedora, (Open)SUSE]

*/

//error variable
//extern int errno;


/* Dependencies installer */
int depInstall()
{
    char c='0', id='0';

    fprintf(stdout, "\nThe following dependencies are requested:\n"
            "\t* build-essential\n"
            "\t* libssl-dev/openssl-devel\n"
			"\t* xterm\n"
            "\t* wget\n"
            "\t* macchanger\n"
            "Continue?  [Y-N]\n"
            );

    //OS check: quit if not supported
    if ((id = checkDistro()) == '0')
		return (EXIT_FAILURE);

	do {
		scanf("%c%*c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && printf("Type only [Y-N]\n"));
    if (c == 'Y') {
        fprintf(stdout, "\nInstalling dependencies...\n\n");
		//Ubuntu
		if (id == 'u')
			system("apt-get install build-essential libssl-dev xterm wget macchanger --install-suggests -y --force-yes");
		//yum-based
		else if (id == 'y')
			system("yum install make automake gcc gcc-c++ kernel-devel openssl-devel xterm wget macchanger -y");
		else if (id == 'a')
			system("pacman -S base-devel openssl xterm wget macchanger --noconfirm");
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

    if (fgets(name, BUFF, fp) == NULL) {
		fprintf(stderr, "File \"%s\" corrupted!\n", DISTRO);
		return id;
	}
    fclose(fp);

    if (strstr(name, UBUNTU) != NULL) {
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
    char c='0', cgdir[BUFF], command[BUFF]={"wget http://download.aircrack-ng.org/"};
    DIR *dp;
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

    sprintf(command, "%s%s && tar -zxvf %s 1> tar.log", command, AIRNAME, AIRNAME);
    system(command);

    fprintf(stdout, "\nConfirm installation?  [Y-N]\n");
	do {
		scanf("%c%*c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && printf("Type only [Y-N]\n"));

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
        fprintf(stdout, "\n INSTALLATION ABORTED!\n\n");
        return (EXIT_FAILURE);
    }
    fprintf(stdout, "\n INSTALLATION COMPLETED!\n\n");
    return (EXIT_SUCCESS);
}
