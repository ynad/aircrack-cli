/**CHeaderFile*****************************************************************

  FileName    [lib.h]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - Library functions]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]

  Revision    [beta-03, 2013-11-21]

******************************************************************************/


#ifndef LIB_H_INCLUDED
#define LIB_H_INCLUDED


#define BUFF 255
#define MACLST 100
#define MACLEN 17
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

/* Check MAC address format */
int checkMac(char *);

/* Determines number of existing/configured CPUs */
int procNumb();


#endif // LIB_H_INCLUDED
