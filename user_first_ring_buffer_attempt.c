#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

typedef struct circular_buffer
{
    void *buffer;     // data buffer
    void *buffer_end; // end of data buffer
    size_t capacity;  // maximum number of items in the buffer
    size_t count;     // number of items in the buffer
    size_t sz;        // size of each item in the buffer
    void *head;       // pointer to head
    void *tail;       // pointer to tail
} circular_buffer;

void cb_init(circular_buffer *cb, size_t capacity, size_t sz)
{
    if(cb != NULL){
        
        cb->buffer = malloc(capacity * sz);

        if(cb->buffer == NULL){
            printf("NULLL");
        }else{
            cb->buffer_end = (char *)cb->buffer + capacity * sz;
            cb->capacity = capacity;
            cb->count = 0;
            cb->sz = sz;
            cb->head = cb->buffer;
            cb->tail = cb->buffer;
        }
        
    }
}

void cb_free(circular_buffer *cb)
{
    free(cb->buffer);
    // clear out other fields too, just to be safe
}

void cb_push_back(circular_buffer *cb, const void *item)
{
    if(cb->count == cb->capacity){
        // handle error
    }
    memcpy(cb->head, item, cb->sz);
    cb->head = (char*)cb->head + cb->sz;

    if(cb->head == cb->buffer_end)
        cb->head = cb->buffer;

   

    cb->count++;
}

void cb_pop_front(circular_buffer *cb, void *item)
{
    if(cb->count == 0){
        // handle error
    }
    memcpy(item, cb->tail, cb->sz);
    cb->tail = (char*)cb->tail + cb->sz;
    if(cb->tail == cb->buffer_end)
        cb->tail = cb->buffer;
    cb->count--;
}

int main(void){

    struct circular_buffer *bf; 
    bf = (struct circular_buffer*) malloc(10 * sizeof(struct circular_buffer));
    cb_init(bf, 10, sizeof(int));

    int itemVal = 1;
    int *item = &itemVal;

    int itemVal2 = 2;
    int *item2 = &itemVal2;

    cb_push_back(bf, item2);
    cb_push_back(bf, item);
    

    int *nextItem;
    nextItem = bf->buffer;

    printf("%i", *((int *)nextItem));

    free(bf);    
    
    
    return 0;
}