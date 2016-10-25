#include "main.h"

int main()
{
    initGLog( "wserver" );
    WebsocketServer::getInstance()->start();
    WebsocketServer::getInstance()->join();
}
