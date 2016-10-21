#include <WebsocketServer.h>

WebsocketServer* WebsocketServer::pInstance = NULL;

WebsocketServer* WebsocketServer::getInstance()
{
    if( pInstance == NULL )
    {
        pInstance = new WebsocketServer();
    }
    return pInstance;
}

char *base64encode( const void *b64_encode_this, int encode_this_many_bytes )
{
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

void WebsocketServer::readTimeoutCB( int intFd )
{
    connectionMap::iterator it;
    it = mapConnection.find( intFd );
    if( it == mapConnection.end() )
    {
        return;
    }
    SocketConnection* pConnection = it->second;
    printf("read timeout, fd=%d\n", intFd);
    delete pConnection;
}

static void readTimeoutCallback( EV_P_ ev_timer *timer, int revents )
{
    (void)loop;
    (void)revents;
    WebsocketServer::getInstance()->readTimeoutCB( ((SocketConnection *)timer->data)->intFd );
}

void WebsocketServer::writeTimeoutCB( int intFd )
{
    connectionMap::iterator it;
    it = mapConnection.find( intFd );
    if( it == mapConnection.end() )
    {
        return;
    }
    SocketConnection* pConnection = it->second;
    printf("write timeout, fd=%d\n", intFd);
    delete pConnection;
}

static void writeTimeoutCallback( EV_P_ ev_timer *timer, int revents )
{
    (void)loop;
    (void)revents;
    WebsocketServer::getInstance()->writeTimeoutCB( ((SocketConnection *)timer->data)->intFd );
}

void WebsocketServer::ackHandshake( SocketConnection *pConnection )
{
    if( pConnection->outBufList.empty() )
    {
        return;
    }

    SocketBuffer *outBuf = pConnection->outBufList.front();
    if( outBuf->intSentLen < outBuf->intLen )
    {
        int n = send( pConnection->intFd, outBuf->data + outBuf->intSentLen, outBuf->intLen - outBuf->intSentLen, 0 );
        if( n > 0 )
        {
            outBuf->intSentLen += n;
        } else {
            if( errno==EAGAIN || errno==EWOULDBLOCK )
            {
                return;
            } else {
                delete pConnection;
                return;
            }
        }
    }

    if( outBuf->intSentLen >= outBuf->intLen )
    {
        printf("handshake succ, fd=%d\n", pConnection->intFd);

        pConnection->outBufList.pop_front();
        delete outBuf;

        pConnection->status = csConnected;
        ev_io_stop( pMainLoop, pConnection->writeWatcher );
        ev_timer_stop( pMainLoop, pConnection->writeTimer );
    }
}

void WebsocketServer::ackMessage( SocketConnection *pConnection )
{
    SocketBuffer *outBuf = NULL;
    while( ! pConnection->outBufList.empty() )
    {
        outBuf = pConnection->outBufList.front();
        if( outBuf->intSentLen < outBuf->intLen )
        {
            int n = send( pConnection->intFd, outBuf->data + outBuf->intSentLen, outBuf->intLen - outBuf->intSentLen, 0 );
            if( n > 0 )
            {
                outBuf->intSentLen += n;
            } else {
                if( errno==EAGAIN || errno==EWOULDBLOCK )
                {
                    return;
                } else {
                    delete pConnection;
                    return;
                }
            }
        }

        if( outBuf->intSentLen >= outBuf->intLen )
        {
            pConnection->outBufList.pop_front();
            delete outBuf;

            if( pConnection->status == csClosing )
            {
                printf("close succ, fd=%d\n", pConnection->intFd);
                delete pConnection;
                return;
            } else {
                printf("ack message succ, fd=%d\n", pConnection->intFd);
            }
        }
    }

    ev_io_stop( pMainLoop, pConnection->writeWatcher );
    ev_timer_stop( pMainLoop, pConnection->writeTimer );
}

void WebsocketServer::writeCB( int intFd )
{
    //printf("write\n");
    connectionMap::iterator it;
    it = mapConnection.find( intFd );
    if( it == mapConnection.end() )
    {
        return;
    }
    SocketConnection* pConnection = it->second;

    if( pConnection->status == csAccepted )
    {
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

void WebsocketServer::parseHandshake( SocketConnection *pConnection )
{
    char *begin = strstr((char*)(pConnection->inBuf->data), "Sec-WebSocket-Key:");
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

    SocketBuffer* outBuf = new SocketBuffer( 200 );
    sprintf( (char*)(outBuf->data), "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: %s\r\n\r\n", base64Code );
    outBuf->intLen = strlen( (char*)(outBuf->data) );
    pConnection->outBufList.push_back( outBuf );

    pConnection->inBuf->intLen = 0;
    pConnection->inBuf->intExpectLen = 0;

    ev_io_init( pConnection->writeWatcher, writeCallback, pConnection->intFd, EV_WRITE );
    ev_io_start( pMainLoop, pConnection->writeWatcher );

    ev_timer_set( pConnection->writeTimer, pConnection->writeTimeout, 0.0 );
    ev_timer_start( pMainLoop, pConnection->writeTimer );
}

void WebsocketServer::recvHandshake( SocketConnection *pConnection )
{
    int n = recv( pConnection->intFd, pConnection->inBuf->data + pConnection->inBuf->intLen, pConnection->inBuf->intSize - pConnection->inBuf->intLen, 0 );
    if( n > 0 )
    {
        pConnection->inBuf->intLen += n;

        if( pConnection->inBuf->data[pConnection->inBuf->intLen-4]=='\r' && pConnection->inBuf->data[pConnection->inBuf->intLen-3]=='\n' &&
                pConnection->inBuf->data[pConnection->inBuf->intLen-2]=='\r' && pConnection->inBuf->data[pConnection->inBuf->intLen-1]=='\n' )
        {
            //接收到完整握手
            printf("recv handshake, fd=%d\n", pConnection->intFd);
            ev_timer_stop( pMainLoop, pConnection->readTimer );
            parseHandshake( pConnection );
        }
    } else if( n == 0 )
    {
        delete pConnection;
        return;
    } else {
        if( errno==EAGAIN || errno==EWOULDBLOCK )
        {
            return;
        } else {
            delete pConnection;
            return;
        }
    }
}

void WebsocketServer::parseMessage( SocketConnection *pConnection )
{
    SocketBuffer* outBuf;
    int intPayloadLen = pConnection->inBuf->data[1] & 0x7f;
    int intRealLen = 0;

    if( intPayloadLen == 126 )
    {
        intRealLen = int(pConnection->inBuf->data[2])*256 + int(pConnection->inBuf->data[3]);
        outBuf = new SocketBuffer( intRealLen + 4 );
        outBuf->intLen = intRealLen + 4;
        outBuf->data[0] = 0x81;
        outBuf->data[1] = intPayloadLen;
        outBuf->data[2] = pConnection->inBuf->data[2];
        outBuf->data[3] = pConnection->inBuf->data[3];

        int curIndex = 4;
        int i;
        for( i=8; i<8+intRealLen; ++i )
        {
            outBuf->data[ curIndex++ ] =  pConnection->inBuf->data[i] ^ pConnection->inBuf->data[(i-8)%4 + 4];
        }
    } else {
        intRealLen = intPayloadLen;
        outBuf = new SocketBuffer( intRealLen + 2 );
        outBuf->intLen = intRealLen + 2;
        outBuf->data[0] = 0x81;
        outBuf->data[1] = intPayloadLen;

        int curIndex = 2;
        int i;
        for( i=6; i<6+intRealLen; ++i )
        {
            outBuf->data[ curIndex++ ] =  pConnection->inBuf->data[i] ^ pConnection->inBuf->data[(i-6)%4 + 2];
        }
    }
    pConnection->outBufList.push_back( outBuf );

    pConnection->inBuf->intLen = 0;
    pConnection->inBuf->intExpectLen = 0;

    ev_io_start( pMainLoop, pConnection->writeWatcher );

    ev_timer_set( pConnection->writeTimer, pConnection->writeTimeout, 0.0 );
    ev_timer_start( pMainLoop, pConnection->writeTimer );
}

void WebsocketServer::closeConnection( SocketConnection *pConnection )
{
    SocketBuffer* outBuf = new SocketBuffer( 2 );
    outBuf->data[0] = 0x88;
    outBuf->data[1] = 0;
    outBuf->intLen = 2;
    pConnection->outBufList.push_back( outBuf );

    pConnection->status = csClosing;
    ev_io_start(pMainLoop, pConnection->writeWatcher);

    ev_timer_set( pConnection->writeTimer, pConnection->writeTimeout, 0.0 );
    ev_timer_start( pMainLoop, pConnection->writeTimer );
}

void WebsocketServer::recvMessage( SocketConnection *pConnection )
{
    int n = recv( pConnection->intFd, pConnection->inBuf->data + pConnection->inBuf->intLen, pConnection->inBuf->intSize - pConnection->inBuf->intLen, 0 );
    if( n > 0 )
    {
        if( pConnection->inBuf->intExpectLen == 0 )
        {
            if( (pConnection->inBuf->data[0] & 0x0f) == 0x01 && (pConnection->inBuf->data[1] & 0x80) == 0x80 )
            {
                //text frame
                int intPayloadLen = pConnection->inBuf->data[1] & 0x7f;
                if( intPayloadLen == 126 )
                {
                    intPayloadLen = (unsigned char)(pConnection->inBuf->data[3]) | (unsigned char)(pConnection->inBuf->data[2]) << 8;
                } else if( intPayloadLen == 127 )
                {
                    printf("unsupported payload len, close\n");
                    ev_io_stop(pMainLoop, pConnection->readWatcher);
                    closeConnection( pConnection );
                    return;
                }
                pConnection->inBuf->intExpectLen = intPayloadLen + 6;    //first 2 byte and 4 mask byte
            } else if( (pConnection->inBuf->data[0] & 0x0f) == 0x08 )
            {
                //close frame
                ev_io_stop(pMainLoop, pConnection->readWatcher);
                closeConnection( pConnection );
                return;
            } else {
                printf("unsupported message, close\n");
                ev_io_stop(pMainLoop, pConnection->readWatcher);
                closeConnection( pConnection );
                return;
            }

            //set timer
            ev_timer_set( pConnection->readTimer, pConnection->readTimeout, 0 );
            ev_timer_start( pMainLoop, pConnection->readTimer );
        }
        pConnection->inBuf->intLen += n;

        if( pConnection->inBuf->intLen >= pConnection->inBuf->intExpectLen )
        {
            //接收到完整frame
            printf("recv message, fd=%d\n", pConnection->intFd);
            ev_timer_stop( pMainLoop, pConnection->readTimer );
            parseMessage( pConnection );
        }
    } else if( n == 0 )
    {
        delete pConnection;
        return;
    } else {
        if( errno==EAGAIN || errno==EWOULDBLOCK )
        {
            return;
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
    if( it == mapConnection.end() )
    {
        return;
    }
    SocketConnection* pConnection = it->second;

    if( pConnection->inBuf->intLen >= pConnection->inBuf->intSize )
    {
        pConnection->inBuf->enlarge();
    }

    if( pConnection->status == csAccepted )
    {
        recvHandshake( pConnection );
    } else if( pConnection->status == csConnected )
    {
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
    if( acceptFd == -1 )
    {
        close( acceptFd );
        return;
    }
    int flag = fcntl(acceptFd, F_GETFL, 0);
    fcntl(acceptFd, F_SETFL, flag | O_NONBLOCK);
    printf("accept fd=%d\n", acceptFd);

    SocketConnection* pConnection = new SocketConnection();
    pConnection->pLoop = pMainLoop;
    pConnection->intFd = acceptFd;
    pConnection->status = csAccepted;
    mapConnection[ acceptFd ] = pConnection;

    ev_io_init( pConnection->readWatcher, readCallback, acceptFd, EV_READ );
    ev_io_start( pMainLoop, pConnection->readWatcher );

    ev_init( pConnection->readTimer, readTimeoutCallback );
    ev_timer_set( pConnection->readTimer, pConnection->readTimeout, 0.0 );
    ev_timer_start( pMainLoop, pConnection->readTimer );

    ev_init( pConnection->writeTimer, writeTimeoutCallback );
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
    if( intRet != 0 )
    {
        printf("bind fail\n");
        return;
    }

    intRet = listen( intListenFd, 255 );
    if( intRet != 0 )
    {
        printf("listen fail\n");
        return;
    }
    LOG(INFO) << "server start, listen succ port=" << intListenPort << " fd=" << intListenFd;

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

    if( intRet == 0 )
    {
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

    if( intRet == 0 )
    {
        return 0;
    } else {
        return 1;
    }
}
