#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "ring_buffer.h"

const int SIMPLE_BUFFER_SIZE = 5;
const int RING_BUFFER_SIZE = 4;

/* function declaration */

int main()
{
    // create simple buffer
    char simple_buffer[SIMPLE_BUFFER_SIZE];

    // create our buffer and a handler pointer to facilitate operations
    char *buffer  = malloc(RING_BUFFER_SIZE * sizeof(char));
    cbuf_handle_t cbuf = circular_buf_init(buffer, RING_BUFFER_SIZE);

    // fill circular buffer with data from simple buffer

	circular_buf_put(cbuf, 'a');
	circular_buf_put(cbuf, 'b');
	circular_buf_put(cbuf, 'c');
	circular_buf_put(cbuf, 'd');
	circular_buf_put(cbuf, 'e');

    printf("\n******\nReading back values: ");
	while(!circular_buf_empty(cbuf))
	{
		char data;
		circular_buf_get(cbuf, &data);
		printf("%c ", data);
	}

    free(buffer);
	circular_buf_free(cbuf);

    return 0;
}