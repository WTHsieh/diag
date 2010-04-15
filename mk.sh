#!/usr/bin

#touch .config
#make memuconfig
make all
cp EVBV1_SQ8000_ARM9_v03.00.bin /tftpboot/sq8000_dia.bin
