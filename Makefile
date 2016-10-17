OBJS = util/SocketBuffer.o util/SocketConnection.o WebsocketServer.o
CFLAGS = -W -Wall -Wunused-value -std=c++11 -g -rdynamic
DEPENDS = -lpthread -lev -lcrypto
INCLUDE = -I. -Iutil/

wserver: server.cpp server.h $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(INCLUDE) $(DEPENDS)

util/SocketBuffer.o: util/SocketBuffer.cpp util/SocketBuffer.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) -o $@

util/SocketConnection.o: util/SocketConnection.cpp util/SocketConnection.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) $(DEPENDS) -o $@

WebsocketServer.o: WebsocketServer.cpp WebsocketServer.h util/SocketConnection.o util/SocketBuffer.o
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) $(DEPENDS) -o $@

.PHONY: clean
clean:
	-$(RM) wserver *.o *.gch
	-$(RM) util/*.o util/*.gch
