#define _GNU_SOURCE
/* fix for the following error on Windows (WSL):
 * fstat: Value too large for defined data type */
#define _FILE_OFFSET_BITS 64
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <execinfo.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/types.h>

#include "bolos_syscalls.h"
#include "emulate.h"

#define DATA_ADDR       ((void *)0x20001000)
#define DATA_SIZE       (4096 * 10)
#define MAX_CODE_SIZE   0x9000  /* arbitrarily and probably invalid value */
#define LOAD_ADDR       ((void *)0x40000000)
#define LOAD_OFFSET     0x00010000

#define MAX_APP       16
#define MAIN_APP_NAME "main"

struct app_s {
  char *name;
  int fd;
};

struct memory_s {
  void *code;
  size_t code_size;
  void *data;
  size_t data_size;
};

static struct memory_s memory;
static struct app_s apps[MAX_APP];
static unsigned int napp;
static bool trace_syscalls;

static ucontext_t *context;
static unsigned long *svc_addr;
static unsigned int n_svc_call;

static void crash_handler(int sig_no)
{
  void *array[10];
  size_t size;

  size = backtrace(array, 10);

  fprintf(stderr, "[-] The app crashed with signal %d\n", sig_no);
  backtrace_symbols_fd(array, size, STDERR_FILENO);

  _exit(1);
}

static bool is_syscall_instruction(unsigned long addr)
{
  unsigned int i;

  for (i = 0; i < n_svc_call; i++) {
    if (svc_addr[i] == addr)
      return true;
  }

  return false;
}

static void sigill_handler(int sig_no, siginfo_t *UNUSED(info), void *vcontext)
{
  unsigned long pc, syscall, ret;
  unsigned long *parameters;
  int retid;

  context = (ucontext_t *)vcontext;
  syscall = context->uc_mcontext.arm_r0;
  parameters = (unsigned long *)context->uc_mcontext.arm_r1;
  pc = context->uc_mcontext.arm_pc;

  if (!is_syscall_instruction(context->uc_mcontext.arm_pc)) {
    fprintf(stderr, "[*] unhandled instruction at pc 0x%08lx\n", pc);
    fprintf(stderr, "    it would have triggered a crash on a real device\n");
    crash_handler(sig_no);
    _exit(1);
  }

  //fprintf(stderr, "[*] syscall: 0x%08lx (pc: 0x%08lx)\n", syscall, pc);

  ret = 0;
  retid = emulate(syscall, parameters, &ret, trace_syscalls);

  /* handle the os_lib_call syscall specially since it modifies the context
   * directly */
  if (syscall == SYSCALL_os_lib_call_ID_IN)
    return;

  /* In some versions of the SDK, a few syscalls don't use SVC_Call to issue
   * syscalls but call the svc instruction directly. I don't remember why it
   * fixes the issue however... */
  if (n_svc_call > 1 && 0) {
    parameters[0] = retid;
    parameters[1] = ret;
  } else {
    /* Default SVC_Call behavior */
    context->uc_mcontext.arm_r0 = retid;
    context->uc_mcontext.arm_r1 = ret;
  }

  /* skip undefined (originally svc) instruction */
  context->uc_mcontext.arm_pc += 2;
}

static int setup_signals(void)
{
  struct sigaction sig_action;

  memset(&sig_action, 0, sizeof(sig_action));

  sig_action.sa_sigaction = sigill_handler;
  sig_action.sa_flags = SA_RESTART | SA_SIGINFO;
  sigemptyset(&sig_action.sa_mask);

  if (sigaction(SIGILL, &sig_action, 0) != 0) {
    warn("sigaction(SIGILL)");
    return -1;
  }

  if (signal(SIGSEGV, &crash_handler) == SIG_ERR) {
    warn("signal(SIGSEGV");
    return -1;
  }

  return 0;
}

/*
 * Replace the SVC instruction with an undefined instruction.
 *
 * It generates a SIGILL upon execution, which is catched to handle that
 * syscall.
 */
static int patch_svc(void *p, size_t size)
{
  unsigned char *addr, *end, *next;
  int ret;

  /* XXX: hardcoded limit to avoid patching bytes after the .text section.
   * XXX:  An ELF parser should be used instead. */
  if (size > MAX_CODE_SIZE)
    size = MAX_CODE_SIZE;

  if (mprotect(p, size, PROT_READ | PROT_WRITE) != 0) {
    warn("mprotect(PROT_READ | PROT_WRITE)");
    return -1;
  }

  n_svc_call = 0;
  svc_addr = NULL;
  addr = p;
  end = addr + size;
  ret = 0;

  while (addr < end - 2) {
    next = memmem(addr, end - addr, "\x01\xdf", 2);
    if (next == NULL)
      break;

    /* instructions are aligned on 2 bytes */
    if ((unsigned long)addr & 1 && 0) {
      addr = (unsigned char *)next + 1;
      continue;
    }

    svc_addr = realloc(svc_addr, (n_svc_call + 1) * sizeof(unsigned long));
    if (svc_addr == NULL)
      err(1, "realloc");
    svc_addr[n_svc_call] = (unsigned long)next;

    /* undefined instruction */
    memcpy(next, "\xff\xde", 2);

    fprintf(stderr, "[*] patching svc instruction at %p\n", next);

    addr = (unsigned char *)next + 2;
    n_svc_call++;
  }

  if (mprotect(p, size, PROT_READ | PROT_EXEC) != 0) {
    warn("mprotect(PROT_READ | PROT_EXEC)");
    return -1;
  }

  if (n_svc_call == 0) {
    warnx("failed to find SVC_call");
    return -1;
  }

  return ret;
}

