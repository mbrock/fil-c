#include <cassert>
#include <cstdio>
#include <variant>
struct S { long tag; union { long i; int *p; } u; };
__attribute__((noinline)) S make(int *p) { S s; s.tag = 1; s.u.p = p; return s; }
__attribute__((noinline)) int take(S s) { return *s.u.p; }
__attribute__((noinline)) std::variant<long, int *> mv(int *p) { return p; }
struct Base { union { long i; int *p; } u; };
struct Derived : Base { long tag; };
__attribute__((noinline)) Derived inherited(Derived value) { return value; }
int main() {
  int x = 42;
  S s = make(&x);
  Derived d;
  d.u.p = &x;
  d.tag = 73;
  d = inherited(d);
  assert(d.tag == 73 && *d.u.p == 42);
  std::printf("%d %d %d\n", *s.u.p, take(s), *std::get<int *>(mv(&x)));
}
