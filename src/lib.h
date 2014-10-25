/**CHeaderFile*****************************************************************

   FileName    [lib.h]

   PackageName [Aircrack-CLI]

   Synopsis    [Aircrack Command Line Interface - Library functions]

   Description [Command Line Interface for Aircrack-ng 
   (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-09-22]

******************************************************************************/


#ifndef LIB_H_INCLUDED
#define LIB_H_INCLUDED


//Version code - keep UPDATED!
#define VERS "1.2.4"
#define BUILD "2014-10-25"

#define BUFF 256
#define MACLST 256
#define MACLEN 17
#define FALSE 0
#define TRUE 1
#define AIRNETW "/tmp/airnetw"
#define WPA 1
#define WEP 2
#define OPN 1

//Type declaration
typedef struct maclist *maclist_t;


/* Set environment variables and strings depending on OS type */
char setDistro(char *, char *, char *, char *);

/* PID files handling */
void pidOpen(char *, char *, char *);

/* Check MAC address format */
int checkMac(char *);

/* MAC address modifier */
void macchanger(char *, int, char *);

/* Little workaround, put up and down wireless interface */
void ifconfUpdown(char *wl);

/* Memory release */
void freeMem(maclist_t);

/* Deauthenticate list of clients */
maclist_t deauthClient(char *, char *, maclist_t);

/* Wash interface */
void wash(char *, char *);

/* Reaver interface */
void reaver(char *, char *, char *);

/* Execute fake authentication (WEP networks) */
void fakeAuth(char *, char *, char *, int);

/* Launch ARP requests replay mode (WEP networks) */
void ARPreqReplay(char *, char *, char *);

/* Launch interactive packet replay (WEP networks) */
void interReplay(char *, char *);

/* WPA/WEP crack - dual method */
void packCrack(char *, char *, int);

/* Print maclist to specified file */
int fprintMaclist(maclist_t, char *);

/* Read MAC addresses from supplied file */
maclist_t freadMaclist(char *);

/* Return single MAC address */
char *getMac(maclist_t, int);

/* Return MAC list dimension */
int getDim(maclist_t);

/* Determine number of existing/configured CPUs */
int procNumb();

/* Check current version with info on online repo */
int checkVersion();

/* Check existance of monitor interface */
int checkMon(char *);

/* Check encryption type chosed */
int checkEncr(char *);

/* Replace old with new in string str */
char *replace_str(const char *, const char *, const char *);

/* Search network interfaces and chek if are wireless */
char *findWiface(int);


#endif // LIB_H_INCLUDED

