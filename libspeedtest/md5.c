#include "md5.h"

char *md5(char *string, size_t length) {
    MD5_CTX c;

    unsigned char *tmp = (unsigned char*)malloc(MD5_DIGEST_LENGTH);
    char *out = (char*)malloc(50);
    char *head = out;

    if (NULL == tmp || out == NULL) {
        fprintf(stderr, "error no memory");
        exit(1);
    }

    MD5_Init(&c);
    MD5_Update(&c, string, length);
    MD5_Final(tmp, &c);

    int m = MD5_DIGEST_LENGTH;
    for (int i=0; i<MD5_DIGEST_LENGTH; i++) {
        sprintf(out, "%02x", tmp[i]);
        out = out +2;
    }
    head[33] = '\0';

    free(tmp);
    return head;
}
