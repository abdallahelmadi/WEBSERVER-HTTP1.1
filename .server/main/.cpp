#include <time.hpp>
int parse(int ac, char* av[]);
int run(long long start, char *envp[]);

int main(int ac, char *av[], char *envp[]) {
  long long c = time::clock();
  int r = parse(ac, av); return r == 2 ? 0 : (r || run(c, envp));
}