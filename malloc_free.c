// includes
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include "goatmalloc.h"

// globals
void* _arena_start;  // address of start of arena
void* _arena_end;  // address of end of arena
size_t global_size;  // size of arena, used for destroy()
int statusno;  // basically just error value for tests

// functions
// WORKING
int init(size_t size)
{  // initialize memory allocator
	int R;
	global_size = size;
	int page_size = getpagesize();  // page size in bytes
	R = page_size;

	if((int) size < 0)
	{  // if invalid size
		R = ERR_BAD_ARGUMENTS;
	} else
	{
		while(size > page_size)
		{
			R += page_size;
			size -= page_size;
		}
		// starter code
		int fd = open("/dev/zero", O_RDWR);  // fills memory with null bytes
		_arena_start = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
		_arena_end = _arena_start + size;
		// set first header
		((node_t*) _arena_start)->is_free = 1;
		((node_t*) _arena_start)->size = (size - sizeof(node_t));
	}
	
	return R;
}

// WORKING
int destroy()
{  // return arena's memory to the os and reset all internal state variables
	int R = 0;
	int unmap_value;
	
	if(_arena_start == NULL)
	{  // if arena uninitialized
		R = ERR_UNINITIALIZED;
	} else
	{
		unmap_value = munmap(NULL, global_size);
		if(unmap_value == -1)
		{  // if unmap encounters error
			R = ERR_UNINITIALIZED;
		}
	}
	
	return R;
}

// WORKING
void* walloc(size_t size)
{  // request new memory allocation of [size] bytes 
	void* R = 0;
	int out_of_memory = 1;  // set this to zero when space is found
	node_t* temp = (node_t*) _arena_start;  // temp header used to iterate through chunk list
	node_t* prev = NULL;  // store previous header in chunk list
	
	if(_arena_start == NULL)
	{ // if arena uninitialized
		statusno = ERR_UNINITIALIZED;
	} else
	{  // if arena initialized
		while(temp != NULL)
		{  // while there is another header
			if((temp->is_free == 1) && (temp->size >= size))
			{  // if current chunk is free and can hold new chunk	
				temp->is_free = 0;
				temp->bwd = prev;
				R = ((void*) temp + sizeof(node_t));
				if((temp->size - size) > sizeof(node_t))
				{  // if still room to for another chunk after this one
					// change next header values
					temp->fwd = (R + size);
					temp->fwd->bwd = temp;
					temp->fwd->is_free = 1;
					temp->fwd->size = (temp->size - size - sizeof(node_t));
					// change curr header values
					temp->size = size;
				} else if(_arena_end == ((void*) temp + sizeof(node_t) + temp->size))
				{  // if this is the last free chunk
					temp->fwd = NULL;
				} else
				{  // not end of arena
					temp->fwd = (R + temp->size);
				}
				out_of_memory = 0;  // found a place to put new chunk
				break;
			}
			prev = temp;
			temp = temp->fwd;
		}
	
		if(out_of_memory)  // put at end
		{
			statusno = ERR_OUT_OF_MEMORY;
		}
	}

	return R;
}

// WORKING
void wfree(void* ptr)
{  // free up existing memory chunks
	node_t* free_header = (node_t*) (ptr - sizeof(node_t));  // temp header used to iterate through chunk list
	
	free_header->is_free = 1;
	while(free_header->bwd != NULL)
	{  // makes free header the first of the adjacent free chunks
		if(free_header->bwd->is_free)
		{
			free_header = free_header->bwd;
		} else
		{
			break;
		}
	}
	
	while(free_header->fwd != NULL)
	{  // while next chunk is freeS
		if(free_header->fwd->is_free)
		{
			free_header->size += (free_header->fwd->size + sizeof(node_t));
			free_header->fwd = free_header->fwd->fwd;
		} else
		{
			break;
		}
	}
	return;
}
