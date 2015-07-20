

#ifndef CSPEEDTESTCLION_UTIL_H
#define CSPEEDTESTCLION_UTIL_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct MemoryChunk {
    char *memory;
    size_t size;
    size_t allocated;
} MemoryChunk;

size_t WriteNuLLCallback(void *ptr, size_t size, size_t nmemb, void *userp);

size_t WriteCallback(void *ptr, size_t size, size_t nmemb, void *userp);

char * getRandomData(size_t bytes);

#endif //CSPEEDTESTCLION_UTIL_H