static struct app_s *search_app_by_name(char *name)
{
  unsigned int i;

  for (i = 0; i < napp; i++) {
    if (strcmp(apps[i].name, name) == 0)
      return &apps[i];
  }

  return NULL;
}

/* open the app file */
static int open_app(char *name, char *filename)
{
  int fd;

  if (napp >= MAX_APP) {
    warnx("too many apps opened");
    return -1;
  }

  if (search_app_by_name(name) != NULL) {
    warnx("can't load 2 apps with the same name (\"%s\")", name);
    return -1;
  }

  fd = open(filename, O_RDONLY);
  if (fd == -1) {
    warn("open(\"%s\")", filename);
    return -1;
  }

  apps[napp].name = name;
  apps[napp].fd = fd;

  napp++;

  return 0;
}

static void reset_memory(void)
{
  memory.code = MAP_FAILED;
  memory.code_size = 0;

  memory.data = MAP_FAILED;
  memory.data_size = 0;
}

void unload_running_app(void)
{
  if (munmap(memory.code, memory.code_size) != 0)
    warn("munmap code");

  if (munmap(memory.data, memory.data_size) != 0)
    warn("munmap data");

  reset_memory();
}

/* map the app to memory */
static void *load_app(char *name)
{
  void *code, *data;
  struct app_s *app;
  struct stat st;
  size_t size;

  code = MAP_FAILED;
  data = MAP_FAILED;

  app = search_app_by_name(name);
  if (app == NULL) {
    warn("failed to find app \"%s\"", name);
    goto error;
  }

  if (fstat(app->fd, &st) != 0) {
    warn("fstat");
    goto error;
  }

  size = st.st_size - LOAD_OFFSET;

  /* load code */
  code = mmap(LOAD_ADDR, size, PROT_READ | PROT_EXEC, MAP_PRIVATE | MAP_FIXED, app->fd, LOAD_OFFSET);
  if (code == MAP_FAILED) {
    warn("mmap code");
    goto error;
  }

  /* setup data */
  if (mmap(DATA_ADDR, DATA_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
    warn("mmap data");
    goto error;
  }

  if (patch_svc(code, size) != 0)
    goto error;

  memory.code = code;
  memory.code_size = size;
  memory.data = DATA_ADDR;
  memory.data_size = DATA_SIZE;

  return code;

 error:
  if (code != MAP_FAILED && munmap(code, size) != 0)
    warn("munmap");

  if (data != MAP_FAILED && munmap(DATA_ADDR, DATA_SIZE) != 0)
    warn("munmap");

  return NULL;
}

static int run_app(char *name, unsigned long *parameters)
{
  void (*f)(unsigned long *);
  void *p;

  p = load_app(name);
  if (p == NULL)
    return -1;

  /* thumb mode */
  f = (void *)((unsigned long)p | 1);

  f(parameters);

  /* should never return */
  errx(1, "the app returned, exiting...");

  return 0;
}

/* This is a bit different than run_app since this function is called within the
 * SIGILL handler. Just setup a fake context to allow the signal handler to
 * return to the lib code. */
int run_lib(char *name, unsigned long *parameters)
{
  unsigned long f;
  void *p;

  p = load_app(name);
  if (p == NULL)
    return -1;

  /* no thumb mode set when returning from signal handler */
  f = (unsigned long)p & 0xfffffffe;

  context->uc_mcontext.arm_r0 = (unsigned long)parameters;
  context->uc_mcontext.arm_pc = f;

  return 0;
}

static char *get_libname(char *arg, char **filename)
{
  char *libname, *p;
  size_t len;

  libname = strdup(arg);
  if (libname == NULL) {
    err(1, "strdup");
  }

  p = strchr(libname, ':');
  if (libname == NULL) {
    warnx("invalid app name (\"%s\"): missing semicolon", libname);
    free(libname);
    return NULL;
  }

  *p = '\x00';

  len = strlen(libname);
  *filename = arg + len + 1;

  return libname;
}

static int load_apps(int argc, char *argv[])
{
  char *filename, *libname;
  int i;

  for (i = 0; i < argc; i++) {
    if (i == 0) {
      libname = MAIN_APP_NAME;
      filename = argv[i];
    } else {
      libname = get_libname(argv[i], &filename);
      if (libname == NULL)
        return -1;
    }

    if (open_app(libname, filename) != 0)
      return -1;
  }

  return 0;
}

static void usage(char *argv0)
{
  fprintf(stderr, "Usage: %s [-t] <app.elf> [libname:lib.elf ...]\n", argv0);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  int opt;

  trace_syscalls = false;

  while((opt = getopt(argc, argv, "t")) != -1) {
    switch(opt) {
    case 't':
      trace_syscalls = true;
      break;
    default:
      usage(argv[0]);
      break;
    }
  }

  if (argc - optind <= 0) {
    usage(argv[0]);
  }

  reset_memory();

  if (load_apps(argc - optind, &argv[optind]) != 0)
    return 1;

  if (setup_signals() != 0)
    return 1;

  run_app(MAIN_APP_NAME, NULL);

  return 0;
}
