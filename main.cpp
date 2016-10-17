#include <main.h>

int main()
{
    WebsocketServer::getInstance()->start();
    WebsocketServer::getInstance()->join();
}
