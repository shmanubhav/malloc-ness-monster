#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "optmalloc.h"

//0 is false
//1 is true

typedef struct node_t {
	int free;
	size_t size;
	struct node_t* next;
	struct node_t* prev;
} node;

typedef struct header_t {
	size_t size;
} header;

typedef struct footer_t {
	size_t size;
} footer;

__thread node* bins[7] = {NULL};
/*
void make_bins() {
	if (bins == NULL) {
		void* ptr = mmap(NULL, 4096*2, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
		node* nodePtr = (node*)ptr;
		nodePtr->size = 4096;
		nodePtr->free = 1;
		nodePtr->next = NULL;
		nodePtr->prev = NULL;
		bins[6] = nodePtr;
	}
}*/

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

int getBinNumber(size_t size) {
	if (size <= 32) {
		return 0;
	} else if (size <= 64) {
		return 1;
	} else if (size <= 128) {
		return 2;
	} else if (size <= 256) {
		return 3;
	} else if (size <= 512) {
		return 4;
	} else if (size <= 1024) {
		return 5;
	} else {
		return 6;
	}
}

node* getBin(int binNo,size_t size) {
	node* temp = bins[binNo];
	if (temp == NULL) {
		//its < 2048
		if (binNo != 6) {
			binNo += 1;
			return getBin(binNo, size);
		} else {
			void* ptr = mmap(NULL, (div_up(size, 4096) * 4096), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
			node* nodePtr = (node*)ptr;
			nodePtr->size = (div_up(size, 4096) * 4096);
			nodePtr->free = 1;
			nodePtr->next = NULL;
			nodePtr->prev = NULL;
			bins[6] = nodePtr;
			//return getBin(binNo, size);
		}
	}
	else{
		while (temp != NULL) {
			int originalBin = getBinNumber(size);
			//break block
			if (originalBin < binNo) {
				void* temp2 = NULL;
				temp2 = (void*)temp + (temp->size/2);
				node* newNode = (node*)temp2;
				newNode->size = (temp->size/2);
				newNode->free = 1;
				temp->size = newNode->size;
				node* tempNode = temp->next;
				if (tempNode != NULL) {
					tempNode->prev = newNode;
				}
				newNode->next = temp->next;
				temp->next = newNode;
				newNode->prev = temp;
				int newBinNum = getBinNumber(newNode->size);
				if (newBinNum != binNo) {
					//move bin to correct place
					if (temp == bins[binNo]) {
						bins[binNo] = newNode->next;
						if (newNode->next /*!= NULL*/) {
							newNode->next->prev = NULL;
						}
						if (bins[newBinNum] /*!= NULL*/) {
							bins[newBinNum]->prev = newNode;
						}
						newNode->next = bins[newBinNum];
						bins[newBinNum] = temp;
					} else {
					//if temp is later in the line
					}
				}
				return getBin(newBinNum, size);

			} else {
				//return current block
				return bins[binNo];

			}
			temp = temp->next;
		}
	}

	return getBin(binNo, size);


}

void* opt_malloc(size_t size) {
	size = size + sizeof(header) + sizeof(footer);
	int binNo = getBinNumber(size);
	node* nodePtr = getBin(binNo, size);
	bins[binNo] = nodePtr->next;
	if (bins[binNo] != NULL) {
		bins[binNo]->prev = NULL;
	}
	void* ptr = (void*)nodePtr;
	header* mhead = (header*)ptr;
	mhead->size = nodePtr->size;
	ptr = ptr + mhead->size - sizeof(footer);
	footer* mfoot = (footer*)ptr;
	//nodePtr->next->prev = NULL;

	mfoot->size = mhead->size;
	ptr = (void*)mhead + sizeof(header);
	return ptr;
	//return 0;
}

void opt_free(void* item) {
	void* ptr = item;
	ptr = ptr - sizeof(header);
	node* nodePtr = (node*)ptr;
	header* mhead = (header*)ptr;
	nodePtr->size = mhead->size;
	nodePtr->free = 1;
	int binNo = getBinNumber(nodePtr->size);
	node* temp = bins[binNo];
	if (temp != NULL) {
		temp->prev = nodePtr;
	}
	nodePtr->next = temp;
	nodePtr->prev = NULL;
	bins[binNo] = nodePtr;
}

void* opt_realloc(void* prev, size_t bytes) {
	void* newMem = opt_malloc(bytes);
	header* mhead = (header*)(prev - sizeof(header));
	size_t totSize = mhead->size - sizeof(header) - sizeof(footer);
	memcpy(newMem, prev, totSize);
	opt_free(prev);
	return newMem;
}
