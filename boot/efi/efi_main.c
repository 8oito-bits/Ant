#include <efi.h>
#include <libefi.h>
#include <fat.h>
#include <elf.h>

void (*_start_ant)(void);

VOID exit(UINTN status)
{
  BS->Exit(IH, 1, 0, NULL);
}

VOID error(CHAR16 *string)
{
  puts(string);
  exit(1);
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
  EFI_STATUS status;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *conout;
  init_lib(image_handle, system_table);

  conout = ST->ConOut;
  status = conout->SetMode(conout, 0);
  if(status != EFI_SUCCESS)
    error(L"SetMode: Failed to set mode. ");
  
  conout->SetAttribute(conout, EFI_BACKGROUND_BLACK | EFI_WHITE);
  
  UINTN columns;
  UINTN rows;
  status = conout->QueryMode(conout, 0, &columns, &rows);
  if(status != EFI_SUCCESS)
    error(L"QueryMode: Failed to query mode.");

  EFI_FILE_PROTOCOL *file_protocol; 
  if(fat_init_file_protocol(&file_protocol))
    error(L"fat_init_file_protocol: Failed to init protocol.");

  EFI_FILE_PROTOCOL *kfile_interface;
  if(fat_open_file(file_protocol, &kfile_interface, L"\\boot\\ant_kernel", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY))
    error(L"fat_open_file: Failed to open file.");

  puts(L"Loading the elf kernel...");

  struct elf kernel_info;
  kernel_info.file_interface = kfile_interface;
  if(elf_parse(&kernel_info))
    error(L"load_kernel: Failed to parse kernel.");

  if(elf_load_kernel(&kernel_info))
    error(L"elf_load_kernel: Failed to load kernel");

  elf_clear_all(&kernel_info);    

  _start_ant = (void (*)(void)) kernel_info.elf_header.e_entry;
  _start_ant();

  return EFI_SUCCESS;
}
