#include "types.h"
#include "syscalls.h"

uint64 __attribute__((section (".usertext")))
syscall(uint64 nr, uint64 param){
    uint64 retval;

    asm volatile("mv a7, %0" : : "r" (nr) : );
    asm volatile("mv a0, %0" : : "r" (param) : );

    //aufrufen eines ecalls
    //dies führt zu einer exception
    //der trap vektor wird dann an die ex.S weiterleiten
    asm volatile("ecall");

    //wenn der exception handler die exception funciton im kernel aufgerufen hat
    //wird die exception funciton im kernal den richtigen syscall ausführen
    //das ergbnis schreibt diese in a0 zurück
    //der exception handler restored alle register (bis auf a0, da return value den wir hier brauchen)
    //kernel exception handler setzt den mepc noch auf + 4 (bytes)
    //damit wir hier landen und nicht oben wieder in der  aufrufenden exception
    asm volatile("mv %0, a0" : "=r" (retval) : :);
    
    //return das ergebnis des syscalls zum user
    return retval;
}

void printastring(char *s) {
    syscall(PRINTASTRING, (uint64)s);   
}

void putachar(char c){
   syscall(PUTACHAR, (uint64)c);
}

char getachar(void) {
    return syscall(GETACHAR, 0);
}

int __attribute__((section (".usertext"))) main(void) {
    char c = 0;
    printastring("Hallo Bamberg!\n");
    do {
      c = getachar();
      if (c >= 'a' && c <= 'z'){
         c = c & ~0x20; //This operation effectively clears the 6th bit (0x20), which is the difference between lowercase and uppercase letters in ASCII
      };
      putachar(c);
    } while (c != 'X');

    printastring("This is the end!\n");
    return 0;
}