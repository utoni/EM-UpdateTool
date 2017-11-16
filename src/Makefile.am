bin_PROGRAMS = utool

utool_CXXFLAGS = -std=c++11 -ffunction-sections -fdata-sections -Wl,--gc-sections $(UT_CFLAGS) $(WX_CFLAGS) -DCSV_IO_NO_THREAD=1
if UT_BUILDGUI
utool_CXXFLAGS += -DUSE_GUI=1
endif
utool_LDADD = $(UT_LIBS) $(WX_LIBS)

utool_SOURCES = \
	Json.cpp \
	UpdateFactory.cpp \
	UpdateTool.cpp
if UT_BUILDGUI
utool_SOURCES += \
	UpdateGUI.cpp \
	JobQueue.cpp
endif
