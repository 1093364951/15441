#include "httpHeader.h"

headerEntry *newHeaderEntry(char *key, char *value)
{
    if(key == NULL) {
        return NULL;
    }
    headerEntry *hd = malloc(sizeof(headerEntry));
    char *thisKey = malloc(strlen(key) + 1);
    char *thisValue = NULL;
    strcpy(thisKey, key);
    if(value != NULL) {
        char *trimed = strTrim(value);
        thisValue = malloc(strlen(trimed) + 1);
        strcpy(thisValue, trimed);
    }
    strLower(thisKey);
    hd->key = thisKey;
    hd->value = thisValue;
    //logger(LogDebug, "Node[%s;%s] inserted\n", hd->key, hd->value);
    return hd;
}

headerEntry *newENVPEntry(char *key, char *value)
{
    if(key == NULL) {
        return NULL;
    }
    headerEntry *hd = malloc(sizeof(headerEntry));
    char *thisKey = malloc(strlen(key) + 1);
    char *thisValue = malloc(strlen(value) + 1);
    strcpy(thisKey, key);
    strcpy(thisValue, value);
    hd->key = thisKey;
    hd->value = thisValue;
    return hd;

}

void printHeaderEntry(void *data)
{
    headerEntry *hd = (headerEntry *)data;
    logger(LogDebug, "-[%s: %s]\n", hd->key, hd->value);
}

void freeHeaderEntry(void *data)
{
    if(data != NULL) {
        headerEntry *hd = (headerEntry *)data;
        free(hd->key);
        free(hd->value);
        free(hd);
    }
}

char *getValueByKey(DLL *headerList, char *key)
{
    headerEntry *target = newHeaderEntry(key, NULL);
    Node *nd = searchList(headerList, target);
    headerEntry *result = (nd == NULL) ? NULL : (headerEntry *)nd->data;
    freeHeaderEntry(target);

    if(result == NULL) {
        return NULL;
    } else {
        return result->value;
    }
}

int compareHeaderEntry(void *data1, void *data2)
{
    headerEntry *h1 = (headerEntry *)data1;
    headerEntry *h2 = (headerEntry *)data2;
    return strcmp(h1->key, h2->key);
}

