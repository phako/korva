dist_man1_MANS = korva-control.1

XSLTPROC_PARAMS = --nonet \
	--stringparam man.output.quietly 1\
	--stringparam funcsynopsis.style ansi

if HAVE_XSLTPROC
korva-control.1: korva-control.xml
	$(AM_V_GEN)$(XSLTPROC) -o $@ $(XSLTPROC_PARAMS) http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

CLEANFILES = $(dist_man1_MANS)
endif

install-data-hook:
	cd $(DESTDIR)/$(man1dir) && test -f korva-list.1 || $(LN_S) korva-control.1 korva-list.1
	cd $(DESTDIR)/$(man1dir) && test -f korva-push.1 || $(LN_S) korva-control.1 korva-push.1
	cd $(DESTDIR)/$(man1dir) && test -f korva-unshare.1 || $(LN_S) korva-control.1 korva-unshare.1

EXTRA_DIST = korva-control.xml
