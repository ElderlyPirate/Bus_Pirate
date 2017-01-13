#!/bin/bash

OS=`uname`

echo -n "Building for $OS..."

case $OS in

 Darwin )
	gcc -O2 -Wall -force_cpusubtype_ALL -arch i386 -arch ppc source/pirate-loader.c -DOS=$OS -o pirate-loader_mac || exit -1
	;;
 Linux )
	gcc -O2 -Wall source/pirate-loader.c -DOS=$OS -o pirate-loader_lnx || exit -1
	;;
 FreeBSD )
	gcc -O2 -Wall source/pirate-loader.c -DOS=$OS -o pirate-loader_fbsd || exit -1
 	;;
 *)
	echo "ERROR"
	echo "Unsupported operating system $OS"
	exit -1
	;;
esac

echo "DONE"
exit 0
