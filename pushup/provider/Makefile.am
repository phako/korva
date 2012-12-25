plugin_LTLIBRARIES = libpushup_play.la
plugindir = /usr/lib/share-ui/plugins

MOC_FILES = moc_pushupmethod.cpp \
			moc_pushupplugin.cpp \
			moc_pushupdeviceselector.cpp

libpushup_play_la_SOURCES = pushupmethod.cpp \
							pushupmethod.h \
							pushupplugin.cpp \
							pushupplugin.h \
							pushupdeviceselector.cpp \
							pushupdeviceselector.h \
							$(MOC_FILES)

moc_%.cpp: %.h
	$(AM_V_GEN)moc $< -o $@

libpushup_play_la_LIBADD = $(SHAREUI_LIBS) $(builddir)/../libdevicemodel/libdevicemodel.a $(DEVICEMODEL_LIBS)
libpushup_play_la_LDFLAGS = -shared -module -avoid-version -fPIC
libpushup_play_la_CXXFLAGS = $(SHAREUI_CFLAGS) -I$(srcdir)/../libdevicemodel
