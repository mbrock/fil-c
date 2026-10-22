__thread int x;
long shared;
int *tls_addr_from_b(void) { return &x; }
