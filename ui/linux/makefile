CPPFLAGS = -g -Wno-multichar -Wno-unused-variable -std=c++11 -D_UNIX -D_REENTRANT $(INCLUDES)
LDFLAGS = -pthread
LIBS = 

TAR = peercast-linux.tar
CC = gcc
AR = ar

INCLUDES = -I../../core -I../../core/common

TARGET = peercast
CORE = ../../core
SRCHTMLTAR = ../html.tar
HTML = html

LIBOBJSTATIC = libpeercast.a

SYSSOURCE = \
	$(CORE)/unix/usys.cpp \
	$(CORE)/unix/usocket.cpp

CORESOURCE = \
	$(CORE)/common/socket.cpp \
	$(CORE)/common/servent.cpp \
	$(CORE)/common/servhs.cpp \
	$(CORE)/common/servmgr.cpp \
	$(CORE)/common/xml.cpp \
	$(CORE)/common/stream.cpp \
	$(CORE)/common/sys.cpp \
	$(CORE)/common/gnutella.cpp \
	$(CORE)/common/html.cpp \
	$(CORE)/common/channel.cpp \
	$(CORE)/common/http.cpp \
	$(CORE)/common/inifile.cpp \
	$(CORE)/common/peercast.cpp \
	$(CORE)/common/stats.cpp \
	$(CORE)/common/mms.cpp \
	$(CORE)/common/mp3.cpp \
	$(CORE)/common/nsv.cpp \
	$(CORE)/common/ogg.cpp \
	$(CORE)/common/url.cpp \
	$(CORE)/common/icy.cpp \
	$(CORE)/common/pcp.cpp \
	$(CORE)/common/jis.cpp \
	$(CORE)/common/flv.cpp \
	$(CORE)/common/jrpc.cpp \
	$(CORE)/common/chandir.cpp \
	$(CORE)/common/uri.cpp \
	$(CORE)/common/LUrlParser.cpp \
	$(CORE)/common/cgi.cpp


COREOBJ = $(notdir $(CORESOURCE:.cpp=.o))
SYSOBJ = $(notdir $(SYSSOURCE:.cpp=.o))

LINUXSOURCE = main.cpp
LINUXOBJ = $(notdir $(LINUXSOURCE:.cpp=.o))

DEPENDFILE = .deps

.PHONY: all clean deps

# Here follow the generic build rules.
all: $(TARGET) $(TAR)

deps: $(DEPENDFILE)

$(SRCHTMLTAR):
	make -C .. html.tar

$(HTML): $(SRCHTMLTAR)
	rm -rf $(HTML)
	tar xf $(SRCHTMLTAR) $(HTML)
	touch $(HTML)

$(TAR): $(TARGET) $(HTML)
	tar cf $(TAR) $(HTML) $(TARGET)

clean:
	rm -f $(LINUXOBJ) $(COREOBJ) $(SYSOBJ) $(TARGET) ${LIBOBJSTATIC} $(COREOBJSTATIC) $(DEPENDFILE) $(SRCHTMLTAR)
	rm -rf $(HTML)

$(COREOBJ): %.o : $(CORE)/common/%.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@

$(SYSOBJ): %.o : $(CORE)/unix/%.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@

$(DEPENDFILE): $(CORESOURCE) $(LINUXSOURCE)
	$(CXX) $(CPPFLAGS) -MM $(CORESOURCE) $(SYSSOURCE) $(LINUXSOURCE) > $(DEPENDFILE)

$(LIBOBJSTATIC): $(COREOBJ) $(SYSOBJ)
	$(AR) rcs $(LIBOBJSTATIC) $(COREOBJ) $(SYSOBJ)

$(TARGET): $(LINUXOBJ) $(LIBOBJSTATIC)
	$(CXX) $(LDFLAGS) -o $@ $(LINUXOBJ) $(LIBOBJSTATIC) $(LIBS)

-include $(DEPENDFILE)
