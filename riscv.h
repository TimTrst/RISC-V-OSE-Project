#define MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3)    // machine-mode interrupt enable.

static inline void
w_mscratch(uint64 x)
{
  asm volatile("csrw mscratch, %0" : : "r" (x));
}

//machine trap value register
//speichern der adresse, welche zu einer exception geführt hat
static inline uint64
r_mtval()
{ 
  uint64 x;
  asm volatile("csrr %0, mtval" : "=r" (x) );
  return x;
}

static inline uint64
r_mcause()
{ 
  uint64 x;
  asm volatile("csrr %0, mcause" : "=r" (x) );
  return x;
}

static inline uint64
r_mstatus()
{ 
  uint64 x;
  asm volatile("csrr %0, mstatus" : "=r" (x) );
  return x;
}

static inline void
w_mstatus(uint64 x)
{
  asm volatile("csrw mstatus, %0" : : "r" (x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception will go.
static inline uint64
r_mepc()
{
  uint64 x;
  asm volatile("csrr %0, mepc" : "=r" (x) );
  return x;
}

static inline void
w_mepc(uint64 x)
{
  asm volatile("csrw mepc, %0" : : "r" (x));
}

// supervisor address translation and protection;
// holds the address of the page table.
static inline void
w_satp(uint64 x)
{
  asm volatile("csrw satp, %0" : : "r" (x));
}

// Machine Interrupt Enable
#define MIE_MEIE (1L << 11) // external
#define MIE_MTIE (1L << 7) // timer
#define MIE_MSIE (1L << 3) // software
static inline uint64
r_mie()
{
  uint64 x;
  asm volatile("csrr %0, mie" : "=r" (x) );
  return x;
}

static inline void
w_mie(uint64 x)
{
  asm volatile("csrw mie, %0" : : "r" (x));
}

// Supervisor Interrupt Enable
#define SIE_SEIE (1L << 9) // external
#define SIE_STIE (1L << 5) // timer
#define SIE_SSIE (1L << 1) // software
static inline uint64
r_sie()
{
  uint64 x;
  asm volatile("csrr %0, sie" : "=r" (x) );
  return x;
}

static inline void
w_sie(uint64 x)
{
  asm volatile("csrw sie, %0" : : "r" (x));
}


//Ansprechen der PMP Config sowie Adressbereich CSR register
//die parameter übergen entweder den adressbereich des segments (für addrx) oder die permission bits fpr cfgx
static inline void
w_pmpcfg0(uint64 x)
{
  asm volatile("csrw pmpcfg0, %0" : : "r" (x));
}

static inline void
w_pmpaddr0(uint64 x)
{
  asm volatile("csrw pmpaddr0, %0" : : "r" (x));
}

static inline void
w_pmpcfg1(uint64 x)
{
  asm volatile("csrw pmpcfg1, %0" : : "r" (x));
}

static inline void
w_pmpaddr1(uint64 x)
{
  asm volatile("csrw pmpaddr1, %0" : : "r" (x));
}

static inline void
w_pmpcfg2(uint64 x)
{
  asm volatile("csrw pmpcfg2, %0" : : "r" (x));
}

static inline void
w_pmpaddr2(uint64 x)
{
  asm volatile("csrw pmpaddr2, %0" : : "r" (x));
}

// Machine-mode interrupt vector
static inline void
w_mtvec(uint64 x)
{
  asm volatile("csrw mtvec, %0" : : "r" (x));
}

typedef struct {
        uint64 ra;
        uint64 sp;
        uint64 gp;
        uint64 tp;
        uint64 t0;
        uint64 t1;
        uint64 t2;
        uint64 s0;
        uint64 s1;
        uint64 a0;
        uint64 a1;
        uint64 a2;
        uint64 a3;
        uint64 a4;
        uint64 a5;
        uint64 a6;
        uint64 a7;
        uint64 s2;
        uint64 s3;
        uint64 s4;
        uint64 s5;
        uint64 s6;
        uint64 s7;
        uint64 s8;
        uint64 s9;
        uint64 s10;
        uint64 s11;
        uint64 t3;
        uint64 t4;
        uint64 t5;
        uint64 t6;
} riscv_regs;