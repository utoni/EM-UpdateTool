AC_PREREQ(2.58)
AC_INIT([UpdateTool], [1.0.1], [], [], [])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/config.h.in])
AC_CONFIG_FILES([Makefile src/Makefile])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
dnl Set build alias to `gcc -dumpmachine` if not set by user
if test x"$cross_compiling" != x"no"; then
	HOST_PREFIX="${host_alias}-"
	HOST_SUFFIX="-$host_alias"
	if test x"${build_alias}" = x; then
		AC_MSG_WARN([Cross compile enabled but no --build *explicitly* specified. Using default --build=$(gcc -dumpmachine)])
		build="$(gcc -dumpmachine)"
		build_alias="$(gcc -dumpmachine)"
	fi
else
	HOST_PREFIX=
	HOST_SUFFIX=
fi

if command -v git >/dev/null; then
	ut_githash="$(command git rev-parse HEAD)"
fi

AM_SILENT_RULES([yes])
AM_INIT_AUTOMAKE
AC_PROG_CXX
AC_PROG_INSTALL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_CHECK_HEADER_STDBOOL
AC_FUNC_VPRINTF
AC_FUNC_STAT

dnl Some C includes required by third-party modules.
AC_CHECK_HEADERS([pthread.h unistd.h signal.h assert.h sys/stat.h],,
	[AC_MSG_ERROR([Missing essential std headers.])])

dnl Some std functions which are required by libgcc/libstdc.
AC_CHECK_FUNCS([malloc calloc realloc free memcmp memchr memcpy memmove memset putc putwc raise rand signal sprintf strchr strcmp strcoll strcpy strerror strftime strlen strncmp strncpy strpbrk strspn strstr strtol strtoul tolower toupper towlower towupper ungetc ungetwc vfprintf wcschr wcscmp wcscoll wcscpy wcspbrk wcsspn wcsstr wcstol wcstoul fopen fgets fprintf fputs fread fwrite getc fputc isalnum isalpha isupper time mktime gmtime close fclose feof exit],,
	[AC_MSG_ERROR([Missing essential std functions.])])
AX_CXX_COMPILE_STDCXX_11(, [mandatory])

dnl C++ header checks, not necessary because we are checking for the CXX11 standart.
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([cstdio iostream iomanip fstream sstream memory regex tuple chrono ctime vector string cstring algorithm utility map list thread mutex exception],,
	[AC_MSG_ERROR([Missing essential C++ functions.])])
AC_LANG_POP([C++])

dnl `--disable-gui`
AC_ARG_ENABLE([gui],
	[AS_HELP_STRING([--disable-gui], [Build the UpdateTool with a wxWidgets based GUI instead of a simple CLI (default enabled).])],,[ut_buildgui=yes])
ut_buildgui=$(echo ${ut_buildgui})
case ${ut_buildgui} in
	1|y|yes) ut_buildgui=yes ;;
	''|0|n|no) ut_buildgui= ;;
	*) AC_MSG_ERROR([unknown option '${ut_buildgui}' for --enable-gui]) ;;
esac

dnl `--enable-host-wxwidgets`
AC_ARG_ENABLE([host-wxwidgets],
	[AS_HELP_STRING([--enable-host-wxwidgets], [Use the system wxWidgets library instead of our own shipped in ./wxWidgets (not recommended).])],,
	[enable_host_wxwidgets=no])
enable_host_wxwidgets=$(echo ${enable_host_wxwidgets})
case ${enable_host_wxwidgets} in
	''|1|y|yes) enable_host_wxwidgets=yes ;;
	0|n|no) enable_host_wxwidgets= ;;
	*) AC_MSG_ERROR([unknown option '${enable_host_wxwidgets}' for --enable-host-wxwidgets]) ;;
esac
if test x"${ut_buildgui}" != x"yes"; then
	enable_host_wxwidgets=
fi
AC_MSG_CHECKING([for host wxWidgets])
if test x"${enable_host_wxwidgets}" != x; then
	AC_MSG_RESULT([${enable_host_wxwidgets}])
else
	AC_MSG_RESULT([disabled])
fi

