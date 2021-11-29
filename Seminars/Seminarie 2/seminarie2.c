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
	uint16_t bfree;		 // 2 bytes, the status of block before
	uint16_t bsize;		 // 2 bytes, the size of block before
	uint16_t free;		 // 2 bytes, the status of the block
	uint16_t size;		 // 2 bytes, the size (max 2^16 i . e . 64 Ki byte )
	struct head *next; // 8 bytes pointer
	struct head *prev; // 8 bytes pointer
};

struct head *after(struct head *block)
{
	return (struct head *)((char *)block + HEAD + block->size);
}

struct head *before(struct head *block)
{
	return (struct head *)((char *)block - block->bsize - HEAD);
}

struct head *split(struct head *block, int size)
{
	/* size of full block - size of requested block - header */
	int rsize = block->size - size - HEAD;

	/* resize block to requested size */
	block->size = rsize;

	/* Split is the unused free block */
	struct head *splt = after(block);
	splt->bsize = block->size;
	splt->bfree = block->free;

	/* resize new block to remaining size */
	splt->size = size;
	splt->free = TRUE;

	struct head *aft = after(splt);
	aft->bsize = splt->size;

	/* Return the free block */
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
	sentinel->bfree = new->free;
	sentinel->bsize = size;
	sentinel->free = FALSE;
	sentinel->size = 0;

	/* this is the only arena we have */
	arena = (struct head *)new;
	return new;
}

struct head *flist;

/* Detach a block from the flist */
void detach(struct head *block)
{
	/**
   * If there is a block after selected
   * Goes to "main blocks" next block and sets that blocks prev to point to the block before "main block"
   */
	if (block->next != NULL)
	{
		block->next->prev = block->prev;
	}
	/*
  * If there is a block before selected
  * Goes to "main blocks" prev block and sets that blocks next to point to the block after "main block" 
  */
	if (block->prev != NULL)
	{
		block->prev->next = block->next;
	}
	/* Set flist to the selected blocks next block, skipping the selected block */
	else
	{
		flist = block->next;
	}
}

/* Inserts the block to the beginning of flist */
void insert(struct head *block)
{
	/* Point new block to first in flist */
	block->next = flist;
	block->prev = NULL;

	/* If there is a block in the flist, set existing blocks prev to new first block */
	if (flist != NULL)
	{
		flist->prev = block;
	}

	/* Set flist to now point at the new inserted first block */
	flist = block;
}

/**
 * Determine a suitable size that is an even multiple of ALIGN and not
 * smaller than the minimum size.
 */
int adjust(int requested)
{
	int min = MIN(requested);
	int rem = min % ALIGN;
	if (rem == 0)
	{
		return requested;
	}
	else
	{
		return min + ALIGN - rem;
	}
}

/**
* Tries to find a block of the given size
* and if block is bigger check if it is possible to split.
*/
struct head *find(int size)
{
	struct head *checkBlock = flist;
	while (checkBlock != NULL)
	{
		if (checkBlock->size >= size)
		{
			detach(checkBlock);
			/**
       * If the block is bigger than the requested size and
       * has enough space to split, split it.
       */
			if (checkBlock->size > LIMIT(size))
			{
				/* Splits the block and gets pointer to free block */
				struct head *splt = split(checkBlock, size);

				/* Get pointer to block to be used */
				struct head *block = before(splt);

				/* Put back the free block to flist */
				insert(splt);

				/**
         * Get pointer to block after "main block" and
         * set its bfree to FALSE because we now use "main block" 
         */
				struct head *aft = after(block);
				aft->bfree = FALSE;

				return block;
			}
			/* Return the whole block if it is not possible to split it */
			else
			{
				checkBlock->free = FALSE;
				struct head *aft = after(checkBlock);
				aft->bfree = FALSE;
				return checkBlock;
			}
		}
		/* Move to next block */
		else
		{
			checkBlock = checkBlock->next;
		}
	}

	/* No Block Found */
	return NULL;
}

void *dalloc(size_t request)
{

	if (request <= 0)
	{
		return NULL;
	}

	int size = adjust(request);
	struct head *taken = find(size);

	if (taken == NULL)
	{
		return NULL;
	}
	else
	{
		return HIDE(taken);
	}
}

void dfree(void *memory)
{
	if (memory != NULL)
	{
		struct head *block = MAGIC(memory);
		struct head *aft = after(block);
		block->free = TRUE;
		aft->bfree = block->free;
		insert(block);
	}

	return;
}
