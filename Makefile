OBJS = WebsocketConnection.o WebsocketServer.o
CFLAGS = -W -Wall -Wunused-value -std=c++11 -g -rdynamic
DEPENDS = -lpthread -lev -lcrypto

wserver: server.cpp $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(DEPENDS)

WebsocketConnection.o: WebsocketConnection.cpp WebsocketConnection.h
	$(CXX) $(CFLAGS) -c $< $(DEPENDS)

WebsocketServer.o: WebsocketServer.cpp WebsocketServer.h
	$(CXX) $(CFLAGS) -c $< $(DEPENDS)

.PHONY: clean
clean:
	-$(RM) wserver *.o *.gch
