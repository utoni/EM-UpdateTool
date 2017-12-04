#!/bin/bash

set -e
set -x

HOSTT="${1}"
if [ "x${HOSTT}" = "x" -o "x${HOSTT}" = "xhost" ]; then
	HOSTARG=""
else
	HOSTARG="--host=${HOSTT}"
fi

WDIR="wxWidgets"
git submodule init
git submodule update
cd ${WDIR}
git checkout .

cd ..
mkdir -p "${WDIR}-${HOSTT:-host}"
cd "${WDIR}-${HOSTT:-host}"

# Verify: Do we need '-Wl,-gc-sections' since we are creating static lib archives?
CXXFLAGS="-ffunction-sections -fdata-sections -Os -Wno-deprecated-declarations -Wno-misleading-indentation -Wno-undef"
../${WDIR}/configure --without-expat --disable-compat28 --disable-compat30 \
	--disable-richtooltip --disable-richmsgdlg --disable-richtext \
	--without-libpng --without-libjpeg --without-regex \
	--disable-ole --disable-mediactrl --disable-dataobj --disable-dataviewctrl \
	--disable-treebook --disable-treelist --disable-stc \
	--disable-webkit --disable-webview --disable-webviewwebkit --disable-webviewie \
	--disable-svg --without-libtiff --without-zlib --without-opengl \
	--without-gtkprint --disable-printfposparam --disable-printarch --disable-ps-in-msw \
	--enable-cxx11 \
	--disable-mshtmlhelp --disable-html --disable-htmlhelp \
	--disable-ribbon --disable-propgrid --disable-aui \
	--disable-sockets --disable-dialupman --disable-fs_inet \
	--disable-shared ${HOSTARG} \
	--disable-sys-libs \
	--disable-debug --disable-debug_flag \
	--disable-autoidman --disable-wxdib \
	--disable-uiactionsim --disable-accessibility \
	--disable-dragimage --disable-metafiles --disable-joystick \
	--disable-hotkey --disable-busyinfo --disable-spline \
	--disable-toolbook \
	CXXFLAGS="${CXXFLAGS}"
make -j${BUILDJOBS:-4} BUILD=release

# fix static lib path for cross compile targets
for lib in lib/*-${HOSTT:-host}.a; do
	NEWNAME="$(echo -n "${lib}" | sed -n "s/-${HOSTT}\.a$//gp").a"
	ln -sr "${lib}" "${NEWNAME}" 2>/dev/null || true
done
