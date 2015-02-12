#!/bin/bash/

for i in `seq 0 19999`:
do
    
    echo poke 0x0800003 0x00
    echo poke 0x0800005 0x