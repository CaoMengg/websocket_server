#include <WebsocketServer.h>

WebsocketServer* WebsocketServer::pInstance = NULL;

WebsocketServer* WebsocketServer::getInstance()
{
    if( pInstance == NULL ) {
        pInstance = new WebsocketServer();
    }
    return pInstance;
}

char *base64encode( const void *b64_encode_this, int encode_this_many_bytes ) {
    int ret;
    BIO *b64_bio, *mem_bio;      //Declares two OpenSSL BIOs: a base64 filter and a memory BIO.
    BUF_MEM *mem_bio_mem_ptr;    //Pointer to a "memory BIO" structure holding our base64 data.
    b64_bio = BIO_new(BIO_f_base64());                      //Initialize our base64 filter BIO.
    mem_bio = BIO_new(BIO_s_mem());                           //Initialize our memory sink BIO.
    BIO_push(b64_bio, mem_bio);            //Link the BIOs by creating a filter-sink BIO chain.
    BIO_set_flags(b64_bio, BIO_FLAGS_BASE64_NO_NL);  //No newlines every 64 characters or less.
    BIO_write(b64_bio, b64_encode_this, encode_this_many_bytes); //Records base64 encoded data.
    ret = BIO_flush(b64_bio);          //Necessary for b64 encoding, because of pad characters.
    if( ret != 1 ) {}
    BIO_get_mem_ptr(mem_bio, &mem_bio_mem_ptr);  //Store address of mem_bio's memory structure.
    ret = BIO_set_close(mem_bio, BIO_NOCLOSE);    //access to mem_ptr after BIOs are destroyed.
    if( ret != 1 ) {}
    BIO_free_all(b64_bio);  //Destroys all BIOs in chain, starting with b64 (i.e. the 1st one).
    BUF_MEM_grow(mem_bio_mem_ptr, (*mem_bio_mem_ptr).length + 1);   //Makes space for end null.
    (*mem_bio_mem_ptr).data[(*mem_bio_mem_ptr).length] = '\0';  //Adds null-terminator to tail.
    return (*mem_bio_mem_ptr).data; //Returns base-64 encoded data. (See: "buf_mem_st" struct).
}

void WebsocketServer::ackHandshake( WebsocketConnection *pConnection )
{
    if( pConnection->outBufSentLen < pConnection->outBufLen ) {
        int n = send( pConnection->intFd, pConnection->outBufPos, pConnection->outBufLen - pConnection->outBufSentLen, 0 );
        if( n > 0 ) {
            pConnection->outBufSentLen += n;
            pConnection->outBufPos += n;
        }
    }

    if( pConnection->outBufSentLen >= pConnection->outBufLen ) {
        printf("handshake succ, fd=%d\n", pConnection->intFd);
        pConnection->inBufLen = 0;
        pConnection->inBufExpectLen = 0;

        pConnection->outBufLen = 0;
        pConnection->outBufSentLen = 0;
        pConnection->outBufPos = pConnection->outBuf;

        pConnection->status = csConnected;
        ev_io_stop(pMainLoop, pConnection->writeWatcher);
        ev_io_start(pMainLoop, pConnection->readWatcher);
    }
}

void WebsocketServer::ackMessage( WebsocketConnection *pConnection )
{
    if( pConnection->outBufSentLen < pConnection->outBufLen ) {
        int n = send( pConnection->intFd, pConnection->outBufPos, pConnection->outBufLen - pConnection->outBufSentLen, 0 );
        if( n > 0 ) {
            pConnection->outBufSentLen += n;
            pConnection->outBufPos += n;
        }
    }

    if( pConnection->outBufSentLen >= pConnection->outBufLen ) {
        pConnection->inBufLen = 0;
        pConnection->inBufExpectLen = 0;

        pConnection->outBufLen = 0;
        pConnection->outBufSentLen = 0;
        pConnection->outBufPos = pConnection->outBuf;

        if( pConnection->status == csClosing ) {
            printf("ack close succ, fd=%d\n", pConnection->intFd);
            delete pConnection;
            return;
        }

        printf("ack message succ, fd=%d\n", pConnection->intFd);
        if( pConnection->status == csAccepted ) {
            pConnection->status = csConnected;
        }
        ev_io_stop(pMainLoop, pConnection->writeWatcher);
        ev_io_start(pMainLoop, pConnection->readWatcher);
    }
}

void WebsocketServer::writeCB( int intFd )
{
    //printf("write\n");
    connectionMap::iterator it;
    it = mapConnection.find( intFd );
    if( it == mapConnection.end() ) {
        return;
    }
    WebsocketConnection* pConnection = it->second;

    if( pConnection->status == csAccepted ) {
        ackHandshake( pConnection );
    } else {
        ackMessage( pConnection );
    }
}

