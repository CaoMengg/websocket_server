#ifndef WEBSOCKETCONNECTION_H
#define WEBSOCKETCONNECTION_H

#include <stdio.h>
#include <stdlib.h>

#include <ev.h>

enum enumConnectionStatus
{
    csInit,
    csAccepted,
    csConnected,
    csClosing,
    csClosed,
};

class WebsocketConnection
{
    public:
        WebsocketConnection(): intFd( 0 ), status( csInit ), readWatcher( NULL ), writeWatcher( NULL ) {
            inBufSize = 1024;
            inBuf = new char[inBufSize];
            inBufPos = inBuf;
            inBufLen = 0;
            inBufExpectLen = 0;

            outBufSize = 1024;
            outBuf = new char[outBufSize];
            outBufPos = outBuf;
            outBufLen = 0;
            outBufSentLen = 0;
        }
        int intFd;
        enumConnectionStatus status;
        ev_io *readWatcher;
        ev_io *writeWatcher;

        char *inBuf;
        char *inBufPos;
        int inBufSize;
        int inBufLen;
        int inBufExpectLen;

        char *outBuf;
        char *outBufPos;
        int outBufSize;
        int outBufLen;
        int outBufSentLen;
};

#endif
