#ifndef WEBSOCKETCONNECTION_H
#define WEBSOCKETCONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ev.h>

#include <list>
#include <WebsocketBuffer.h>

typedef std::list<WebsocketBuffer *> bufferList;

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
        WebsocketConnection() {
            inBuf = new WebsocketBuffer( 1024 );
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
                delete inBuf;
            }

            while( ! outBufList.empty() ) {
                delete outBufList.front();
                outBufList.pop_front();
            }

            printf("delete connection, fd=%d\n", intFd);
        }
        struct ev_loop *pLoop = NULL;
        int intFd = 0;
        enumConnectionStatus status = csInit;
        ev_io *readWatcher = NULL;
        ev_io *writeWatcher = NULL;

        WebsocketBuffer *inBuf = NULL;
        bufferList outBufList;
};

#endif
