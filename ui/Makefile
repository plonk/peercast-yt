CATALOGS := $(wildcard catalogs/*.json)
TEMPLATES := $(wildcard html-master/*.* html-master/images/*.* html-master/js/*.* Templates/*.*)

all: html.tar

html.tar: $(CATALOGS) $(TEMPLATES)
	rm -rf html/
	RUBYOPT='--disable-gems' ./generate-html
	tar cf html.tar html

clean:
	rm -rf html/ html.tar
