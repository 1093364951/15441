#include "routingTable.h"



int compareRoutingEntry(void *data1, void *data2)
{
    routingEntry *re1 = (routingEntry *)data1;
    routingEntry *re2 = (routingEntry *)data2;
    return (re1->nodeID) - (re2->nodeID);
}

void freeRoutingEntry(void *data)
{
    routingEntry *re = (routingEntry *)data;
    free(re->host);
}

void newAdvertisement()
{
    LSA *newAds = NULL;
    struct timeval curTime;
    struct timeval *oldTime = tRouting->oldTime;
    gettimeofday(&curTime, NULL);
    routingEntry *myEntry = getMyRoutingEntry();
    long diffTime = curTime.tv_sec - oldTime->tv_sec;
    printf("DiffTime = %ld, cycleTime = %d\n", diffTime, tRouting->cycleTime);
    int needAdv = diffTime > tRouting->cycleTime;
    if(myEntry->lastLSA == NULL) {
        newAds = newLSA(myEntry->nodeID, 0);
        fillLSAWithLink(newAds);
        fillLSAWithObj(newAds, myEntry->tRes);
        myEntry->lastLSA = newAds;
        newAds = LSAfromLSA(myEntry->lastLSA);
        printLSA(newAds);
        addLSAWithDest(getLocalLSABuffer(), newAds, 0);
        updateTime();
    } else if(needAdv) {
        incLSASeq(myEntry->lastLSA);
        newAds = headerLSAfromLSA(myEntry->lastLSA);
        fillLSAWithLink(newAds);
        fillLSAWithObj(newAds, myEntry->tRes);
        replaceLSA(&(myEntry->lastLSA), newAds);
        addLSAWithDest(getLocalLSABuffer(), newAds, 0);
        updateTime();
    }
}

void addLSAWithDest(DLL *list, LSA *lsa, unsigned int ignoreID)
{
    DLL *table = tRouting->table;
    int i = 0;
    while(i < table->size) {
        routingEntry *entry = getNodeDataAt(table, i);
        if(entry->distance == 1 && entry->nodeID != ignoreID) {
            LSA *thisLSA = LSAfromLSA(lsa);
            setLSADest(thisLSA, entry->host, entry->routingPort);
            printf("LSA Dest: %s:%d\n", thisLSA->dest, thisLSA->destPort);
            insertNode(list, thisLSA);
        }
        i++;
    }
    freeLSA(lsa);
}
void addLSAWithOneDest(DLL *list, LSA *lsa, unsigned int destID)
{
    routingEntry *entry = getRoutingEntry(destID);
    setLSADest(lsa, entry->host, entry->routingPort);
    insertNode(list, lsa);
}

void fillLSAWithLink(LSA *lsa)
{
    DLL *table = tRouting->table;
    int i = 0;
    while(i < table->size) {
        routingEntry *entry = getNodeDataAt(table, i);
        if(entry->distance == 1) {
            insertLSALink(lsa, entry->nodeID);
        }
        i++;
    }
}

/* Called from connHandler */
void updateRoutingTableFromLSA(LSA *lsa)
{
    int isNew, isHigher, isSame, isLower, isZero, isMine;
    unsigned int nodeID = lsa->senderID;
    if(isLSAAck(lsa)) {
        //Update hasACK
        routingEntry *senderEntry = getRoutingEntry(lsa->senderID);
        if(!hasLSAAck(senderEntry->lastLSA)) {
            gotLSAAck(senderEntry->lastLSA);
            printf("LSA ACK checked in.\n");
        } else {
            printf("Got ACK for something we didn't send...\n");
        }
    } else {
        routingEntry *entry = getRoutingEntry(nodeID);
        routingEntry *myEntry = getMyRoutingEntry();
        isZero = (lsa->TTL == 0);
        isNew = (entry == NULL || entry->lastLSA == NULL) && !isZero;
        isHigher =  !isNew && !isZero && entry != NULL && entry->lastLSA->seqNo < lsa->seqNo;
        isSame =    !isNew && !isZero && entry != NULL && entry->lastLSA->seqNo == lsa->seqNo;
        isLower =   !isNew && !isZero && entry != NULL && entry->lastLSA->seqNo > lsa->seqNo;
        isMine = (entry != NULL &&  entry == myEntry);
        //Send back ACK (ANY)
        LSA *ack = headerLSAfromLSA(lsa);
        setLSAAck(ack);
        setLSADest(ack, lsa->src, lsa->srcPort);
        addLSAtoBuffer(ack);
        //Remove routing entry. Flood
        if(isZero) {
            printf("isZero\n");
            removeRoutingEntry(nodeID);
            LSA *floodLSA = LSAfromLSA(lsa);
            unsigned int lastNodeID = getLastNodeID(lsa);
            addLSAWithDest(getLocalLSABuffer(), floodLSA, lastNodeID);
        } else if(isMine && isHigher) {
            printf("isMine & higher\n");
            //I rebooted, catch up to higher seq. No flood.
            if(isNew) {
                myEntry->lastLSA = lsa;
            } else {
                myEntry->lastLSA->seqNo = lsa->seqNo;
            }
        } else if(isLower) {
            printf("isLower\n");
            //neiggbor rebooted. Echo back his last LSA. No Flood
            LSA *backLSA = LSAfromLSA(entry->lastLSA);
            addLSAWithOneDest(getLocalLSABuffer(), backLSA, backLSA->senderID);
        } else if(isNew) {
            //Make new routing entry. Flood
            printf("isNew\n");
            routingEntry *newEntry;
            if(entry == NULL) {
                newEntry = malloc(sizeof(routingEntry));
                newEntry->nodeID = nodeID;
                newEntry->isMe = 1;
                newEntry->host = NULL;
                newEntry->routingPort = 0;
                newEntry->localPort = 0;
                newEntry->serverPort = 0;
                newEntry->tRes = NULL;
                newEntry->distance = 2;
                insertNode(tRouting->table, newEntry);
            } else {
                newEntry = entry;
            }
            newEntry->lastLSA = lsa;
            LSA *floodLSA = LSAfromLSA(lsa);
            decLSATTL(floodLSA);
            if(getLSATTL(floodLSA) > 0) {
                addLSAWithDest(getLocalLSABuffer(), floodLSA, getLastNodeID(lsa));
            }
        } else if(isHigher) {
            printf("isHigher\n");
            //Update routing table. Flood
            replaceLSA(&(entry->lastLSA), lsa);
            //Flood if TTL > 0
            LSA *floodLSA = LSAfromLSA(lsa);
            decLSATTL(floodLSA);
            if(getLSATTL(floodLSA) > 0) {
                addLSAWithDest(getLocalLSABuffer(), floodLSA, getLastNodeID(floodLSA));
            }
        } else {
            printf("What kind of LSA is this?!\n");
        }
    }
}