static void writeCallback( EV_P_ ev_io *watcher, int revents )
{
    (void)loop;
    (void)revents;
    WebsocketServer::getInstance()->writeCB( watcher->fd );
}

void WebsocketServer::parseHandshake( WebsocketConnection *pConnection )
{
    char *begin = strstr(pConnection->inBuf, "Sec-WebSocket-Key:");
    begin += 19;
    char *end = strchr(begin, '\r');
    int len = end - begin;

    char clientKey[50];
    memcpy( clientKey, begin, len );
    clientKey[len] = '\0';

    char joinedKey[100];
    sprintf( joinedKey, "%s%s", clientKey, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" );

    unsigned char sha1Code[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)joinedKey, strlen( joinedKey ), sha1Code);

    char* base64Code = base64encode( sha1Code, SHA_DIGEST_LENGTH );
    sprintf( pConnection->outBuf, "HTTP/1.1 101 Switching Protocols\r\n"
                                  "Upgrade: websocket\r\n"
                                  "Connection: Upgrade\r\n"
                                  "Sec-WebSocket-Accept: %s\r\n\r\n", base64Code );
    pConnection->outBufLen = strlen( pConnection->outBuf );

    ev_io *writeWatcher = new ev_io();
    ev_io_init( writeWatcher, writeCallback, pConnection->intFd, EV_WRITE );
    pConnection->writeWatcher = writeWatcher;
    ev_io_start( pMainLoop, writeWatcher );
}

void WebsocketServer::recvHandshake( WebsocketConnection *pConnection )
{
    int n = recv( pConnection->intFd, pConnection->inBuf+pConnection->inBufLen, pConnection->inBufSize-pConnection->inBufLen, 0 );
    if( n > 0 ) {
        pConnection->inBufLen += n;

        if( pConnection->inBuf[pConnection->inBufLen-4]=='\r' && pConnection->inBuf[pConnection->inBufLen-3]=='\n' &&
                pConnection->inBuf[pConnection->inBufLen-2]=='\r' && pConnection->inBuf[pConnection->inBufLen-1]=='\n'
          ) {
            ev_io_stop(pMainLoop, pConnection->readWatcher);
            parseHandshake( pConnection );
        }
    } else if( n == 0 ) {
        delete pConnection;
        return;
    } else {
        if( errno==EAGAIN || errno==EWOULDBLOCK ) {
        } else {
            delete pConnection;
            return;
        }
    }
}

void WebsocketServer::parseMessage( WebsocketConnection *pConnection )
{
    int payloadLen = pConnection->inBuf[1] & 0x7f;
    pConnection->outBuf[0] = 0x81;
    pConnection->outBuf[1] = payloadLen;
    int curIndex = 2;

    int i;
    for( i=6; i<6+payloadLen; ++i ) {
        pConnection->outBuf[ curIndex++ ] =  pConnection->inBuf[i] ^ pConnection->inBuf[(i-6)%4 + 2];
    }
    pConnection->outBufLen = payloadLen + 2;

    ev_io_start(pMainLoop, pConnection->writeWatcher);
}

void WebsocketServer::closeConnection( WebsocketConnection *pConnection )
{
    pConnection->outBuf[0] = 0x88;
    pConnection->outBuf[1] = 0;
    pConnection->outBufLen = 2;

    pConnection->status = csClosing;
    ev_io_start(pMainLoop, pConnection->writeWatcher);
}

void WebsocketServer::recvMessage( WebsocketConnection *pConnection )
{
    int n = recv( pConnection->intFd, pConnection->inBuf+pConnection->inBufLen, pConnection->inBufSize-pConnection->inBufLen, 0 );
    printf("%d %d\n", pConnection->inBufLen, pConnection->inBufSize);
    if( n > 0 ) {
        if( pConnection->inBufExpectLen == 0 ) {
            if( (pConnection->inBuf[0] & 0x0f) == 0x01 && (pConnection->inBuf[1] & 0x80) == 0x80 )
            {
                //text
                int intPayloadLen = pConnection->inBuf[1] & 0x7f;
                if( intPayloadLen == 126 ) {
                    intPayloadLen = int(pConnection->inBuf[2])*256 + int(pConnection->inBuf[3]);
                } else if( intPayloadLen == 127 ) {
                    printf("unsupported payload len, close\n");
                    ev_io_stop(pMainLoop, pConnection->readWatcher);
                    closeConnection( pConnection );
                    return;
                }
                pConnection->inBufExpectLen = intPayloadLen + 6;    //first 2 byte and 4 mask byte
                printf("len=%d\n", pConnection->inBufExpectLen);
            } else if( (pConnection->inBuf[0] & 0x0f) == 0x08 ) {
                //close
                ev_io_stop(pMainLoop, pConnection->readWatcher);
                closeConnection( pConnection );
                return;
            } else {
                printf("unsupported message, close\n");
                ev_io_stop(pMainLoop, pConnection->readWatcher);
                closeConnection( pConnection );
                return;
            }
        }
        pConnection->inBufLen += n;

        if( pConnection->inBufLen >= pConnection->inBufExpectLen ) {
            printf("recv message, fd=%d\n", pConnection->intFd);
            ev_io_stop(pMainLoop, pConnection->readWatcher);
            parseMessage( pConnection );
        }
    } else if( n == 0 ) {
        delete pConnection;
        return;
    } else {
        if( errno==EAGAIN || errno==EWOULDBLOCK ) {
        } else {
            delete pConnection;
            return;
        }
    }
}

