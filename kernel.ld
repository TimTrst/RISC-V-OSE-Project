OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

SECTIONS
{
  /*
   * ensure that entry.S / _entry is at 0x80000000,
   * where qemu's -kernel jumps.
   */
  . = 0x80000000;

  .text : {
    *(.text .text.*)
    . = ALIGN(0x1000);
    PROVIDE(etext = .);
  }	

  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  }

  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  }

  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  }

	/* user address space, main und syscall stub befinden sich dann hier*/

  . = 0x80100000; 
  .usertext : {
    *(.usertext .usertext.*)
    . = ALIGN(0x1000);
    PROVIDE(etext = .);
  }
  .userdata : {
    *(.userdata .userdata.*)
    . = ALIGN(0x1000);
    PROVIDE(etext = .);
  }

  PROVIDE(end = .);
}
