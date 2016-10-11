#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <map>
#include <WebsocketConnection.h>

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

typedef std::map<int, WebsocketConnection*> connectionMap;
typedef std::pair<int, WebsocketConnection*> connectionPair;

class WebsocketServer
{
    private:
        WebsocketServer() : intThreadId( 0 ), pMainLoop( EV_DEFAULT ), intListenPort( 8088 ), intListenFd( 0 ) {}
        static WebsocketServer *pInstance;

        pthread_t intThreadId;
        struct ev_loop *pMainLoop = EV_DEFAULT;
        int intListenPort;
        int intListenFd;
        ev_io *listenWatcher;
        connectionMap mapConnection;
    public:
        static WebsocketServer *getInstance();
        void run();
        int start();
        int join();
        void acceptCB();
        void readCB( int intFd );
        void writeCB( int intFd );
        void recvHandshake( WebsocketConnection *pConnection );
        void recvMessage( WebsocketConnection *pConnection );
        void parseHandshake( WebsocketConnection *pConnection );
        void parseMessage( WebsocketConnection *pConnection );
        void ackHandshake( WebsocketConnection *pConnection );
        void ackMessage( WebsocketConnection *pConnection );
        void closeConnection( WebsocketConnection *pConnection );
};

#endif
