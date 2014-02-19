/**CFile***********************************************************************

  FileName    [lib.c]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - Library functions]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad]

  License     [GPLv2, see LICENSE.md]
  
  Revision    [2014-02-17]

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
#include <stddef.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/wireless.h>

//Library functions header
#include "lib.h"
//Installer handler header
#include "install.h"

//URL to repo and file containing version info
#define REPO "github.com/ynad/aircrack-cli/"
#define URLVERS "https://raw.github.com/ynad/aircrack-cli/master/VERSION"

//Local functions prototypes
static maclist_t *getList(maclist_t *);
static int check_wireless(const char *, char *);

//error variable
//extern int errno;

//Definition of MAC address struct
struct maclist {
	char **macs;
	int dim;
};


/* Set environment variables and strings depending on OS type */
char setDistro(char *stdwlan, char *netwstart, char *netwstop, char *manag)
{
	char id, *tmp;
	FILE *fp;

	//clear error value
	errno = 0;	

	id = checkDistro();

	//set network manager
	if (access(AIRNETW, F_OK) == 0 && (fp = fopen(AIRNETW, "r")) != NULL) {
		fscanf(fp, "%s", manag);
		fclose(fp);
	}
	//network-manager
    else if (access("/usr/sbin/NetworkManager", F_OK) == 0) {
		if (id == 'y' || id == 'a')
			strcpy(manag, "NetworkManager");
		else
			strcpy(manag, "network-manager");
	}
	//Wicd
	else if (access("/usr/bin/wicd", F_OK) == 0)
		strcpy(manag, "wicd");
	//default
	else
		strcpy(manag, "network-manager");

	//set system service controller
	if (access("/usr/bin/systemctl", F_OK) == 0) {
		sprintf(netwstart, "systemctl start %s.service", manag);
		sprintf(netwstop, "systemctl stop %s.service", manag);
	}
	else if (access("/usr/bin/service", F_OK) == 0) {
		sprintf(netwstart, "service %s start", manag);
		sprintf(netwstop, "service %s stop", manag);
	}

	//set network wireless interface name, if automatic search fails use distro dependant settings
	tmp = findWiface(TRUE);
	if (tmp != NULL && strlen(tmp) > 0)
		sprintf(stdwlan, " %s ", tmp);
	else {
		//Debian-based
		if (id == 'u')
			strcpy(stdwlan, " wlan0 ");
		//Redhat-based
		else if (id == 'y' || id == 'a')
			strcpy(stdwlan, " wlp2s0 ");
		//Default for unknown distribution
		else if (id == '0') {
			fprintf(stdout, "Warning: there may be misbehavior!\n");
			strcpy(stdwlan, " wlan0 ");
		}
	}
	return id;
} 


/* PID files handling */
void pidOpen(char *inmon, char *pidpath, char *stdwlan)
{
    FILE *fpid;

	//clear error value
	errno = 0;

    while ((fpid = fopen(pidpath, "r")) != NULL)     //if NULL I found the next free PID
        pidpath[strlen(pidpath)-1]++;
    inmon[strlen(inmon)-1] = pidpath[strlen(pidpath)-1];    //setting monitor interface number

    if ((fpid = fopen(pidpath, "w")) == NULL)
        fprintf(stderr, "Warning: Unable to write PID file \"%s\" (%s), this may cause problems running multiple instances of this program!\n\n", pidpath, strerror(errno));
	else {
		fprintf(fpid, "%s\n", stdwlan);
		fclose(fpid);
	}
}


/* Check MAC address format */
int checkMac(char *mac)
{
	int i, len, punt=2;

	//address of wrong lenght
	if ((len = strlen(mac)) != MACLEN)
		return FALSE;

	//check last bit of first octet, must be even
	if (isdigit(mac[1]) && mac[1] % 2)
		return FALSE;
	else if (isalpha(mac[1]) && mac[1] % 2 == 0)
		return FALSE;

	//scan char by char to check for hexadecimal values and ':' separators
	for (i=0; i<len; i++) {
		if (i == punt) {
			if (mac[i] != ':')
				return FALSE;
			punt += 3;
		}
		else if (isxdigit((int)mac[i]) == FALSE)
			return FALSE;
	}
	return TRUE;
}


