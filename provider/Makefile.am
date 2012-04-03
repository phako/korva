plugin_LTLIBRARIES = libpushupshare.la
plugindir = $(libdir)/share-ui/plugins

MOC_FILES = moc_pushupmethod.cpp \
			moc_pushupplugin.cpp \
			moc_pushupdeviceselector.cpp \
			moc_korva-controller1.cpp \
			moc_pushupcontroller.cpp \
			moc_pushupdevicemodel.cpp

libpushupshare_la_SOURCES = pushupmethod.cpp \
							pushupmethod.h \
							pushupplugin.cpp \
							pushupplugin.h \
							pushupdeviceselector.cpp \
							pushupdeviceselector.h \
							korva-controller1.cpp \
							korva-controller1.h \
							pushupcontroller.cpp \
							pushupcontroller.h \
							pushupdevicemodel.cpp \
							pushupdevicemodel.h \
							$(MOC_FILES)

moc_%.cpp: %.h
	$(AM_V_GEN)moc $< -o $@

libpushupshare_la_LIBADD = $(SHAREUI_LIBS)
libpushupshare_la_LDFLAGS = -shared -module -avoid-version -fPIC
libpushupshare_la_CXXFLAGS = $(SHAREUI_CFLAGS)
