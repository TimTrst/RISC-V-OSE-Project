#include "types.h"
#include "syscalls.h"

#define REGION_1_START 0x00000000ULL
#define REGION_1_END 0x7FFFFFFFULL
#define REGION_2_START 0x80000000ULL
#define REGION_2_END 0x800FFFFFULL
#define REGION_3_START 0x80100000ULL
#define REGION_3_END 0x80ffffffULL

__attribute__((section(".userdata"), aligned(16))) char userstack[4096];


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

void __attribute__((section (".usertext")))
test_memory_region(uint64 start, uint64 end, char *msg){
    uint64 *ptr;
    char *message = msg;
    printastring(message);
    printastring("\n");

    // Schreibe nur einmal an die Startadresse des Bereichs
    ptr = (uint64 *)start;
    *ptr = 0x80ff0000ULL;

    printastring("Region test succeeded. \n");
}


int __attribute__((section (".usertext"))) main(void) {
    char c = 0;
    printastring("Hallo Bamberg!\n");
	printastring("Starting the PMP test.. \n");

    test_memory_region(REGION_1_START, REGION_1_END, "Testing Region 1 (no permissions)");
    test_memory_region(REGION_2_START, REGION_2_END, "Testing Region 2 (no permissions)");
    test_memory_region(REGION_3_START, REGION_3_END, "Testing Region 3 (all permissions)");

    printastring("PMP test completed");

    //do {
    //  c = getachar();
    //  if (c >= 'a' && c <= 'z'){
    //     c = c & ~0x20; //This operation effectively clears the 6th bit (0x20), which is the difference between lowercase and uppercase letters in ASCII
    //  };
    //  putachar(c);
    //} while (c != 'X');
    return 0;
}