/* MAC address modifier */
void macchanger(char *inmon, int flag)
{
    char tmp[25];

    //shutting down interface
    sprintf(tmp, "ifconfig %s down", inmon);
    system(tmp);

    //changing MAC with a new one randomly generated or reset to permanent
	if (flag == FALSE)
		sprintf(tmp, "macchanger -p %s", inmon);
	else
		sprintf(tmp, "macchanger -a %s", inmon);
    system(tmp);

    //restarting interface
    //sprintf(tmp, "ifconfig %s up", inmon);
    //system(tmp);
}


/* Memory release */
void freeMem(maclist_t *maclst)
{
	int i;
	if (maclst != NULL) {
		for (i=0; i<maclst->dim; i++)
			free(maclst->macs[i]);
		free(maclst->macs);
		free(maclst);
	}
}


/* Deauthenticate list of clients */
maclist_t *deauthClient(char *bssid, char *inmon, maclist_t *maclst)
{
	char death[]={"aireplay-ng --deauth 1 -a"}, cmd[BUFF];
	int i;

	//if empty list, malloc and acquisition (standard dimension in MACLST)
	if (maclst == NULL) {
		maclst = (maclist_t *)malloc(sizeof(maclist_t));
		if (maclst == NULL) {
			fprintf(stderr, "Error allocating MAC struct.\n");
			return NULL;
		}
		maclst->macs = (char **)malloc(MACLST * sizeof(char *));
		if (maclst->macs == NULL) {
			fprintf(stderr, "Error allocating MAC list, dim %d.\n", MACLST);
			free(maclst);
			return NULL;
		}
		maclst->dim = 0;
		maclst = getList(maclst);
		//returns NULL if realloc fails
		if (maclst == NULL) {
			free(maclst);
			return NULL;
		}
		//if dim=0: error in first allocation or acquisition aborted
		if (maclst->dim == 0) {
			free(maclst);
			maclst = NULL;
			return NULL;
		}
	}
	else {
		printf("\nCurrent content MAC list:\n");
		for (i=0; i<maclst->dim; i++) {
			fprintf(stdout, "\t(%d) %s", i+1, maclst->macs[i]);
			if (i%2 == 0 && i != 0)
				printf("\n");
		}
		fprintf(stdout, "\n");	
		fprintf(stdout, "Choose an option:\n\t1. Run deauthentication\n\t2. Add MAC addresses\n\t3. Remove MAC address\n");
		scanf("%d", &i);
		switch (i) {
		    case 1:
				break;
			//Add MACs
		    case 2:
			   maclst = getList(maclst);
			   //returns NULL if realloc fails
			   if (maclst == NULL) {
		   			free(maclst);
				   return NULL;
			   }
			   break;
		   //Remove MAC (simplified)
		   case 3:
			   fprintf(stdout, "\nInput index you want to remove:\t");
			   scanf("%d", &i);
			   if (--i >= 0 && i < maclst->dim) {
				   free(maclst->macs[i]);
				   maclst->macs[i] = NULL;
			   }
			   else
				   fprintf(stdout, "Invalid index.\n");
			   break;
		   default:			
			   fprintf(stdout, "\nError: wrong option! Using actual list.\n");
		}	   
	}
	printf("\n");
	for (i=0; i<maclst->dim; i++) {
		if (maclst->macs[i] != NULL) {
			sprintf(cmd, "%s %s -c %s %s --ignore-negative-one", death, bssid, maclst->macs[i], inmon);
			fprintf(stdout, "\tDeAuth n.%d:\n", i+1);
			system(cmd);
		}
	}
	return maclst;
}


