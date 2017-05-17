CPPFLAGS = -g -Wno-multichar -Wno-unused-variable -std=c++11 -D_UNIX -D_REENTRANT $(INCLUDES)
LDFLAGS = -pthread
LIBS = 

OS = $(shell uname -s | tr A-Z a-z)
ARCH = $(shell uname -p | tr A-Z a-z)

TAR = peercast-yt-$(OS)-$(ARCH).tar.gz
AR = ar

INCLUDES = -I../../core -I../../core/common

TARGET = peercast
CORE = ../../core
SRCHTMLTAR = ../html.tar
HTML = html
SRCPUBLICTAR = ../public.tar
PUBLIC = public

LIBOBJSTATIC = libpeercast.a

SYSSOURCE = $(wildcard $(CORE)/unix/*.cpp)

CORESOURCE = $(wildcard $(CORE)/common/*.cpp) 

COREOBJ = $(notdir $(CORESOURCE:.cpp=.o))
SYSOBJ = $(notdir $(SYSSOURCE:.cpp=.o))

LINUXSOURCE = main.cpp
LINUXOBJ = $(notdir $(LINUXSOURCE:.cpp=.o))

DEPENDFILE = .deps

.PHONY: all clean build-html deps

# Here follow the generic build rules.
all: $(TARGET) build-html $(TAR)

deps: $(DEPENDFILE)

build-html:
	$(MAKE) -C ..

$(SRCHTMLTAR):
	$(MAKE) -C .. html.tar

$(HTML): $(SRCHTMLTAR)
	rm -rf $(HTML)
	tar xf $(SRCHTMLTAR) $(HTML)
	touch $(HTML)

$(SRCPUBLICTAR):
	$(MAKE) -C .. public.tar

$(PUBLIC): $(SRCPUBLICTAR)
	rm -rf $(PUBLIC)
	tar xf $(SRCPUBLICTAR) $(PUBLIC)
	touch $(PUBLIC)

$(TAR): $(TARGET) $(HTML) $(PUBLIC)
	rm -rf peercast-yt
	mkdir peercast-yt
	cp -R $(HTML) $(PUBLIC) $(TARGET) peercast-yt/
	strip peercast-yt/$(TARGET)
	tar cvzf $(TAR) peercast-yt

clean:
	rm -f $(LINUXOBJ) $(COREOBJ) $(SYSOBJ) $(TARGET) ${LIBOBJSTATIC} $(COREOBJSTATIC) $(DEPENDFILE) $(SRCHTMLTAR)
	rm -rf $(HTML) $(PUBLIC)
	rm -rf peercast-yt/

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
