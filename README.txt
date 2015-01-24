Aircrack-CLI - Command Line Interface
=====================================

Version:
--------
1.2.5 (2015-01-24)
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
  - Reaver: capable to launch brute force attack against Wifi Protected Setup (WPS), using Reaver (visit: https://code.google.com/p/reaver-wps/ , all credits go to project's creators)


Installation:
-------------
Compatible with Linux ONLY!
System requirements (automatic install provided):
  * build-essential ('make' and 'gcc')
  * xterm
  * wget
  * macchanger

For compiling and installing Aircrack-ng and Reaver-WPS:
  * libssl-dev/openssl-devel
  * libnl-3-dev/libnl3-devel
  * libnl-genl-3-dev/libnl-genl3-devel
  * libpcap0.8(-dev)
  * libsqlite3-0(-dev)
  * pkg-config

  - Compile:
      `make`

  - Then install:
      `sudo make install`

  - Remove build files:
      `make clean`

  - Uninstall:
      `make uninstall`


Usage:
------
If launching from source folder:
$ sudo ./bin/aircrack-cli.bin [N] [WLAN-IF]

Or if it was installed to system path:
$ sudo aircrack-cli.bin [N] [WLAN-IF]

Optional parameters:
	[N]	    skips installation of dependencies
	[WLAN-IF]   specifies wireless interface (if different from system default)


Notes:
------
Reaver-WPS seems to be not working with the new version of libpcap & libpcap-dev, probably for some incompatibility or some bugs.
You can downgrade them to version 1.4.0-2 with the provided packages (debian-like) under "util" or downloading from here:
http://mirrors.kernel.org/ubuntu/pool/main/libp/libpcap/
Other packages may be found in software resources of each distro.
(Source: https://code.google.com/p/reaver-wps/issues/detail?id=217#c19)


License:
--------
GPLv2, see LICENSE.md


Author:
-------
ynad (github.com/ynad/aircrack-cli)

