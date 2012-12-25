bin_PROGRAMS = pushup-control

MOC_FILES = moc_qmlapplicationviewer.cpp
RESOURCE_FILES = resources.cpp

pushup_control_SOURCES = \
	qmlapplicationviewer.h \
	qmlapplicationviewer.cpp \
	main.cpp \
	$(MOC_FILES) \
	$(RESOURCE_FILES)

moc_%.cpp: %.h
	$(AM_V_GEN)moc $< -o $@

%.cpp: %.qrc
	$(AM_V_GEN)rcc $< -o $@

pushup_control_LDADD = \
	$(UI_LIBS) \
	$(builddir)/../libdevicemodel/libdevicemodel.a \
	$(DEVICEMODEL_LIBS)

pushup_control_CXXFLAGS = \
	$(UI_CFLAGS) \
	$(DEVICEMODEL_CFLAGS) \
	-DHARMATTAN_BOOSTER \
	-I$(srcdir)/../libdevicemodel \
	-I/usr/share/qt4/mkspecs/linux-g++-maemo

#    ^ Need to find out how to get that dynamically
