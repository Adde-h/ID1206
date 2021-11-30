#include <stdio.h>
#include <stdlib.h>
#include "dlmall.c"
//#include "dlmall.h"

int main(int argc, char const *argv[])
{

	initialize();

	/* Traverse all memory blocks from arena until end */
	printf("\n** FIRST TRAVERSE ** \n");
	traverse();
	printf("\n");

  struct head *block1 = dalloc(100);

	printf("\n** SECOND TRAVERSE ** \n");
	traverse();
	printf("\n");

	printf("\n*******************\n");
  printf("memory allocated at %p \n", &block1);
  dfree(block1);
  printf("memory freed at %p", &block1);
	printf("\n*******************\n\n");

  sanity();

  printf("\n** THIRD TRAVERSE ** \n");
	traverse();
	printf("\n");

  return 0;
}
