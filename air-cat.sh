#!/bin/bash

# Simple interface to start cracking of WPA with Aircrack-ng / Hashcat

function syntax {
	printf "Usage: $0 [air|cat] [packet-file] [dictionary] [hashcat-binary]\n"
	printf "\t[air] starts Aircrack-ng, [cat] starts Hashcat\n"
}

# argument check
if [ $# -lt 3 ]; then
	syntax
	exit 1
fi

# vars
pack=$2
dict=$3

# if chosed Aircrack-ng
if [ "$1" == "air" ]; then
	printf "\n = Aircrack-ng =\n"
	aircrack-ng $pack -w $dict

# if chosed Hashcat
elif [ "$1" == "cat" ]; then
	printf "\n = Hashcat =\n"
	if [ $# -ne 4 ]; then
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

