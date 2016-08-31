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
      Options() :
         host("127.0.0.1"), port(3128), pass(""),
         brief(false), full(false), zero(false), detail(false),
         ui(true),
         compactlongurls(true), compactsameurls(true),
         strip_user_domain(true),
         freeze(false), do_refresh(true), sleep_sec(2),
         showhelp(false), showhelphint(false),
         speed_mode(SPEED_MIXED), sort_order(SORT_SIZE)
#ifdef WITH_RESOLVER
         ,dns_resolution(true),
         strip_host_domain(true),
         resolve_mode(SHOW_BOTH)
#endif
      {};

      std::string host; int port; std::string pass;
      bool brief; bool full; bool zero; bool detail;
      bool ui;
      bool compactlongurls; bool compactsameurls;
      bool strip_user_domain;
      bool freeze; bool do_refresh; int sleep_sec;
      bool showhelp; bool showhelphint;

      enum SPEED_MODE {
         SPEED_MIXED,
         SPEED_AVERAGE,
         SPEED_CURRENT
      };
      SPEED_MODE speed_mode;

      enum SORT_ORDER {
         SORT_SIZE,
         SORT_CURRENT_SPEED,
         SORT_AVERAGE_SPEED,
         SORT_MAX_TIME
      };
      SORT_ORDER sort_order;
#ifdef WITH_RESOLVER
      bool dns_resolution;
      bool strip_host_domain;

      enum RESOLVE_MODE {
         SHOW_BOTH,
         SHOW_NAME,
         SHOW_IP
      };
      RESOLVE_MODE resolve_mode;
#endif

      std::vector<std::string> Hosts;
      std::vector<std::string> Users;
};

}
#endif /* __OPTIONS_H */

// vim: ai ts=3 sts=3 et sw=3 expandtab
