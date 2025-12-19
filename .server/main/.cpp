#include <time.hpp>
int parse(int ac, char* av[]);
int run(long long start);

int main(int ac, char *av[]) {
  long long c = time::clock();
  int r = parse(ac, av); return r == 2 ? 0 : (r || run(c));
}