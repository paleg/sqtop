bin_PROGRAMS = sqtop
sqtop_SOURCES = Base64.cpp Utils.cpp options.cpp sqconn.cpp sqstat.cpp
sqtop_LDADD = @LIBOBJS@

AM_CPPFLAGS = -Wall

if ENABLE_UI
   sqtop_SOURCES += ncui.cpp
   AM_CPPFLAGS += -DENABLE_UI
endif

sqtop_SOURCES += sqtop.cpp
