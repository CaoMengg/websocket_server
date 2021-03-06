#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <pthread.h>
#include <ev.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/pem.h>
#include <map>

#include "GLog.h"
#include "YamlConf.h"
#include "SocketConnection.h"

typedef std::map<int, SocketConnection*> connectionMap;
typedef std::pair<int, SocketConnection*> connectionPair;

class WebsocketServer
{
    private:
        WebsocketServer()
        {
            config = new YamlConf( "conf/wserver.yaml" );
            intListenPort = config->getInt( "listen" );
            listenWatcher = new ev_io();
        }
        ~WebsocketServer()
        {
            if( listenWatcher )
            {
                ev_io_stop(pMainLoop, listenWatcher);
                delete listenWatcher;
            }

            if( intListenFd )
            {
                close( intListenFd );
            }
        }
        static WebsocketServer *pInstance;

        YamlConf *config = NULL;
        pthread_t intThreadId = 0;
        struct ev_loop *pMainLoop = EV_DEFAULT;
        int intListenPort = 0;
        int intListenFd = 0;
        ev_io *listenWatcher = NULL;
        connectionMap mapConnection;
    public:
        static WebsocketServer *getInstance();
        void run();
        int start();
        int join();
        void acceptCB();
        void readCB( int intFd );
        void writeCB( int intFd );
        void readTimeoutCB( int intFd );
        void writeTimeoutCB( int intFd );
        void recvHandshake( SocketConnection *pConnection );
        void recvMessage( SocketConnection *pConnection );
        void parseHandshake( SocketConnection *pConnection );
        void parseMessage( SocketConnection *pConnection );
        void ackHandshake( SocketConnection *pConnection );
        void ackMessage( SocketConnection *pConnection );
        void closeConnection( SocketConnection *pConnection );
};

#endif
