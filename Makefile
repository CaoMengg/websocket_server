OBJS = WebsocketBuffer.o WebsocketConnection.o WebsocketServer.o
CFLAGS = -W -Wall -Wunused-value -std=c++11 -g -rdynamic
DEPENDS = -lpthread -lev -lcrypto

wserver: server.cpp server.h $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(DEPENDS)

WebsocketBuffer.o: WebsocketBuffer.cpp WebsocketBuffer.h
	$(CXX) $(CFLAGS) -c $< $(DEPENDS)

WebsocketConnection.o: WebsocketConnection.cpp WebsocketConnection.h
	$(CXX) $(CFLAGS) -c $< $(DEPENDS)

WebsocketServer.o: WebsocketServer.cpp WebsocketServer.h WebsocketConnection.o WebsocketBuffer.o
	$(CXX) $(CFLAGS) -c $< $(DEPENDS)

.PHONY: clean
clean:
	-$(RM) wserver *.o *.gch
