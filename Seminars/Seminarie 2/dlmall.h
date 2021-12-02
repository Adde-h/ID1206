#include <stddef.h>
void *dalloc(size_t request);
void dfree(void *memory);
void sanity();
void traverse();
void initialize();
void block_sizes(int max);
int flist_size(unsigned int *size);