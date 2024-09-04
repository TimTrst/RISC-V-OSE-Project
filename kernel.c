#include "types.h"
#include "riscv.h"
#include "hardware.h"
#include "syscalls.h"
#include "kernel.h"

extern int main(void);
extern void ex(void);
extern uint64 pt[8][512*3];

#define SATP_SV39 (8L << 60)
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))

__attribute__ ((aligned (16))) char stack0[4096];

void printhex(uint64);

volatile struct uart* uart0 = (volatile struct uart *)0x10000000;

//erstellen von 8 instanzen dieser datenstruktur
//hier -> 8 pcb (1 für jeden prozess)
//enthält für jeden prozess die infos, welceh wir vor dem switchen speichern müssen
//pcb = prozesssteuerblock
pcbentry pcb[MAXPROCS];
uint64 current_pid;
int was_syscall = 0;
uint64 ticks = 0;


// konvertieren einer virtuellen in eine physische adresse für den momentanen prozess
uint64 virt2phys(uint64 addr){
  return pcb[current_pid].physbase + addr;
}

// konvertiert physische zu virtuellen adresse
// durch abziehen der physischen basis adresse
uint64 phys2virt(uint64 addr){
  return addr - pcb[current_pid].physbase;
}

// Syscall 1: printstring. Takes a char *, prints the string to the UART, returns nothing
// Syscall 2: putachar.    Takes a char, prints the character to the UART, returns nothing
// Syscall 3: getachar.    Takes no parameter, reads a character from the UART (keyboard), returns the char
// Syscall 23: yield. Takes no parameter, gives up the cpu
// Syscall 42: exit. No parameter, exits the current process

static void putachar(char c) {
  while ((uart0->LSR & (1<<5)) == 0)
    ; // polling!
  uart0->THR = c;
}

void printastring(char *s) {
  while (*s) {
    putachar(*s);
    s++;
  }
}

static char getachar(void) {
  char c;
  while ((uart0->LSR & (1<<0)) == 0)
    ; // polling!
  c = uart0->RBR;
  return c;
}

// This is a useful helper function (after you implemented putachar and printstring)
void printhex(uint64 x) {
  int i;
  char s;

  printastring("0x");
  for (i=60; i>=0; i-=4) {
    int d =  ((x >> i) & 0x0f);
    if (d < 10)
      s = d + '0';
    else
      s = d - 10 + 'a';
    putachar(s);
  }
}

int first_run = 1;

