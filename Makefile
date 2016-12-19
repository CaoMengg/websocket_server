OBJS = util/GLog.o util/YamlConf.o util/SocketBuffer.o util/SocketConnection.o WebsocketServer.o
CFLAGS = -W -Wall -Wunused-value -std=c++11 -g -rdynamic
DEPENDS = lib/glog/libglog.a lib/yaml/libyaml-cpp.a -lpthread -lev -lcrypto
INCLUDE = -I. -Iutil/ -Ilib/

bin/wserver: main.cpp main.h $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(INCLUDE) $(DEPENDS)

util/GLog.o: util/GLog.cpp util/GLog.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) -o $@

util/YamlConf.o: util/YamlConf.cpp util/YamlConf.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) -o $@

util/SocketBuffer.o: util/SocketBuffer.cpp util/SocketBuffer.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) -o $@

util/SocketConnection.o: util/SocketConnection.cpp util/SocketConnection.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) -o $@

WebsocketServer.o: WebsocketServer.cpp WebsocketServer.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) -o $@

.PHONY: clean start stop
clean:
	-$(RM) util/*.o util/*.gch
	-$(RM) bin/wserver *.o *.gch
	-$(RM) -rf run/supervise
	-$(RM) log/*

start:
	-make stop
	-make
	./bin/supervise.wserver run/ &

stop:
	-killall supervise.wserver 2>/dev/null
	-killall wserver 2>/dev/null
