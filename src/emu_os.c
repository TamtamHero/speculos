#include "emulate.h"

#define BOLOS_TAG_APPNAME			0x01
#define BOLOS_TAG_APPVERSION	0x02

#define PATH_MAX  1024

unsigned long sys_os_flags(void)
{
  /* not in recovery and mcu unsigned */
  return 0;
}

unsigned long sys_os_registry_get_current_app_tag(unsigned int tag, uint8_t *buffer, size_t length)
{
  if (length < 1) {
    return 0;
  }

  length = 0;

  /* TODO */
  switch (tag) {
  case BOLOS_TAG_APPNAME:
    break;
  case BOLOS_TAG_APPVERSION:
    break;
  default:
    break;
  }

  buffer[length] = '\x00';

  return length;
}

unsigned long sys_os_lib_call(unsigned long *call_parameters)
{
  char libname[PATH_MAX];

  /* libname must be on the stack */
  strncpy(libname, (char *)call_parameters[0], sizeof(libname));

  /* unmap current app */
  unload_running_app();

  /* map lib code and jump to main */
  run_lib(libname, &call_parameters[1]);

  return 0xdeadbeef;
}
