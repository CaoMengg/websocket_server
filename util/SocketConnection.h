#ifndef SOCKETCONNECTION_H
#define SOCKETCONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ev.h>

#include <list>
#include <SocketBuffer.h>

typedef std::list<SocketBuffer *> bufferList;

enum enumConnectionStatus
{
    csInit,
    csAccepted,
    csConnected,
    csClosing,
};

class SocketConnection
{
    public:
        SocketConnection() {
            inBuf = new SocketBuffer( 1024 );

            readWatcher = new ev_io();
            writeWatcher = new ev_io();
            readTimer = new ev_timer();
            readTimer->data = this;
        }
        ~SocketConnection() {
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
        ev_timer *readTimer = NULL;
        ev_tstamp readTimeout = 2.0;

        SocketBuffer *inBuf = NULL;
        bufferList outBufList;
};

#endif
