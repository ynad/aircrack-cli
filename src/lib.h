/**CHeaderFile*****************************************************************

  FileName    [lib.h]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - Library functions]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]

  Revision    [beta-04, 2014-01-05]

******************************************************************************/


#ifndef LIB_H_INCLUDED
#define LIB_H_INCLUDED


//Version code - keep UPDATED!
#define VERS "beta-04"
#define BUILD "2014-01-05"

#define BUFF 255
#define MACLST 100
#define MACLEN 17
#define FALSE 0
#define TRUE 1

//Type declaration
typedef struct maclist maclist_t;


/* PID files handling */
FILE *pidOpen(char *, char *);

/* MAC address modifier */
void macchanger(char*);

/* Memory release */
void freeMem(maclist_t *);

/* Deauthenticate list of clients */
maclist_t *deauthClient(char *, char *, maclist_t *);

/* Acquisition of MAC list */
maclist_t *getList(maclist_t *);

/* Print maclist to specified file */
int fprintMaclist(maclist_t *, char *);

/* Read MAC addresses from supplied file */
maclist_t *freadMaclist(char *);

/* Return single MAC address */
char *getMac(maclist_t *, int);

/* Return MAC list dimension */
int getDim(maclist_t *);

/* Determines number of existing/configured CPUs */
int procNumb();

/* Check current version with info on online repo */
int checkVersion();

/* Check MAC address format */
int checkMac(char *);

/* Replace old with new in string str */
char *replace_str(const char *, const char *, const char *);


#endif // LIB_H_INCLUDED