/* Execute fake authentication for WEP networks */
void fakeAuth(char *bssid, char *mon, char *mac, int mode)
{
	char cmd[BUFF];

	//simple fake auth
	if (mode == 1) {
		sprintf(cmd, "aireplay-ng -1 0 -a %s -h %s %s --ignore-negative-one", bssid, mac, mon);
	}
	//fake auth with keep alive, for tricky AP
	else if (mode == 2) {
		sprintf(cmd, "xterm 2> /dev/null -T FakeAuth -e aireplay-ng -1 6000 -o 1 -q 10 -a %s -h %s %s --ignore-negative-one &", bssid, mac, mon);
	}
	system(cmd);
}


/* Launch ARP requests replay mode */
void ARPreqReplay(char *bssid, char *mon, char *mac)
{
	char cmd[BUFF];

	sprintf(cmd, "xterm 2> /dev/null -T ARPreqReplay -e aireplay-ng -3 -b %s -h %s %s --ignore-negative-one &", bssid, mac, mon);
	system(cmd);
}


/* WPA/WEP crack - dual method */
void packCrack(char *pack, char *dict, int mode)
{
	char cmd[BUFF];
	if (mode == 1)
		sprintf(cmd, "xterm 2> /dev/null -T WpaCrack -e \"aircrack-ng %s*.cap -w %s ; read\" &", pack, dict);
	//PTW method
	else if (mode == 2)
		sprintf(cmd, "xterm 2> /dev/null -T WepCrack-PTW -e \"aircrack-ng %s*.cap ; read\" &", pack);
	//FMS/Korek method
	else if (mode == 3)
		sprintf(cmd, "xterm 2> /dev/null -T WepCrack-FMS_Korek -e \"aircrack-ng -K %s*.cap ; read\" &", pack);
	system(cmd);
}


/* Acquisition of MAC list */
static maclist_t *getList(maclist_t *maclst)
{
	char mac[BUFF];

	//clear error value
	errno = 0;

	//sanity check
	if (maclst == NULL || maclst->macs == NULL) {
		fprintf(stderr, "\nError: MAC list not initialized yet!\n");
		return NULL;
	}

	fprintf(stdout, "\nInput one MAC address per line (0 to stop):\n");
	while (fprintf(stdout, " %d:\t", maclst->dim+1) && fscanf(stdin, "%s", mac) == 1 && strcmp(mac, "0")) {
		if (checkMac(mac) == FALSE) {
			fprintf(stderr, "Incorrect address or wrong format.\n");
		}
		else {
			maclst->macs[maclst->dim] = strdup(mac);
			if (maclst->macs[maclst->dim] == NULL) {
				fprintf(stderr, "\nError allocating MAC address (pos. %d): %s.\n\n", maclst->dim, strerror(errno));
				return maclst;
			}
			//number of collected addresses
			maclst->dim++;
			//check if list (array as this case) is full, if so realloc
			if (maclst->dim == MACLST) {
				maclst->macs = (char **)realloc(maclst->macs, MACLST*2 * sizeof(char *));
				if (maclst->macs == NULL) {
					fprintf(stderr, "\nError reallocating MAC list (dim. %d): %s.\n\n", MACLST*2, strerror(errno));
					maclst->dim = 0;
					return NULL;
				}
			}
		}
	}
	return maclst;
}


/* Print maclist to specified file */
int fprintMaclist(maclist_t *maclst, char *file)
{
	FILE *fp;
	int i;

	//clear error value
	errno = 0;	

	if ((fp = fopen(file, "w")) == NULL) {
		fprintf(stderr, "\nError writing MAC list file \"%s\": %s.\n", file, strerror(errno));
		return (EXIT_FAILURE);
	}
	for (i=0; i<maclst->dim; i++) {
		if (maclst->macs[i] != NULL)
			fprintf(fp, "%s\n", maclst->macs[i]);
	}
	fclose(fp);

	return (EXIT_SUCCESS);
}