dnl Build the host triplet which we use to check for an existing wxWidgets build.
if test x"${enable_host_wxwidgets}" != x"yes" -a x"${ut_buildgui}" = xyes; then
	if test "${cross_compiling}" != "no"; then
		utool_wxdir="wxWidgets-${host_alias}/"
	else
		utool_wxdir="wxWidgets-host/"
	fi
	AC_MSG_CHECKING([for wxWidgets])
	if ! test -d "${utool_wxdir}lib"; then
		AC_MSG_ERROR([Missing wxWidgets builddir ${utool_wxdir}: Please run \`./build_wxwidgets.sh ${host_alias:-host}\` first or \`./configure --disable-gui\` or \`./configure --enable-host-wxwidgets\`.])
	fi
	AC_MSG_RESULT([yes])
else
	utool_wxdir=""
fi

AC_ARG_VAR(WINDRES, [Windows resource compiler command])

ut_cflags="${CFLAGS}"
ut_libs="${LIBS}"
PROGRAM_EXT=
windres=
case "${host}" in
	*-*-cygwin* | *-*-mingw32* )
		dnl Some Windows includes required by third-party modules.
		AC_CHECK_HEADERS([winsock2.h windows.h ws2tcpip.h],,
			[AC_MSG_ERROR([Missing essential Windows std headers.])])
		AC_CHECK_TOOL(WINDRES, [windres])
		PROGRAM_EXT=".exe"
		windres="${WINDRES:-}"
		ut_use_msw=yes
		ut_cflags="${ut_cflags} -D_UNICODE=1 -municode"
		ut_libs="${ut_libs} -static -lws2_32 -dynamic"
	;;
esac

dnl wxWidgets checks
if test x"${ut_buildgui}" = x"yes"; then
	if test x"${ut_use_msw}" = x"yes" -a x"${enable_host_wxwidgets}" = x"yes"; then
		AC_MSG_ERROR([Building UpdateTool for Windows based platforms is not supported with --enable-host-wxwidgets])
	fi
	AC_MSG_CHECKING([for wxWidgets version])
	wx_version=$(${utool_wxdir}wx-config --version-full)
	AC_MSG_RESULT([${wx_version}])
	AC_MSG_CHECKING([for wxWidgets CFLAGS])
	wx_cflags=$(${utool_wxdir}wx-config --cflags)
	AC_MSG_RESULT([yes])
	AC_MSG_CHECKING([for wxWidgets LIBS])
	wx_libs=$(${utool_wxdir}wx-config --libs std)
	AC_MSG_RESULT([yes])
fi

dnl set final ut (UpdateTool) CFLAGS
ut_cflags="${ut_cflags} -Wall -Wno-deprecated-declarations"
if `echo $CXXFLAGS $CFLAGS | grep " -O" >/dev/null`; then
	AC_MSG_WARN([CXXFLAGS/CFLAGS already contains -O flag; ignoring -Os])
else
	ut_cflags="${ut_cflags} -Os"
fi

dnl set automake conf vars
AM_CONDITIONAL([UT_BUILDGUI], [test x"${ut_buildgui}" = xyes])
AM_CONDITIONAL([HAVE_WINDRES], [test x"${windres}" != x])
AC_SUBST([UT_CFLAGS], [${ut_cflags}])
AC_SUBST([UT_LIBS],   [${ut_libs}])
AC_SUBST([WX_CFLAGS], [${wx_cflags}])
AC_SUBST([WX_LIBS],   [${wx_libs}])
AC_SUBST([WINDRES],   [${windres}])
AC_DEFINE_UNQUOTED([UT_GITHASH], ["${ut_githash:-unknown}"],[UpdateTool GIT SHA-1 hash])
AC_DEFINE_UNQUOTED([UT_BUILDGUI],["${ut_buildgui:-disabled}"],[UpdateTool build with GUI])
AC_DEFINE_UNQUOTED([UT_BUILDWIN],["${ut_use_msw:-disabled}"],[UpdateTool build for Windows])
AC_DEFINE_UNQUOTED([UT_CFLAGS],  ["${ut_cflags}"],[UpdateTool CFLAGS as string])
AC_DEFINE_UNQUOTED([UT_LIBS],    ["${ut_libs}"],  [UpdateTool LIBS as string])
AC_DEFINE_UNQUOTED([WX_CFLAGS],  ["${wx_cflags}"],[UpdateTool wxWidgets CFLAGS as string])
AC_DEFINE_UNQUOTED([WX_LIBS],    ["${wx_libs}"],  [UpdateTool wxWidgets LIBS as string])
AC_DEFINE_UNQUOTED([WX_VERSION], ["${wx_version:-unknown}"],[UpdateTool wxWidgets version])

dnl output config headers
AC_CONFIG_HEADERS([src/config.h:src/config.h.in])
AC_OUTPUT

dnl print configuration to stdout
cat << EOF

UpdateTool was configured with:

	CFLAGS/CXXFLAGS..........: ${ut_cflags}
	Build GUI................: ${ut_buildgui:-disabled}
	Build for Windows........: ${ut_use_msw:-disabled}
	Have windres.............: ${windres:-disabled}
	Host wxWidgets...........: ${enable_host_wxwidgets:-disabled}
	wxWidgets Version........: ${wx_version:-unknown}
	wxWidgets CFLAGS/CXXFLAGS: ${wx_cflags}
	wxWidgets LDFLAGS........: ${wx_libs}

EOF
