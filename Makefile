WXDIR=../wxWidgets
WXLIBDIR=$(WXDIR)/lib

CC=i686-w64-mingw32-g++
CFLAGS=-static -I. -I$(WXDIR)/include -D_FILE_OFFSET_BITS=64 -D__WXMSW__ -mthreads
LDFLAGS_WIN_EXTRA=$(WXLIBDIR)/libwxpng-3.1-i686-w64-mingw32.a $(WXLIBDIR)/libwxzlib-3.1-i686-w64-mingw32.a
LDFLAGS_WIN=-Wl,--subsystem,windows -mwindows $(WXLIBDIR)/libwx_mswu_xrc-3.1-i686-w64-mingw32.a $(WXLIBDIR)/libwx_mswu_qa-3.1-i686-w64-mingw32.a $(WXLIBDIR)/libwx_baseu_net-3.1-i686-w64-mingw32.a $(WXLIBDIR)/libwx_mswu_html-3.1-i686-w64-mingw32.a $(WXLIBDIR)/libwx_mswu_adv-3.1-i686-w64-mingw32.a $(WXLIBDIR)/libwx_mswu_core-3.1-i686-w64-mingw32.a $(WXLIBDIR)/libwx_baseu_xml-3.1-i686-w64-mingw32.a $(WXLIBDIR)/libwx_baseu-3.1-i686-w64-mingw32.a -lrpcrt4 -loleaut32 -lole32 -luuid -lwinspool -lwinmm -lshell32 -lshlwapi -lcomctl32 -lcomdlg32 -ladvapi32 -lversion -lwsock32 -lgdi32 $(LDFLAGS_WIN_EXTRA)
DEPS=UpdateTool.cpp
OBJ=UpdateTool.o

%.o: $(DEPS)
	$(CC) -Wall -c -o $@ $< $(CFLAGS)

utool.exe: $(OBJ)
	$(CC) -Wall -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LDFLAGS_WIN)
