# Uncomment following line to enabled RTMP fetch support
WITH_RTMP = yes

CPPFLAGS = -g -Wno-multichar -Wno-unused-variable -std=c++11 -DWIN32 -D_REENTRANT -D_UNICODE -DUNICODE $(INCLUDES) $(shell pkg-config openssl --cflags)
ifeq ($(WITH_RTMP),yes)
  CPPFLAGS += -DWITH_RTMP $(shell pkg-config librtmp --cflags)
endif
LDFLAGS = 
LIBS = -lwinpthread -lws2_32 -lShlwapi $(shell pkg-config openssl --libs)
ifeq ($(WITH_RTMP),yes)
  LIBS += $(shell pkg-config librtmp --libs)
endif

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

DLLS=$(MSYSTEM_PREFIX)/bin/lib{winpthread-*,gcc_s_*-*,stdc++-*,rtmp-*,gmp-*,hogweed-*,gnutls-*,nettle-*,p11-kit-*,idn2-*,tasn1-*,unistring-*,ffi-*,iconv-*,intl-*,crypto-*,ssl-*,zstd}.dll
ifeq ($(MSYSTEM),MINGW64)
  DLLS += $(MSYSTEM_PREFIX)/bin/zlib1.dll
endif

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

deps:
	$(CXX) $(CPPFLAGS) -MM $(CORESOURCE) $(SYSSOURCE) $(WINSOURCE) $(RTMPSOURCE) > $(DEPENDFILE)

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
	cp $(DLLS) peercast-yt/
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

$(LIBOBJSTATIC): $(COREOBJ) $(SYSOBJ)
	$(AR) rcs $(LIBOBJSTATIC) $(COREOBJ) $(SYSOBJ)

$(TARGET): $(WINOBJ) $(LIBOBJSTATIC)
	$(CXX) -municode -mwindows $(LDFLAGS) -o $@ $(WINOBJ) $(LIBOBJSTATIC) $(LIBS)

$(RTMPSERVER): $(RTMPOBJ) $(LIBOBJSTATIC)
	$(CXX) $(LDFLAGS) -o $@ $(RTMPOBJ) $(LIBOBJSTATIC) $(LIBS)

-include $(DEPENDFILE)
