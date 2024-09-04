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

#define BUFFER_SIZE 32
char ringbuffer[BUFFER_SIZE];
int head, tail, full_flag = 0, nelem = 0;

void printhex(uint64);

volatile struct uart* uart0 = (volatile struct uart *)0x10000000;

//erstellen von 8 instanzen dieser datenstruktur
//hier -> 8 pcb (1 für jeden prozess)
//enthält für jeden prozess die infos, welceh wir vor dem switchen speichern müssen
//pcb = prozesssteuerblock
pcbentry pcb[MAXPROCS];
uint64 current_pid;
uint64 waiting_pid;
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

// welcher interrupt als nächstes
int plic_claim(void){
  int irq = *(uint32*)PLIC_MCLAIM; // beinhaltet adresse des highest prio interrupts
  return irq;
}

// plic mitteilen, dass aktueller interrupt fertig behandelt wurde
void plic_complete(int irq){
  *(uint32*)PLIC_MCLAIM = irq;
}

int buffer_is_full(void){
  return (full_flag == 1); // return 1 if full
}

int buffer_is_empty(void){
  return (nelem == 0); // return 1 if empty
}

// einfügen in buffer von neuen schreiboperationen
int rb_write(char c){
  int retval;

  if(buffer_is_full()){
    retval = -1;
  }else{
    ringbuffer[head] = c; // setzen des zu schreibenden elementes an nächste stelle im buffer (head)
    head = (head + 1) % BUFFER_SIZE; // setzen des heads an nächste pos (oder wrap around, wenn head == rb size)
    nelem++; // inkrement anzahl an elemente im buffer
    if(head == tail){
      full_flag = 1;
    }
    retval = 0;
  }
  return retval;
}

// auslesen von einträgen in den buffer
int rb_read(char *c){
  int retval;

  if(buffer_is_empty()){
    retval = -1;
  }else{
    *c = ringbuffer[tail]; // auslesen des items im buffer als pointer (adresse)
    tail = (tail + 1) % BUFFER_SIZE; 
    nelem--; // dekrementieren der anzahl elemente im buffer
    full_flag = 0;
    retval = 0;
  }
  return retval;
}

unsigned char readachar(void){
  char c;
  int retval;
  retval = rb_read(&c); // passes eines pointers anstatt eines values
  // dadurch kann die adresse von c (welches noch keinen wert hat) 
  // als parameter verwendet werden, anstatt ein value
  // die read function des buffers wird der adresse einen wert aus dem buffer zuweisen!

  if(retval == 0){
    return c;
  }else {
    return 0;
  }
}

// switching processes
void schedule(){
  while(1){
    current_pid = (current_pid + 1) % MAXPROCS; //wrap around if current is last process 
    if(pcb[current_pid].state == READY){
      pcb[current_pid].state == RUNNING;
      break;
    }
  }
  w_mscratch(pcb[current_pid].physbase);
  pcb[current_pid].state = RUNNING;
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

    // timer interrupt ? (CLINT)
    if((mcause & ~(1ULL << 63)) == MTI ) { // cleared das interrupt indicator bit an stelle 64 und lässt die unteren unverändert -> so kann egal was für eine art von interrupt (exception oder async) die lower bits ausgelesen werden (== 7?)
        int interval = 50000;

        *(uint64*)CLINT_MTIMECMP(0) = *(uint64*)CLINT_MTIME + interval;
        ticks++;

        if((ticks % 10) == 0) {
          pcb[current_pid].state = READY;

          // any process sleeping?
          // wird im moment noch nicht benötigt, kein process called syscall sleep
          for(int i = 0; i<MAXPROCS; i++){
            if(pcb[i].state == SLEEPING){
              if(ticks >= pcb[i].wakeuptime){
                pcb[i].state = READY;
                pcb[i].wakeuptime = 0;
              }
            }
          }
          schedule();
        }
    } else if((mcause & ~(1ULL<<63)) == MEI){ // external interrupt (PLIC)
      int irq = plic_claim();
      if(irq == UART0_IRQ){
        char c = uart0->RBR; // auslesen des read registers aus uart
        rb_write(c); // hinzufügen in unseren buffer
        if(full_flag){
          putachar('*');
        }
        plic_complete(irq);
        pcb[waiting_pid].state = READY; // blocked prozess kann wieder ready gesetzt werden
      }
    }
  } else {
    // all exceptions end up here
    if ((mcause & ~(1ULL<<63)) == MSI) { // it's an ECALL!
      was_syscall = 1;

      switch(nr) {
      case SLEEP:
        if (param > 0){
          pcb[current_pid].state = SLEEPING;
          pcb[current_pid].wakeuptime = param;
        }
        schedule();
        break;
      case PRINTASTRING:
        // funktion printastring erwartet eine physische adresse
        printastring((char *)virt2phys(param));
        break;
      case PUTACHAR:
        putachar((char)param);
        break;
      case GETACHAR:
        // speichern des zeichens als return value
        retval = readachar();
        if(retval == 0){
          was_syscall = 0;
          pcb[current_pid].state = BLOCKED;
          waiting_pid = current_pid;
          schedule();
        }else{
          was_syscall = 1;
        }
        break;
      case EXIT:
        pcb[current_pid].state = NONE;
        // solange noch kein weiterer process im ready state ist, überpring das break und schau erneut
        schedule();
        // wenn einer ready ist, setzen wir diesen auf running
        pcb[current_pid].state = RUNNING;
        break;
      case YIELD:
        // handelt sich um eigenes aufgeben
        // kann direkt wieder auf "ready" gesetzt werden
        pcb[current_pid].state = READY;
        //dasselbe wie oben bei exit case
        //wir schauen einfach nach dem nächsten prozess im ready (kann auch der ursprüngliche prozess sein)
        schedule();
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