void WebsocketServer::readCB( int intFd )
{
    //printf("read\n");
    connectionMap::iterator it;
    it = mapConnection.find( intFd );
    if( it == mapConnection.end() ) {
        return;
    }
    WebsocketConnection* pConnection = it->second;

    if( pConnection->inBufLen >= pConnection->inBufSize ) {
        pConnection->enlargeInBuf();
    }

    if( pConnection->status == csAccepted ) {
        recvHandshake( pConnection );
    } else if( pConnection->status == csConnected ) {
        recvMessage( pConnection );
    }
}

static void readCallback( EV_P_ ev_io *watcher, int revents )
{
    (void)loop;
    (void)revents;
    WebsocketServer::getInstance()->readCB( watcher->fd );
}

void WebsocketServer::acceptCB()
{
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int acceptFd = accept( intListenFd, (struct sockaddr*)&ss, &slen );
    if( acceptFd == -1 ) {
        close( acceptFd );
        return;
    }
    int flag = fcntl(acceptFd, F_GETFL, 0);
    fcntl(acceptFd, F_SETFL, flag | O_NONBLOCK);
    printf("accept fd=%d\n", acceptFd);

    WebsocketConnection* pConnection = new WebsocketConnection();
    pConnection->pLoop = pMainLoop;
    pConnection->intFd = acceptFd;
    pConnection->status = csAccepted;

    connectionMap::iterator it;
    mapConnection.find( acceptFd );
    if( it != mapConnection.end() ) {
        //TODO close connect
    }
    mapConnection[ acceptFd ] = pConnection;

    ev_io *readWatcher = new ev_io();
    ev_io_init( readWatcher, readCallback, acceptFd, EV_READ );
    pConnection->readWatcher = readWatcher;
    ev_io_start( pMainLoop, readWatcher );
}

static void acceptCallback( EV_P_ ev_io *watcher, int revents )
{
    (void)loop;
    (void)watcher;
    (void)revents;
    WebsocketServer::getInstance()->acceptCB();
}

void WebsocketServer::run()
{
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons( intListenPort );

    intListenFd = socket(AF_INET, SOCK_STREAM, 0);
    int flag = 1;
    setsockopt(intListenFd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
    flag = fcntl(intListenFd, F_GETFL, 0);
    fcntl(intListenFd, F_SETFL, flag | O_NONBLOCK);

    int intRet;
    intRet = bind( intListenFd, (struct sockaddr*)&sin, sizeof(sin) );
    if( intRet != 0 ) {
        printf("bind fail\n");
        return;
    }

    intRet = listen( intListenFd, 255 );
    if( intRet != 0 ) {
        printf("listen fail\n");
        return;
    }
    printf("listen succ port=%d fd=%d\n", intListenPort, intListenFd);

    listenWatcher = new ev_io();
    ev_io_init( listenWatcher, acceptCallback, intListenFd, EV_READ );
    ev_io_start( pMainLoop, listenWatcher );
    ev_run( pMainLoop, 0 );
}

void *runWebsocketServer( void* )
{
    WebsocketServer::getInstance()->run();
    return NULL;
}

int WebsocketServer::start()
{
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
    int intRet;
    intRet = pthread_create( &(intThreadId), &attr, runWebsocketServer, NULL );
    pthread_attr_destroy( &attr );

    if( intRet == 0 ) {
        printf( "start thread succ\n" );
        return 0;
    } else {
        printf( "start thread fail\n" );
        return 1;
    }
}

int WebsocketServer::join()
{
    void *pStatus;
    int intRet;
    intRet = pthread_join( intThreadId, &pStatus );

    if( intRet == 0 ) {
        return 0;
    } else {
        return 1;
    }
}
