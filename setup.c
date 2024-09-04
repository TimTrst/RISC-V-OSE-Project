#include "types.h"
#include "riscv.h"
#include "kernel.h"
#include "hardware.h"

extern int main(void);
extern void ex(void);
extern void printastring(char *);
extern void printhex(uint64);

extern pcbentry pcb[MAXPROCS];
extern uint64 current_pid;

#define NPROC 8 //acht maximale prozesse supported
#define PGSHIFT 12 //anzahl bits zum shiften wenn eine virtuelle in eine physische adresse übersetzt wird
//derived von 2¹² = 4096 bytes (12 entspricht dann hier der page size)
#define PERMSHIFT 10 // anzahl bits zum shiften von permissions im page table eintrag
//bit positions für die permission bits jedes eintrags
#define PERM_V 0 //valid bit
#define PERM_W 2 //writable
#define PERM_R 1 //readable
#define PERM_U 4 //user mode
#define PERM_X 3 // executable 

__attribute__ ((aligned (4096))) uint64 pt[NPROC][512*3];
//2D array mit D1 = nummer des momentanen prozesses und D2 = maximale einträge von des prozesses
//512 * 3, weil wir 3 page table hierarchien haben -> Page direcitory, page upper directiory, page table
//3 Level page table
//hier teilen wir den speicher in segmente auf, welceh für 8 maximal konkurrierende prozesse gedacht sind
//dadurch hat jeder prozess seinen eigenen bereich mit eigenen übersetzungen von virtuellen zu physischen adressen

//initialisieren von den ersten beiden page tables L3 und L2
//der jeweils erste eintrag von jedem Page table wird mit einem "entry point" versehen
//dieser entry point verweist auf die nächsten einträge des nächsten page tables
//&pt[proc][512] ist der eintrag des nächseten tables und wird als erser eintrag im L3 gesetzt (als valid gesetzt und mit hilfe des PGshift für das übersetzen)
uint64 init_pt(int proc) {
  for (int i=0; i<512; i++) {
    if (i == 0) {
      pt[proc][i] = ((uint64)&pt[proc][512]) >> PGSHIFT << PERMSHIFT;  //1. adresse des nächsten pagetables (konvertieren in eine uint64 wert adresse)
      //2. pgshift: verschieben der adresse um 12 bits nach rechts -> extrahieren der Seitenadresse und offset informationen der virtuellen adresse zu entfernen (Seitenadresse)
      //3. permshift: um 10 bits nach links verschieben um platz für die permission bits zu schaffen
      //Ergebnis: Vorbereiten des nächsten Pagetable mit korrekter physischer adresse 
      pt[proc][i] |= (1 << PERM_V); //dieser eintrag wird als valid gesetzt (ansonsten ist dieser beim nächsten aufruf invalid und pagewalk wird abgebrochen)
    } else {
      pt[proc][i] = 0; //alle anderen einträge auf invalid setzen
    }
  }
  //initialisieren der zweiten ebene page table
  //zeigt auf spezifischen speicherbereich (0x80000000ULL)
  //setzen von valid und allen weiteren permissions + zuriffsrechte im user mode (accessd und dirty bits auch zugreifbar)
  //hier geht es um die direkte adressierung von physischen speicher (virt -> phy)
  for(int i=512; i<1024; i++){
    if(i == 512){
      pt[proc][i] = (0x80000000ULL + (proc+1) * 0x200000ULL) >> PGSHIFT << PERMSHIFT;
      //startet bei physischen speicher
      //unterschiedliche für alle processe + (proc +1)
      //proc*0x200000ULL -> 2MB eintrag fenster pro prozess (dadurch ist jeder adressraum 2MB groß) 1 Proc * 2 MB = bei 2MB von start, 2 proc * 2MB = 4MB von start, usw,... + die startadresse 0x800...
      //Stichwort: Segemntierung (des physischen RAM)
      pt[proc][i] |= 0x7f; //alle permissions werden gesetzt für diesen eintrag (und folgend für alle einträge in diesem page table)
    }else{
      pt[proc][i] = 0; //alle anderen werden auf invalid gesetzt
    }
  }
  return (uint64)&pt[proc][0];
}

//vorteil von mehrstufigen paging:
// es müssen nicht jederzeit alle teile der seitentabelle im speicher gehalten werden müssen (SPeichereffizient!)


void timerinit(void){
  // cpu kern id
  int id = 0;

  int interval = 50000; // zyklen bis interrupt
  // init cmp register mit dem intervall zyklus (20.000 warten bis interrupt)
  *(uint64*)CLINT_MTIMECMP(id) = *(uint64*)CLINT_MTIME + interval;
  
  // aktivieren von machine mode interrupts
  // w_mstatus(r_mstatus | MSTATUS_MIE);

  // akt von timer interrupts (im machine mode)
  w_mie(r_mie() | MIE_MTIE);
}

