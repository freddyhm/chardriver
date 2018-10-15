#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "ring_buffer.h"

const int SIMPLE_BUFFER_SIZE = 5;
const int RING_BUFFER_SIZE = 10;

/* function declaration */
void fill_simple_buffer(int *simple_buffer, int size);

int main()
{
    // create simple buffer
    int simple_buffer[SIMPLE_BUFFER_SIZE];

    // create our buffer and a handler pointer to facilitate operations
    uint8_t * buffer  = malloc(RING_BUFFER_SIZE * sizeof(uint8_t));
    cbuf_handle_t cbuf = circular_buf_init(buffer, RING_BUFFER_SIZE);

    fill_simple_buffer(simple_buffer, SIMPLE_BUFFER_SIZE);

    // fill circular buffer with data from simple buffer
	for(int i = 0; i < SIMPLE_BUFFER_SIZE; i++)
	{
		circular_buf_put(cbuf, simple_buffer[i]);
	}

    printf("\n******\nReading back values: ");
	while(!circular_buf_empty(cbuf))
	{
		uint8_t data;
		circular_buf_get(cbuf, &data);
		printf("%u ", data);
	}

    free(buffer);
	circular_buf_free(cbuf);

    return 0;
}


void fill_simple_buffer(int *simple_buffer, int size){

    for(int i = 0; i < size ; i++)
	{
		simple_buffer[i] = (i * 2);
	}
}

