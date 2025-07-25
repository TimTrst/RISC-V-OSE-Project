#define MAXPROCS 8

typedef enum { NONE, READY, RUNNING, BLOCKED, SLEEPING } procstate_t;

// hält alle relevanten informationen eines processes
// diese infos müssen wir abspeichern
// bevor wir mit bspw. yield() einem anderen Prozess CPU Zeit geben können
typedef struct {
  uint64 pc;
  uint64 sp;
  uint64 physbase;
  uint64 pagetablebase;
  uint64 wakeuptime;
  procstate_t state;
} pcbentry;

