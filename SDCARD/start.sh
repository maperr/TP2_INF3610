#/bin/sh
/mnt/softuart.elf &
/mnt/rwmem.elf 0xfffffff0 0x18000000
sleep 1
/mnt/sendpacket.elf
