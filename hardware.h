struct uart {
   union {
     uint8_t THR; // W = transmit hold register (offset 0)
     uint8_t RBR; // R = receive buffer register (also offset 0)
     uint8_t DLL; // R/W = divisor latch low (offset 0 when DLAB=1)
   };
   union {
     uint8_t IER; // R/W = interrupt enable register (offset 1)
     uint8_t DLH; // R/W = divisor latch high (offset 1 when DLAB=1)
   };
   union {
     uint8_t IIR; // R = interrupt identif. reg. (offset 2)
     uint8_t FCR; // W = FIFO control reg. (also offset 2)
   };
   uint8_t LCR; // R/W = line control register (offset 3)
   uint8_t MCR; // R/W = modem control register (offset 4)
   uint8_t LSR; // R   = line status register (offset 5)
};

// core local interruptor (CLINT), which contains the timer.
#define CLINT 0x2000000L // base adresse des clints
// base adresse des compare regs
// + einen offset für jeden cmp reg jedes cpu kerns
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
// adresse der momentanen time 
// zyklen, seit system booted wurde 
// wird kontinuierlich inkrementiert und mit cmp verglichen
#define CLINT_MTIME (CLINT + 0xBFF8)


// platform level interrupt controller (PLIC)
// -> handled external interrupts (eg. hardware)
#define PLIC 0x0c000000L // basis adresse plic
#define PLIC_PRIORITY (PLIC + 0x0) // jeder interrupt kann eine prio haben
#define PLIC_PENDING (PLIC + 0x1000) // adrese wo wartende interrupts zu finden sind
#define PLIC_MENABLE (PLIC + 0x2000) // adresse register, welches interrupts im M mode enabled
#define PLIC_SENABLE (PLIC + 0x2080) // selbe für S mode
#define PLIC_MPRIORITY (PLIC + 0x200000) // prio schranke -> alles was darunter ist an interrupts, wird keinen auslössen in M mode
#define PLIC_SPRIORITY (PLIC + 0x201000) // selbe für interrupts während S mode
#define PLIC_MCLAIM (PLIC + 0x200004) // adresse wo der highest-prio interrupts hinterlegt ist und dann serviced wird
#define PLIC_SCLAIM (PLIC + 0x201004) // selbe für S mode interrupts

#define UART0_IRQ 10

#define MTI 7 // machine timer inerrupt
#define MEI 11 // machine external interrupt
#define MSI 8 // machine software interrupt