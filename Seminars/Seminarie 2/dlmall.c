#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <errno.h>
#include <math.h>
#include "dlmall.h"

#define TRUE 1
#define FALSE 0
#define HEAD (sizeof(struct head))
#define MIN(size) (((size) > (8)) ? (size) : (8))
#define LIMIT(size) (MIN(0) + HEAD + size)
#define MAGIC(memory) ((struct head *)memory - 1)
#define HIDE(block) (void *)((struct head *)block + 1)
#define ALIGN 8
#define ARENA (64 * 1024)

/* The linked list */
struct head
{
	uint16_t bfree;		 // 2 bytes, the status of block before
	uint16_t bsize;		 // 2 bytes, the size of block before
	uint16_t free;		 // 2 bytes, the status of the block
	uint16_t size;		 // 2 bytes, the size (max 2^16 i.e. 64 Ki byte )
	struct head *next; // 8 bytespointer
	struct head *prev; // 8 bytespointer
};

// Points the block after
struct head *after(struct head *block)
{
	return (struct head *)((char *)block + HEAD + block->size); // take the current pointer, cast it to a character pointer then add the size of the block plus the size of a header.
}
// Points to the block before
struct head *before(struct head *block)
{
	return (struct head *)((char *)block - block->bsize - HEAD); // same as before but now bsize bc before size and -HEAD bc previous block is before
}
// Splits a block into two and puts the pointer on the second block
struct head *split(struct head *block, int size)
{
	// Calculate the remaining size
	int rsize = block->size - size - HEAD;
	block->size = rsize;

	// Fix the pointers
	struct head *splt = after(block);
	splt->bsize = block->size;
	splt->bfree = block->free;
	splt->size = size;
	splt->free = FALSE;
	struct head *aft = after(splt);
	aft->bsize = splt->size;
	return splt;
}

struct head *arena = NULL;

// Creates a new arena
struct head *new ()
{
	if (arena != NULL)
	{
		printf("one arena already allocated \n");
		return NULL;
	}

