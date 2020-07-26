

#include <stdlib.h>
#include <unistd.h>

#include "optmalloc.h"
//#include "par_malloc.h"


void*
xmalloc(size_t bytes)
{
    return opt_malloc(bytes);
    //return hmalloc(bytes);
    //return 0;
}

void
xfree(void* ptr)
{
    opt_free(ptr);
}

void*
xrealloc(void* prev, size_t bytes)
{
    return opt_realloc(prev, bytes);
    //return 0;
}

