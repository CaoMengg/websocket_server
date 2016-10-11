#ifndef WEBSOCKETCONNECTION_H
#define WEBSOCKETCONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ev.h>

enum enumConnectionStatus
{
    csInit,
    csAccepted,
    csConnected,
    csClosing,
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
        ~WebsocketConnection() {
            if( readWatcher && pLoop ) {
                ev_io_stop( pLoop, readWatcher );
                delete readWatcher;
            }
            if( writeWatcher && pLoop ) {
                ev_io_stop( pLoop, writeWatcher );
                delete writeWatcher;
            }

            if( intFd ) {
                close( intFd );
            }

            if( inBuf ) {
                delete[] inBuf;
            }
            if( outBuf ) {
                delete[] outBuf;
            }

            printf("delete connection, fd=%d\n", intFd);
        }
        struct ev_loop *pLoop = NULL;
        int intFd;
        enumConnectionStatus status;
        ev_io *readWatcher = NULL;
        ev_io *writeWatcher = NULL;

        char *inBuf = NULL;
        char *inBufPos;
        int inBufSize;
        int inBufLen;
        int inBufExpectLen;

        char *outBuf = NULL;
        char *outBufPos;
        int outBufSize;
        int outBufLen;
        int outBufSentLen;
};

#endif