void getLSAFromRoutingTable(DLL **list)
{
    /* Add Advertisement LSA */
    printf("check newAdvertisement\n");
    newAdvertisement();
    /* Timeout old LSA */
    printf("expire old LSA\n");
    expireOldLSA();
    /* Retran LSA without ACK */
    printf("check neighbor\n");
    checkNeighborDown();
    /* Add buffered LSA into list */
    DLL *localList = getLocalLSABuffer();
    *list = localList;
    /* Anything more to add? */
}

/* Routing Table */
unsigned int getLastNodeID(LSA *lsa)
{
    routingEntry *lastEntry = getRoutingEntryByHost(lsa->src, lsa->srcPort);
    return lastEntry->nodeID;
}
void removeRoutingEntry(unsigned int nodeID)
{
    routingTable *tRou = tRouting;
    int i = 0;
    for(i = 0; i < tRou->table->size; i++) {
        routingEntry *entry = getNodeDataAt(tRou->table, i);
        if(entry->nodeID == nodeID) {
            removeNodeAt(tRou->table, i);
        }
    }
}

void updateTime()
{
    gettimeofday(tRouting->oldTime, NULL);
}

int initRoutingTable(int nodeID, char *rouFile, char *resFile,
                     int cycleTime, int neighborTimeout,
                     int retranTimeout, int LSATimeout)
{
    tRouting = malloc(sizeof(routingTable));

    tRouting->oldTime = malloc(sizeof(struct timeval));
    updateTime();
    tRouting->cycleTime = cycleTime;
    tRouting->neighborTimeout = neighborTimeout;
    tRouting->retranTimeout = retranTimeout;
    tRouting->LSATimeout = LSATimeout;

    tRouting->LSAList = malloc(sizeof(DLL));
    initList(tRouting->LSAList, compareLSA, freeLSA, NULL, copyLSA);
    tRouting->table = malloc(sizeof(DLL));
    initList(tRouting->table,
             compareRoutingEntry, freeRoutingEntry, NULL, NULL);
    return loadRoutingTable(tRouting, nodeID, rouFile, resFile);
}

int loadRoutingTable(routingTable *tRou, unsigned int nodeID, char *rouFile, char *resFile)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    fp = fopen(rouFile, "r");
    if(fp == NULL) {
        printf("Error reading routing table config.\n");
        return -1;
    }
    while(getline(&line, &len, fp) != -1) {
        printf("configLine: %s", line);
        routingEntry *re = parseRoutingLine(line);
        if(re->nodeID == nodeID) {
            re->isMe = 1;
            re->distance = 0;
            initResourceTable(re->tRes, resFile);
        } else {
            initResourceTable(re->tRes, NULL);
        }
        insertNode(tRou->table, re);
    }

    if(line != NULL) {
        free(line);
    }
    fclose(fp);
    return 0;
}

routingEntry *parseRoutingLine(char *line)
{
    int numMatch;
    routingEntry *newObj;
    unsigned int nodeID;
    char *host = NULL;
    int routingPort;
    int localPort;
    int serverPort;

    numMatch = sscanf(line, "%u %ms %d %d %d",
                      &nodeID, &host, &routingPort, &localPort, &serverPort);
    if(numMatch != 5) {
        if(host != NULL) {
            free(host);
        }
        return NULL;
    }
    newObj = malloc(sizeof(routingEntry));
    newObj->nodeID = nodeID;
    newObj->isMe = 0;
    newObj->host = host;
    newObj->routingPort = routingPort;
    newObj->localPort = localPort;
    newObj->serverPort = serverPort;
    newObj->tRes = malloc(sizeof(resourceTable));
    newObj->distance = 1;
    newObj->lastLSA = NULL;
    return newObj;
}


