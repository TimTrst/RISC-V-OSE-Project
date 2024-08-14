#include "types.h"
#include "riscv.h"

extern int main(void);
extern void ex(void);
extern void printstring(char *);
extern void printhex(uint64);

void setup(void) {
  //setzen des letzten modes auf user mode
  unsigned long x = r_mstatus(); //auslesen des mstatus registers (zu finden im riscv header file)
  x &= ~MSTATUS_MPP_MASK; //setzt die bits 11 und 12 im MPP des mstatus auf 00 (durch negation) -> ohne die anderen bits zu beinflussen  //entspricht dem user mode dann (00 im 11 und 12 bit)
  x |= MSTATUS_MPP_U; //setzen der mstatus bits auf 1, welche im mstatus_mpp_u 1 sind (keine? :D)
  w_mstatus(x);

  // enable machine-mode interrupts.
  w_mstatus(r_mstatus() | MSTATUS_MIE);

  //aktivieren von ecalls (software interrupts) im machine mode 
  //bitwise OR -> alle bits im mie register bleiben unverändert, außer die bits welche wir für den software interupt setzen müssen
  //siehe riscv header file MIE_MSIE = (1L << 3) --> 1 bit wird um 3 stellen nach links verschoben (software interrupt enable bit im mie)
  w_mie(r_mie() | MIE_MSIE);

  // hier wird der einstiegspunkt gesetzt wenn eine trap auftritt (exception, syscall, interrupt)
  w_mtvec((uint64)ex);

  // disable paging for now.
  w_satp(0);

  // configure Physical Memory Protection to give user mode access to all of physical memory.
  w_pmpaddr0(0x3fffffffffffffULL);
  w_pmpcfg0(0xf);

  // set M Exception Program Counter to main, for mret, requires gcc -mcmodel=medany
  w_mepc((uint64)main);

  // switch to user mode (configured in mstatus) and jump to address in mepc CSR -> main().
  asm volatile("mret");
}

