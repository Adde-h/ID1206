#include <stdio.h>
#include "greenMutex.h"

int flag = 0;
green_cond_t cond;
green_mutex_t mutex;

void *test(void *arg)
{
	int id = *(int *)arg;
	int loop = 1000;
	while (loop > 0)
	{
		printf("thread %d: %d\n", id, loop);
		green_mutex_lock(&mutex);
		
		while (flag != id)
		{
			green_mutex_unlock(&mutex);
			green_cond_wait(&cond);
			green_mutex_lock(&mutex);
		}
		flag = (id + 1) % 2;
		green_cond_signal(&cond);
		green_mutex_unlock(&mutex);
		loop--;
	}
}

int main()
{

	green_t g0, g1;
	int a0 = 0;
	int a1 = 1;
	green_create(&g0, test, &a0);
	green_create(&g1, test, &a1);
	green_join(&g0, NULL);
	green_join(&g1, NULL);
	printf("done\n");
	return 0;
}