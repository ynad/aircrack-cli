#ifndef INSTALL_H_INCLUDED
#define INSTALL_H_INCLUDED

/**
        Aircrack - Command Line Interface

        Gestore installazioni
        install.h
**/

/* installatore dipendenze */
int dep_install();

/* controllo versione distribuzione OS  */
char checkDistro();

/* installatore aircrack */
int akng_install();


#endif // INSTALL_H_INCLUDED