// exception wird von ex.S aufgerufen, sobald eine exception in trap-handler gefangen wird
// hier übergeben wir einen pointer als parameter
// dieser pointer zeigt auf die aktuellen Registereinträge des aktuellen prozesses
// regs kommt aus ex.S (durch register a0, welche den pointer für die aktuelle adresse des sp enthält)
uint64 exception(riscv_regs *regs) {
  uint64 nr;
  uint64 param;
  uint64 retval = 0;

  was_syscall = 1;

  // rausziehen der richtigen register aus pointer parameter
  nr = regs->a7;
  param = regs->a0;

  // nicht mehr notwendig durch das übergeben der gesamten register als pointer zur struktur
  //asm volatile("mv %0, a7" : "=r" (nr) : : );
  //asm volatile("mv %0, a0" : "=r" (param) : : );

  uint64 pc = r_mepc();
  uint64 mcause = r_mcause();
  uint64 mtval = r_mtval();

  pcb[current_pid].pc = pc;
  // physikalische adresse des register structs wird in eine virtuelle umgewandelt und als stack pointer gesetzt
  pcb[current_pid].sp = phys2virt((uint64)regs);
  pcb[current_pid].state = READY;

#ifdef DEBUG
 printastring("mcause = ");
  printhex(mcause);
  printastring("\n");
#endif

  if (mcause & (1ULL<<63)) {
    // Interrupt - async
    was_syscall = 0;

    // auslesen ob es sich um einen tier interrupts (von CLINT handelt)
    if((mcause & ~(1ULL << 63)) == 7 ) { // cleared das interrupt indicator bit an stelle 64 und lässt die unteren unverändert -> so kann egal was für eine art von interrupt (exception oder async) die lower bits ausgelesen werden (== 7?)
        int interval = 20000;

        *(uint64*)CLINT_MTIMECMP(0) = *(uint64*)CLINT_MTIME + interval;
        ticks++;

        if((ticks % 10) == 0) {
          pcb[current_pid].state = READY;

          while(1){
            current_pid = (current_pid + 1) % MAXPROCS;
            if(pcb[current_pid].state == READY) {
              pcb[current_pid].state = RUNNING;
              break;
            }
          }
          w_mscratch(pcb[current_pid].physbase);
          pc = pcb[current_pid].pc;
        }
    }
    printastring("INT\n");
  } else {
    // all exceptions end up here
    if ((mcause & ~(1ULL<<63)) == 8) { // it's an ECALL!

      #ifdef DEBUG
      printastring("SYSCALL ");
      printhex(nr);
      printastring(" PARAM ");
      printhex(param);
      printastring("\n");
      #endif

      was_syscall = 1;

      switch(nr) {
      case PRINTASTRING:
        // funktion printastring erwartet eine physische adresse
        printastring((char *)virt2phys(param));
        break;
      case PUTACHAR:
        putachar((char)param);
        break;
      case GETACHAR:
        // speichern des zeichens als return value
        retval = (uint64)getachar();
          break;
      case EXIT:
        pcb[current_pid].state = NONE;
        // solange noch kein weiterer process im ready state ist, überpring das break und schau erneut
        while(1){
          current_pid = (current_pid+1) % MAXPROCS;
          if(pcb[current_pid].state != READY) continue;
          break;
        }
        // wenn einer ready ist, setzen wir diesen auf running
        pcb[current_pid].state = RUNNING;
        break;
      case YIELD:
        // handelt sich um eigenes aufgeben
        // kann direkt wieder auf "ready" gesetzt werden
        pcb[current_pid].state = READY;
        //dasselbe wie oben bei exit case
        //wir schauen einfach nach dem nächsten prozess im ready (kann auch der ursprüngliche prozess sein)
        while(1){
          current_pid = (current_pid + 1) % MAXPROCS;
          if(pcb[current_pid].state == READY){
            pcb[current_pid].state = RUNNING;
            break;
          }
        }
        // hier speichern wir die physbase im mscratch für den exception handler später 
        // somit kann dieser korrekt platz auf dem stack machen für diesen prozess
        // im falle einer neuen exception
        w_mscratch(pcb[current_pid].physbase);

        //müssen noch den page table switchen!
        //#define SATP_SV39 (8L << 60)
        //helfer methode zum erstellen des satp eintrags
        //#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64) pagetable) >> 12))
        //schreiben in das satp registers
        //mit page table des neuen prozesses
        w_satp(MAKE_SATP((uint64)&pt[current_pid][0]));
        // flushen des mmu buffers
        __asm__ __volatile__("sfence.vma");

        pc = pcb[current_pid].pc;
        break;

      default:
        printastring("*** INVALID SYSCALL NUMBER!!! ***\n");
        break;
      }
    } else { 
      printastring("EXC mcause = ");
      printhex(mcause);
      printastring(", mepc = ");
      printhex(pc);
      printastring("\n");
    }
  }

  first_run = 0;

  w_satp(MAKE_SATP(pcb[current_pid].pagetablebase));
  asm volatile("sfence.vma zero, zero");

  w_mscratch(pcb[current_pid].physbase);

  // Here, we adjust return value - we want to return to the instruction _after_ the ecall! (at address mepc+4)
  // wenn es kein syscall war, dann müssen wir dort weiter machen, wo der aufrufende prozess unterbrochen wurde
  if(was_syscall){
    w_mepc(pcb[current_pid].pc + 4);
  }else{
    w_mepc(pcb[current_pid].pc);
  }

  regs = (riscv_regs*)virt2phys(pcb[current_pid].sp);

  regs->a0 = retval;
  regs->sp = (uint64)regs;

  // this function returns neuer stackpointer zu ex.S
  // wir müssen in ex.S dann die Register des neuen prozesses wiederherstellen
  return (uint64)regs;
}
