CPPFLAGS = -g -Wno-multichar -Wno-unused-variable -std=c++11 -U__STRICT_ANSI__ -DWIN32 -D_REENTRANT -D_UNICODE -DUNICODE $(INCLUDES)
LDFLAGS = -static-libgcc -static-libstdc++
LIBS = -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic -lws2_32 -lShlwapi

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
HTML = html
ASSETS = ../linux/assets
CGIBIN = ../linux/cgi-bin

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

.PHONY: all clean build-html deps $(SRCHTMLTAR) $(SRCASSETSTAR)

# Here follow the generic build rules.
all: $(TARGET) $(LIBOBJSTATIC) $(DISTARCHIVE) $(RTMPSERVER)

deps: $(DEPENDFILE)

resource.o: Simple.rc
	windres Simple.rc resource.o

$(SRCHTMLTAR):
	$(MAKE) -C .. html.tar

$(HTML): $(SRCHTMLTAR)
	rm -rf $(HTML)
	tar xf $(SRCHTMLTAR)
	touch $(HTML)

$(SRCASSETSTAR):
	$(MAKE) -C .. assets.tar

$(DISTARCHIVE): $(TARGET) $(HTML) $(ASSETS) $(RTMPSERVER)
	rm -rf peercast-yt
	mkdir peercast-yt
	cp -R $(HTML) $(TARGET) $(ASSETS) $(CGIBIN) $(RTMPSERVER) ../../licenses ../../core/LICENSE peercast-yt/
	strip peercast-yt/$(TARGET)
	strip peercast-yt/$(RTMPSERVER)
	tar czf $(DISTARCHIVE) peercast-yt

clean:
	$(MAKE) -C .. clean
	rm -f $(WINOBJ) $(COREOBJ) $(SYSOBJ) $(TARGET) ${LIBOBJSTATIC} $(COREOBJSTATIC) $(DEPENDFILE) $(RTMPSERVER)
	rm -rf $(HTML)
	rm -rf peercast-yt/
	rm -f $(DISTARCHIVE)

$(COREOBJ): %.o : $(CORE)/common/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(SYSOBJ): %.o : $(CORE)/win32/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(RTMPOBJ): %.o : $(RTMP)/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(DEPENDFILE): $(CORESOURCE) $(WINSOURCE) $(RTMPSOURCE)
	$(CXX) $(CPPFLAGS) -MM $(CORESOURCE) $(SYSSOURCE) $(WINSOURCE) $(RTMPSOURCE) > $(DEPENDFILE)

$(LIBOBJSTATIC): $(COREOBJ) $(SYSOBJ)
	$(AR) rcs $(LIBOBJSTATIC) $(COREOBJ) $(SYSOBJ)

$(TARGET): $(WINOBJ) $(LIBOBJSTATIC)
	$(CXX) -municode -mwindows $(LDFLAGS) -o $@ $(WINOBJ) $(LIBOBJSTATIC) $(LIBS)

$(RTMPSERVER): $(RTMPOBJ) $(LIBOBJSTATIC)
	$(CXX) $(LDFLAGS) -o $@ $(RTMPOBJ) $(LIBOBJSTATIC) $(LIBS)

-include $(DEPENDFILE)
