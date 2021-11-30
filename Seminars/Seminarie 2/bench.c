#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "rand.h"
#include "dlmall.c"

/**
 * NOT FULLY FUNCTIONAL
 * 
 */


#define ROUNDS 1000
#define LOOP 100000
#define BUFFER 100
#define SIZES 100

int frequency[SIZES];

int cmp(const void *a, const void *b){
	return *(int*)a - *(int*)b;
}

void bench(int alloc, char *name){
	/* initiatlizing arena */
	initialize();

	void *buffer[BUFFER];
	for(int i = 0; i < BUFFER; i++){
		buffer[i] = NULL; // NULL all buffer positions
	}
	
	for(int i = 0; i < alloc; i++){
		int index = rand() % BUFFER;
		if(buffer[index] != NULL) {
			dfree(buffer[index]);
			buffer[index] = NULL;
		} else {
			size_t size = (size_t)request();
			int *memory;
			memory = dalloc(size);
			
			buffer[index] = memory;
			/* writing to the memory so we know it exists */
			*memory = 123;
		}
		
		FILE *file = fopen(name, "w");
		
		sizes(frequency, SIZES);
		int length = freelistlength();
		qsort(frequency, length, sizeof(int), cmp);
		
		for(int i = 0; i < length; i++){
			fprintf(file, "%d\n", frequency[i]);
		}
		
		fclose(file);
		
		printf("Length of the free list: %d\n", length);
	}
} 

/* argc = argument count     argv = argument vector */
int main(int argc, char *argv[])
{
	if(argc < 3){
		printf("number of allocations as arg\n");
	}
	
	int first = atoi(argv[1]);
	char *name = argv[2];
	
	bench(first,name);
	
	return 0;
}
