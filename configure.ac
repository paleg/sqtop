AC_PREREQ([2.64])
AC_INIT([sqtop], [2015-02-08], [o.palij@gmail.com])

AC_CONFIG_AUX_DIR(cfgaux)
m4_include([cfgaux/ax_lib_socket_nsl.m4])

AC_CONFIG_SRCDIR([src/sqtop.cpp])
AC_CONFIG_HEADERS([src/config.h])

AM_INIT_AUTOMAKE([foreign])

AC_CANONICAL_HOST

AC_GNU_SOURCE

AC_ARG_ENABLE(ui,
[  --enable-ui     build with ncurses user interface],
[case "${enableval}" in
  yes) enable_ui=true ;;
  no)  enable_ui=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-ui) ;;
esac],[enable_ui=true])
if test x$enable_ui = xtrue; then
  AC_DEFINE(ENABLE_UI, 1, [build with ncurses user interface])
fi
AM_CONDITIONAL(ENABLE_UI, test x$enable_ui = xtrue)

AC_ARG_WITH(resolver,
[  --with-resolver=TYPE     technique sqtop should use for name resolution.
       netdb               use gethostbyaddr_r in multiple threads
       netdb_getnameinfo   use getnameinfo (guess single or multiple threads)
       netdb_gethostbyaddr use gethostbyaddr in single thread
       guess               try to guess reasonable value (default).],
    [resolver=$withval],
    [resolver=guess])

if test x$resolver = xyes; then
   resolver=guess
fi

AC_LANG([C++])
AC_PROG_CXX
AC_PROG_INSTALL

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h getopt.h sys/socket.h], [], [AC_MSG_ERROR([Unable to find some C headers]); break;])
AC_CHECK_HEADERS([sstream iomanip algorithm iostream string vector map cstdlib cstring cerrno climits], [], [AC_MSG_ERROR([Unable to find some C++ headers]); break;])

# Checks for library functions.
AX_LIB_SOCKET_NSL
AC_SEARCH_LIBS([hstrerror], [resolv])
AC_CHECK_FUNCS([gethostbyname memset socket strerror], [], [AC_MSG_ERROR([Unable to find some C functions]); break;])

test_pthread=no

if test x$enable_ui = xtrue; then
   AC_CHECK_HEADERS(ncurses.h, [], [
            AC_CHECK_HEADERS(ncurses/ncurses.h, [
                AC_DEFINE([NCURSES_IN_SUBDIR], [ncurses/], [Look for ncurses headers in subdir])
            ], [AC_MSG_ERROR([Unable to find the ncurses headers]); break;])
        ])
   AC_CHECK_LIB([ncurses], [initscr], [], [AC_MSG_ERROR([Unable to find the ncurses library]); break;])
   AC_CHECK_FUNCS([clear refresh endwin mvaddstr mvchgat mvhline getch beep initscr nonl keypad noecho cbreak halfdelay addnstr], [], [AC_MSG_ERROR([Unable to find some ncurses functions]); break;])
   test_pthread=yes
fi

