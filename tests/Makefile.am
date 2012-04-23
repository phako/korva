check_PROGRAMS=test-upnp

TESTS=$(check_PROGRAMS)

test_upnp_SOURCES=test-upnp.c
test_upnp_CFLAGS=-I$(srcdir)/.. -I$(top_srcdir)/server \
	-I$(top_srcdir)/server/upnp \
	$(GUPNP_CFLAGS) $(COVERAGE_CFLAGS) -DTEST_DATA_DIR='"$(srcdir)"'
test_upnp_LDADD=$(top_builddir)/server/upnp/libkorva-upnp-backend.a \
	$(top_builddir)/server/libkorva-core.a $(GUPNP_LIBS) $(COVERAGE_LIBS)

if ENABLE_COVERAGE

TRACE_FILE=$(builddir)/korva.coverage

clean-coverage:
	find $(top_builddir) -name "*.gcda" -print0 | xargs -0 rm -f

coverage: clean-coverage check gen-coverage

gen-coverage:
	rm -f $(TRACE_FILE)
	$(LCOV) --directory $(top_builddir) \
			--capture \
			--output-file $(TRACE_FILE)
	$(LCOV) --output-file $(TRACE_FILE) \
			--remove $(TRACE_FILE) *.h
	$(LCOV) --output-file $(TRACE_FILE) \
			--remove $(TRACE_FILE) tests/*
	rm -rf $(builddir)/coverage
	$(mkdir_p) $(builddir)/coverage
	$(GENHTML) --title "@PACKAGE_STRING@" \
			   --output-directory $(builddir)/coverage \
			   $(TRACE_FILE)

.PHONY: gen-coverage clean-coverage
endif


EXTRA_DIST=test-upnp-image.jpg
