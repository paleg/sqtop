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
   compactsameurls = true;
   freeze = false;
   do_refresh = true;
   sleep_sec = 2;
   showhelp = false;
   speed_mode = SPEED_MIXED;
   sort_order = SORT_SIZE;
#ifdef WITH_RESOLVER
   pResolver = new Resolver();
   dns_resolution = true;
   strip_domain = true;
   resolve_mode = SHOW_BOTH;
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
   compactsameurls = pOrig->compactsameurls;
   freeze = pOrig->freeze;
   sleep_sec = pOrig->sleep_sec;
   showhelp = pOrig->showhelp;
   speed_mode = pOrig->speed_mode;
   sort_order = pOrig->sort_order;
#ifdef WITH_RESOLVER
   strip_domain = pOrig->strip_domain;
   dns_resolution = pOrig->dns_resolution;
   resolve_mode = pOrig->resolve_mode;
#endif
}

}
// vim: ai ts=3 sts=3 et sw=3 expandtab
