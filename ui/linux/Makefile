# Uncomment following line to enabled RTMP fetch support
WITH_RTMP = yes

CPPFLAGS = -g -Wall -std=c++11 -D_UNIX -D_REENTRANT -DADD_BACKTRACE $(INCLUDES) $(shell pkg-config openssl --cflags)
ifeq ($(WITH_RTMP),yes)
  CPPFLAGS += -DWITH_RTMP
endif

LDFLAGS = -fuse-ld=gold -pthread -rdynamic
LIBS = $(shell pkg-config openssl --libs)
ifeq ($(WITH_RTMP),yes)
  LIBS += -lrtmp
endif

OS = $(shell uname -s | tr A-Z a-z)
ARCH = $(shell uname -m | tr A-Z a-z)

DISTARCHIVE = peercast-yt-$(OS)-$(ARCH).tar.gz
APPIMAGE = Peercast_YT-$(ARCH).AppImage
APPIMAGETOOL = appimagetool-$(ARCH).AppImage
LINUXDEPLOY = linuxdeploy-$(ARCH).AppImage
AR = ar

INCLUDES = -I../../core/common -I../../core/unix

TARGET = peercast
RTMPSERVER = rtmp-server
CORE = ../../core
RTMP = ../../rtmp-server
SRCHTMLTAR = ../html.tar
HTML = html
SRCPUBLICTAR = ../public.tar
PUBLIC = public
ASSETS = ../assets/
CGIBIN = ../cgi-bin

LIBOBJSTATIC = libpeercast.a

SYSSOURCE = $(wildcard $(CORE)/unix/*.cpp)

CORESOURCE = $(wildcard $(CORE)/common/*.cpp) 

COREOBJ = $(notdir $(CORESOURCE:.cpp=.o))
SYSOBJ = $(notdir $(SYSSOURCE:.cpp=.o))

LINUXSOURCE = main.cpp
LINUXOBJ = $(notdir $(LINUXSOURCE:.cpp=.o))

RTMPSOURCE = $(wildcard $(RTMP)/*.cpp)
RTMPOBJ = $(notdir $(RTMPSOURCE:.cpp=.o))

DEPENDFILE = .deps

PREFIX ?= /usr/local
bindir=$(PREFIX)/bin
docdir=$(PREFIX)/share/doc/peercast
sharedir=$(PREFIX)/share/peercast

-include Makefile.local

.PHONY: all clean build-html deps $(SRCHTMLTAR) $(SRCPUBLICTAR)

# Here follow the generic build rules.
all: $(TARGET) $(LIBOBJSTATIC) $(DISTARCHIVE) $(RTMPSERVER)

deps:
	$(CXX) $(CPPFLAGS) -MM $(CORESOURCE) $(SYSSOURCE) $(LINUXSOURCE) $(RTMPSOURCE) > $(DEPENDFILE)

$(SRCHTMLTAR):
	$(MAKE) -C .. html.tar

$(HTML): $(SRCHTMLTAR)
	rm -rf $(HTML)
	tar xf $(SRCHTMLTAR)
	touch $(HTML)

$(SRCPUBLICTAR):
	$(MAKE) -C .. public.tar

$(PUBLIC): $(SRCPUBLICTAR)
	rm -rf $(PUBLIC)
	tar xf $(SRCPUBLICTAR)
	touch $(PUBLIC)

$(DISTARCHIVE): $(TARGET) $(HTML) $(PUBLIC) $(ASSETS) $(RTMPSERVER)
	rm -rf peercast-yt
	mkdir peercast-yt
	cp -R $(HTML) $(PUBLIC) $(TARGET) $(ASSETS) $(CGIBIN) $(RTMPSERVER) ../../licenses ../../core/LICENSE peercast-yt/
	strip peercast-yt/$(TARGET)
	strip peercast-yt/$(RTMPSERVER)
	tar czf $(DISTARCHIVE) peercast-yt

install:
	mkdir -p $(bindir)
	install -s $(TARGET) $(RTMPSERVER) $(bindir)
	mkdir -p $(docdir)
	cp -R ../../licenses ../../core/LICENSE $(docdir)
	mkdir -p $(sharedir)
	cp -R $(HTML) $(PUBLIC) $(ASSETS) $(CGIBIN) $(sharedir)

$(APPIMAGE): $(APPIMAGETOOL) $(LINUXDEPLOY) $(TARGET) $(RTMPSERVER) peercast.desktop peercast.png
	rm -rf AppDir/
	PREFIX=AppDir/usr make install
	./$(LINUXDEPLOY) --appdir=AppDir -e AppDir/usr/bin/peercast  --deploy-deps-only="AppDir/usr/bin/rtmp-server" -d peercast.desktop -i peercast.png
	./$(APPIMAGETOOL) AppDir

$(APPIMAGETOOL):
	if [ ! -f $(APPIMAGETOOL) ]; then wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-$(ARCH).AppImage; chmod +x $(APPIMAGETOOL); fi

$(LINUXDEPLOY):
	if [ ! -f $(LINUXDEPLOY) ]; then wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$(ARCH).AppImage; chmod +x $(LINUXDEPLOY); fi

clean:
	$(MAKE) -C .. clean
	rm -f $(LINUXOBJ) $(COREOBJ) $(SYSOBJ) $(TARGET) ${LIBOBJSTATIC} $(COREOBJSTATIC) $(DEPENDFILE) $(RTMPSERVER) $(RTMPOBJ)
	rm -rf $(HTML) $(PUBLIC)
	rm -rf peercast-yt/
	rm -f $(DISTARCHIVE)

$(COREOBJ): %.o : $(CORE)/common/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(SYSOBJ): %.o : $(CORE)/unix/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(RTMPOBJ): %.o : $(RTMP)/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(LIBOBJSTATIC): $(COREOBJ) $(SYSOBJ)
	$(AR) rcs $(LIBOBJSTATIC) $(COREOBJ) $(SYSOBJ)

$(TARGET): $(LINUXOBJ) $(LIBOBJSTATIC)
	$(CXX) $(LDFLAGS) -o $@ $(LINUXOBJ) $(LIBOBJSTATIC) $(LIBS)

$(RTMPSERVER): $(RTMPOBJ) $(LIBOBJSTATIC)
	$(CXX) $(LDFLAGS) -o $@ $(RTMPOBJ) $(LIBOBJSTATIC) $(LIBS)

-include $(DEPENDFILE)
