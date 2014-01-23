/**CFile***********************************************************************

  FileName    [main.c]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - Main module]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]
  
  Revision    [2014-01-23]

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
#include <signal.h>

//Library functions header
#include "lib.h"
//Installer handler header
#include "install.h"

//Module defines
#define CLEAR "clear"
#define MON "mon0"
#define PIDPTH "/tmp/aircli-pid-0"


//Local functions prototypes
static void printHeader();
static int argCheck(int, char **, char, char *, char *);
static void printSyntax(char *, char, char *);
static void installer();
static void printMenu(char *, char *, char *, maclist_t *);
static int jammer(char *, char *, char *, maclist_t *, int);
static char netwPrompt(char *, char *, char *, char *);
static void netwCheck(char, char *, char *, char *);
static int checkExit(char c, char *, char *, char *, char *, char *, char *, char *);
static void stopMonitor(char*, char*);
static void sigHandler(int);

//error variable
//extern int errno;


int main(int argc, char *argv[])
{
    //general variables
    int opz, inst=TRUE;
    FILE *fpid;
	maclist_t *maclst=NULL;

    //program strings
    char c, id, can[5], bssid[20], fout[BUFF], manag[BUFF], inmon[10]={MON}, pidpath[20]={PIDPTH}, *stdwlan=NULL, *netwstart=NULL, *netwstop=NULL,
         startmon[30]={"airmon-ng start"}, stopmon[30]={"airmon-ng stop "},
         montmp[BUFF]={"xterm 2> /dev/null -T MonitorTemp -e airodump-ng --encrypt wpa"},
		 scanmon[BUFF*2]={"xterm 2> /dev/null -T MonitorHandshake -e airodump-ng --bssid"};


	/** INITIAL STAGE **/

	//set signal handler for SIGINT (Ctrl-C)
	signal(SIGINT, sigHandler);

    //print initial message
    printHeader();

	//set environment variables and strings depending on OS type
	id = setDistro(&stdwlan, &netwstart, &netwstop, manag);

	//check arguments and set stuff
	inst = argCheck(argc, argv, id, startmon, stdwlan);

    //check execution rights
    if (getgid() != 0) {
        fprintf(stdout, "Run the program with administrator rights!\n\n");
		free(stdwlan); free(netwstart); free(netwstop);
		return (EXIT_FAILURE);
    }

    //run installer if not skipped
    if (inst == TRUE)
        installer();

	//prompt wheter to stop network-manager
	c = netwPrompt(netwstart, netwstop, stdwlan, manag);


    /** STAGE 1 **/

    fprintf(stdout, "\n\n === Start temporary monitor interface ===\n");
    system(startmon);

    //PID file handling and set inmon
    fpid = pidOpen(inmon, pidpath);
	if (fpid != NULL)
		fclose(fpid);

	//check monitor, if not found exit
	if (checkMon(inmon) == EXIT_FAILURE)
		checkExit(c, "-1", NULL, pidpath, netwstart, netwstop, stdwlan, manag);

    //monitor MAC changer (use macchanger)
	fprintf(stdout, "Changing MAC of interface \"%s\"...\n", inmon);
    macchanger(inmon, TRUE);

    //set stop monitor
    strcat(stopmon, inmon);

    //network scanner in detached terminal
    sprintf(montmp, "%s %s &", montmp, inmon);
    system(montmp);

    //data acquisition
    fprintf(stdout, "\nChannel number (-1 to clean-exit):\t");
	do {
		fscanf(stdin, "%s", can);
		sscanf(can, "%d", &opz);
	} while ((opz < 1 || opz > 14) && opz != -1 && fprintf(stdout, "Allowed only channels in range [1-14]:\t"));
    checkExit(c, can, stopmon, pidpath, netwstart, netwstop, stdwlan, manag);

    fprintf(stdout, "\nTarget BSSID (-1 to clean-exit):\t");
	do {
		fscanf(stdin, "%s", bssid);
	} while (strcmp(bssid, "-1") && checkMac(bssid) == FALSE && fprintf(stdout, "Incorrect address or wrong format:\t"));
    checkExit(c, bssid, stopmon, pidpath, netwstart, netwstop, stdwlan, manag);

    //close monitor and remove PID
	stopMonitor(stopmon, pidpath);
    sleep(1);


    /** STAGE 2 **/

    fprintf(stdout, "\n === Start monitor interface ===\n");
    strcat(startmon, can);
    system(startmon);

    //PID file handling and set inmon
	inmon[strlen(inmon)-1] = '0';
	pidpath[strlen(pidpath)-1] = '0';
    fpid = pidOpen(inmon, pidpath);
	if (fpid != NULL)
		fclose(fpid);
	//fix monitor number
	stopmon[strlen(stopmon)-1] = inmon[strlen(inmon)-1];

	//check monitor, if not found exit
	if (checkMon(inmon) == EXIT_FAILURE)
		checkExit(c, "-1", NULL, pidpath, netwstart, netwstop, stdwlan, manag);

    //monitor MAC changer (use macchanger)
	fprintf(stdout, "Changing MAC of interface \"%s\"...\n", inmon);
    macchanger(inmon, TRUE);

    //output file
    fprintf(stdout, "\nOutput file (-1 to clean-exit):\t");
	getchar();
	fgets(fout, BUFF, stdin);
	fout[strlen(fout)-1] = '\0';
    checkExit(c, fout, stopmon, pidpath, netwstart, netwstop, stdwlan, manag);

    //scanner handshake
    sprintf(scanmon, "%s %s -c %s -w \"%s\" %s --ignore-negative-one &", scanmon, bssid, can, fout, inmon);
    system(scanmon);

	//ciclic menu and action chooser
	printMenu(argv[0], bssid, inmon, maclst);


	/** FINAL STAGE **/
	fprintf(stdout, "Exiting...\n");

    //close monitor, network manager check and memory free
    stopMonitor(stopmon, pidpath);
    netwCheck(c, netwstart, stdwlan, manag);
	free(stdwlan); free(netwstart); free(netwstop);
	freeMem(maclst);

    //greetings
    fprintf(stdout, "\nTerminated.\nData written to file \"%s\". Launch \"./air-cat.sh\" to start cracking.\n\n", fout);

    return (EXIT_SUCCESS);
}


