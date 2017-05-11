CATALOGS := $(wildcard catalogs/*.json)
TEMPLATES := $(wildcard html-master/*.* html-master/images/*.* html-master/js/*.* Templates/*.*)
PUBLIC_TEMPLATES := $(wildcard public-master/*.* public-master/images/*.* public-master/js/*.* Templates/*.*)

all: html.tar public.tar

html.tar: $(CATALOGS) $(TEMPLATES)
	rm -rf html/
	./generate-html
	tar cf html.tar html

public.tar: $(CATALOGS) $(PUBLIC_TEMPLATES)
	rm -rf public/
	./generate-public
	tar cf public.tar public

clean:
	rm -rf html/ html.tar
	rm -rf public/ public.tar
