/**CFile***********************************************************************

  FileName    [main.c]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - Main module]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]
  
  Revision    [beta-04, 2013-12-16]

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

//Library functions header
#include "lib.h"
//Installer handler header
#include "install.h"

//Module defines
#define CLEAR "clear"


//Prototypes
static void printHeader();
static char setDistro(char **, char **, char **);
static void printSyntax(char *, char, char *);
static void installer();
static int jammer(char *, char *, maclist_t *, char *);
static void stopMonitor(char*, char*);
static void netwCheck(char, char *);
static int checkExit(char c, char *, char *, char *, char *);

//error variable
//extern int errno;


int main(int argc, char *argv[])
{
    //variabili generali
    int opz, inst=TRUE;
    FILE *fpid;
	maclist_t *maclst=NULL;

    //stringhe programma
    char c, id, can[5], bssid[20], fout[BUFF], inmon[10]={"mon0"}, pidpath[20]={"/tmp/aircli-pid-0"}, *stdwlan=NULL, *netwstart=NULL, *netwstop=NULL,
         startmon[30]={"airmon-ng start"}, stopmon[30]={"airmon-ng stop "},
         montmp[BUFF]={"xterm 2> /dev/null -T MonitorTemp -e airodump-ng --encrypt wpa"},
		 scanmon[BUFF*2]={"xterm 2> /dev/null -T MonitorHandshake -e airodump-ng --bssid"};


    //stampa messaggio iniziale
    printHeader();

	//Set environment variables and strings depending on OS type
	id = setDistro(&stdwlan, &netwstart, &netwstop);

    //imposto nome interfaccia di rete e altri parametri
    if (argc >= 2) {
        if (!strcmp(argv[1], "N") || !strcmp(argv[1], "n")) {     //esclusione da parametro dell'installatore
            inst = FALSE;
            if (argc == 3)
                sprintf(startmon, "%s %s ", startmon, argv[2]);
            else
                strcat(startmon, stdwlan);
        }
        else
            sprintf(startmon, "%s %s ", startmon, argv[1]);
    }
    else {    //interfaccia standard
        strcat(startmon, stdwlan);	
		printSyntax(argv[0], id, stdwlan);
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
        system(netwstop);


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
	do {
		scanf("%s", can);
		sscanf(can, "%d", &opz);
	} while ((opz < 1 || opz > 14) && opz != -1 && printf("Allowed only channels in range [1-14]:\t"));

    checkExit(c, can, stopmon, pidpath, netwstart);

    printf("\nIdentificativo BSSID:\t");
	do {
		scanf("%s", bssid);
	} while (checkMac(bssid) == FALSE && printf("Incorrect address or wrong format:\t"));

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
    checkExit(c, fout, stopmon, pidpath, netwstart);

    //costruisco stringa
    sprintf(scanmon, "%s %s -c %s -w %s %s --ignore-negative-one &", scanmon, bssid, can, fout, inmon);

    //scanner handshake
    system(scanmon);

	opz = -1;
    //scelta azione
    do {
        printf("\n\nCosa vuoi fare?\n===============\n\t1. De-autentica client\n\t2. Jammer\n\t0. Termina la scansione\n");
        scanf("%d", &opz);
        switch (opz) {
            case 1:
				maclst = deauthClient(bssid, inmon, maclst);
                break;
		    case 2:
				jammer(bssid, inmon, maclst, argv[0]);
				break;
            case 0:
                break;
            default:
                printf("\nErrore: opzione non prevista!\n");
        }
    } while (opz != 0);

    //chiusura monitor, controllo network-manager e free memoria
    stopMonitor(stopmon, pidpath);
    netwCheck(c, netwstart);
	freeMem(maclst);
	free(stdwlan); free(netwstart); free(netwstop);

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


/* Set environment variables and strings depending on OS type */
static char setDistro(char **stdwlan, char **netwstart, char **netwstop)
{
	char id;

	//clear error value
	errno = 0;	

	id = checkDistro();
	//Debian-based
	if (id == 'u') {
		*stdwlan = strdup(" wlan0 ");
		*netwstart = strdup("service network-manager start");
		*netwstop = strdup("service network-manager stop");
		if (*stdwlan == NULL || *netwstart == NULL || *netwstop == NULL) {
			fprintf(stderr, "\nError allocating string(s): %s\n", strerror(errno));
			free(*stdwlan); free(*netwstart); free(*netwstop);
			return '0';
		}
	}
	//Redhat-based
	else if (id == 'y') {
		*stdwlan = strdup(" wlsp2s0 ");
		*netwstart = strdup("systemctl start NetworkManager.service");
		*netwstop = strdup("systemctl stop NetworkManager.service");
		if (*stdwlan == NULL || *netwstart == NULL || *netwstop == NULL) {
			fprintf(stderr, "\nError allocating string(s): %s\n", strerror(errno));
			free(*stdwlan); free(*netwstart); free(*netwstop);
			return '0';
		}
	}
	//Default for unknown distribution
	else if (id == '0') {
		fprintf(stdout, "\nWarning: there may be misbehavior!\n");
		*stdwlan = strdup(" wlan0 ");
		*netwstart = strdup("service network-manager start");
		*netwstop = strdup("service network-manager stop");
		if (*stdwlan == NULL || *netwstart == NULL || *netwstop == NULL) {
			fprintf(stderr, "\nError allocating string(s): %s\n", strerror(errno));
			free(*stdwlan); free(*netwstart); free(*netwstop);
			return '0';
		}
	}
	return id;
}


