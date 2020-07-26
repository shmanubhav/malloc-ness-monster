#ifndef OPTMALLOC_H
#define OPTMALLOC_H

void* opt_malloc(size_t size);
void opt_free(void* item);
void* opt_realloc(void* prev, size_t bytes);

#endif