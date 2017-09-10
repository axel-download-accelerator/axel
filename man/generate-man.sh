#!/bin/bash

# Generate the manpage
# Copyright 2016-2017 Joao Eriberto Mota Filho <eriberto@eriberto.pro.br>
# This file is under BSD-3-Clause

P_DATE="10 September 2017"
P_NAME=axel
P_VERSION=2.14
P_MANLEVEL=1
P_DESCRIPT="light command line download accelerator"

TEST=$(txt2man -h 2> /dev/null)

[ ! "$TEST" ] && { echo "ERROR: You need install txt2man program."; exit 1; }

[ -e $P_NAME.txt ] || { echo "ERROR: $P_NAME.txt not found."; exit 1; }

txt2man -d "$P_DATE" -t $P_NAME -r $P_NAME-$P_VERSION -s $P_MANLEVEL -v "$P_DESCRIPT" $P_NAME.txt > $P_NAME.$P_MANLEVEL
