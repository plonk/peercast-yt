# WIN32: g++ は WIN32 を定義しないが、ソースコードは WIN32 で Windows
# を検出しているのでここで定義する。

# __USE_MINGW_ANSI_STDIO: 定義されていると vsnprintf が stdio.h で定義
# され、libmingwex.a 内の定義と衝突するので未定義にする。

CPPFLAGS = -g -Wno-multichar -Wno-unused-variable -std=c++11 -U__STRICT_ANSI__ -DWIN32 -D_REENTRANT $(INCLUDES)
LDFLAGS = -static-libgcc -static-libstdc++ -pthread -L../../Onigmo/.libs
LIBS = -Wl,-Bstatic -lonigmo -Wl,-Bdynamic -lws2_32 -lShlwapi

OS = $(shell uname -s | tr A-Z a-z)
ARCH = $(shell uname -m | tr A-Z a-z)

DISTARCHIVE = peercast-yt-$(OS)-$(ARCH).tar.gz
AR = ar

INCLUDES = -I../../core -I../../core/common -I../../Onigmo

TARGET = peercast.exe
CORE = ../../core
SRCHTMLTAR = ../html.tar
HTML = html
SRCPUBLICTAR = ../public.tar
PUBLIC = public
ASSETS = ../linux/assets
CGIBIN = ../linux/cgi-bin

LIBOBJSTATIC = libpeercast.a

SYSSOURCE = $(wildcard $(CORE)/win32/*.cpp)
CORESOURCE = $(wildcard $(CORE)/common/*.cpp)
MISSINGSOURCE = ../../missing/strdup.cpp

COREOBJ = $(notdir $(CORESOURCE:.cpp=.o))
SYSOBJ = $(notdir $(SYSSOURCE:.cpp=.o))
MISSINGOBJ = $(notdir $(MISSINGSOURCE:.cpp=.o))

LINUXSOURCE = main.cpp
LINUXOBJ = $(notdir $(LINUXSOURCE:.cpp=.o))

DEPENDFILE = .deps

.PHONY: all clean build-html deps $(SRCHTMLTAR) $(SRCPUBLICTAR) $(SRCASSETSTAR)

# Here follow the generic build rules.
all: $(TARGET) $(LIBOBJSTATIC) $(DISTARCHIVE)

deps: $(DEPENDFILE)

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

$(SRCASSETSTAR):
	$(MAKE) -C .. assets.tar

$(DISTARCHIVE): $(TARGET) $(HTML) $(PUBLIC) $(ASSETS)
	rm -rf peercast-yt
	mkdir peercast-yt
	cp -R $(HTML) $(PUBLIC) $(TARGET) $(ASSETS) $(CGIBIN) ../../licenses ../../core/LICENSE peercast-yt/
	strip peercast-yt/$(TARGET)
	tar czf $(DISTARCHIVE) peercast-yt

clean:
	$(MAKE) -C .. clean
	rm -f $(LINUXOBJ) $(COREOBJ) $(SYSOBJ) $(TARGET) ${LIBOBJSTATIC} $(COREOBJSTATIC) $(DEPENDFILE)
	rm -rf $(HTML) $(PUBLIC)
	rm -rf peercast-yt/
	rm -f $(DISTARCHIVE)

$(COREOBJ): %.o : $(CORE)/common/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(SYSOBJ): %.o : $(CORE)/win32/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(MISSINGOBJ): %.o : ../../missing/%.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(DEPENDFILE): $(CORESOURCE) $(LINUXSOURCE)
	$(CXX) $(CPPFLAGS) -MM $(CORESOURCE) $(SYSSOURCE) $(LINUXSOURCE) $(MISSINGSOURCE)> $(DEPENDFILE)

$(LIBOBJSTATIC): $(COREOBJ) $(SYSOBJ) $(MISSINGOBJ)
	$(AR) rcs $(LIBOBJSTATIC) $(COREOBJ) $(SYSOBJ) $(MISSINGOBJ)

$(TARGET): $(LINUXOBJ) $(LIBOBJSTATIC)
	$(CXX) $(LDFLAGS) -o $@ $(LINUXOBJ) $(LIBOBJSTATIC) $(LIBS)

-include $(DEPENDFILE)
