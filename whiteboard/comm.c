#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "comm.h"
#include "context.h"

pthread_t thread_id;
bool hidden = false;

void* comm_thread_handler(void* arg)
{
	while (true)
	{
		char c;
		fread(&c, 1, 1, stdin);

		if (c == 'h')
		{
			hidden = !hidden;

			if (hidden)
				glfwHideWindow(window);
			else
				glfwShowWindow(window);
		}
		else if (c == 'q')
			glfwSetWindowShouldClose(window, true);

	}
	return NULL;
}

int start_comm_thread()
{
	return pthread_create(&thread_id, NULL, comm_thread_handler, NULL);
}