/* Prints program syntax */
static void printSyntax(char *name, char id, char *stdwlan)
{
    printf("\nSyntax: ");	
    if (id == 'u')         //Debian-based
		printf("sudo ");
    else if (id == 'y')    //RedHat-based
		printf("su -c \"");
	else    //others
		printf("su -c \"");
    printf("%s [N] [WLAN-IF]", name);
	if (id != 'u')
		printf("\"");
    printf("\nOptional parameters:\n\t[N] skips installation\n\t[WLAN-IF] specifies wireless interface (default \"%s\")\n\n", stdwlan);
}


/* Installer client */
static void installer()
{
    char c=0;

	//check program updates
	fprintf(stdout, "\nDo you want to check for available updates? (requires internet connection!)  [Y-N]\n");
	do {
		scanf("%c%*c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && printf("Type only [Y-N]\n"));
    if (c == 'Y')
		checkVersion();

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


/* Interface to AirJammer */
static int jammer(char *bssid, char *inmon, maclist_t *maclst, char *argv0)
{
	char cmd[BUFF], path[BUFF], argvz[BUFF], list[]={"/tmp/maclist.txt"};
	int i, stop=0;

	if (maclst == NULL) {
		fprintf(stdout, "\nError: MAC list is still empty!\n");
		return (EXIT_FAILURE);
	}
	if (fprintMaclist(maclst, list) == EXIT_FAILURE) {
		return (EXIT_FAILURE);
	}
	
	if (getcwd(path, BUFF) == NULL) {
        fprintf(stderr, "\nError reading current path.\n");
        return (EXIT_FAILURE);
    }
	//use a copy of argv[0], to not modify it
	strcpy(argvz, argv0);

	//calculate path of AirJammer executable
	for (i=strlen(argvz)-1; i>0 && !stop; i--) {
		if (argvz[i] == '/') {
			argvz[i] = '\0';
			stop = 1;
		}
	}
	sprintf(cmd, "xterm 2> /dev/null -T AirJammer -e %s/%s/airjammer.bin %s %s %s &", path, argvz, bssid, inmon, list);
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
static void netwCheck(char c, char *netwstart)
{
    if (c == 'Y') {
      printf("\nRestarting \"network-manager\"\n");
      system(netwstart);
      printf("\n");
    }
}


/* Check wheter to clean-exit when given "-1" string */
static int checkExit(char c, char *string, char *stopmon, char *pidpath, char *netwstart)
{
    if (!strcmp(string, "-1")) {
        netwCheck(c, netwstart);
        printf("Exiting...\n");
        stopMonitor(stopmon, pidpath);
        exit(EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

