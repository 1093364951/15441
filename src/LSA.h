#ifndef LSA_H
#define LSA_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>


typedef struct LSA {
    uint8_t version;
    uint8_t TTL;
    uint16_t type;
    uint32_t senderID;
    uint32_t seqNo;
    uint32_t numLink;
    uint32_t numObj;
    uint32_t *listLink;
    char **listObj;
} LSA;

/* Constructor */
LSA *LSAfromBuffer(char *, ssize_t);
LSA *LSAfromLSA(LSA *);
LSA *newLSA(uint32_t, uint32_t);

int compareLSA(void *data1, void *data2);
void freeLSA(void *data);
int mapLSA(void *data);

void LSAtoBuffer(LSA *, char **, int *);
/* Getters and Setters */


#endif
