
#include "util.h"

size_t WriteNuLLCallback(void *ptr, size_t size, size_t nmemb, void *userp) {
    /* we are not interested in the downloaded bytes itself,
     so we only return the size we would have saved ... */
    (void)ptr;  /* unused */
    (void)userp; /* unused */
    return (size_t)(size * nmemb);
}

size_t WriteCallback(void *ptr, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryChunk *mem = (struct MemoryChunk *)userp;

    size_t c = mem->size + realsize + 1;

    if (c > mem->allocated) {
        size_t newAllocatedValue = c+1024;
        char* newpointer  = (char*)realloc(mem->memory, newAllocatedValue);
        if(newpointer == NULL) {
            /* out of memory! */
            free(mem->memory);
            mem->allocated = 0;
            mem->size = 0;
            printf("not enough memory (realloc returned NULL)\n");
            exit(1);
        } else {
            mem->memory = newpointer;
            mem->allocated = newAllocatedValue;
        }
    }


    memcpy(&(mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

char* getRandomData(size_t bytes) {
    char* ptr = (char*)malloc(bytes+1);

    if (NULL == ptr) {
        return NULL;
    }

    FILE* fp = fopen("/dev/urandom", "r");
    if (fp == NULL) {
        return NULL;
    }

    size_t readed = fread(ptr, 1, bytes, fp);
    fclose(fp);

    return ptr;
}