int getRoutingInfo(char *objName, routingInfo *rInfo)
{
    routingTable *tRou = tRouting;
    int i = 0;
    int minDistance = INT_MAX;
    char *path;
    int found = 0;
    for(i = 0; i < tRou->table->size; i++) {
        routingEntry *entry = getNodeDataAt(tRou->table, i);
        path = getPathByName(entry->tRes, objName);
        if(path != NULL) {
            found = 1;
            if(minDistance > entry->distance) {
                rInfo->host = entry->host;
                rInfo->port = entry->serverPort;
                rInfo->path = path;
                minDistance = entry->distance;
            }
        }
    }
    return found;
}

routingEntry *getRoutingEntryByHost(char *host, int port)
{
    routingTable *tRou = tRouting;
    int i = 0;
    printf("Searching:%s:%d",host, port);
    for(i = 0; i < tRou->table->size; i++) {
        routingEntry *entry = getNodeDataAt(tRou->table, i);
        printf("Got:%s:%d;\n",entry->host, entry->routingPort);
        if(entry->host != NULL &&
                entry->routingPort == port &&
                strcmp(entry->host, host) == 0) {
            return entry;
        }
    }
    return NULL;
}

routingEntry *getRoutingEntry(unsigned int nodeID)
{
    routingTable *tRou = tRouting;
    int i = 0;
    for(i = 0; i < tRou->table->size; i++) {
        routingEntry *entry = getNodeDataAt(tRou->table, i);
        if(entry->nodeID == nodeID) {
            return entry;
        }
    }
    return NULL;
}

routingEntry *getMyRoutingEntry()
{
    routingTable *tRou = tRouting;
    int i = 0;
    for(i = 0; i < tRou->table->size; i++) {
        routingEntry *entry = getNodeDataAt(tRou->table, i);
        if(entry->isMe) {
            return entry;
        }
    }
    return NULL;
}

int getRoutingPort(unsigned int nodeID)
{
    routingEntry *entry = getRoutingEntry(nodeID);
    if(entry == NULL) {
        return -1;
    } else {
        return entry->routingPort;
    }
}

int getLocalPort(unsigned int nodeID)
{
    routingEntry *entry = getRoutingEntry(nodeID);
    if(entry == NULL) {
        return -1;
    } else {
        return entry->localPort;
    }
}

DLL *getLocalLSABuffer()
{
    return tRouting->LSAList;
}

void insertLocalResource(char *objName, char *objPath)
{
    routingEntry *entry = getMyRoutingEntry();
    insertResource(entry->tRes, objName, objPath);
}

void addLSAtoBuffer(LSA *lsa)
{
    DLL *list = getLocalLSABuffer();
    insertNode(list, lsa);
}

void expireOldLSA()
{
    DLL *table = tRouting->table;
    routingEntry *entry;
    int i;
    long diffTime;
    LSA *lsa;
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    for(i = 0; i < table->size; i++) {
        entry = getNodeDataAt(table, i);
        lsa = entry->lastLSA;
        if(lsa != NULL) {
            diffTime = curTime.tv_sec - (lsa->timestamp).tv_sec;
            if(diffTime >= tRouting->LSATimeout) {
                replaceLSA(&(entry->lastLSA), NULL);
            }
        }
    }
}

void checkNeighborDown()
{
    DLL *table = tRouting->table;
    routingEntry *entry;
    int i;
    long diffTime;
    LSA *lsa;
    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    for(i = 0; i < table->size; i++) {
        entry = getNodeDataAt(table, i);
        if(entry->distance == 1 && entry->lastLSA != NULL) {
            lsa = entry->lastLSA;
            if(!hasLSAAck(lsa)) {
                diffTime = curTime.tv_sec - (lsa->timestamp).tv_sec;
                int needRetran = diffTime > tRouting->retranTimeout
                                 && hasLSARetran(lsa)
                                 && diffTime < tRouting->neighborTimeout;
                int isDown = diffTime >= tRouting->neighborTimeout
                             && !isLSADown(lsa);
                if(needRetran) {
                    LSA *retranLSA = LSAfromLSA(lsa);
                    routingEntry *myEntry = getMyRoutingEntry();
                    addLSAWithOneDest(getLocalLSABuffer(),
                                      retranLSA, myEntry->nodeID);
                    setLSARetran(lsa);
                    printf("Neighbor[%d] needs retran\n", entry->nodeID);
                } else if(isDown) {
                    //Flood with TTL=0
                    setLSADown(lsa);
                    LSA *downLSA = LSAfromLSA(lsa);
                    addLSAWithDest(getLocalLSABuffer(), downLSA, entry->nodeID);
                    printf("Neighbor[%d] is down\n", entry->nodeID);
                } else {
                    printf("Still waiting\n");
                }
            }
        }
    }
}










