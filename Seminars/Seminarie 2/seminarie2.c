#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#define TRUE 1
#define FALSE 0
#define HEAD (sizeof(struct head))
#define MIN(size) (((size) > (8)) ? (size) : (8))
#define LIMIT(size) (MIN(0) + HEAD + size)
#define MAGIC(memory) ((struct head *)memory - 1)
#define HIDE(block) (void *)((struct head *)block + 1)
#define ALIGN 8
#define ARENA (64 * 1024)

struct head
{
  uint16_t bfree;    // 2 bytes, the status of block before
  uint16_t bsize;    // 2 bytes, the size of block before
  uint16_t free;     // 2 bytes, the status of the block
  uint16_t size;     // 2 bytes, the size (max 2^16 i . e . 64 Ki byte )
  struct head *next; // 8 bytes pointer
  struct head *prev; // 8 bytes pointer
};

struct head *after(struct head *block)
{
  return (struct head *)((char *)block + block->size + HEAD);
}

struct head *before(struct head *block)
{
  return (struct head *)((char *)block - block->bsize - HEAD);
}

struct head *split(struct head *block, int size)
{
  int rsize = block->size - size - HEAD;
  block->size = size;

  struct head *splt = after(block);
  splt->bsize = block->size;
  splt->bfree = block->free;
  splt->size = rsize;
  splt->free = TRUE;

  struct head *aft = after(splt);
  aft->bsize = rsize;
  return splt;
}

struct head *arena = NULL;

struct head *new ()
{
  if (arena != NULL)
  {
    printf("one arena already allocated\n");
    return NULL;
  }

  // using mmap, but could have used sbrk
  struct head *new = mmap(NULL, ARENA, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (new == MAP_FAILED)
  {
    printf("mmap failed : error %d\n", errno);
    return NULL;
  }

  /* make room for head and dummy */
  uint size = ARENA - 2 * HEAD;
  new->bfree = FALSE;
  new->bsize = 1; //dummy
  new->free = TRUE;
  new->size = size;

  struct head *sentinel = after(new);
  /* only touch the status fields */
  sentinel->bfree = new->free;
  sentinel->bsize = size;
  sentinel->free = FALSE;
  sentinel->size = 0;

  /* this is the only arena we have */
  arena = (struct head *)new;
  return new;
}