/* Read MAC addresses from supplied file */
maclist_t *freadMaclist(char *source)
{
	FILE *fp;
	char cmd[BUFF];
	int i, dim;
	maclist_t *maclst;

	//clear error value
	errno = 0;

	//opening file containing MAC list
	if ((fp = fopen(source, "r")) == NULL) {
		fprintf(stderr, "Error opening file \"%s\": %s.\n", source, strerror(errno));
		return NULL;
	}
	//checking list lenght
	dim = 0;
	while (fgets(cmd, BUFF, fp) != NULL)
		dim++;
	rewind(fp);

	maclst = (maclist_t *)malloc(sizeof(maclist_t));
	if (maclst == NULL) {
		printf("Error allocating MAC struct.\n");
		return NULL;
	}
	maclst->dim = 0;
	if ((maclst->macs = (char **)malloc(dim * sizeof(char *))) == NULL) {
		fprintf(stderr, "Error allocating MAC list (dim. %d): %s.\n", dim, strerror(errno));
		free(maclst);
		return NULL;
	}
	//reading file
	for (i=0; i<dim; i++) {
		fscanf(fp, "%s", cmd);
		//check correct format before adding to list
		if (checkMac(cmd) == FALSE) {
			fprintf(stderr, "Incorrect address or wrong format: \"%s\"\n", cmd);
		}
		else {
			maclst->macs[maclst->dim] = strdup(cmd);
			if (maclst->macs[maclst->dim] == NULL) {
				fprintf(stderr, "Error allocating MAC address (pos. %d): %s.\n", maclst->dim, strerror(errno));
				freeMem(maclst);
				return NULL;
			}
			maclst->dim++;
		}
	}
	return maclst;
}


/* Return single MAC address */
char *getMac(maclist_t *maclst, int i)
{
	return (maclst->macs[i]);
}


/* Return MAC list dimension */
int getDim(maclist_t *maclst)
{
	return (maclst->dim);
}


/* Determines number of existing/configured CPUs */
int procNumb()
{
	int nprocs = -1, nprocs_max = -1;

	//clear error value
	//errno = 0;

#ifdef _SC_NPROCESSORS_ONLN
	nprocs = sysconf(_SC_NPROCESSORS_ONLN);
	if (nprocs < 1) {
		//fprintf(stderr, "Could not determine number of CPUs online:\n%s\n", strerror(errno));
		return nprocs;
	}
	nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
	if (nprocs_max < 1) {
		//fprintf(stderr, "Could not determine number of CPUs configured:\n%s\n", strerror(errno));
		return nprocs_max;
	}
	//fprintf (stdout, "%d of %d processors online\n", nprocs, nprocs_max);
	return nprocs;
#else
	//fprintf(stderr, "Could not determine number of CPUs");
	return nprocs;
#endif
}


/* Check current version with info on online repo */
int checkVersion()
{
	char cmd[BUFF], vers[BUFF], build[BUFF];
	FILE *fp;

	//clear error value
	errno = 0;

	//download file
	sprintf(cmd, "wget %s -O /tmp/VERSION -q", URLVERS);
	system(cmd);

	if ((fp = fopen("/tmp/VERSION", "r")) == NULL) {
		fprintf(stderr, "Error reading from file \"%s\": %s\n", "/tmp/VERSION", strerror(errno));
		return (EXIT_FAILURE);
	}
	if (fscanf(fp, "%s %s", vers, build) != 2) {
		fprintf(stderr, "No data collected or no internet connection.\n\n");
		vers[0] = '-';
		vers[1] = '\0';
		build[0] = '-';
		build[1] = '\0';
	}
	else {
		if (strcmp(vers, VERS) < 0 || (strcmp(vers, VERS) == 0 && strcmp(build, BUILD) < 0)) {
			fprintf(stdout, "Newer version is in use (local: %s [%s], repo: %s [%s]).\n\n", VERS, BUILD, vers, build);
		}
		else if (strcmp(vers, VERS) == 0 && strcmp(build, BUILD) == 0) {
			fprintf(stdout, "Up-to-date version is in use (%s [%s]).\n\n", VERS, BUILD);
		}
		else {
			fprintf(stdout, "Version in use: %s (%s), available: %s (%s)\nCheck \"%s\" for updates!\n\n", VERS, BUILD, vers, build, REPO);
		}
	}
	fclose(fp);
	system("rm -f /tmp/VERSION");
	return (EXIT_SUCCESS);
}


