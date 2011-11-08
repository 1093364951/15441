#include "LSA.h"

LSA *newLSA(uint32_t senderID, uint32_t seqNo)
{
    LSA *newObj = malloc(sizeof(LSA));
    newObj->version = 1;
    newObj->TTL = 32;
    newObj->senderID = senderID;
    newObj->seqNo = seqNo;
    return newObj;

}

LSA *LSAfromBuffer(char *buf, ssize_t length)
{
    int i;
    uint8_t version, TTL;
    uint16_t type;
    uint32_t senderID, seqNo, numLink, numObj;
    version = *(uint8_t *)buf;
    TTL = ntohs(*(uint8_t *)(buf + 1));
    type = ntohs(*(uint16_t *)(buf + 2));

    senderID = ntohl(*(uint32_t *)(buf + 4));
    seqNo = ntohl(*(uint32_t *)(buf + 8));
    numLink = ntohl(*(uint32_t *)(buf + 12));
    numObj = ntohl(*(uint32_t *)(buf + 16));

    LSA *newObj = malloc(sizeof(LSA));
    newObj->version = version;
    newObj->TTL = TTL;
    newObj->type = type;
    newObj->senderID = senderID;
    newObj->seqNo = seqNo;
    newObj->numLink = numLink;
    newObj->numObj = numObj;
    /* Get node ID */
    newObj->listLink = malloc(sizeof(uint32_t) * numLink);
    for(i = 0; i < numLink; i++) {
        listLink[i] = ntohl(*(uint32_t *)(buf + 20 + sizeof(uint32_t) * i));
    }
    /* Get object list */
    newObj->listObj = malloc(sizeof(char *) * numObj);
    buf = buf + 20 + sizeof(uint32_t) * i;
    for(i = 0; i < numObj; i++) {
        listObj[i] = malloc(strlen(buf) + 1);
        strcpy(listObj[i], buf);
        while(buf != '\0') {
            buf++;
        }
        buf++;
    }
    return newObj;
}

void LSAtoBuffer(LSA *lsa, char **buf, int *bufSize)
{
    int size = 20 + lsa->numLink * sizeof(uint32_t);
    int i;
    char *ptr;
    char *buffer = malloc(size);
    *buf = NULL;
    *bufSize = 0;
    memcpy(buffer, &(lsa->version), 1);
    memcpy(buffer + 1, &(lsa->TTL), 1);
    memcpy(buffer + 2, &htons(lsa->type), 2);
    memcpy(buffer + 4, &htonl(lsa->senderID), 4);
    memcpy(buffer + 8, &htonl(lsa->seqNo), 4);
    memcpy(buffer + 12, &htonl(lsa->numLink), 4);
    memcpy(buffer + 16, &htonl(lsa->numObj), 4);
    ptr = buffer + 20;
    for(i = 0; i < lsa->numLink; i++) {
        memcpy(ptr + i * 4, &htonl(lsa->listLink[i]), 4);
    }
    ptr += i * 4;
    for(i = 0; i < lsa->numObj; i++) {
        char *objPtr = lsa->listObj[i];
        size = strlen(objPtr) + 1;
        buffer = realloc(buffer, ptr - buffer + size);
        strcpy(ptr, objPtr);
        ptr += size;
    }
    *buf = buffer;
    *bufSize = ptr - buffer;
}
