bin_PROGRAMS = sqtop
sqtop_SOURCES = Base64.cpp Utils.cpp sqconn.cpp sqstat.cpp
sqtop_LDADD = @LIBOBJS@

AM_CPPFLAGS = -Wall

if ENABLE_UI
   sqtop_SOURCES += ncui.cpp
endif

if WITH_RESOLVER
   sqtop_SOURCES += resolver.cpp
endif

sqtop_SOURCES += sqtop.cpp
