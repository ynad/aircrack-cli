#!/bin/bash

# Simple interface to start cracking of WPA keys with Aircrack-ng / Hashcat

function syntax {
	printf "Usage: $0 [air|wep|kwep|cat] [packet-file] <dictionary> <hashcat-binary>\n"
	printf "\t[air]\tstarts Aircrack-ng, WPA mode\n\t[wep]\tstarts Aircrack-ng, WEP (PTW method)\n\t[kwep]\tstarts Aircrack-ng, WEP (FMS/Korek method)\n\t[cat]\tstarts Hashcat\n"
}

# argument check
if [ $# -lt 2 ]; then
	syntax
	exit 1
fi

# vars
pack=$2
dict=$3

# if chosed Aircrack-ng
if [ "$1" == "air" ]; then
	# argument check
	if [ $# -lt 3 ]; then
		syntax
		exit 1
	fi
	printf "\n = Aircrack-ng - WPA =\n"
	aircrack-ng $pack -w $dict

elif [ "$1" == "wep" ]; then
	printf "\n = Aircrack-ng - WEP (PTW method) =\n"
	aircrack-ng $pack

elif [ "$1" == "kwep" ]; then
	printf "\n = Aircrack-ng - WEP (FMS/Korek method) =\n"
	aircrack-ng -K $pack

# if chosed Hashcat
elif [ "$1" == "cat" ]; then
	printf "\n = Hashcat =\n"
	if [ $# -lt 4 ]; then
		printf "Missing Hashcat binary.\n"
		syntax
		exit 1
	fi
	hashcat=$4
	# converts packet to suitable format
	aircrack-ng $pack -J $pack
	pack=$pack.hccap
	# starts cracking
	$hashcat -m 2500 $pack $dict

else
	printf "Wrong parameter: '$1'\n"
	syntax
	exit 1
fi

exit 0

