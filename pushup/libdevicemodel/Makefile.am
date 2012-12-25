noinst_LIBRARIES = libdevicemodel.a

MOC_FILES = \
	moc_korva-controller1.cpp \
	moc_pushupdevicemodel.cpp \
	moc_pushupcontroller.cpp

moc_pushupcontroller.cpp: pushupcontroller.h
moc_pushupdevicemodel.cpp: pushupdevicemodel.h
moc_korva-controller1.cpp: korva-controller1.h

moc_%.cpp: %.h
	$(AM_V_GEN)moc $< -o $@

libdevicemodel_a_SOURCES = \
	korva-controller1.cpp \
	korva-controller1.h \
	pushupcontroller.cpp \
	pushupcontroller.h \
	pushupdevicemodel.cpp \
	pushupdevicemodel.h \
	$(MOC_FILES)

libdevicemodel_a_CXXFLAGS = -fPIC $(DEVICEMODEL_CFLAGS)
