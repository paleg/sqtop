/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#ifndef __OPTIONS_H
#define __OPTIONS_H

#include <string>
#include <vector>

#include "config.h"

#ifdef WITH_RESOLVER
#include <resolver.hpp>
#endif

namespace sqtop {

class Options {
   public:
      Options();
      ~Options();

      void CopyFrom(Options* pOrig);

      std::string host;
      int port;
      std::string pass;
      std::vector<std::string> Hosts;
      std::vector<std::string> Users;
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
      SPEED_MODE speed_mode;
      SORT_ORDER sort_order;
#ifdef WITH_RESOLVER
      Resolver* pResolver;
#endif
};

}
#endif /* __OPTIONS_H */

// vim: ai ts=3 sts=3 et sw=3 expandtab
