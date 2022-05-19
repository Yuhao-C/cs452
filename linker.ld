SECTIONS {
  /* below 0x44f88 reserved for/by RedBoot */
  . = 0x100000;

  /* mark BSS segment */
  __bss_start = ADDR(.bss);
  __bss_end   = ADDR(.bss) + SIZEOF(.bss);

  /* needed for certain newlib routines that (potentially) call _sbrk */
  end = __bss_end;

  /* mark constructor initialization array */
  /* __init_array_start = ADDR(.init_array); */
  /* __init_array_end   = ADDR(.init_array) + SIZEOF(.init_array); */
}

