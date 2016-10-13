#include <WebsocketBuffer.h>

void WebsocketBuffer::enlarge() {
    int intNewSize = intSize * 2;
    unsigned char *newData = new unsigned char[intNewSize];
    memcpy( newData, data, intSize );
    delete[] data;

    intSize = intNewSize;
    data = newData;
}