/* Prints program header */
static void printHeader()
{
    fprintf(stdout, "\n =======================================================================\n");
    fprintf(stdout, "    Aircrack-CLI  |  Command Line Interface - v. %s (%s)\n", VERS, BUILD);
    fprintf(stdout, " =======================================================================\n\n");
}


/* Check arguments and set stuff */
static int argCheck(int argc, char **argv, char id, char *startmon, char *stdwlan)
{
	int inst=TRUE;

    //set network interface name and other parameters
    if (argc >= 2) {
        if (!strcmp(argv[1], "N") || !strcmp(argv[1], "n")) {     //installer skipped
            inst = FALSE;
            if (argc == 3)
                sprintf(startmon, "%s %s ", startmon, argv[2]);
            else
                strcat(startmon, stdwlan);
        }
        else {
            sprintf(startmon, "%s %s ", startmon, argv[1]);
			if (argc == 3 && (!strcmp(argv[2], "N") || !strcmp(argv[2], "n")))
				inst = FALSE;
		}
    }
    else {    //standard interface
        strcat(startmon, stdwlan);	
		printSyntax(argv[0], id, stdwlan);
    }
	return inst;
}


/* Prints program syntax */
static void printSyntax(char *argv0, char id, char *stdwlan)
{
    fprintf(stdout, "\nSyntax: ");	
    if (id == 'u')         //Debian-based
		fprintf(stdout, "sudo ");
    else if (id == 'y')    //RedHat-based
		fprintf(stdout, "su -c \"");
	else    //others
		fprintf(stdout, "su -c \"");
    fprintf(stdout, "%s [N] [WLAN-IF]", argv0);
	if (id != 'u')
		fprintf(stdout, "\"");
    fprintf(stdout, "\nOptional parameters:\n\t[N]\t    skips installation\n\t[WLAN-IF]   specifies wireless interface (default \"%s\")\n\n", stdwlan);
}


