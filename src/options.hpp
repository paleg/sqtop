/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#ifndef __OPTIONS_H
#define __OPTIONS_H

#include <string>
#include <vector>

enum SPEED_MODE {
   SPEED_MIXED,
   SPEED_AVERAGE,
   SPEED_CURRENT
};

enum SORT_ORDER {
   SORT_SIZE,
   SORT_CURRENT_SPEED,
   SORT_AVERAGE_SPEED,
   SORT_MAX_TIME
};

class options_c {
   public:
      options_c();

      options_c copy();

      std::string host;
      int port;
      std::string pass;
      std::vector <std::string> Hosts;
      std::vector <std::string> Users;
      bool brief;
      bool full;
      bool zero;
      bool detail;
      bool ui;
      bool compactlongurls;
      bool compactsameurls;
      bool freeze;
      bool do_refresh;
      int sleep_sec;
      bool showhelp;
      bool showhelphint;
      SPEED_MODE speed_mode;
      SORT_ORDER sort_order;
};

#endif /* __OPTIONS_H */

// vim: ai ts=3 sts=3 et sw=3 expandtab