// externe interrupts (PLIC)
void interruptinit(){
  // setting priority for UART0 device
  // PLIC = base adresse des plics, UART0_IRQ = identifier des uart0 devices
  // * 4 für korrekten offset im plics prio memory
  *(uint32*)(PLIC + UART0_IRQ*4) = 1; // 0 = disabled

  // enables UART0 interrupt enable bit for M mode
  // gilt für momentanen processor core 
  // enabled interrupts dieses devices für diesen kern
  *(uint32*)PLIC_MENABLE = (1 << UART0_IRQ); // setzt bit in plic_menable auf 1 das für diese device gilt

  // setzt interrupt priority für diesen kern im m mode
  // minimum prio die ein interrupt haben muss, damit dieser kern diesen interrupt behandelt
  // 0 -> alle interrupts werden behandelt
  *(uint32*)PLIC_MPRIORITY = 0;

  // enablen von externen interrupts im machine mode
  // schreiben von 1 an die 11 stelle im MIE CSR (MIE_MEIE = (1L << 11))
  w_mie(r_mie() | MIE_MEIE);
}


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

  //initialisieren einer gültigen seitentabelle (seiten tabllen adressen) mit 512 einträgen pro level pro prozess
  //für MT auch noch den pcb füllen für jeden prozess!
  for(int i = 0; i < NPROC; i++){
    pcb[i].pc = -4;
    pcb[i].sp = 0x1ffff8 - 256; // feste position relativ zur virtuellen adresse
    // obereres ende des virtuellen adress raumes
    pcb[i].physbase = 0x80200000ULL + 0x200000 * i;  
    // erzeugen einer pagetable für diesen prozess
    // gibt die basis adresse des page tables zurück
    pcb[i].pagetablebase = init_pt(i);
    pcb[i].state = NONE;
  }
  //supervisor address translation and protection -> gibt paging modus im RISCV an 
  //wir verwenden hier den SV39 modus (39 virtuelle bit adressen unterstützt) 3Level page design unterstützung
  #define SATP_SV39 (8L << 60) // setzt die oberen bits im satp register um den SV39 modus zu konfigurieren
  //makro, dass den wert für das satp register erzezugt
  //nimmt adresse des ersten pagetable und verschiebt diese adresse um 12 bits nach rechts (entfernt die seitenoffset bits)
  //verschobene adresse wird mit SV39 modus kombiniert um den engültigen wert für das satp register zu erstellen
  #define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))
  //aktivierung des pagings
  //wir teilen prozessor mit, wie die initiale adresse des ersten page tables lautet und welcher modus (sv39)
  //--> dadurch weiß MMU wie die virtuellen adressen übersetzt werden sollen
  //füllen des satp registers mit dem wert, welcher durch MAKE_SATP erstellt wurde
  //---> ausgangspunkt der MMU für page-table-walk um die physischen adressen einer virtuellen zu bestimmen
  w_satp(MAKE_SATP((uint64)&pt[0][0]));
  //flushen der TLB (translation lookaside buffer informationen gecashde adressen)
  //zero = flushen des vollständigen buffers
  asm volatile("sfence.vma zero, zero");

  // configure Physical Memory Protection to give user mode access to all of physical memory.
  w_pmpaddr0(0x3fffffffffffffULL);
  w_pmpcfg0(0xf);

  //konfiguieren der pmp für die folgenden segmente
  //w_pmpaddr0(0x7fffffffULL);
  //w_pmpcfg0(0x0);

  //w_pmpaddr1(0x800fffffULL);
  //w_pmpcfg1(0x0);

  //w_pmpaddr2(0x80ffffffULL);
  //w_pmpcfg2(0xF);


  // set M Exception Program Counter to main, for mret, requires gcc -mcmodel=medany
  //main ist jetzt bei startadresse 0 des jeweiligen prozesses (virtuelle adresse 0 -> wird dann durch unser paging ordentlich mapped in physische)
  w_mepc((uint64)0);

  //setzen der prozess informationen für den ersten run
  //process 0 ist running, die anderen wartend
  current_pid = 0;
  pcb[0].state = RUNNING;
  pcb[1].state = READY;
  pcb[2].state =  READY;

  
  //schreiben in das CSR mscratch register, der physikalischen adresse 
  //des ersten prozesses (das machen wir dann im kernel extra für alle prozesse, bevor diese switched werden)
  //im ex.S file kann dann diese adresse ausgelesen werden, damit der stack korrekt justiert werden kann
  w_mscratch(pcb[0].physbase);

  timerinit();
  interruptinit();

  // enable receive interrupts via uart0
  // IER -> UART interrupt enable register
  extern volatile struct uart* uart0;
  uart0->IER=0x1;

  printastring("Setup completed. User programs may start.");
  printastring("\n");

  // switch to user mode (configured in mstatus) and jump to address in mepc CSR -> main().
  asm volatile("mret");
}