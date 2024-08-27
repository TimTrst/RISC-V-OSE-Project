# Multitasking

- a **process** is a *program in execution*

- a **program** is static, represented by the bytes of the executable on a file system

- a process that has a *state* that changes throughout its execution
    1. current program counter (pc)
    2. values of processor registers
    3. values of variables
    4. Additional information about resources, user ids, permissions, ...

    -> diese ganzen infos über einen Prozess müssen gespeichert werden, wenn dieser nicht mehr aktiv ist (nicht mehr runned)

## Process behavior
- im Moment kann ein Prozess entweder *running* oder *ready* sein
- wir müssen weitere states einführen
    - *blocked*

1. How many processes can be in the "Running" state at the same time on a single processor system?
    - Only one can be in the state "running" at any given time on a single-processor system
    (single processor can only execute one instruction stream at a time)

2. Why is there no transition from "Blocked" to "Running"?
    - cpu might be busy with another process
    - it needs to first move from *blocked* to *ready*
        - waits in queue until scheduler allocates the cpu to it

3. At which points in time do the transitions occur?
    a. Running to blocked:
        - process requests an I/O operation or an event that cannot be immediately completed
        - cooperative multitasking: each process "frees" cpu time by changing into "blocked" state (yield)
        - preemptive mt: scheduler checks for processes in ready and changes the running process
    b. Running to ready
        - running process is interrupt
    c. Blocked to ready
        - when the interrupting operation the process is waiting for is completed and would be abled to complete its purpose
    d. Ready to running
        - scheduler selects a process from the ready queue to run on the cpu

4. What happens if all processes are blocked
    - cpu becomes idle (idle state)
        - waiting unilt a process becomes ready again (continuously checking the ready state for available processes)
        - entering a low power state, executing the idle loop

## Cooperative Multitasking
- nutzen von **yield()**
    - process wirft yield system call, damit dieser in ready state geht (ohne den aufrufenden prozess zu terminieren)

### And what about the stack
- storing the process state
    - local variables für alle function im call stack
    - return addresses
    (for all but the leaf function, which is the last function that does not call any other)

- *virtual memory makes the life here easier*
    - alle programme können an die selbe adresse gelinked werden
    - vm subsystem nutzt die MMU separiert die virtuellen adressebreiche

    -> sonst bräuchten wir laut folie hier ein seperates linker script für jeden user process
        - individuelle startbereiche für jeden linker 

## What other data is important
1. Register values (we have one set of register values for all processes)
    - executing another process would overwrite these registers
    - ex.S macht dies bereits

## The OS stack
- das OS speichert seinen context in einer seperaten address range
- was müssen wir speichern?
    - exception benötigt:
        1. "current_process" process ID
        2. "pcb" kontrolliert block array (ein eintrag per prozess)

--> seperieren von kernel und user stack

## Scheduling
- simpler **round-robin scheduler**
-> OS switches auf anderen prozess bei jedem aufrug von yield()
- **pcp** struct enthält den momentanen *state* des prozesses
    - wir brauhen darin:
        1. PC (program counter)
        2. SP (Stack pointer)
        - damit wir wissen wo das program weiter machen kann und wo die stack informationen sitzen

### Process state
- vlaue of **pc** wurde durch cpu gespeichert als die exception invoked wurde
- parameter s to muss von der ex.S assembler file kommen

    - *exception* function im Kernel bekommt jetzt im parameter informationen mit, welche aus dem *ex.S* assembler code kommen
        - stackframe *s des momentanen prozesses
        (den wir speichern wollen)

## Passing the stack pointer to ex.S
- value des stackpointers s needs to come from the ex.S assembler file
- passed als parameter zu der kernel exception function 
    - über das register a0 register
    - stack pointer value nachdem wir alle register gespeichert haben

    nr = s->a7; // read syscall number from stack
    param = s->a0; // read parameter from stack


## Protection, again (PMP)
- enablen von zusätzlicher protection 
    - protection von programmen gegen einander
- definieren von 3 PMP regionen...
    ---

## Cooperation Extended
- Idee: Enable switching at every system call
    - dadurch können wir die 3-state process model implementieren
    - switch from running to blocked when I/O syscall
    - ansonsten zu "ready"

## Event loops
- erlauben OS die kontrolle zurückzugewinnen
    - aber nicht immer