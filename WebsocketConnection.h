#ifndef WEBSOCKETCONNECTION_H
#define WEBSOCKETCONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ev.h>
#include <WebsocketBuffer.h>

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
        WebsocketConnection(): intFd( 0 ), status( csInit ) {
            inBuf = new WebsocketBuffer( 200 );

            outBufSize = 1024;
            outBuf = new unsigned char[outBufSize];
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

        WebsocketBuffer *inBuf = NULL;

        unsigned char *outBuf = NULL;
        int outBufSize;
        int outBufLen;
        int outBufSentLen;
};

#endif
