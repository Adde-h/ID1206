#include <stdio.h>
#include <stdlib.h>
#include "dlmall.h"
int main(int argc, char const *argv[])
{
  struct head *block1 = dalloc(100);

  printf("memory allocated at %p \n", &block1);

  free(block1);
  
  printf("memory freed at %p \n", &block1);

  return 0;
}
