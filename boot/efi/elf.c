#include <libefi.h>
#include <elf.h>
#include <fat.h>
#include <align.h>

// Alloc and load program header of elf.
static INTN load_program_header(struct elf *elf_info)
{
  UINTN size = elf_info->elf_header.e_phentsize * elf_info->elf_header.e_phnum;
  elf_info->program_header = malloc(size);
  if(elf_info->program_header == NULL)
    return 1;
  
  EFI_FILE_PROTOCOL *file_interface = elf_info->file_interface;
  fat_set_position(file_interface, elf_info->elf_header.e_phoff);
  if(fat_read_file(file_interface, &size, elf_info->program_header))
    return 1;

  return 0;
} 

// Alloc and load section header of elf.
static INTN load_section_header(struct elf *elf_info)
{
  UINTN size = elf_info->elf_header.e_shentsize * elf_info->elf_header.e_shnum;  
  elf_info->section_header = malloc(size);
  if(elf_info->section_header == NULL)
    return 1;

  EFI_FILE_PROTOCOL *file_interface = elf_info->file_interface;
  fat_set_position(file_interface, elf_info->elf_header.e_shoff);
  if(fat_read_file(file_interface, &size, elf_info->section_header))
    return 1;

  return 0;
}

// Compute kernel size.
static void compute_kernel_size(struct elf *elf_info)
{
  UINTN i;
  UINTN kernel_size = 0;
  UINT64 p_memsz, p_align;
  for(i = 0; i < elf_info->elf_header.e_phnum; i++)
  {
    // Check if program header entry need to be loaded
    if(elf_info->program_header[i].p_type == PT_LOAD)
    {
      p_align = elf_info->program_header[i].p_align;
      p_memsz = elf_info->program_header[i].p_memsz;
      // Is aligned?
      if(p_memsz & p_align - 1)
        // Align size.
        kernel_size += ALIGNUP(p_memsz, p_align) /*p_memsz + (p_align - 1) & ~(p_align - 1)*/;
    }
  }

  elf_info->kernel_size = kernel_size;
}

// Check if file is an elf.
static INTN is_elf(struct elf *elf_info)
{
  return *((UINT32 *) &elf_info->elf_header.e_ident) == 0x464c457f;
}

INTN elf_parse(struct elf *elf_info)
{
  EFI_FILE_PROTOCOL *file_interface = elf_info->file_interface;

  UINTN size = sizeof(struct elf_64_header);
  if(fat_read_file(file_interface, &size, &elf_info->elf_header))
    return 1;

  if(!is_elf(elf_info))
    return 1;

  if(load_program_header(elf_info))
    return 1;

  compute_kernel_size(elf_info);

  if(load_section_header(elf_info))
    return 1;

  return 0;
}

INTN elf_load_kernel(struct elf *elf_info)
{
  EFI_STATUS status;

  EFI_PHYSICAL_ADDRESS address = elf_info->elf_header.e_entry;
  status = BS->AllocatePages(AllocateMaxAddress,
                             EfiLoaderData,
                             elf_info->kernel_size / PAGE_SIZE,
                             &address);

  if(status != EFI_SUCCESS)
    return 1;
  
  printf(L"kernel_size: %i\r\n", elf_info->kernel_size);

  UINTN i;
  UINTN size;
  UINTN position;
  EFI_FILE_PROTOCOL *file_interface = elf_info->file_interface;
  for(i = 0; i < elf_info->elf_header.e_phnum; i++)
  {
    size = ALIGNUP(elf_info->program_header[i].p_memsz, elf_info->program_header[i].p_align);
    address = elf_info->program_header[i].p_paddr;
    position = elf_info->program_header[i].p_offset;
    fat_set_position(file_interface, position);
    if(fat_read_file(file_interface, &size, (EFI_PHYSICAL_ADDRESS *) address))
      return 1;
  }

  return 0;
}
