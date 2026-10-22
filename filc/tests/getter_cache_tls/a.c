#include <pthread.h>
#include <stdio.h>
#include <stdfil.h>
extern int x;            /* defined __thread in b.c: ill-formed, but must stay memory safe */
extern long shared;      /* an ordinary extern global */
int *tls_addr_from_b(void);
__attribute__((noinline)) int *addr(void) { return &x; }
__attribute__((noinline)) long *shared_addr(void) { return &shared; }
static int *worker_x;
static void *worker(void *arg) { worker_x = addr(); *worker_x = 7; return 0; }
/* The fillers make the first calls to shared_addr(), released together, so
   they race to fill its cache slot. */
static pthread_barrier_t start;
static void *filler(void *arg) { pthread_barrier_wait(&start); long *p = shared_addr(); __atomic_fetch_add(p, 1, __ATOMIC_RELAXED); return p; }
int main(void) {
  pthread_t t; void *r;
  pthread_create(&t, 0, worker, 0); pthread_join(t, &r);
  zgc_request_and_wait();
  int *mine = addr();
  printf("tls: worker %p main %p b-main %p -> %s\n", (void*)worker_x, (void*)mine, (void*)tls_addr_from_b(),
         mine == tls_addr_from_b() && mine != worker_x ? "OK" : "FAIL");
  *mine = 1;
  pthread_t ts[16];
  pthread_barrier_init(&start, 0, 16);
  for (int i = 0; i < 16; i++) pthread_create(&ts[i], 0, filler, 0);
  int ok = 1;
  for (int i = 0; i < 16; i++) { pthread_join(ts[i], &r); ok &= r == (void*)&shared; }
  printf("shared: %ld fills, same pointer %s\n", shared, ok && r == (void*)shared_addr() ? "OK" : "FAIL");
  return 0;
}
