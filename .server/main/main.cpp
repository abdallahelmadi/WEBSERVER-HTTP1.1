int parse(int ac, char* av[]);
int run(void) { return 0; };

int main(int ac, char *av[]) {
  int r = parse(ac, av); return r == 2 ? 0 : (r || run());
}