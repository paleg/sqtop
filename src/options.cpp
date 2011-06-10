/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#include "options.hpp"

namespace sqtop {

Options::Options() {
   host = "127.0.0.1";
   port = 3128;
   pass = "";
   brief = false;
   full = false;
   zero = false;
   detail = false;
   ui = true;
   compactlongurls = true;
   nocompactsameurls = false;
   freeze = false;
   do_refresh = true;
   sleep_sec = 2;
   showhelp = false;
   speed_mode = SPEED_MIXED;
   sort_order = SORT_SIZE;
#ifdef WITH_RESOLVER
   pResolver = new Resolver();
#endif
}

Options::~Options() {
#ifdef WITH_RESOLVER
   delete pResolver;
#endif
}

void Options::CopyFrom(Options* pOrig) {
   host = pOrig->host;
   port = pOrig->port;
   pass = pOrig->pass;
   brief = pOrig->brief;
   full = pOrig->full;
   zero = pOrig->zero;
   detail = pOrig->detail;
   ui = pOrig->ui;
   compactlongurls = pOrig->compactlongurls;
   nocompactsameurls = pOrig->nocompactsameurls;
   freeze = pOrig->freeze;
   sleep_sec = pOrig->sleep_sec;
   showhelp = pOrig->showhelp;
   speed_mode = pOrig->speed_mode;
   sort_order = pOrig->sort_order;
}

}
// vim: ai ts=3 sts=3 et sw=3 expandtab
