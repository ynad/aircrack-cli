/**CFile***********************************************************************

  FileName    [lib.c]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - Library functions]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]
  
  Revision    [beta-03, 2013-11-22]

******************************************************************************/


#ifndef __linux__
#error Compatible with Linux only!
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

//library functions header
#include <lib.h>

//error variable
//extern int errno;


/* PID files handling */
FILE *pidOpen(char *inmon, char *pidpath)
{
    FILE *fpid;

	//clear error value
	errno = 0;

    while ((fpid = fopen(pidpath, "r")) != NULL)     //if NULL I found the next free PID
        pidpath[strlen(pidpath)-1]++;
    inmon[strlen(inmon)-1] = pidpath[strlen(pidpath)-1];    //setting monitor interface number

    if ((fpid = fopen(pidpath, "w")) == NULL)
        fprintf(stderr, "Unable to write PID file \"%s\" (%s), this may cause problems running multiple instances of this program!\n\n", pidpath, strerror(errno));

    return fpid;
}


/* MAC address modifier */
void macchanger(char *inmon)
{
    char ifcnf[23], macchg[25]={"macchanger -a "};

    //shutting down interface
    sprintf(ifcnf, "ifconfig %s down", inmon);
    system(ifcnf);

    //changing MAC with a new one randomly generated
    strcat(macchg, inmon);
    system(macchg);

    //restarting interface
    sprintf(ifcnf, "ifconfig %s up", inmon);
    system(ifcnf);
}


/* Memory release */
void freeMem(char **maclist, int dim)
{
	int i;
	for (i=0; i<dim; i++)
		free(maclist[i]);
    free(maclist);
}


/* Acquisition of MAC list */
char **getList(char **maclist, int *dim)
{
	char mac[BUFF];

	//clear error value
	errno = 0;

	fprintf(stdout, "\nInput one MAC address per line (-1 to stop):\n");
	while (fprintf(stdout, " %d:\t", (*dim)+1) && fscanf(stdin, "%s", mac) == 1 && strcmp(mac, "-1")) {
		if (checkMac(mac) == FALSE) {
			fprintf(stderr, "Incorrect address or wrong format.\n");
		}
		else {
			maclist[*dim] = strdup(mac);
			if (maclist[*dim] == NULL) {
				fprintf(stderr, "Error allocating MAC address (pos. %d): %s.\n\n", *dim, strerror(errno));
				return maclist;
			}
			//number of collected addresses
			(*dim)++;
			//check if list (array as this case) is full, if so realloc
			if (*dim == MACLST) {
				maclist = (char**)realloc(maclist, MACLST*2 * sizeof(char*));
				if (maclist == NULL) {
					fprintf(stderr, "\nError reallocating MAC list (dim. %d): %s.\n\n", MACLST*2, strerror(errno));
					*dim = 0;
					return NULL;
				}
			}
		}
	}
	return maclist;
}


/* Check MAC address format */
int checkMac(char *mac)
{
	int i, len, punt=2;

	//address of wrong lenght
	if ((len = strlen(mac)) != MACLEN)
		return FALSE;

	//scan char by char to check for hexadecimal values and ':' separators
	for (i=0; i<len; i++) {
		if (i == punt) {
			if (mac[i] != ':')
				return FALSE;
			punt += 3;
		}
		else if (isxdigit((int)mac[i]) == FALSE)
			return FALSE;
	}
	return TRUE;
}


/* Determines number of existing/configured CPUs */
int procNumb()
{
	int nprocs = -1, nprocs_max = -1;

	//clear error value
	//errno = 0;

#ifdef _SC_NPROCESSORS_ONLN
	nprocs = sysconf(_SC_NPROCESSORS_ONLN);
	if (nprocs < 1) {
		//fprintf(stderr, "Could not determine number of CPUs online:\n%s\n", strerror(errno));
		return nprocs;
	}
	nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
	if (nprocs_max < 1) {
		//fprintf(stderr, "Could not determine number of CPUs configured:\n%s\n", strerror(errno));
		return nprocs_max;
	}
	//fprintf (stdout, "%d of %d processors online\n", nprocs, nprocs_max);
	return nprocs;
#else
	//fprintf(stderr, "Could not determine number of CPUs");
	return nprocs;
#endif
}

