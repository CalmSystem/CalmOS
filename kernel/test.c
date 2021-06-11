#include "scheduler.h"
#include "interrupt.h"
#include "stdio.h"

#define DUMMY_VAL 78

#define TRUE 1
#define FALSE 0

/*******************************************************************************
 * Division 64 bits
 ******************************************************************************/
unsigned long long div64(unsigned long long x, unsigned long long div,
                         unsigned long long *rem) {
  unsigned long long mul = 1;
  unsigned long long q;

  if ((div > x) || !div) {
    if (rem) *rem = x;
    return 0;
  }

  while (!((div >> 32) & 0x80000000ULL)) {
    unsigned long long newd = div + div;
    if (newd > x) break;
    div = newd;
    mul += mul;
  }

  q = mul;
  x -= div;
  while (1) {
    mul /= 2;
    div /= 2;
    if (!mul) {
      if (rem) *rem = x;
      return q;
    }
    if (x < div) continue;
    q += mul;
    x -= div;
  }
}

/*******************************************************************************
 * Pseudo random number generator
 ******************************************************************************/
/*static unsigned long long mul64(unsigned long long x, unsigned long long y) {
  unsigned long a, b, c, d, e, f, g, h;
  unsigned long long res = 0;
  a = x & 0xffff;
  x >>= 16;
  b = x & 0xffff;
  x >>= 16;
  c = x & 0xffff;
  x >>= 16;
  d = x & 0xffff;
  e = y & 0xffff;
  y >>= 16;
  f = y & 0xffff;
  y >>= 16;
  g = y & 0xffff;
  y >>= 16;
  h = y & 0xffff;
  res = d * e;
  res += c * f;
  res += b * g;
  res += a * h;
  res <<= 16;
  res += c * e;
  res += b * f;
  res += a * g;
  res <<= 16;
  res += b * e;
  res += a * f;
  res <<= 16;
  res += a * e;
  return res;
}

typedef unsigned long long uint_fast64_t;
typedef unsigned long uint_fast32_t;

static const uint_fast64_t _multiplier = 0x5DEECE66DULL;
static const uint_fast64_t _addend = 0xB;
static const uint_fast64_t _mask = (1ULL << 48) - 1;
static uint_fast64_t _seed = 1;

// Assume that 1 <= _bits <= 32
static uint_fast32_t randBits(int _bits) {
  uint_fast32_t rbits;
  uint_fast64_t nextseed = (mul64(_seed, _multiplier) + _addend) & _mask;
  _seed = nextseed;
  rbits = nextseed >> 16;
  return rbits >> (32 - _bits);
}

static void setSeed(uint_fast64_t _s) { _seed = _s; }

static unsigned long rand() { return randBits(32); }*/

/*******************************************************************************
 * Unmask interrupts for those who are working in kernel mode
 ******************************************************************************/
static void test_it() {
  __asm__ volatile(
      "pushfl; testl $0x200,(%%esp); jnz 0f; sti; nop; cli; 0: addl "
      "$4,%%esp\n" ::
          : "memory");
}

/*******************************************************************************
 * Test 1
 *
 * Demarrage de processus avec passage de parametre
 * Terminaison normale avec valeur de retour
 * Attente de terminaison (cas fils avant pere et cas pere avant fils)
 ******************************************************************************/
static int dummy1(void *arg) {
  printf("1");
  assert((int)arg == DUMMY_VAL);
  return 3;
}

static int dummy1_2(void *arg) {
  printf(" 5");
  assert((int)arg == DUMMY_VAL + 1);

  return 4;
}

static void test1(void) {
  int pid1;
  int r;
  int rval;

  pid1 = start(dummy1, 4000, 192, "paramRetour", (void *)DUMMY_VAL);
  assert(pid1 > 0);
  printf(" 2");
  r = waitpid(pid1, &rval);
  assert(r == pid1);
  assert(rval == 3);
  printf(" 3");
  pid1 = start(dummy1_2, 4000, 100, "paramRetour", (void *)(DUMMY_VAL + 1));
  assert(pid1 > 0);
  printf(" 4");
  r = waitpid(pid1, &rval);
  assert(r == pid1);
  assert(rval == 4);
  printf(" 6.\n");
}

/*******************************************************************************
 * Test 2
 *
 * kill() de fils suspendu pas demarre
 * waitpid() de ce fils termine par kill()
 * waitpid() de fils termine par exit()
 ******************************************************************************/
