SECTIONS {
  /* below 0x44f88 reserved for/by RedBoot */
  . = 0x100000;

  /* mark text section */
  __text_start = ADDR(.text);
  __text_end = ADDR(.text) + SIZEOF(.text);

  /* mark data section */
  __data_start = ADDR(.data);
  __data_end = ADDR(.data) + SIZEOF(.data);

  /* mark BSS segment */
  __bss_start = ADDR(.bss);
  __bss_end   = ADDR(.bss) + SIZEOF(.bss);

  /* needed for certain newlib routines that (potentially) call _sbrk */
  end = __bss_end;

  /* mark constructor initialization array */
  /* __init_array_start = ADDR(.init_array); */
  /* __init_array_end   = ADDR(.init_array) + SIZEOF(.init_array); */
}

