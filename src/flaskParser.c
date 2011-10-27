#include "flaskParser.h"

int flaskParse(char *readBuf, ssize_t rSize, char *writeBuf, ssize_t *wSize, int full)
{
    int numMatch;
    char *objName;
    char *objPath;
    char *tmpBuf = calloc(rSize + 1, 1);
    memcpy(tmpBuf, readBuf, rSize);
    
    numMatch = sscanf(tmpBuf, "GETRD %ms", objName);
    if(numMatch == 1) {
        free(tmpBuf);
        return flaskGETResponse(objName, writeBuf, wSize);
    }
    
    numMatch = sscanf(tmpBuf, "ADDFILE %ms %ms", objName, objPath);
    if(numMatch == 2) {
        free(tmpBuf);
        return flaskADDResponse(objName, objPath, writeBuf, wSize);
    }
    
    free(tmpBuf);
    if(full) {
        return -1;
    } else {
        return 0;
    }
    return -1;
}

int flaskGETResponse(char *objName, char *writeBuf, ssize_t *wSize)
{
    char *resPath = NULL;
    char *resHost = NULL;
    int *resPort = 0;
    getResourcePath(objName, &resHost, &resPort, &resPath);
    if(resHost == NULL) {
        return -1;
    } else {
        int retSize = snprintf(writeBuf, wSize,
                               "OK http://%s:%d%s", resHost, resPort, resPath);
        if(retSize < wSize) {
            *wSize=retSize;
            return 1; //Write succeed.
        } else {
            return -1; //Write buffer overflowed.
        }
    }
}


int flaskADDResponse(char *objName, char *objPath, char *writeBuf, ssize_t *wSize)
{
    int retVal = insertLocalResource(objName, objPath);
    int retSize = snprintf(writeBuf, wSize, "OK");
    if(retSize < wSize) {
        *wSize=retSize;
        return 1; //Write succeed.
    } else {
        return -1; //Write buffer overflowed.
    }
}