static int dummy2(void *args) {
  printf(" X");
  return (int)args;
}

static int dummy2_2(void *args) {
  printf(" 5");
  exit((int)args);
  assert(0);
  return 0;
}

static void test2(void) {
  int rval;
  int r;
  int pid1;
  int val = 45;

  printf("1");
  pid1 = start(dummy2, 4000, 100, "procKill", (void *)val);
  assert(pid1 > 0);
  printf(" 2");
  r = kill(pid1);
  assert(r == 0);
  printf(" 3");
  r = waitpid(pid1, &rval);
  assert(rval == 0);
  assert(r == pid1);
  printf(" 4");
  pid1 = start(dummy2_2, 4000, 192, "procExit", (void *)val);
  assert(pid1 > 0);
  printf(" 6");
  r = waitpid(pid1, &rval);
  assert(rval == val);
  assert(r == pid1);
  assert(waitpid(getpid(), &rval) < 0);
  printf(" 7.\n");
}

/*******************************************************************************
 * Test 3
 *
 * chprio() et ordre de scheduling
 * kill() d'un processus qui devient moins prioritaire
 ******************************************************************************/
static int proc_prio4(void *arg) {
  /* arg = priority of this proc. */
  int r;

  assert(getprio(getpid()) == (int)arg);
  printf("1");
  r = chprio(getpid(), 64);
  assert(r == (int)arg);
  printf(" 3");
  return 0;
}

static int proc_prio5(void *arg) {
  /* Arg = priority of this proc. */
  int r;

  assert(getprio(getpid()) == (int)arg);
  printf(" 7");
  r = chprio(getpid(), 64);
  assert(r == (int)arg);
  printf("error: I should have been killed\n");
  assert(0);
  return 0;
}

static void test3(void) {
  int pid1;
  int p = 192;
  int r;

  assert(getprio(getpid()) == 128);
  pid1 = start(proc_prio4, 4000, p, "prio", (void *)p);
  assert(pid1 > 0);
  printf(" 2");
  r = chprio(getpid(), 32);
  assert(r == 128);
  printf(" 4");
  r = chprio(getpid(), 128);
  assert(r == 32);
  printf(" 5");
  assert(waitpid(pid1, 0) == pid1);
  printf(" 6");

  assert(getprio(getpid()) == 128);
  pid1 = start(proc_prio5, 4000, p, "prio", (void *)p);
  assert(pid1 > 0);
  printf(" 8");
  r = kill(pid1);
  assert(r == 0);
  assert(waitpid(pid1, 0) == pid1);
  printf(" 9");
  r = chprio(getpid(), 32);
  assert(r == 128);
  printf(" 10");
  r = chprio(getpid(), 128);
  assert(r == 32);
  printf(" 11.\n");
}

/*******************************************************************************
 * Test 4
 *
 * Boucles d'attente active (partage de temps)
 * chprio()
 * kill() de processus de faible prio
 * kill() de processus deja mort
 ******************************************************************************/
static const int loop_count0 = 5000;
static const int loop_count1 = 10000;

static int busy_loop1(void *arg) {
  (void)arg;
  while (1) {
    int i, j;

    printf(" A");
    for (i = 0; i < loop_count1; i++) {
      test_it();
      for (j = 0; j < loop_count0; j++)
        ;
    }
  }
  return 0;
}

/* assume the process to suspend has a priority == 64 */
static int busy_loop2(void *arg) {
  int i;

  for (i = 0; i < 3; i++) {
    int k, j;

    printf(" B");
    for (k = 0; k < loop_count1; k++) {
      test_it();
      for (j = 0; j < loop_count0; j++)
        ;
    }
  }
  i = chprio((int)arg, 16);
  assert(i == 64);
  return 0;
}

static void test4(void) {
  int pid1, pid2;
  int r;
  int arg = 0;

  assert(getprio(getpid()) == 128);
  pid1 = start(busy_loop1, 4000, 64, "busy1", (void *)arg);
  assert(pid1 > 0);
  pid2 = start(busy_loop2, 4000, 64, "busy2", (void *)pid1);
  assert(pid2 > 0);
  printf("1 -");
  r = chprio(getpid(), 32);
  assert(r == 128);
  printf(" - 2");
  r = kill(pid1);
  assert(r == 0);
  assert(waitpid(pid1, 0) == pid1);
  r = kill(pid2);
  assert(r < 0); /* kill d'un processus zombie */
  assert(waitpid(pid2, 0) == pid2);
  printf(" 3");
  r = chprio(getpid(), 128);
  assert(r == 32);
  printf(" 4.\n");
}

