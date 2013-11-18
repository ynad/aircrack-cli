/**
        Aircrack - Command Line Interface

        Gestore installazioni
        install.c
**/

#ifndef __linux__
#error Compatible with Linux only!
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

//header installatore
#include "install.h"

#define BUFF 255
#define FAIL 1
#define SUCCESS 0
#define FALSE 0
#define TRUE 1
#define TMPDIR "/tmp/ngtmp/"
#define AIRNAME "aircrack-ng-1.2-beta1.tar.gz"
#define DISTRO "/proc/version"
#define UBUNTU "Ubuntu"
#define FEDORA "fedora"
#define SUSE "SUSE"

/*
    NOTE:
        - supportati OS debian-based e RedHad-based (YUM/rpm) [Fedora, (Open)SUSE]

*/


/* installatore dipendenze */
int dep_install()
{
    char c='0', id='0';

    //controllo OS, se non supportato esco
    if ((id = checkDistro()) == '0')
      return FAIL;

    printf("\nE' necessario installare le seguenti dipendenze:\n"
            "\t* build-essential\n"
            "\t* libssl-dev/openssl-devel\n"
            "\t* wget\n"
            "\t* macchanger\n"
            "Continuare?  [S-N]\n"
            );

    while (c != 'S' && c != 'N') {
        scanf("%c", &c);
        c = toupper(c);
    }
    if (c == 'S') {
        printf("\nInstallazione dipendenze...\n");
	//Ubuntu
	if (id == 'u')
	  system("apt-get install build-essential libssl-dev wget macchanger --install-suggests -y --force-yes");
	//yum-based
	else if (id == 'y')
	  system("yum install make automake gcc gcc-c++ kernel-devel openssl-devel wget macchanger -y");
	else {
	  printf("ERRORE\n");
	  return FAIL;
	}
    }
    else {
        printf("Esecuzione interrotta!\n");
        return FAIL;
    }
    return SUCCESS;
}


/* controllo versione distribuzione OS  */
char checkDistro()
{
    FILE *fp;
    char name[BUFF], id;

    //controllo versione distribuzione
    if ((fp = fopen(DISTRO, "r")) == NULL)
      printf("Errore apertura file %s, potrebbero verificarsi errori.\n", DISTRO);

    if (fgets(name, BUFF, fp) == NULL)
      printf("File %s non corretto!\n", DISTRO);

    if (strstr(name, UBUNTU) != NULL) {
      id = 'u';
    }
    else if (strstr(name, FEDORA) != NULL || strstr(name, SUSE) != NULL) {
      id = 'y';
    } 
    else {
      id = '0';
      printf("\nDistribuzione non supportata:\n%s\n", name);
    }
    fclose(fp);

    return id;
}


/* installatore aircrack */
int akng_install()
{
    int go=TRUE;
    char c='0', cgdir[BUFF], command[BUFF]={"wget http://download.aircrack-ng.org/"};
    DIR *dp;
    struct dirent *dirp;
    struct stat statbuf;

    printf("\n\tScaricamento ed estrazione sorgenti...\n\n");

    if (getcwd(cgdir, BUFF) == NULL) {
        printf("Errore lettura cartella corrente.\n");
        exit(FAIL);
    }
    if (mkdir(TMPDIR, S_IRWXU) == -1) {     //creazione cartella temporanea
        printf("Errore crezione cartella \"%s\"\n", TMPDIR);
        exit(FAIL);
	}
	if (chdir(TMPDIR) == -1) {              //spostamento in cartella temporanea
        printf("Errore spostamento in directory \"%s\".\n", TMPDIR);
        exit(FAIL);
    }

    //scaricamento ed estrazione sorgenti
    sprintf(command, "%s%s && tar -zxvf %s", command, AIRNAME, AIRNAME);
    printf("%s\n", command);
    system(command);

    printf("\nConfermi l'installazione?  [S-N]\n");
    while( c!='S' && c!='N') {
        scanf("%c", &c);
        c=toupper(c);
    }

    if( c=='S' ) {
        printf("\tEseguo compilazione e installo... (in caso di errori verificare le dipendenze)\n\n");

        if ( (dp = opendir(TMPDIR)) == NULL ) {
            printf("Errore apertura directory %s.\n", TMPDIR);
            exit(FAIL);
        }
        while ((dirp = readdir(dp)) != NULL && go == TRUE) {    //ricerca cartella estratta e entrata in essa
            sprintf(command, "%s%s", TMPDIR, dirp->d_name);
            if (lstat(command, &statbuf) < 0) {
                printf("Errore lettura stat (%s).\n", command);
                exit(FAIL);
            }
            if (S_ISDIR(statbuf.st_mode) != 0 && strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, "..")) {
                if (chdir(command) == -1) {
                    printf("Errore spostamento in directory \"%s\".\n", command);
                    exit(FAIL);
                }
                go = FALSE;
            }
        }

        //compilo e installo
        system("make && make install");

        if (closedir(dp) < 0) {
            printf("Errore chiusura directory %s\n", TMPDIR);
            exit(FAIL);
        }
        go = FALSE;
    }

    if (chdir(cgdir) == -1) {       //ritorno al path originale
        printf("Errore spostamento in directory \"%s\".\n", cgdir);
        exit(FAIL);
    }
    printf("\n\tRimuovo cartelle temporanee...\n");
    sprintf(command, "rm -fr %s", TMPDIR);
    system(command);

    if (go == TRUE) {
        printf("\n INSTALLAZIONE ANNULLATA!\n");
        go = FAIL;
    }
    else {
        printf("\n INSTALLAZIONE COMPLETATA!\n");
        go = SUCCESS;
    }

    return go;
}
