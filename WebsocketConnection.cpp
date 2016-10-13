#include <WebsocketConnection.h>

void WebsocketConnection::enlargeInBuf() {
    int intNewSize = inBufSize * 2;
    unsigned char *newBuf = new unsigned char[intNewSize];
    memcpy( newBuf, inBuf, inBufSize );
    delete[] inBuf;

    inBufSize = intNewSize;
    inBuf = newBuf;
}
