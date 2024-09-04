#include "types.h"
#include "syscalls.h"

__attribute__ ((aligned (16))) char userstack[4096];

uint64 syscall(uint64 nr, uint64 param) {
    uint64 retval;

    asm volatile("mv a7, %0" : : "r" (nr) : );
    asm volatile("mv a0, %0" : : "r" (param) : );

    // ecall
    asm volatile("ecall");

    // Here we return the return value...
    asm volatile("mv %0, a0" : "=r" (retval) : : );
    return retval;
}

void printastring(char *s) {
    syscall(PRINTASTRING, (uint64)s);
}

void putachar(char c) {
    syscall(PUTACHAR, (uint64)c);
}

char getachar(void) {
    return syscall(GETACHAR, 0);
}

char yield(void) {
    syscall(YIELD, 0);
}

// ----


int main(void) {
    char c;
    printastring("Hello from Process 0!\n");
    while (1) {
      c = getachar();
      putachar(c & ~0x20); // ~0x20 konvertiert die lowercase letter in uppercase letter durch inertieren des 6th bits 
    }

    printastring("This is the end!\n");
    return 0;
}