/*******************************************************************************
 * Test 5
 *
 * Tests de quelques parametres invalides.
 * Certaines interdictions ne sont peut-etre pas dans la spec. Changez les pour
 * faire passer le test correctement.
 ******************************************************************************/
static int no_run(void *arg) {
  (void)arg;
  assert(0);
  return 1;
}

static int waiter(void *arg) {
  int pid = (int)arg;
  assert(kill(pid) == 0);
  assert(waitpid(pid, 0) < 0);
  return 1;
}

static void test5(void) {
  int pid1, pid2;
  int r;

  // Le processus 0 et la priorite 0 sont des parametres invalides
  assert(kill(0) < 0);
  assert(chprio(getpid(), 0) < 0);
  assert(getprio(getpid()) == 128);
  pid1 = start(no_run, 4000, 64, "norun", 0);
  assert(pid1 > 0);
  assert(kill(pid1) == 0);
  assert(kill(pid1) < 0);         // pas de kill de zombie
  assert(chprio(pid1, 128) < 0);  // changer la priorite d'un zombie
  assert(chprio(pid1, 64) < 0);   // changer la priorite d'un zombie
  assert(waitpid(pid1, 0) == pid1);
  assert(waitpid(pid1, 0) < 0);
  pid1 = start(no_run, 4000, 64, "norun", 0);
  assert(pid1 > 0);
  pid2 = start(waiter, 4000, 65, "waiter", (void *)pid1);
  assert(pid2 > 0);
  assert(waitpid(pid2, &r) == pid2);
  assert(r == 1);
  assert(waitpid(pid1, &r) == pid1);
  assert(r == 0);
  printf("ok.\n");
}

/*******************************************************************************
 * Test 6
 *
 * Waitpid multiple.
 * Creation de processus avec differentes tailles de piles.
 ******************************************************************************/
extern int __proc6_1(void *arg);
extern int __proc6_2(void *arg);

__asm__(
    ".text\n"
    ".globl __proc6_1\n"
    "__proc6_1:\n"
    "movl $3,%eax\n"
    "ret\n"
    ".globl __proc6_2\n"
    "__proc6_2:\n"
    "movl 4(%esp),%eax\n"
    "pushl %eax\n"
    "popl %eax\n"
    "ret\n"
    ".previous\n");

static void test6(void) {
  int pid1, pid2, pid3;
  int ret;

  assert(getprio(getpid()) == 128);
  pid1 = start(__proc6_1, 0, 64, "proc6_1", 0);
  assert(pid1 > 0);
  pid2 = start(__proc6_2, 4, 66, "proc6_2", (void *)4);
  assert(pid2 > 0);
  pid3 = start(__proc6_2, 0xffffffff, 65, "proc6_3", (void *)5);
  assert(pid3 < 0);
  pid3 = start(__proc6_2, 8, 65, "proc6_3", (void *)5);
  assert(pid3 > 0);
  assert(waitpid(-1, &ret) == pid2);
  assert(ret == 4);
  assert(waitpid(-1, &ret) == pid3);
  assert(ret == 5);
  assert(waitpid(-1, &ret) == pid1);
  assert(ret == 3);
  assert(waitpid(pid1, 0) < 0);
  assert(waitpid(-1, 0) < 0);
  assert(waitpid(getpid(), 0) < 0);
  printf("ok.\n");
}

/*******************************************************************************
 * Test 7
 *
 * Test de l'horloge (ARR et ACE)
 * Tentative de determination de la frequence du processeur et de la
 * periode de scheduling
 ******************************************************************************/

static int proc_timer1(void *arg) {
  unsigned long quartz;
  unsigned long ticks;
  unsigned long dur;
  int i;

  (void)arg;

  clock_settings(&quartz, &ticks);
  dur = (quartz + ticks) / ticks;
  printf(" 2");
  for (i = 4; i < 8; i++) {
    wait_clock(current_clock() + dur);
    printf(" %d", i);
  }
  return 0;
}

static volatile unsigned long timer;

static int proc_timer(void *arg) {
  (void)arg;
  while (1) {
    unsigned long t = timer + 1;
    timer = t;
    while (timer == t) test_it();
  }
  while (1)
    ;
  return 0;
}

