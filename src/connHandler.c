#include "connHandler.h"

int newConnectionHandler(connObj *connPtr)
{
    struct sockaddr_in clientAddr;
    socklen_t clientLength = sizeof(clientAddr);
    int listenFd = getConnObjSocket(connPtr);
    int newFd = accept(listenFd, (struct sockaddr *)&clientAddr, &clientLength);
    if(newFd == -1) {
        logger(LogProd, "Error accepting socket.\n");
        return -1;
    } else {
        return newFd;
    }
}

void processConnectionHandler(connObj *connPtr)
{
    char *buf;
    ssize_t size, retSize;
    int done, full;
    if(connPtr->isOpen == 0) {
        logger(LogDebug, "Skip connection set to close\n");
        return;
    }
    getConnObjReadBufferForRead(connPtr, &buf, &size);
    full = isFullConnObj(connPtr);
    switch(httpParse(connPtr->req, buf, &size, full)) {
    case Parsing:
        removeConnObjReadSize(connPtr, size);
        break;
    case ParseError:
    case Parsed:
        removeConnObjReadSize(connPtr, size);
        if(connPtr->res == NULL) {
            logger(LogDebug, "Create new response\n");
            connPtr->res = createResponseObj();
            buildResponseObj(connPtr->res, connPtr->req);
        }
        /* Dump response to buffer */
        logger(LogDebug, "Dump response to buffer\n");
        getConnObjWriteBufferForWrite(connPtr, &buf, &size);
        logger(LogDebug, "Write buffer has %d bytes free\n", size);
        done = writeResponse(connPtr->res, buf, size, &retSize);
        logger(LogDebug, "%d bytes dumped, done? %d\n", retSize, done);
        addConnObjWriteSize(connPtr, retSize);
        connPtr->wbStatus = writingRes;
        if(done) {
            logger(LogDebug, "All dumped\n");
            connPtr->wbStatus = lastRes;
        }
        logger(LogDebug, "Return from httpParse\n");
        break;
    default:
        break;
    }
    return;
}



void readConnectionHandler(connObj *connPtr)
{
    if(!isFullConnObj(connPtr)) {
        char *buf;
        ssize_t size;
        int connFd = getConnObjSocket(connPtr);
        ssize_t readret = 0;
        getConnObjReadBufferForWrite(connPtr, &buf, &size);

        readret = recv(connFd, buf, size, 0);
        if (readret == -1) {
            logger(LogProd, "Error reading from client.\n");
            setConnObjClose(connPtr);
            return;
        } else if(readret == 0) {
            logger(LogDebug, "Client Closed [%d]", connFd);
            setConnObjClose(connPtr);
            return;
        } else {
            logger(LogDebug, "Read %d bytes\n", readret);
            addConnObjReadSize(connPtr, readret);
        }

    }

}


void writeConnectionHandler(connObj *connPtr)
{
    char *buf;
    ssize_t size, retSize;
    int connFd = getConnObjSocket(connPtr);
    getConnObjWriteBufferForRead(connPtr, &buf, &size);
    logger(LogDebug, "Ready to write %d bytes...", size);
    if(size <= 0) {
        return;
    }
    retSize = send(connFd, buf, size, 0);
    if(retSize == -1 && errno == EINTR) {
        return ;
    }
    if (retSize != size) {
        logger(LogProd, "WTFError sending to client.\n");
        setConnObjClose(connPtr);
    } else {
        if(connPtr->wbStatus == lastRes) {
            connPtr->wbStatus = doneRes;
            if(1 == toClose(connPtr->res)) {
                logger(LogDebug, "[%d] set to close.\n", connPtr->connFd);
                setConnObjClose(connPtr);
            } else {
                /* Prepare for next request */
                freeResponseObj(connPtr->res);
                connPtr->res = NULL;
                freeRequestObj(connPtr->req);
                connPtr->req = createRequestObj();
            }
        }
        logger(LogDebug, "Done\n");
        removeConnObjWriteSize(connPtr, size);
    }

}


int closeConnectionHandler(connObj *connPtr)
{
    connPtr = connPtr;
    return EXIT_SUCCESS;
}
