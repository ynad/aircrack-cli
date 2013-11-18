/**
        Aircrack - Command Line Interface

		Library functions for Aircrack-CLI
        lib.c
**/

#ifndef __linux__
#error Compatible with Linux only!
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


/* PID files handling */
FILE *pidOpen(char *inmon, char *pidpath)
{
    FILE *fpid;

    while ((fpid = fopen(pidpath, "r")) != NULL)     //if NULL I found the next free PID
        pidpath[strlen(pidpath)-1]++;
    inmon[strlen(inmon)-1] = pidpath[strlen(pidpath)-1];    //setting monitor interface number

    #ifdef DEBUG
    fprintf(stderr, "pidpath: %s\tinterf mon: %s\n", pidpath, inmon);
    #endif

    if ((fpid = fopen(pidpath, "w")) == NULL)
        fprintf(stderr, "Unable to write PID file, this may cause problems running multiple instances of this program!\n");

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
	char mac[20];

	fprintf(stdout, "\nInput one MAC address per line (-1 to stop):\n");
	while (fprintf(stdout, " %d:\t", (*dim)+1) && fscanf(stdin, "%s", mac) == 1 && strcmp(mac, "-1")) {
	   maclist[*dim] = strdup(mac);
	   if (maclist[*dim] == NULL) {
		   fprintf(stderr, "Error allocating MAC list, pos. %d (2).\n", *dim);
		   return maclist;
	   }
	   (*dim)++;    //number of collected addresses
	}
	return maclist;
}


/* Determines number of existing/configured CPUs */
int procNumb()
{
	int nprocs = -1, nprocs_max = -1;

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

