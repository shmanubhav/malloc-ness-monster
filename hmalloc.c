//Used node_t from lecture slides.
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "hmalloc.h"

/*
  typedef struct hm_stats {
  long pages_mapped;
  long pages_unmapped;
  long chunks_allocated;
  long chunks_freed;
  long free_length;
  } hm_stats;
*/

pthread_mutex_t mutex;

typedef struct node_t
{
    size_t size;
    struct node_t* next;  
} node;

typedef struct header_t
{
    long size;
} header;

const size_t PAGE_SIZE = 4096;
static hm_stats stats; // This initializes the stats to 0.
node* head = NULL;

void coalesce() {
  pthread_mutex_lock(&mutex);
  node* temp = head;
  while (temp != NULL && temp->next != NULL) {
    node* temp2 = temp->next;
    void* ptr1 = (void*)temp;
    void* ptr2 = (void*)temp2;
    if ((ptr1 + sizeof(node) + temp->size) == ptr2) {
      temp->size = temp->size + sizeof(node) + temp2->size;
      temp->next = temp2->next;
      continue;
    }
    temp = temp->next;
  }
  pthread_mutex_unlock(&mutex);
}

long
free_list_length()
{
  coalesce();
    long count = 0;
    node* tempNode = head;
    while (tempNode != NULL) {
      count += 1;
      tempNode = tempNode->next;
    }
    return count;
}

hm_stats*
hgetstats()
{
    stats.free_length = free_list_length();
    return &stats;
}

void
hprintstats()
{
    stats.free_length = free_list_length();
    fprintf(stderr, "\n== husky malloc stats ==\n");
    fprintf(stderr, "Mapped:   %ld\n", stats.pages_mapped);
    fprintf(stderr, "Unmapped: %ld\n", stats.pages_unmapped);
    fprintf(stderr, "Allocs:   %ld\n", stats.chunks_allocated);
    fprintf(stderr, "Frees:    %ld\n", stats.chunks_freed);
    fprintf(stderr, "Freelen:  %ld\n", stats.free_length);
}

static
size_t
div_up(size_t xx, size_t yy)
{
    // This is useful to calculate # of pages
    // for large allocations.
    size_t zz = xx / yy;

    if (zz * yy == xx) {
        return zz;
    }
    else {
        return zz + 1;
    }
}

void*
hmalloc(size_t size)
{
    stats.chunks_allocated += 1;
    size += sizeof(size_t);
    void* ptr = NULL;

    if (size < (PAGE_SIZE - sizeof(node))) {
      if (head == NULL) {
        head = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
        head->size = PAGE_SIZE - sizeof(node);
        head->next = NULL;
        stats.pages_mapped += 1;
        pthread_mutex_init(&mutex, 0);
      }

      pthread_mutex_lock(&mutex);
      node* tempNode = head;
      while (tempNode != NULL) {
        if (tempNode == head) {
          if (tempNode->size > size) {
            header* mhead = (header*)tempNode;
            ptr = (void*)mhead;
            node* newNode = (node*)(ptr + size);
            ptr += sizeof(size_t);
            newNode->next = head->next;
            newNode->size = tempNode->size - size;
            head = newNode;
            if (newNode->next == NULL) {
              head->next = NULL;
            }
            mhead->size = size;
            break;
          }
          if ((tempNode->size <= size) && (tempNode->next == NULL)) {
            node* newPage = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
            newPage->size = PAGE_SIZE - sizeof(node);
            newPage->next = NULL;
            stats.pages_mapped += 1;
            if (((void*)newPage) < ((void*)head)) {
              newPage->next = head;
              head = newPage;
              tempNode = head;
              continue;
            } else {
              tempNode->next = newPage;
            }
          }
        } 
        else {
          if (tempNode-> next != NULL) {
            node* temp2 = tempNode->next;
            if (temp2->size > size) {
              header* mhead = (header*)temp2;
              ptr = (void*)temp2;
              node* newNode = (node*)(ptr + size);
              newNode->size = temp2->size - size;
              mhead->size = size;
              ptr += sizeof(size_t);
              newNode->next = temp2->next;
              tempNode->next = newNode;
              break;
            }
          } else {
            node* newPage = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
            newPage->size = PAGE_SIZE - sizeof(node);
            newPage->next = NULL;
            stats.pages_mapped += 1;
            if (((void*)newPage) < ((void*)head)) {
              newPage->next = head;
              head = newPage;
              tempNode = head;
              continue;
            } else {
              tempNode->next = newPage;
            }
          }
        }
        tempNode = tempNode->next;
      }
    } else {
      size_t noPages = div_up(size, PAGE_SIZE);
      ptr = mmap(NULL ,(noPages * PAGE_SIZE), PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
      header* mhead = (header*)ptr;
      mhead->size = noPages * PAGE_SIZE;
      ptr += sizeof(size_t);
      stats.pages_mapped += div_up(size, PAGE_SIZE);
    }

    pthread_mutex_unlock(&mutex);
    return ptr;
}

void
hfree(void* item)
{
    stats.chunks_freed += 1;

    item -= sizeof(size_t);
    header* mhead = (header*)item;

    if (mhead->size < PAGE_SIZE) {
      node* newNode = (node*)mhead;
      newNode->size = mhead->size - sizeof(node);

      pthread_mutex_lock(&mutex);
      node* temp = head;
      while(temp != NULL) {
        if ((temp < newNode) && (temp->next > newNode)) {
          newNode->next = temp->next;
          temp->next = newNode;
          break;
        }
        temp = temp->next;
      }

      //if first block is cleared
      temp = head;
      if (newNode < temp) {
        newNode->next = temp;
        head = newNode;
      }
      pthread_mutex_unlock(&mutex);
      coalesce();
    } else {
      int numPages = div_up(mhead->size, PAGE_SIZE);
      long size = numPages * PAGE_SIZE;
      munmap((void*)mhead, size);
      stats.pages_unmapped += numPages;
    }
}

void*
hrealloc(void* prev, size_t bytes) {
  // pthread_mutex_lock(&mutex);
  // void* ptr = hmalloc(bytes);
  // void* headerCount = prev - sizeof(size_t);
  // header* mhead = (header*)headerCount;
  // size_t prevSize = mhead->size;
  // memcpy(ptr, prev, prevSize);
  // hfree(prev);
  // pthread_mutex_unlock(&mutex);
  // return ptr;
  void* ptr = hmalloc(bytes);
  pthread_mutex_lock(&mutex);
  void* headerCount = prev - sizeof(size_t);
  header* mhead = (header*)headerCount;
  size_t prevSize = mhead->size;
  memcpy(ptr, prev, prevSize);
  pthread_mutex_unlock(&mutex);
  hfree(prev);
  return ptr;
}
