#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define MEMORY 100 // size of memory

int memory[MEMORY]; // the memory

typedef struct array
{
  int size;           // the size of the array
  int *segment;       // pointer to the allocated memory
  struct array *next; // pointer to next array
} array;

/* The linked list of allocated arrays alwyas have two elements. The
   first one a the dummy element of size 1, its segment pointer is
   pointing to one position before the real memory. The last segment is
   a sentinel that always has the size zero. 
   Having these two element makes it easier to work with the list. We
   will never add something before the dummy and never after the
   sentinel nor will we ever remove the first or last element in the list. 
   The elements in the list will be ordered based on the value of the
   segment pointer. 
*/

array dummy = {1, &memory[-1], &sentinel};
array sentinel = {0, &memory[MEMORY], NULL};

array *allocated = &dummy;

/* A procedure that gives us some information about which segments that
   we now have allocated. Very handy when it it time for debugging. 
*/

void check()
{
  array *nxt = allocated;
  while (nxt != NULL)
  {
    printf("array (%p) : size %2d, segment %3d, next %14p \n",
           nxt,
           nxt->size,
           (int)(nxt->segment - memory),
           nxt->next);
    nxt = nxt->next;
  }
  return;
}

/* Allocate a new array with a segment of a given size. The procedure
   will go through the list of allocated arrays and search for two
   qarrays after each other were we can squeze in the requested segment.
   Remember that the array are ordered. If we look at one array that
   has a segment pointer to 0x1200 and a size of 0x20, and the next
   array starts on 0x1260 then we know that we have 0x40 free space
   that could possibly be used. 
*/
array *allocate(int size)
{
  array *nxt = allocated;

  // until we find the sentinel
  while (nxt->size != 0)
  {
    if ((nxt->next->segment - (nxt->segment + nxt->size)) >= size)
    {
      // space available for a size large segment

      // we need an array struct and let's use malloc
      array *new = (array *)malloc(sizeof(array));

      // initialize the new array segment
      new->size = size;
      new->segment = (nxt->segment + nxt->size);
      new->next = nxt->next;
      nxt->next = new;

      // return the allocated array
      return new;
    }
    nxt = nxt->next;
  }
  // no space is found, return NULL
  return NULL;
}

array *create(int size)
{
  printf("create an array of size %4d\n", size);
  array *new = allocate(size);
  /*
  if(new == NULL) {
    printf(".. almost panic, time for gc\n");    
    compact();
    check();
    new = allocate(size);
  }

  printf(".. pray for the best ..");    
  */
  if (new == NULL)
  {
    printf("panic!  memory full\n");
    exit(-1);
  }
  printf("done\n");
  return new;
}

void delete (array *arr)
{

  printf("delete array (%p) of size %4d", arr, arr->size);
  array *nxt = allocated;
  array *prev = NULL;
  while (nxt != arr)
  {
    prev = nxt;
    nxt = nxt->next;
  }
  prev->next = arr->next;
  free(arr);
  printf("done\n");
  return;
}

void set(array *arr, int pos, int val)
{
  /*
  if(pos > arr->size || pos =< 0) {
    printf("Segmentation fault: out of bounds\n");
    exit(-2);
  }
  */
  arr->segment[pos - 1] = val;
}

int get(array *arr, int pos)
{
  /*
  if(pos > arr->size || pos =< 0 ) {
    printf("Segmentation fault: out of bounds\n");
    exit(-2);
  }
  */
  return arr->segment[pos - 1];
}

void compact()
{

  array *prev = allocated;
  array *nxt = prev->next;

  while (nxt->size != 0)
  {
    for (int i = 0; i < nxt->size; i++)
    {
      prev->segment[prev->size + i] = nxt->segment[i];
    }
    nxt->segment = &prev->segment[prev->size];
    prev = nxt;
    nxt = nxt->next;
  }
}

void bench1()
{

  array *a = create(20);
  array *b = create(30);

  set(a, 10, 110);
  set(a, 14, 114);

  set(b, 8, 208);
  set(b, 12, 212);

  printf(" a[10] + a[14] = %d\n", get(a, 10) + get(a, 14));
  printf(" b[8] + b[12] = %d\n", get(b, 8) + get(b, 12));

  delete (a);
  delete (b);
}

void bench2()
{

  array *a = create(20);
  array *b = create(30);
  array *c = create(30);
  check();
  delete (b);
  check();
  array *d = create(20);
  check();
  delete (a);
  delete (c);
  delete (d);
}

void bench3()
{

  array *a = create(20);
  array *b = create(30);

  set(a, 22, 100);
  set(b, 0, 200);

  printf("a[20] + b[2] = %d\n", get(a, 20) + get(b, 2));
}

void bench4()
{

  array *a = create(20);
  array *b = create(30);
  array *c = create(30);
  delete (b);
  check();
  array *d = create(50);

  delete (a);
  delete (c);
  delete (d);
}

int main()
{
  bench2();
  return 0;
}