OUTPUT_ARCH(m68k)

MEMORY
{
    rom (rx) : ORIGIN = 0x00000000, LENGTH = 0x00400000
    ram (rwx) : ORIGIN = 0x00FF0000, LENGTH = 0x00010000
}

SECTIONS
{
  .text :
  {
    KEEP(*(.text.keepboot))
	. = ALIGN(2);
  } > rom
  _stext = SIZEOF (.text);

  .data :
  /*AT ( LOADADDR (.text) + SIZEOF (.text) )*/
  {
    *(.data .data.*)
    _edata = .;
    . = ALIGN(2);
  } > ram AT > rom
  
  .ramtext :
  {      
    *(.text.*) *(.text)
    . = ALIGN(2);
    *(.rodata .rodata.*)
	  . = ALIGN(2);
  } > ram AT > rom
  _sdata = SIZEOF (.data) + SIZEOF (.ramtext);

  .bss (NOLOAD) :
  {
    __bss_start = . ;
    *(.bss .bss.*)
    *(COMMON)
    _end =  ALIGN (2);
    __end = _end;
    end = _end;
  } > ram
}
