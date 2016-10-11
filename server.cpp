#include <server.h>

int main()
{
    WebsocketServer::getInstance()->start();
    WebsocketServer::getInstance()->join();
}
