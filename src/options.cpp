/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#include "options.hpp"

options_c::options_c() {
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
}

options_c options_c::copy() {
   options_c result;
   result.host = host;
   result.port = port;
   result.pass = pass;
   result.brief = brief;
   result.full = full;
   result.zero = zero;
   result.detail = detail;
   result.ui = ui;
   result.compactlongurls = compactlongurls;
   result.nocompactsameurls = nocompactsameurls;
   result.freeze = freeze;
   result.sleep_sec = sleep_sec;
   result.showhelp = showhelp;
   result.speed_mode = speed_mode;
   result.sort_order = sort_order;
   return result;
}

// vim: ai ts=3 sts=3 et sw=3 expandtab
