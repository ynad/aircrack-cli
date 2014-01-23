/**CHeaderFile*****************************************************************

  FileName    [lib.h]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - Library functions]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]

  Revision    [2014-01-23]

******************************************************************************/


#ifndef LIB_H_INCLUDED
#define LIB_H_INCLUDED


//Version code - keep UPDATED!
#define VERS "1.1.8"
#define BUILD "2014-01-23"

#define BUFF 255
#define MACLST 100
#define MACLEN 17
#define FALSE 0
#define TRUE 1
#define AIRNETW "/tmp/airnetw"

//Type declaration
typedef struct maclist maclist_t;


/* Set environment variables and strings depending on OS type */
char setDistro(char **, char **, char **, char *);

/* PID files handling */
FILE *pidOpen(char *, char *);

/* Check MAC address format */
int checkMac(char *);

/* MAC address modifier */
void macchanger(char *, int);

/* Memory release */
void freeMem(maclist_t *);

/* Deauthenticate list of clients */
maclist_t *deauthClient(char *, char *, maclist_t *);

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

/* Check existance of monitor interface */
int checkMon(char *);

/* Replace old with new in string str */
char *replace_str(const char *, const char *, const char *);

/* Search network interfaces and chek if are wireless */
char *findWiface(int);


#endif // LIB_H_INCLUDED