/* Installer client */
static void installer()
{
    char c=0;

	//check program updates
	fprintf(stdout, "\nDo you want to check for available updates? (requires internet connection!)  [Y-N]\n");
	do {
		fscanf(stdin, "%c%*c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && fprintf(stdout, "Type only [Y-N]\n"));
    if (c == 'Y')
		checkVersion();

    //dependencies
    fprintf(stdout, "\nInstall requested dependencies to run this program?  [Y-N]\n");
	do {
		fscanf(stdin, "%c%*c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && fprintf(stdout, "Type only [Y-N]\n"));
    if (c == 'Y')
        if (depInstall())
            fprintf(stdout, "Attention: without all dependencies this program may not work properly!\n\n");
    c = '0';

    //aircrack-ng
    fprintf(stdout, "\nDownload and install \"Aircrack-ng\"?  [Y-N]\n");
	do {
		fscanf(stdin, "%c%*c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && fprintf(stdout, "Type only [Y-N]\n"));
    if (c == 'Y')
        if (akngInstall())
            fprintf(stdout, "Attention: \"Aircrack-ng\" NOT installed!\n\n");
}


/* Ciclic print menu and exec actions */
static void printMenu(char *argv0, char *bssid, char *inmon, maclist_t *maclst)
{
	int opz = -1;

    //action menu
    do {
        fprintf(stdout, "\n\nChoose an action:\n=================\n\t1. De-authenticate client(s)\n\t2. Jammer (MAC list)\n\t3. Jammer (broadcast)\n\t0. Stop scanner\n");
        fscanf(stdin, "%d", &opz);
        switch (opz) {
            case 1:
				maclst = deauthClient(bssid, inmon, maclst);
                break;
		    case 2:
				jammer(argv0, bssid, inmon, maclst, 1);
				break;
		    case 3:
			    jammer(argv0, bssid, inmon, maclst, 2);
			    break;
            case 0:
                break;
            default:
                fprintf(stdout, "\nError: unexpected option!\n");
        }
    } while (opz != 0);
}


/* Interface to AirJammer */
static int jammer(char *argv0, char *bssid, char *inmon, maclist_t *maclst, int flag)
{
	char cmd[BUFF], path[BUFF], argvz[BUFF], list[]={"/tmp/maclist.txt"};
	int i, stop=0;

	//clear error value
	errno = 0;

	if (flag == 1) {
		if (maclst == NULL) {
			fprintf(stdout, "\nError: MAC list is still empty!\n");
			return (EXIT_FAILURE);
		}
		if (fprintMaclist(maclst, list) == EXIT_FAILURE) {
			return (EXIT_FAILURE);
		}
	}
	
	if (getcwd(path, BUFF) == NULL) {
        fprintf(stderr, "\nError reading current path: %s\n", strerror(errno));
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
	if (flag == 2)
		sprintf(cmd, "xterm 2> /dev/null -T AirJammer -e %s/%s/airjammer.bin %s %s &", path, argvz, bssid, inmon);
	else
		sprintf(cmd, "xterm 2> /dev/null -T AirJammer -e %s/%s/airjammer.bin %s %s %s &", path, argvz, bssid, inmon, list);
	system(cmd);

	return (EXIT_SUCCESS);
}


/* Prompt whether to stop network manager */
static char netwPrompt(char *netwstart, char *netwstop, char *stdwlan, char *manag)
{
	char c, tmp[BUFF], newman[BUFF];
	FILE *fp;

	//clear error value
	errno = 0;

    c = '0';
    //stop potentially unwanted processes
    fprintf(stdout, "\nDo you want to stop \"%s\" (E to input different manager)?  [Y-N]\n", manag);
	do {
		fscanf(stdin, "%c%*c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && c != 'E' && fprintf(stdout, "Type only [Y-N]\n"));

	//edit actual network manager
	if (c == 'E') {
		fprintf(stdout, "Input new manager name:\t");
		fgets(tmp, BUFF, stdin);
		sscanf(tmp, "%s", newman);
		strcpy(netwstart, replace_str(netwstart, manag, newman));
		strcpy(netwstop, replace_str(netwstop, manag, newman));
		strcpy(manag, newman);
		c = 'Y';
	}
	//in case of Y or E, stop network manager and change MAC of stdwlan
    if (c == 'Y') {
		if (access(AIRNETW, F_OK) == 0)
			fprintf(stdout, "\"%s\" is already stopped.\n", manag);
		else {
			fprintf(stdout, "Stopping \"%s\"...\n", manag);
			system(netwstop);
			if ((fp = fopen(AIRNETW, "w")) == NULL)
				fprintf(stderr, "Unable to write %s ID file \"%s\" (%s), this may cause problems running multiple instances of this program!\n\n", manag, AIRNETW, strerror(errno));
			else {
				fprintf(fp, "%s\n", manag);
				fclose(fp);
			}
			fprintf(stdout, "\nChanging MAC of interface \"%s\"...\n", stdwlan);
			macchanger(stdwlan, TRUE);
		}
	}
	return c;
}


/* Restart network manager if stopped */
static void netwCheck(char c, char *netwstart, char *stdwlan, char *manag)
{
	char pidpath[20]={PIDPTH};
	int i, flag=TRUE;
	FILE *fp;

	//clear error value
	errno = 0;

	//check for program ID files
	if (access(AIRNETW, F_OK) == 0) {
		flag = FALSE;
		for (i=0; i<10 && flag == FALSE; i++) {
			pidpath[strlen(pidpath)-1] = '0'+i;
			if (access(pidpath, F_OK) == 0)
				flag = TRUE;
		}
		//if at least one is found
		if (flag == TRUE) {
			fprintf(stdout, "\nOther sessions of Aircrack-CLI running, restart of \"%s\" skipped.\n\n", manag);
			return;
		}
		//compare network manager name from arguments to the one written to file
		if ((fp = fopen(AIRNETW, "r")) == NULL)
			fprintf(stderr, "Unable to open %s ID file \"%s\": %s\n", manag, AIRNETW, strerror(errno));
		else {
			if (fscanf(fp, "%s", pidpath) == 1)
				if (strcmp(manag, pidpath) != 0)
					strcpy(manag, pidpath);
			fclose(fp);
		}
	}
    if (c == 'Y' || flag == FALSE) {
		fprintf(stdout, "\nRestoring permanent MAC of interface \"%s\"...\n", stdwlan);
		macchanger(stdwlan, FALSE);
		fprintf(stdout, "\nRestarting \"%s\"...\n", manag);
		system(netwstart);
		sprintf(pidpath, "rm -f %s", AIRNETW);
		system(pidpath);
		fprintf(stdout, "\n");
	}
}


/* Stops monitor interface and removes PID file */
static void stopMonitor(char *stopmon, char *pidpath)
{
    char clean[25]={"rm -f "};

	if (stopmon != NULL)
		system(stopmon);
    //remove PID file
    strcat(clean, pidpath);
    system(clean);
}


/* Check wheter to clean-exit when given "-1" string */
static int checkExit(char c, char *string, char *stopmon, char *pidpath, char *netwstart, char *netwstop, char *stdwlan, char *manag)
{
    if (!strcmp(string, "-1")) {
        fprintf(stdout, "Exiting...\n");
		stopMonitor(stopmon, pidpath);
        netwCheck(c, netwstart, stdwlan, manag);
		free(stdwlan); free(netwstart); free(netwstop);
		fprintf(stdout, "\nTerminated.\n\n");
        exit(EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}


/* Signal handler */
static void sigHandler(int sig)
{
	if (sig == SIGINT)
		fprintf(stderr, "\nReceived signal SIGINT (%d): exiting.\n", sig);
	else
		fprintf(stderr, "\nReceived signal %d: exiting.\n", sig);

	//exit
	exit(EXIT_FAILURE);
}

