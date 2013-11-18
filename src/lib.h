#ifndef LIB_H_INCLUDED
#define LIB_H_INCLUDED

/**
        Aircrack - Command Line Interface

		Library functions for Aircrack-CLI
        lib.h
**/

#define BUFF 255
#define FAIL 1
#define SUCCESS 0
#define FALSE 0
#define TRUE 1

/* PID files handling */
FILE *pidOpen(char *, char *);

/* MAC address modifier */
void macchanger(char*);

/* Memory release */
void freeMem(char **, int);

/* Acquisition of MAC list */
char **getList(char **, int *);

/* Determines number of existing/configured CPUs */
int procNumb();


#endif // LIB_H_INCLUDED