static int sleep_pr1(void *args) {
  (void)args;
  wait_clock(current_clock() + 2);
  printf(" not killed !!!");
  assert(0);
  return 1;
}

static void test7(void) {
  int pid1, pid2, r;
  unsigned long c0, c, quartz, ticks, dur;

  assert(getprio(getpid()) == 128);
  printf("1");
  pid1 = start(proc_timer1, 4000, 129, "timer", 0);
  assert(pid1 > 0);
  printf(" 3");
  assert(waitpid(-1, 0) == pid1);
  printf(" 8 : ");

  timer = 0;
  pid1 = start(proc_timer, 4000, 127, "timer1", 0);
  pid2 = start(proc_timer, 4000, 127, "timer2", 0);
  assert(pid1 > 0);
  assert(pid2 > 0);
  clock_settings(&quartz, &ticks);
  dur = 2 * quartz / ticks;
  test_it();
  c0 = current_clock();
  do {
    test_it();
    c = current_clock();
  } while (c == c0);
  wait_clock(c + dur);
  assert(kill(pid1) == 0);
  assert(waitpid(pid1, 0) == pid1);
  assert(kill(pid2) == 0);
  assert(waitpid(pid2, 0) == pid2);
  printf("%lu changements de contexte sur %lu tops d'horloge", timer, dur);
  pid1 = start(sleep_pr1, 4000, 192, "sleep", 0);
  assert(pid1 > 0);
  assert(kill(pid1) == 0);
  assert(waitpid(pid1, &r) == pid1);
  assert(r == 0);
  printf(".\n");
}

/*******************************************************************************
 * Test 8
 *
 * Creation de processus se suicidant en boucle. Test de la vitesse de creation
 * de processus.
 ******************************************************************************/
static int suicide(void *arg) {
  (void)arg;
  kill(getpid());
  assert(0);
  return 0;
}

static int suicide_launcher(void *arg) {
  int pid1;
  (void)arg;
  pid1 = start(suicide, 4000, 192, "suicide_launcher", 0);
  assert(pid1 > 0);
  return pid1;
}

static void test8(void) {
  unsigned long long tsc1;
  unsigned long long tsc2;
  int i, r, pid, count;

  assert(getprio(getpid()) == 128);

  /* Le petit-fils va passer zombie avant le fils mais ne pas
  etre attendu par waitpid. Un nettoyage automatique doit etre
  fait. */
  pid = start(suicide_launcher, 4000, 129, "suicide_launcher", 0);
  assert(pid > 0);
  assert(waitpid(pid, &r) == pid);
  assert(chprio(r, 192) < 0);

  count = 0;
  __asm__ __volatile__("rdtsc" : "=A"(tsc1));
  do {
    for (i = 0; i < 10; i++) {
      pid = start(suicide_launcher, 4000, 200, "suicide_launcher", 0);
      assert(pid > 0);
      assert(waitpid(pid, 0) == pid);
    }
    test_it();
    count += i;
    __asm__ __volatile__("rdtsc" : "=A"(tsc2));
  } while ((tsc2 - tsc1) < 1000000000);
  printf("%lu cycles/process.\n",
         (unsigned long)div64(tsc2 - tsc1, 2 * count, 0));
}

/* End */
static void quit(void) { exit(0); }

static struct {
	const char *name;
	void (*f) (void);
} commands[] = {
	{"1", test1},
	{"2", test2},
	{"3", test3},
	{"4", test4},
	{"5", test5},
	{"6", test6},
	{"7", test7},
	{"8", test8},
	/*{"9", test9},
	{"10", test10},
	{"11", test11},
	{"12", test12},
	{"13", test13},
	{"14", test14},
	{"15", test15},
	{"16", test16},
	{"17", test17},
	{"18", test18},
	{"19", test19},
	{"20", test20},*/
	{"q", quit},
	{"quit", quit},
	{"exit", quit},
	{0, 0},
};

int test_proc(void* arg) {
  const int n = (int)arg;
  assert(getprio(getpid()) == 128);
  if ((n < 1) || (n > 20)) {
    printf("%d: unknown test\n", n);
  } else {
    commands[n - 1].f();
  }
  return 0;
}

void test_n(int n) {
  start(test_proc, 4000, 128, "test", (void*)n);
}
