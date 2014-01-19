/**CHeaderFile*****************************************************************

  FileName    [install.h]

  PackageName [Aircrack-CLI]

  Synopsis    [Aircrack Command Line Interface - Installer handler]

  Description [Command Line Interface for Aircrack-ng 
  (credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>)]

  Author      [ynad, stethewwolf]

  License     [GPLv2, see LICENSE.md]

  Revision    [1.1.7, 2013-11-22]

******************************************************************************/


#ifndef INSTALL_H_INCLUDED
#define INSTALL_H_INCLUDED


/* Dependencies installer */
int depInstall();

/* OS version checker */
char checkDistro();

/* Aircrack-ng downloader and installer */
int akngInstall();


#endif // INSTALL_H_INCLUDED
