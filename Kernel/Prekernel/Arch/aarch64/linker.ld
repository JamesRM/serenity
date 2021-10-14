ENTRY(start)

PHDRS
{
  text PT_LOAD ;
  data PT_LOAD ;
  bss PT_LOAD ;
}

SECTIONS
{
    . = 0x00080000;

    .text ALIGN(4K) : AT (ADDR(.text))
    {
        *(.text.first)
        *(.text*)
    } :text

    .rodata ALIGN(4K) : AT (ADDR(.rodata))
    {
        *(.rodata*)
    } :data

    .data ALIGN(4K) : AT (ADDR(.data))
    {
        *(.data*)
    } :data

    .bss ALIGN(4K) (NOLOAD) : AT (ADDR(.bss))
    {
	start_of_bss = .;
        *(.bss)
	end_of_bss = .;
    } :bss
}

size_of_bss_divided_by_8 = (end_of_bss - start_of_bss) / 8;

page_tables_size = ((3 + 512) * 0x1000);
page_tables_phys_start = (end_of_bss + 4K) & ~(4k -1);
page_tables_phys_end = page_tables_phys_start + page_tables_size;
