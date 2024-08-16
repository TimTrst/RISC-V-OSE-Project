....
(abschließende notizen zum VM kapitel, vor der implementation)

## Page tables in memory
- page tables werden für jeden prozess erstellt und in-memory gespeichert
    - diese werden durch physische adressen accessed (durch die virtullen adressen, können die jeweiligen physischen adressen der pagetables, bzw. dann der pages im page table ermittelt werden)

- 512 Einträge pro Table (2⁹ (9 bits pro level))
    - 512 * 8 (8bytes pro eintrag)
    - -> 4096bytes pro table
    --> das ist identisch mit der page size

### Simple case: Single level Page table
- nur eine einzige 4096byte page notwendig


### Für unseren Case 2-Level page table
- 512 einträge pro table
- erster table 512*8byte = 4096byte
--> jeder dieser einträge kann 
--> in disem ersten table kann ich mit 512 einträgen auf 512 anderere, 4096 byte große page tables verweisen
---> das ergibt dann 512 * 4096 mögliche page tables und somit 2MB große pages


## Paging and Multitasking
- jeder prozess hat eigenen *logischen adressbereich*
(jeder prozess kann dadurch bei virtueller adresse 0 starten, die jeweiligen individuellen translations erlauben das)

- jeder prozess benötigt individuelle page tables -> mappings dürfen sich nicht überschneiden mit anderen prozessen

- was müssen wir machen?
    1. mappings durch context swithching 
    2. vorher den TCB (MMU cache) flushen
    3. speichern des satp CSR values für jeden prozess!

# Simpe two-level page table setup
- nur eine 2MB page pro prozess für den moment in unserem programm
- dadurch brauchen wir auch eine 2MB page frame für jeden Prozess (Physische pages im speicher)

- neues physisches adresslayout
    - kernel at 0x8000_0000 -> 0x801F_FFFF
    - process 1 loaded at 0x8020_0000 -> 0x803F_FFFF
    - process 2 at 0x8040_0000 -> 0x805F_FFFF
    .. uns so weiter (immer 2 MB auseinander)

- Physische Adresse des einzigen page frames des prozess N ist
    - PA = 0x8000_0000 + n*0x0020_0000
    -> somit teilen wir allen prozessen genau die 2MB auseinanderliegenden pageframes im RAM zu 


- mit diesem setup brauchen wir nur ein Page Directiory (page table lookup) und einen weiteren page-table 
    - zwei 4096 byte pages


### Benefit for our compilation process
- alle prozesse haben die illusion, dass diese an adresse 0 starten
    -> verwenden des selben linker skritps!