/* Check existance of monitor interface */
int checkMon(char *mon)
{
	char *iface, *token;
	int flag;

	flag = 1;
	if ((iface = strdup(findWiface(FALSE))) == NULL)
		fprintf(stderr, "Error: can't determine network intefaces.\n");
	else {
		token=strtok(iface, " ");
        while (token && flag) {
			if (strcmp(token, mon) == 0)
				flag = 0;
            token=strtok(NULL, " ");
        }
		if (token == NULL && flag) {
			fprintf(stderr, "Error: wireless interface \"%s\" not found, no such device.\n", mon);
			free(iface);
			return (EXIT_FAILURE);
		}
	}
	free(iface);
	return (EXIT_SUCCESS);
}


/* Check encryption type chosed */
int checkEncr(char *str)
{
	int i;

	for (i=0; i<strlen(str); i++)
		str[i] = toupper(str[i]);
	i = 0;
	if (!strcmp(str, "WPA"))
		i = WPA;
	if (!strcmp(str, "WEP"))
		i = WEP;
	if (!strcmp(str, "OPN"))
		i = OPN;
	return i;
}


/* Replace old with new in string str */
char *replace_str(const char *str, const char *old, const char *new)
{
	char *ret, *r;
	const char *p, *q;
	size_t oldlen = strlen(old);
	size_t count, retlen, newlen = strlen(new);

	if (oldlen != newlen) {
		for (count = 0, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen)
			count++;
		/* this is undefined if p - str > PTRDIFF_MAX */
		retlen = p - str + strlen(p) + count * (newlen - oldlen);
	} else
		retlen = strlen(str);

	if ((ret = malloc(retlen + 1)) == NULL)
		return NULL;

	for (r = ret, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen) {
		/* this is undefined if q - p > PTRDIFF_MAX */
		ptrdiff_t l = q - p;
		memcpy(r, p, l);
		r += l;
		memcpy(r, new, newlen);

		r += newlen;
	}
	strcpy(r, p);

	return ret;
}


/* Search network interfaces and chek if are wireless */
char *findWiface(int flag)
{
	char *res;
	struct ifaddrs *ifaddr, *ifa;

	//clear error value
	errno = 0;
 
	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		return NULL;
	}
	//results string
	if ((res = (char *)malloc(BUFF * sizeof(char))) == NULL) {
		fprintf(stderr, "Error allocating string (dim. %d): %s.\n", BUFF, strerror(errno));
	}
	res[0] = '\0';
 
	/* Walk through linked list, maintaining head pointer so we
	   can free list later */
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		char protocol[IFNAMSIZ]  = {0};
 
		if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_PACKET) 
			continue;
 
		if (check_wireless(ifa->ifa_name, protocol)) {
			strcat(res, ifa->ifa_name);
			//returns firt result if flag is TRUE
			if (flag == TRUE) {
				freeifaddrs(ifaddr);
				return res;
			}
			//else concatenates each found interface
			strcat(res, " ");
		} 
	}
	freeifaddrs(ifaddr);
	res[strlen(res)-1] = '\0';
	return res;
}


/* Check if is wireless or not */
static int check_wireless(const char *ifname, char *protocol)
{
	int sock = -1;
	struct iwreq pwrq;
	memset(&pwrq, 0, sizeof(pwrq));
	strncpy(pwrq.ifr_name, ifname, IFNAMSIZ);
 
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return 0;
	}
 
	if (ioctl(sock, SIOCGIWNAME, &pwrq) != -1) {
		if (protocol) strncpy(protocol, pwrq.u.name, IFNAMSIZ);
		close(sock);
		return 1;
	}
 
	close(sock);
	return 0;
}

