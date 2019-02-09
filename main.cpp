#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096

static void check(bool ret)
{
  if (not ret) {
    perror("syscall failed");
    exit(EXIT_FAILURE);
  }
}

static void set_signal_handler(int signum, void (*handler)(int))
{
  struct sigaction sa {};
  int rc;

  sa.sa_handler = handler;
  rc = sigaction(signum, &sa, nullptr);
  check(rc == 0);
}

static void signal_handler(int sig)
{
  switch (sig) {
  case SIGSEGV:
    // ud0 didn't execute and needed to fetch more instruction bytes for an unused ModRM byte.
    printf("This is probably a Core microarchitecture CPU.\n");
    break;
  case SIGILL:
    // ud0 actually executed.
    printf("This is probably an Atom microarchitecture CPU.\n");
    break;
  default:
    printf("Unexpected signal?\n");
    break;
  };

  exit(EXIT_SUCCESS);
}

int main()
{
  char *pages = (char *)mmap(nullptr, 2*PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  static const unsigned char ud0_bytes[] = { 0x0f, 0xff };
  int rc;

  check(pages != MAP_FAILED);

  char *target = pages + PAGE_SIZE - sizeof(ud0_bytes);

  memcpy(target, ud0_bytes, sizeof(ud0_bytes));

  rc = mprotect(pages, PAGE_SIZE, PROT_EXEC | PROT_READ);
  check(rc == 0);

  rc = mprotect(pages + PAGE_SIZE, PAGE_SIZE, PROT_NONE);
  check(rc == 0);

  for (auto sig : { SIGILL, SIGSEGV} )
    set_signal_handler(sig, signal_handler);

  asm ("jmp *%0"
       :: "r" (target), "m" (*(unsigned int (*)[2])target));

  // Never reached.
  return EXIT_FAILURE;
}
