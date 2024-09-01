#!/bin/bash
echo "Running QEMU..."
qemu-system-riscv64 -nographic -machine virt -smp 1 -bios none -kernel kernel -device loader,addr=0x80200000,file=user1.bin -device loader,addr=0x80400000,file=user2.bin -device loader,addr=0x80600000,file=user3.bin
echo "Cleaning directiory..."
rm -f *.o *.bin kernel user1 user2 user3 userprogs1.h userprogs2.h userprogs3.h
