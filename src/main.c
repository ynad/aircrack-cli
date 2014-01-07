/**CFile***********************************************************************

  FileName    [main.c]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - Main module]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]
  
  Revision    [beta-04, 2014-01-07]

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
static int argCheck(int, char **, char *, char *, char);
static void printSyntax(char *, char, char *);
static void installer();
static void printMenu(char *, char *, maclist_t *, char *);
static int jammer(char *, char *, maclist_t *, char *, int);
static void stopMonitor(char*, char*);
static char netwPrompt(char *, char *, char, char *);
static void netwCheck(char, char *, char *);
static int checkExit(char c, char *, char *, char *, char *, char *);

//error variable
//extern int errno;


int main(int argc, char *argv[])
{
    //general variables
    int opz, inst=TRUE;
    FILE *fpid;
	maclist_t *maclst=NULL;

    //program strings
    char c, id, can[5], bssid[20], fout[BUFF], manag[BUFF], inmon[10]={"mon0"}, pidpath[20]={"/tmp/aircli-pid-0"}, *stdwlan=NULL, *netwstart=NULL, *netwstop=NULL,
         startmon[30]={"airmon-ng start"}, stopmon[30]={"airmon-ng stop "},
         montmp[BUFF]={"xterm 2> /dev/null -T MonitorTemp -e airodump-ng --encrypt wpa"},
		 scanmon[BUFF*2]={"xterm 2> /dev/null -T MonitorHandshake -e airodump-ng --bssid"};


	/** INITIAL STAGE **/

    //print initial message
    printHeader();

	//set environment variables and strings depending on OS type
	id = setDistro(&stdwlan, &netwstart, &netwstop, manag);

	//check arguments and set stuff
	inst = argCheck(argc, argv, startmon, stdwlan, id);

    //check execution rights
    if (getgid() != 0) {
        fprintf(stdout, "Run the program with administrator rights!\n\n");
		return (EXIT_FAILURE);
    }

    //run installer if not skipped
    if (inst == TRUE)
        installer();

	//prompt wheter to stop network-manager
	c = netwPrompt(netwstop, netwstart, id, manag);


    /** STAGE 1 **/

    fprintf(stdout, "\n\n === Start temporary monitor interface ===\n");
    system(startmon);

    //PID file handling
    fpid = pidOpen(inmon, pidpath);
	if (fpid != NULL)
		fclose(fpid);

    //monitor MAC changer (use macchanger)
    macchanger(inmon);

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
    checkExit(c, can, stopmon, pidpath, netwstart, manag);

    fprintf(stdout, "\nTarget BSSID (-1 to clean-exit):\t");
	do {
		fscanf(stdin, "%s", bssid);
	} while (strcmp(bssid, "-1") && checkMac(bssid) == FALSE && fprintf(stdout, "Incorrect address or wrong format:\t"));
    checkExit(c, bssid, stopmon, pidpath, netwstart, manag);

    //close scanner and monitor
    system(stopmon);
    sleep(1);


    /** STAGE 2 **/

    fprintf(stdout, "\n === Start monitor interface ===\n");
    strcat(startmon, can);
    system(startmon);

    //monitor MAC changer (use macchanger)
    macchanger(inmon);

    //output file
    fprintf(stdout, "\nOutput file (-1 to clean-exit):\t");
	getchar();
	fgets(fout, BUFF, stdin);
	fout[strlen(fout)-1] = '\0';
    checkExit(c, fout, stopmon, pidpath, netwstart, manag);

    //scanner handshake
    sprintf(scanmon, "%s %s -c %s -w \"%s\" %s --ignore-negative-one &", scanmon, bssid, can, fout, inmon);
    system(scanmon);

	//ciclic menu and action chooser
	printMenu(bssid, inmon, maclst, argv[0]);


	/** FINAL STAGE **/

    //close monitor, network manager check and memory free
    stopMonitor(stopmon, pidpath);
    netwCheck(c, netwstart, manag);
	freeMem(maclst);
	free(stdwlan); free(netwstart); free(netwstop);

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
static int argCheck(int argc, char **argv, char *startmon, char *stdwlan, char id)
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
static void printSyntax(char *name, char id, char *stdwlan)
{
    fprintf(stdout, "\nSyntax: ");	
    if (id == 'u')         //Debian-based
		fprintf(stdout, "sudo ");
    else if (id == 'y')    //RedHat-based
		fprintf(stdout, "su -c \"");
	else    //others
		fprintf(stdout, "su -c \"");
    fprintf(stdout, "%s [N] [WLAN-IF]", name);
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
static void printMenu(char *bssid, char *inmon, maclist_t *maclst, char *argv0)
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
				jammer(bssid, inmon, maclst, argv0, 1);
				break;
		case 3:
			    jammer(bssid, inmon, maclst, argv0, 2);
			    break;
            case 0:
                break;
            default:
                fprintf(stdout, "\nError: unexpected option!\n");
        }
    } while (opz != 0);
}


/* Interface to AirJammer */
static int jammer(char *bssid, char *inmon, maclist_t *maclst, char *argv0, int flag)
{
	char cmd[BUFF], path[BUFF], argvz[BUFF], list[]={"/tmp/maclist.txt"};
	int i, stop=0;

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
	if (flag == 2)
		sprintf(cmd, "xterm 2> /dev/null -T AirJammer -e %s/%s/airjammer.bin %s %s &", path, argvz, bssid, inmon);
	else
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


/* Prompt whether to stop network manager */
static char netwPrompt(char *netwstop, char *netwstart, char id, char *manag)
{
	char c, tmp[BUFF], newman[BUFF];

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
        system(netwstop);
		c = 'Y';
	}
    else if (c == 'Y')
        system(netwstop);
	return c;
}


/* Restart network manager if stopped */
static void netwCheck(char c, char *netwstart, char *manag)
{
    if (c == 'Y') {
		fprintf(stdout, "\nRestarting \"%s\"...\n", manag);
      system(netwstart);
      fprintf(stdout, "\n");
    }
}


/* Check wheter to clean-exit when given "-1" string */
static int checkExit(char c, char *string, char *stopmon, char *pidpath, char *netwstart, char *manag)
{
    if (!strcmp(string, "-1")) {
        netwCheck(c, netwstart, manag);
        stopMonitor(stopmon, pidpath);
        fprintf(stdout, "Exiting...\n");
        exit(EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

