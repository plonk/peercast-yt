CPPFLAGS = -DENABLE_BINRELOC -pthread  -D__cplusplus -D_UNIX -D_REENTRANT $(INCLUDES) 
LDFLAGS = -pthread 
LDSOFLAGS = -shared -Wl,-soname

#CPPFLAGS = -static -pthread -D__cplusplus -D_UNIX -D_LINUX_STATIC -D_REENTRANT $(INCLUDES)
#LDFLAGS = -static -pthread 

LD = g++
TAR = peercast-linux.tgz
CC = gcc
AR = ar

INCLUDES = -I../../core -I../../core/common 

TARGET = peercast
CORE = ../../core
SRCHTML = ../html
HTML = html

LIBSONAME = libpeercast.so.1
LIBOBJSHARED = libpeercast.so.1.0
LIBOBJSTATIC = libpeercast.a

CORESOURCE = $(CORE)/unix/usys.cpp \
	 $(CORE)/unix/usocket.cpp \
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
	 $(CORE)/common/jis.cpp 



COREOBJ = $(CORESOURCE:.cpp=.o)

COREOBJSTATIC = $(CORESOURCE:.cpp=.a)

LINUXSOURCE = main.cpp 
LINUXOBJ = $(LINUXSOURCE:.cpp=.o)

# Here follow the generic build rules.
all:  $(TARGET)
	
tar: $(TARGET)
	rm -rf $(TAR)
	rm -rf $(HTML)
	mkdir -p $(HTML)/ja/images
	cp $(SRCHTML)/ja/images/*.* $(HTML)/ja/images
	cp $(SRCHTML)/ja/*.* $(HTML)/ja/
	mkdir -p $(HTML)/de/images
	cp $(SRCHTML)/de/images/*.* $(HTML)/de/images
	cp $(SRCHTML)/de/*.* $(HTML)/de/
	mkdir -p $(HTML)/en/images
	cp $(SRCHTML)/en/images/*.* $(HTML)/en/images
	cp $(SRCHTML)/en/*.* $(HTML)/en/
	mkdir -p $(HTML)/fr/images
	cp $(SRCHTML)/fr/images/*.* $(HTML)/fr/images
	cp $(SRCHTML)/fr/*.* $(HTML)/fr/
	tar -czf $(TAR) $(HTML) $(TARGET)

clean:
	rm  -f $(LINUXOBJ) $(COREOBJ) $(TARGET) ${LIBOBJSTATIC} ${LIBOBJSHARED} $(COREOBJSTATIC)

%.o: %.cpp
	$(CC) $(INCLUDES) $(CPPFLAGS) -fPIC -c $< -o $@

%.a: %.cpp
	$(CC) $(INCLUDES) $(CPPFLAGS) -c $< -o $@

DEPENDFILE = makedepend


depend: 
	echo > $(DEPENDFILE)
	$(CC) $(INCLUDES) $(CPPFLAGS) -MD $(CORESOURCE) $(LINUXSOURCE) >> $(DEPENDFILE)

peercast-so: $(COREOBJSTATIC) $(COREOBJ) $(LINUXOBJ)
	$(LD) $(LDSOFLAGS),$(LIBSONAME) -o ${LIBOBJSHARED} $(COREOBJ) -lc
	$(LD) $(LDFLAGS) -o $@ $(LINUXOBJ) $(LIBOBJSHARED) -ldl
	${AR} rcs $(LIBOBJSTATIC) $(COREOBJSTATIC)


$(TARGET): $(LINUXOBJ) $(COREOBJ)
	$(LD) $(LDFLAGS) -o $@ $(LINUXOBJ) $(COREOBJ) $(LIBS)
	


include $(DEPENDFILE)
