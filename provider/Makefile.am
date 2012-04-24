plugin_LTLIBRARIES = libpushup_play.la
plugindir = /usr/lib/share-ui/plugins

MOC_FILES = moc_pushupmethod.cpp \
			moc_pushupplugin.cpp \
			moc_pushupdeviceselector.cpp \
			moc_korva-controller1.cpp \
			moc_pushupcontroller.cpp \
			moc_pushupdevicemodel.cpp

libpushup_play_la_SOURCES = pushupmethod.cpp \
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

libpushup_play_la_LIBADD = $(SHAREUI_LIBS)
libpushup_play_la_LDFLAGS = -shared -module -avoid-version -fPIC
libpushup_play_la_CXXFLAGS = $(SHAREUI_CFLAGS) -DICON_DIR='"$(datadir)/korva/icons"'
