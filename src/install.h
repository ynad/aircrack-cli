/**CHeaderFile*****************************************************************

   FileName    [install.h]

   PackageName [Aircrack-CLI]

   Synopsis    [Aircrack Command Line Interface - Installer handler]

   Description [Command Line Interface for Aircrack-ng 
   (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

   Author      [ynad, stethewwolf]

   License     [GPLv2, see LICENSE.md]

   Revision    [2014-10-24]

******************************************************************************/


#ifndef INSTALL_H_INCLUDED
#define INSTALL_H_INCLUDED


/* Dependencies installer */
int depInstall();

/* OS version checker */
char checkDistro();

/* Aircrack-ng downloader and installer */
int akngInstall(char *);


#endif // INSTALL_H_INCLUDED
