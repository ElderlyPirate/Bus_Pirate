#!/bin/sh

# This file is part of the Bus Pirate project
# (http://code.google.com/p/the-bus-pirate/).
#
# Written and maintained by the Bus Pirate project.
#
# To the extent possible under law, the project has
# waived all copyright and related or neighboring rights to Bus Pirate. This
# work is published from United States.
#
# For details see: http://creativecommons.org/publicdomain/zero/1.0/.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# TODO: Spin up Ubuntu on Windows and see what is the output of uname(1)

OS=`uname`
UPLOADER="./pirate-loader"

echo "Running on ${OS}..."

case ${OS} in

  Darwin)
    PORT=${1:-"/dev/tty.usbmodem00000001"}
    ;;

  Linux|FreeBSD)
    PORT=${1:-"/dev/ttyACM0"}
    ;;

  *)
    echo "ERROR"
    echo "Unsupported operating system ${OS}"
    exit -1
    ;;

esac

if [ ! -f ${UPLOADER} ]
then
  echo "ERROR"
  echo "Cannot find bootloader update program!"
  exit -2
fi

${UPLOADER} --dev=${PORT} --hello

echo "DONE"
exit 0

# vim:sts=2:sw=2:ts=2:et:syn=sh:fdm=marker:ff=unix:number:cc=80:fenc=utf8
