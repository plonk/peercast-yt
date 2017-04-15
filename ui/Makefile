CATALOGS := $(wildcard catalogs/*.json)
TEMPLATES := $(wildcard html-master/*.* html-master/images/*.* html-master/js/*.* Templates/peercast.dwt)

html.tar: $(CATALOGS) $(TEMPLATES)
	rm -rf html/
	./generate-html
	tar cf html.tar html

clean:
	rm -rf html/ html.tar
