Aircrack-CLI - Command Line Interface
=====================================

Version:
--------
1.2.3 (2014-06-23)
See ChangeLog for details. 

    TODO:
    

    BUGS:


Description:
------------

Basic simplified command line interface for Aircrack-ng	(credits to Thomas d'Otreppe <tdotreppe@aircrack-ng.org>).
Official documentation and tutorials of Aircrack-ng can be found on http://www.aircrack-ng.org, see also manpages and the forum.

List of features:
  - start/stop monitor interfaces (promiscuous mode) on selected wireless interface and transmission channels (use of "airmon-ng")
  - capture of WPA/WPA2 handshakes of the selected Access Point (use of "airodump-ng")
  - crack WEP keys: use fake authentications and ARP request replay to generate new unique IVs, then used to crack the WEP key
  - start single or multiple deauthentications on provided clients forcing them to reauthenticate, to capture WPA/WPA2 handshakes (use of "aireplay-ng")
  - Wifi Jammer (AirJammer): jamming on chosen wireless network sending disassociate packets to list of clients or in broadcast mode, optimized for multi-thread execution. Useful to capture WPA/WPA2 handshakes or jam a wireless network
  - install requested system dependencies to run this program and Aircrack-ng suite
  - download, compile and install Aircrack-ng source code from official website
  - capable to run multiple instances of Aircrack-cli and all subsequent operations
  - automatic recognition of: OS, network manager in use and wireless interface to use
  - start/stop network manager if requested by user, to avoid conflicts with monitor interfaces
  - online version checks


Installation:
-------------
Compatible with Linux ONLY!
System requirements (automatic install provided):
  * build-essential ('make' and 'gcc')
  * xterm
  * wget
  * macchanger

For compiling and installing Aircrack-ng:
  * libssl-dev/openssl-devel
  * libnl-3-dev/libnl3-devel
  * libnl-genl-3-dev/libnl-genl3-devel

  - Compile:
      `make`

  - Compile and install:
      `make install`

  - Remove only objects:
      `make clean`

  - Uninstall:
      `make uninstall`


License:
--------
GPLv2, see LICENSE.md


Author:
-------
ynad (github.com/ynad/aircrack-cli)
