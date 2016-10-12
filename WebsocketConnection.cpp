#include <WebsocketConnection.h>

void WebsocketConnection::enlargeInBuf() {
    int intNewSize = inBufSize * 2;
    char *newBuf = new char[intNewSize];
    memcpy( newBuf, inBuf, inBufSize );
    delete[] inBuf;

    inBufSize = intNewSize;
    inBuf = newBuf;
}