dnl From iftop's configure.in:
if test x$resolver != xno; then
   specified_resolver=$resolver
   if test x$resolver = xguess; then
      resolver=netdb
   fi

   if test x$resolver = xnetdb ; then
      AC_SEARCH_LIBS(gethostbyaddr_r, [nsl], , [resolver=guess])
      if test x$resolver = xguess && test x$specified_resolver != xguess ; then
         dnl They wanted gethostbyaddr_r, but they can't have it, so stop.
         AC_MSG_ERROR([no library defines gethostbyaddr_r])
      fi
   fi

   dnl We still might do gethostbyaddr_r.  Figure out whether we have
   dnl glibc-style or Solaris-style gethostbyaddr_r (or neither...).
   dnl Separate determining how to call gethostbyaddr_r from testing
   dnl whether it works so we can support cross-compilation.
   if test x$resolver = xnetdb ; then
       AC_MSG_CHECKING([how to call gethostbyaddr_r])
       dnl Try 7 arguments returning a struct hostent*.
       AC_LINK_IFELSE([AC_LANG_SOURCE([`cat cfgaux/hostentp_ghba_r.c`])],
                      [AC_MSG_RESULT([7 args])
                       ghba_args=7
                       AC_DEFINE(GETHOSTBYADDR_R_RETURNS_HOSTENT_P, 1,
                       [7-argument gethostbyaddr_r returns struct hostent*])],
                      [dnl Try 8 arguments returning an int.
                       AC_LINK_IFELSE([AC_LANG_SOURCE([`cat cfgaux/int_ghba_r.c`])],
                                      [AC_MSG_RESULT([8 args, int return])
                                       ghba_args=8
                                       AC_DEFINE(GETHOSTBYADDR_R_RETURNS_INT, 1,
                                       [8-argument gethostbyaddr_r returns int])],
                                      [dnl Neither.
                                       AC_MSG_RESULT([don't know how])
                                       resolver=guess])])

       if test x$resolver = xguess && test x$specified_resolver != xguess ; then
          dnl They wanted gethostbyaddr_r, but they can't have it, so stop.
          AC_MSG_ERROR([gethostbyaddr_r has no known calling convention])
       fi
   fi

   dnl If we still want to do gethostbyaddr_r, and we aren't
   dnl cross-compiling, test it.
   if test x$resolver = xnetdb ; then
       if test x$ghba_args = x8 ; then
          testfile=int_ghba_r
       else
          testfile=hostentp_ghba_r
       fi
       AC_MSG_CHECKING(gethostbyaddr_r usability)
       AC_RUN_IFELSE([AC_LANG_SOURCE(`cat cfgaux/$testfile.c`)],
                     [AC_MSG_RESULT([yes])],
                     [AC_MSG_RESULT([no])
                      resolver=guess],
                     [AC_MSG_RESULT([can't test because we are cross-compiling])])
       if test x$resolver = xguess ; then
           if test x$specified_resolver = xguess ; then
              AC_MSG_RESULT([gethostbyaddr_r doesn't work, so we'll try something else])
           else
              dnl They wanted gethostbyaddr_r, but it doesn't work, so stop.
              AC_MSG_ERROR([gethostbyaddr_r doesn't work])
           fi
       fi
   fi

   if test x$resolver = xnetdb ; then
       AC_DEFINE(USE_GETHOSTBYADDR_R, 1, [use gethostbyaddr_r for name resolution])
   fi

   if test x$resolver = xguess ; then
      resolver=netdb_getnameinfo
   fi

   if test x$resolver = xnetdb_getnameinfo ; then
      AC_SEARCH_LIBS(getnameinfo, [nsl], , [resolver=guess])
      if test x$resolver = xguess && test x$specified_resolver != xguess ; then
         dnl They wanted getnameinfo, but they can't have it, so stop.
         AC_MSG_ERROR([no library defines getnameinfo])
      fi
   fi

   if test x$resolver = xnetdb_getnameinfo ; then
      AC_DEFINE(USE_GETNAMEINFO, 1, [use getnameinfo for name resolution])

      case "$host_os" in
         darwin10.[[0-4]]*) threadsafe_getnameinfo=0 ;;
         darwin1[[0-1]].*) threadsafe_getnameinfo=1 ;;
         linux*) threadsafe_getnameinfo=1 ;;
         *) threadsafe_getnameinfo=0 ;;
      esac
      if test $threadsafe_getnameinfo = 1; then
         AC_DEFINE(THREADSAFE_GETNAMEINFO, 1, [resolving via getnameinfo can be done in multiple threads])
      fi
   fi

   if test x$resolver = xguess ; then
      resolver=netdb_gethostbyaddr
   fi

   if test x$resolver = xnetdb_gethostbyaddr; then
      AC_SEARCH_LIBS(gethostbyaddr, [nsl], , [resolver=guess])
      if test x$resolver = xguess && test x$specified_resolver != xguess ; then
         dnl They wanted gethostbyaddr, but they can't have it, so stop.
         AC_MSG_ERROR([no library defines gethostbyaddr])
      fi
   fi

   if test x$resolver = xnetdb_gethostbyaddr; then
      AC_DEFINE(USE_GETHOSTBYADDR, 1, [use gethostbyaddr for name resolution])
   fi

   if test x$resolver = xguess; then
      AC_MSG_NOTICE([failed to find reasonable resolver, name resolution will not be available!!!])
   else
      AC_DEFINE(WITH_RESOLVER, 1, [use name resolution])
   fi

   test_pthread=yes
fi
AM_CONDITIONAL(WITH_RESOLVER, test x$resolver != xno)

if test x$test_pthread = xyes; then
   AC_CHECK_HEADERS([pthread.h csignal], [], [AC_MSG_ERROR([Unable to find the threading headers]); break;])
   AC_CHECK_LIB([pthread], [pthread_create], [], [AC_MSG_ERROR([Unable to find the pthread library]); break;])
   AC_CHECK_FUNCS([pthread_mutex_lock pthread_mutexattr_init pthread_mutexattr_settype pthread_mutex_init pthread_mutexattr_destroy pthread_mutex_destroy pthread_mutex_unlock pthread_create pthread_cancel], [], [AC_MSG_ERROR([Unable to find some pthread functions]); break;])
fi

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile])
AC_OUTPUT
