#ifndef WEBSOCKETBUFFER_H
#define WEBSOCKETBUFFER_H

#include <stdio.h>
#include <string.h>

class WebsocketBuffer
{
    public:
        WebsocketBuffer( int intBufferSize ) {
            intSize = intBufferSize;
            data = new unsigned char[intSize];
        }
        ~WebsocketBuffer() {
            if( data ) {
                delete[] data;
            }
        }
        void enlarge();

        unsigned char *data = NULL;
        int intSize = 0;
        int intLen = 0;
        int intExpectLen = 0;
        int intPos = 0;
};

#endif