	// using mmap, but could have used sbrk
	struct head *new = mmap(NULL, ARENA, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (new == MAP_FAILED)
	{
		printf("mmap failed: error %d\n", errno);
		return NULL;
	}

	/* make room for head and dummy */
	// head
	uint size = ARENA - 2 * HEAD;
	new->bfree = FALSE;
	new->bsize = 0;
	new->free = TRUE;
	new->size = size;
	// back

	struct head *sentinel = after(new);
	/* only touch the status fields */
	sentinel->bfree = new->free;
	sentinel->bsize = new->size;
	sentinel->free = FALSE;
	sentinel->size = 0;

	/* this is the only arena we have */
	arena = (struct head *)new;
	return new;
}

/* Free-List */
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
		/* Change the references */
		block->next->prev = block->prev;
	}
	/*
	 * If there is a block before selected
	 * Goes to "main blocks" prev block and sets that blocks next to point to the block after "main block"
	 */
	if (block->prev != NULL)
	{
		/* Change the references */
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
int adjust(size_t requested)
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
 * Go through freelist and find the first block which is large enough to meet our request. If there is no freelist create one.
 * If the size of the block found is large enough that it can be split into two, do so.
 * Mark the block as taken, and update the block after the taken block
 * Return a pointer to the start of the block.
 */
struct head *find(size_t size)
{

	if (arena == NULL)
	{
		flist = new ();
	}

	struct head *traverse = flist;
	struct head *aft;
	while (traverse != NULL)
	{
		/* If a block is big */
		if (traverse->size >= size)
		{
			detach(traverse);
			/**
			 * If the block is bigger than the requested size and
			 * has enough space to split, split it.
			 */
			if (traverse->size >= LIMIT(size))
			{
				/* Splits the block and gets pointer to free block */
				struct head *splt = split(traverse, size);

				/* Put back the free block to flist */
				insert(traverse);

				/**
				 * Get pointer to block after "main block" and
				 * set its bfree to FALSE because we now use "main block"
				 */
				aft = after(splt);
				aft->bfree = FALSE;
				aft->free = FALSE;

				return splt;
			}
			/* Return the whole block if it is not possible to split it */
			else
			{
				traverse->free = FALSE;
				aft = after(traverse);
				aft->bfree = FALSE;
				return traverse;
			}
		}
		/* Move to next block */
		else
		{
			traverse = traverse->next;
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

struct head *merge(struct head *block)
{
	struct head *aft = after(block);
	if (block->bfree)
	{
		/* unlink the block before */
		struct head *bef = before(block);
		detach(bef);

		/* calculate and set the total size of the merged blocks */
		bef->size = bef->size + block->size + HEAD;

		/* update the block after the merged blocks */
		aft->bsize = bef->size;

		/* continue with the merged block */
		block = bef;
	}

	if (aft->free)
	{
		/* unlink the block */
		detach(aft);

		/* calculate and set the total size of merged blocks */
		block->size = block->size + aft->size + HEAD;

		/* update the block after the merged block */
		aft = after(block);
		aft->bsize = block->size;
	}

	return block;
}

/* Free something from the heap */
void dfree(void *memory)
{
	if (memory != NULL)
	{
		struct head *block = MAGIC(memory);
		block = merge(block);
		struct head *aft = after(block);
		block->free = TRUE;
		aft->bfree = TRUE;
		insert(block);
	}
	return;
}

/* Checks the free-list */
void sanity()
{
	struct head *sanityCheck = flist;
	struct head *prevCheck = sanityCheck->prev;

	/* While the block size is bigger than zero and not the next aint aint null */
	while (sanityCheck->size != 0 && sanityCheck->next != NULL)
	{
		/* In free-list but not free? */
		if (sanityCheck->free != TRUE)
		{
			printf("Block at index in the list was found but was not free\n");
			exit(1);
		}

		/* Not a block that alligns */
		if (sanityCheck->size % ALIGN != 0)
		{
			printf("Block at index in the list had a size which does not align\n");
			exit(1);
		}

		/* Not a block that's too small */
		if (sanityCheck->size < MIN(sanityCheck->size))
		{
			printf("The size of the block at index in the list is smaller than the mininmum.\n");
			exit(1);
		}

		/* Reference error */
		if (sanityCheck->prev != prevCheck)
		{
			printf("Block at index in the list had a prev that didn't match with the previous block.\n");
			exit(1);
		}

		prevCheck = sanityCheck;
		sanityCheck = sanityCheck->next;
	}

	printf("No problems were found during the sanity check\n");
}

/* Checks the arena */
void traverse()
{
	struct head *before = arena;
	struct head *aft = after(before);

	while (aft->size != 0)
	{
		printf("Block: %p\n", before);
		printf("Size: %d\n", before->size);
		printf("Free: %d\n", before->free);
		printf("bfree: %d\n", before->bfree);
		printf("bsize: %d\n", before->bsize);
		printf("\n");

		// printf("%u\n", aft->size);
		if (aft->bsize != before->size)
		{
			printf("the size doesn't match!\n");
			exit(1);
		}

		if (aft->bfree != before->free)
		{
			printf("the status of free doesn't match!\n");
		}

		before = aft;
		aft = after(aft);
	}
	printf("No problems deteced.\n");
}

/* Initialises the arena then adds everything to free-List. This is the heap */
void initialize()
{
	struct head *first = new ();
	insert(first);
}

void block_sizes(int max)
{
	int sizes[max];
	struct head *block = flist;
	int i = 0;

	while (i < max)
	{
		if (block != NULL)
		{
			sizes[i] = (int)block->size;
			block = after(block);
			printf("%d\t", sizes[i]);
		}
		++i;
	}
	printf("\n");
}

int flist_size(unsigned int *size)
{
	int count = 0;
	*size = 0;
	struct head *block = flist;
	while (block != NULL)
	{
		++count;
		*size += block->size;
		block = block->next;
	}
	return count;
}
