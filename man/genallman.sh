#!/bin/bash

# Generate several manpages at the same time.
# Copyright (C) 2015 Joao Eriberto Mota Filho <eriberto@eriberto.pro.br>
# v0.3, available at https://github.com/eribertomota/genallman
#
# Inside the Axel, this file is under GPL-2+ license.

[ -f /usr/bin/txt2man ] || { echo "ERROR: txt2man not found."; exit; }

for NAME in $(ls | grep header | cut -d'.' -f1)
do
    LEVEL=$(cat $NAME.header | cut -d" " -f3 | tr -d '"')
    cat $NAME.header > $NAME.$LEVEL
    txt2man $NAME.txt | grep -v '^.TH ' >> $NAME.$LEVEL
    echo "Generated $NAME.$LEVEL."
done
