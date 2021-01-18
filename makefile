# Top level makefile, the real shit is at src/makefile

TARGETS=noopt 32bit

all:
	cd src && $(MAKE) $@

install:
	cd src && $(MAKE) $@

$(TARGETS):
	cd src && $(MAKE) $@

doc:
	cd doc && $(MAKE)

test:
	cd app/m_jerry && $(MAKE) && cd -
	cd app/m_tom && $(MAKE) && cd -

TAGS:
	find . -type f -name "*.[ch]" | xargs etags -

clean:
	cd src && $(MAKE) $@
	cd app/m_jerry && $(MAKE) $@ && cd -
	cd app/m_tom && $(MAKE) $@ && cd -
	rm -f TAGS

distclean: clean

dummy:

.PHONY: doc install test TAGS
