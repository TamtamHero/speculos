#include <stdio.h>
#include <unistd.h>

#include "emulate.h"

#define BOLOS_TAG_APPNAME			0x01
#define BOLOS_TAG_APPVERSION	0x02

#define PATH_MAX  1024

static try_context_t *try_context;

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

unsigned long sys_try_context_set(try_context_t *context)
{
  try_context_t *previous_context;

  previous_context = try_context;
  try_context = context;

  return (unsigned long)previous_context;
}

unsigned long sys_try_context_get(void)
{
  return (unsigned long)try_context;
}

unsigned long sys_os_sched_last_status(void)
{
  return 0; /* XXX */
}

unsigned long sys_check_api_level(void)
{
  return 0;
}

unsigned long sys_os_sched_exit(void)
{
  fprintf(stderr, "[*] exit syscall called (skipped)\n");
  return 0;
}

unsigned long sys_reset(void)
{
  fprintf(stderr, "[*] reset syscall called (skipped)\n");
  return 0;
}

unsigned long sys_os_lib_throw(unsigned int exception)
{
  fprintf(stderr, "[*] os_lib_throw(0x%x) unhandled\n", exception);
  _exit(1);
  return 0;
}
