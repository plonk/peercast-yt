CPPFLAGS = -g -Wno-multichar -Wno-unused-variable -std=c++11 -U__STRICT_ANSI__ -DWIN32 -D_REENTRANT -D_UNICODE -DUNICODE $(INCLUDES) $(shell pkg-config openssl --cflags)
LDFLAGS = -static-libgcc -static-libstdc++
LIBS = -Wl,-Bstatic -lwinpthread $(shell pkg-config openssl --libs) -Wl,-Bdynamic -lws2_32 -lShlwapi

OS = $(shell uname -s | tr A-Z a-z)
ARCH = $(shell uname -m | tr A-Z a-z)

DISTARCHIVE = peercast-yt-$(OS)-$(ARCH).tar.gz
AR = ar

INCLUDES = -I../../core -I../../core/common -I../../core/win32

TARGET = peercast.exe
RTMPSERVER = rtmp-server.exe
CORE = ../../core
RTMP = ../../rtmp-server
SRCHTMLTAR = ../html.tar
SRCPUBLICTAR = ../public.tar
ASSETS = ../assets/
CGIBIN = ../cgi-bin/

LIBOBJSTATIC = libpeercast.a

SYSSOURCE = $(wildcard $(CORE)/win32/*.cpp)
CORESOURCE = $(wildcard $(CORE)/common/*.cpp)

COREOBJ = $(notdir $(CORESOURCE:.cpp=.o))
SYSOBJ = $(notdir $(SYSSOURCE:.cpp=.o))

WINSOURCE = Simple.cpp gui.cpp
WINOBJ = $(notdir $(WINSOURCE:.cpp=.o)) resource.o

RTMPSOURCE = $(wildcard $(RTMP)/*.cpp)
RTMPOBJ = $(notdir $(RTMPSOURCE:.cpp=.o))

DEPENDFILE = .deps

-include Makefile.local

.PHONY: all clean build-html deps $(SRCHTMLTAR) $(SRCPUBLICSTAR) $(SRCASSETSTAR)

# Here follow the generic build rules.
all: $(TARGET) $(LIBOBJSTATIC) $(DISTARCHIVE) $(RTMPSERVER)

deps: $(DEPENDFILE)

resource.o: Simple.rc
	windres Simple.rc resource.o

$(SRCHTMLTAR):
	$(MAKE) -C .. html.tar

$(SRCASSETSTAR):
	$(MAKE) -C .. assets.tar

$(SRCPUBLICTAR):
	$(MAKE) -C .. public.tar

$(DISTARCHIVE): $(TARGET) $(ASSETS) $(RTMPSERVER) $(SRCPUBLICTAR) $(SRCHTMLTAR)
	rm -rf peercast-yt
	mkdir peercast-yt
	cp -R $(TARGET) $(ASSETS) $(CGIBIN) $(RTMPSERVER) ../../licenses ../../core/LICENSE peercast-yt/
	tar xf $(SRCHTMLTAR) -C peercast-yt/
	tar xf $(SRCPUBLICTAR) -C peercast-yt/
	strip peercast-yt/$(TARGET)
	strip peercast-yt/$(RTMPSERVER)
	tar czf $(DISTARCHIVE) peercast-yt

clean:
	$(MAKE) -C .. clean
	rm -f $(WINOBJ) $(COREOBJ) $(SYSOBJ) $(TARGET) ${LIBOBJSTATIC} $(COREOBJSTATIC) $(DEPENDFILE) $(RTMPSERVER)
	rm -rf peercast-yt/
	rm -f $(DISTARCHIVE)

$(COREOBJ): %.o : $(CORE)/common/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(SYSOBJ): %.o : $(CORE)/win32/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(RTMPOBJ): %.o : $(RTMP)/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(DEPENDFILE): $(CORESOURCE) $(SYSSOURCE) $(WINSOURCE) $(RTMPSOURCE)
	$(CXX) $(CPPFLAGS) -MM $(CORESOURCE) $(SYSSOURCE) $(WINSOURCE) $(RTMPSOURCE) > $(DEPENDFILE)

$(LIBOBJSTATIC): $(COREOBJ) $(SYSOBJ)
	$(AR) rcs $(LIBOBJSTATIC) $(COREOBJ) $(SYSOBJ)

$(TARGET): $(WINOBJ) $(LIBOBJSTATIC)
	$(CXX) -municode -mwindows $(LDFLAGS) -o $@ $(WINOBJ) $(LIBOBJSTATIC) $(LIBS)

$(RTMPSERVER): $(RTMPOBJ) $(LIBOBJSTATIC)
	$(CXX) $(LDFLAGS) -o $@ $(RTMPOBJ) $(LIBOBJSTATIC) $(LIBS)

-include $(DEPENDFILE)
