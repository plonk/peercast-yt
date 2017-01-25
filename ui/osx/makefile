CPPFLAGS = -D__cplusplus -D_UNIX -D_APPLE -D_REENTRANT $(INCLUDES) 
LDFLAGS = 

LD = g++
TAR = peercast-osx.tgz
CC = gcc

INCLUDES = -I../../core -I../../core/common 

# Library and include paths.
LIBS = 

TARGET = peercast
CORE = ../../core
HTML = html
SRCHTML = ../html


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
	 $(CORE)/common/jis.cpp \




COREOBJ = $(CORESOURCE:.cpp=.o)

MACOSXSOURCE = main.cpp
MACOSXOBJ = $(MACOSXSOURCE:.cpp=.o)

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
	rm  -f $(MACOSXOBJ) $(COREOBJ) $(TARGET)

%.o: %.cpp
	$(CC) $(INCLUDES) $(CPPFLAGS) -c $< -o $@

DEPENDFILE = makedepend


depend: 
	echo > $(DEPENDFILE)
	$(CC) $(INCLUDES) $(CPPFLAGS) -MD $(CORESOURCE) $(MACOSXSOURCE) >> $(DEPENDFILE)

$(TARGET): $(MACOSXOBJ) $(COREOBJ) 
	$(LD) $(LDFLAGS) -o $@ $(MACOSXOBJ) $(COREOBJ) $(LIBS) 


include $(DEPENDFILE)
