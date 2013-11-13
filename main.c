/**
        Aircrack - Command Line Interface

        Interfaccia principale
        main.c
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

//header installatore
#include "install.h"

#define BUFF 255
#define FAIL 1
#define SUCCESS 0
#define FALSE 0
#define TRUE 1
#define MACLEN 17
#define STDWLAN " wlan0 "    //LASCIARE gli spazi prima e dopo il nome interfaccia!
#define NETWSTOP "service network-manager stop"
#define NETWSTART "service network-manager start"
#define CLEAR "clear"

#define VERS "beta-1"    //codice versione  -  TENERE AGGIORNATO!
//#define DEBUG

//prototipi
void printHeader();
void printSyntax(char*);
FILE *pidOpen(char *, char *);
void installer();
void macchanger(char*);
void deauthClient(char*, char*, char*);
void stopMonitor(char*, char*, FILE*);
void netwCheck(char);
int checkExit(char c, char *, char *, char *, FILE *);

/*
    TODO:
        - opzioni da argomenti: file di output, terminale, etc.

*/


int main(int argc, char *argv[])
{
    //variabili generali
    int opz, inst=TRUE;
    FILE *fpid;

    //stringhe programma
    char c, can[5], bssid[20], mac[20]={"0"}, fout[BUFF], inmon[]={"mon0"}, pidpath[]={"/tmp/aircli-pid-0"},
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
    /*macchanger(ifcnf, macchg, inmon);     fa casino con le interfacce */

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
        printf("\nCosa vuoi fare?\n\t1. De-autentica un client\n\t2. Termina la scansione\n");
        scanf("%d", &opz);
        switch (opz) {
            case 1:
                deauthClient(bssid, inmon, mac);
                break;
            case 2:
                break;

            default:
                printf("\nErrore: opzione non prevista!\n");
        }
    } while (opz != 2);

    //chiusura scanner e monitor
    stopMonitor(stopmon, pidpath, fpid);
    netwCheck(c);

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


/* gestione PID file */
FILE *pidOpen(char *inmon, char *pidpath)
{
    FILE *fpid;

    while ((fpid = fopen(pidpath, "r")) != NULL)     //se NULL ho trovato il pid libero
        pidpath[strlen(pidpath)-1]++;
    inmon[strlen(inmon)-1] = pidpath[strlen(pidpath)-1];    //setto numero interfaccia monitor

    #ifdef DEBUG
    printf("pidpath: %s\tinterf mon: %s\n", pidpath, inmon);
    #endif

    if ((fpid = fopen(pidpath, "w")) == NULL)
        printf("Impossibile scrivere il PID file, potrebbero verificarsi problemi con piu' istanze del programma!\n");

    return fpid;
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


/* modifica del MAC */
void macchanger(char *inmon)
{
    char ifcnf[23], macchg[25]={"macchanger -a "};

    //spengo interfaccia
    sprintf(ifcnf, "ifconfig %s down", inmon);
    system(ifcnf);

    //cambio del MAC
    strcat(macchg, inmon);
    system(macchg);

    //riavvio interfaccia
    sprintf(ifcnf, "ifconfig %s up", inmon);
    system(ifcnf);
}


/* deautenticazione client */
void deauthClient(char *bssid, char *inmon, char *mac_prev)
{
    char mac[20], death[BUFF]={"aireplay-ng --deauth 1 -a"};

    printf("\nIndirizzo MAC del client");
    if (strlen(mac_prev) == MACLEN) {
        printf(" (-1 per l'ultimo utilizzato %s)", mac_prev);
    }
    printf(":\t");
    scanf("%s", mac);
    if (!strcmp(mac, "-1"))
        strcpy(mac, mac_prev);

    sprintf(death, "%s %s -c %s %s --ignore-negative-one", death, bssid, mac, inmon);
    system(death);

    //salvo mac
    strcpy(mac_prev, mac);
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
