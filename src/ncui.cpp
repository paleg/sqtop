/*
 * Idea taken from iftop's ui.c
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#include <pthread.h>
//stringstream
#include <sstream>
#include <climits>
#include <algorithm>
//bind2nd, ptr_fun
#include <functional>

#include "ncui.hpp"
#include "Utils.hpp"
#include "strings.hpp"

#ifdef NCURSES_IN_SUBDIR
    #include <ncurses/ncurses.h>
#else
    #include <ncurses.h>
#endif

namespace sqtop {

using std::string;
using std::vector;
using std::endl;

std::ostream& operator<<( std::ostream& os, const Options::SPEED_MODE& mode )
{
   switch (mode) {
      case Options::SPEED_AVERAGE: os << "average only"; break;
      case Options::SPEED_CURRENT: os << "current only (if available)"; break;
      case Options::SPEED_MIXED: os << "both current and average"; break;
   }
   return os;
}

std::ostream& operator<<( std::ostream& os, const Options::SORT_ORDER& order ) {
   switch (order) {
      case Options::SORT_SIZE: os << "by size"; break;
      case Options::SORT_CURRENT_SPEED: os << "by current speed"; break;
      case Options::SORT_AVERAGE_SPEED: os << "by average speed"; break;
      case Options::SORT_MAX_TIME: os << "by max time"; break;
   }
   return os;
}

inline void operator++(Options::SPEED_MODE& mode, int) {
   if (mode >= Options::SPEED_CURRENT) {
      mode = Options::SPEED_MIXED;
   } else {
      mode = Options::SPEED_MODE(mode + 1);
   }
}

inline void operator++(Options::SORT_ORDER& order, int) {
   if (order >= Options::SORT_MAX_TIME) {
      order = Options::SORT_SIZE;
   } else {
      order = Options::SORT_ORDER(order + 1);
   }
}

#ifdef WITH_RESOLVER
std::ostream& operator<<( std::ostream& os, const Options::RESOLVE_MODE& mode )
{
   switch (mode) {
      case Options::SHOW_NAME: os << "host name only"; break;
      case Options::SHOW_IP: os << "host ip only"; break;
      case Options::SHOW_BOTH: os << "both host name and ip"; break;
   }
   return os;
}

inline void operator++(Options::RESOLVE_MODE& mode, int) {
   if (mode >= Options::SHOW_IP) {
      mode = Options::SHOW_BOTH;
   } else {
      mode = Options::RESOLVE_MODE(mode + 1);
   }
}

ncui::ncui(Options* pgOpts, Resolver* pResolver) : pResolver(pResolver) {
#else
ncui::ncui(Options* pgOpts) {
#endif
   pthread_mutexattr_init(&mattr);
   pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK);
   pthread_mutex_init(&tick_mutex, &mattr);
   foad = 0;
   pGlobalOpts = pgOpts;
   pGlobalOpts->showhelphint = false;
   helptimer = 0;
   helphint_time = 2;
   selected_index = 0;
   increment = 0;
   page_size = 10;
   y_coef = 0;
   start = 0;
   //ticks = 0;
   Opts = *pGlobalOpts;
}

ncui::~ncui() {
   pthread_mutexattr_destroy(&mattr);
   pthread_mutex_destroy(&tick_mutex);
}

void ncui::CursesInit() {
   (void) initscr();      /* initialize the curses library */
   keypad(stdscr, TRUE);  /* enable keyboard mapping */
   (void) nonl();         /* tell curses not to do NL->CR/NL on output */
   (void) cbreak();       /* take input chars one at a time, no wait for \n */
   (void) noecho();       /* don't echo input */
   halfdelay(2);
}

void ncui::CursesFinish() {
   clear();
   refresh();
   endwin();
}

void ncui::Tick() {
   debug = "";
   //ticks++;
   //AddWatch("ticks", Utils::itos(ticks));
   pthread_mutex_lock(&tick_mutex);
   if (pGlobalOpts->showhelphint && (time(NULL) - helptimer > helphint_time)) {
      pGlobalOpts->showhelphint = false;
   }
   Print();
   pthread_mutex_unlock(&tick_mutex);
}

void ncui::ShowHelpHint(string text) {
   if (error.empty()) {
      helphintmsg = text;
      helptimer = time(NULL);
      pGlobalOpts->showhelphint = true;
   }
}

void ncui::SetError(string text) {
   error = text;
   Tick();
}

void ncui::ClearError() {
   error.erase();
}

void ncui::SetStat(SquidStats stats) {
   sqstats = stats;
}

string ncui::b2s(bool value) {
   if (value) {
      return "(ON)";
   } else {
      return "(OFF)";
   }
}

void ncui::AddWatch(string prefix, string value) {
   if (!debug.empty())
      debug += ", ";
   debug += prefix + "=" + value;
}

string ncui::helpmsg() {
   std::stringstream ss;
   ss << "Output:" << endl;
   ss << " d - " << detail_help << " " << b2s(pGlobalOpts->detail) << endl;
   ss << " z - " << zero_help << " " << b2s(pGlobalOpts->zero) << endl;
   ss << " f - " << full_help << " " << b2s(pGlobalOpts->full) << endl;
   ss << " b - " << brief_help << " " << b2s(pGlobalOpts->brief) << endl;
   ss << " C - compact long urls to fit one line" << " " << b2s(pGlobalOpts->compactlongurls) << endl;
   ss << " c - " << compact_same_help << " " << b2s(pGlobalOpts->compactsameurls) << endl;
   ss << " Z - " << strip_user_domain_help << " " << b2s(pGlobalOpts->strip_user_domain) << endl;
   ss << " s - " << "speed showing mode (" << pGlobalOpts->speed_mode << ")" << endl;
   ss << " o - " << "connections sort order (" << pGlobalOpts->sort_order << ")" << endl;
   ss << " SPACE - stop refreshing " << b2s(!pGlobalOpts->do_refresh) << endl;
   ss << " UP/DOWN/PAGE_UP/PAGE_DOWN/HOME/END keys - scroll display" << endl;
   ss << " ENTER - toggle showing/hiding: urls (for connections), full details (for urls)" << endl;
   ss << endl;
   ss << "Filtering:" << endl;
   string hosts = "";
   if (!pGlobalOpts->Hosts.empty())
      hosts = " (" + Utils::JoinVector(pGlobalOpts->Hosts, ",") + ")";
   ss << " H - " << hosts_help << hosts << endl;
   string users = "";
   if (!pGlobalOpts->Users.empty())
      users = " (" + Utils::JoinVector(pGlobalOpts->Users, ",") + ")";
   ss << " u - " << users_help << users << endl;
   ss << endl;
   ss << "Squid connection:" << endl;
   ss << " h - " << host_help << " (" << pGlobalOpts->host << ")" << endl;
   ss << " p - " << port_help << " (" << Utils::itos(pGlobalOpts->port) << ")" << endl;
   string pass = "";
   if (!pGlobalOpts->pass.empty())
      pass = " (" + pGlobalOpts->pass + ")";
   ss << " P - " << passwd_help << pass << endl;
   ss << " r - " << refresh_interval_help << " (" << Utils::itos(pGlobalOpts->sleep_sec) << ")" << endl;
   ss << endl;
#ifdef WITH_RESOLVER
   ss << "Resolver (working in " << pResolver->ResolveMode() << " mode with "
                                 << pResolver->ResolveFunc() << " in "
                                 << pResolver->MaxThreads() << " threads):" << endl;
   ss << " n - " << dns_resolution_help << " " << b2s(pGlobalOpts->dns_resolution) << endl;
   ss << " S - " << strip_host_domain_help << " " << b2s(pGlobalOpts->strip_host_domain) << endl;
   ss << " R - " << "hosts showing mode (" << pGlobalOpts->resolve_mode << ")" << endl;
   ss << endl;
#endif
   ss << "General:" << endl;
   ss << " / - search for substring in hosts, usernames and urls" << endl;
   ss << " ? - this help" << endl;
   ss << " q - quit" << endl;
   ss << endl;
   ss << "Press '?' again to return" << endl;
   return ss.str();
}

int ncui::CompactLongLine(string &line) {
   int len = line.size();
   int coef = len / COLS;
   if (len > COLS*coef) {
      coef++;
   }
   if (( coef > 1) && Opts.compactlongurls) {
      int extra_chars = len - COLS + 3;
      line.erase(len/2-extra_chars/2, extra_chars);
      line.insert(len/2-extra_chars/2, "...");
      //line = line.substr(0, COLS-2);
      return 1;
   }
   return coef;
}

/* static */ bool ncui::Filter(SquidConnection scon, Options* pOpts) {
   if (((pOpts->Hosts.size() == 0) || Utils::IPMemberOf(pOpts->Hosts, scon.peer)) &&
       ((pOpts->Users.size() == 0) || Utils::UserMemberOf(pOpts->Users, scon.usernames))) {
         return false;
   }
   return true;
}

vector<SquidConnection> ncui::FilterConns(vector<SquidConnection> in) {
   vector<SquidConnection>::iterator it;
   it = std::remove_if( in.begin(), in.end(), std::bind2nd(std::ptr_fun(Filter), pGlobalOpts) );
   in.erase(it, in.end());
   return in;
}

bool ncui::SearchString(SquidConnection scon, string search_string) {
   bool ret = false;
   if (!search_string.empty()) {
      bool in_host = false;
      bool in_ip = (scon.peer.find(search_string) != string::npos);
#ifdef WITH_RESOLVER
      bool in_name = (scon.hostname.find(search_string) != string::npos);
      switch (pGlobalOpts->resolve_mode) {
         case Options::SHOW_NAME:
            in_host = in_name;
            break;
         case Options::SHOW_IP:
            in_host = in_ip;
            break;
         case Options::SHOW_BOTH:
            in_host = (in_name || in_ip);
            break;
      };
#else
      in_host = in_ip;
#endif
      bool in_users = Utils::SetFindSubstr(scon.usernames, search_string);
      ret = (in_host || in_users);
   }
   return ret;
}

vector<formattedline_t> ncui::FormatConnections(vector<SquidConnection> conns, int offset) {
//   AddWatch("orig", Utils::itos(sqconns.size()));
   vector<formattedline_t> result;
   int coef = 0;
   unsigned int y = offset;

   bool (*compareFunc)(SquidConnection, SquidConnection) = NULL;
   switch (pGlobalOpts->sort_order) {
      case Options::SORT_SIZE:
         compareFunc = &sqstat::CompareSIZE;
         break;
      case Options::SORT_CURRENT_SPEED:
         compareFunc = &sqstat::CompareCURRSPEED;
         break;
      case Options::SORT_AVERAGE_SPEED:
         compareFunc = &sqstat::CompareAVSPEED;
         break;
      case Options::SORT_MAX_TIME:
         compareFunc = &sqstat::CompareTIME;
         break;
   };

   if (compareFunc != NULL)
      sort(conns.begin(), conns.end(), compareFunc);

   for (vector<SquidConnection>::iterator it = conns.begin(); it != conns.end(); ++it) {
      SquidConnection scon = *it;
      Opts = *pGlobalOpts;
      if ((not pGlobalOpts->brief) && (Utils::MemberOf(collapsed, scon.peer))) {
         Opts.brief = true;
      } else if ((pGlobalOpts->brief) && (Utils::MemberOf(collapsed, scon.peer))) {
         Opts.brief = false;
      }
      string header_str = sqstat::ConnFormat(&Opts, scon);
      coef = CompactLongLine(header_str);
      result.push_back( formattedline_t(header_str, y, coef, scon, "") );
      if (SearchString(scon, search_string)) {
         if (selected_index > result.size() - 1)
            increment = -1;
         else
            increment = 1;
         selected_index = result.size() - 1;
         search_string.erase();
      }
      y += coef;

      if (((not pGlobalOpts->brief) && (not Utils::MemberOf(collapsed, scon.peer))) ||
          ((pGlobalOpts->brief) && (Utils::MemberOf(collapsed, scon.peer)))) {
         for (vector<UriStats>::iterator itu = scon.stats.begin(); itu != scon.stats.end(); ++itu) {
            UriStats ustat = *itu;
            Opts = *pGlobalOpts;
            if (Utils::MemberOf(detailed, ustat.id)) {
               Opts.detail = true;
               Opts.full = true;
               Opts.compactlongurls = false;
            }
            string url_str = sqstat::StatFormat(&Opts, scon, ustat);
            coef = CompactLongLine(url_str);
            result.push_back( formattedline_t(url_str, y, coef, scon, ustat.id));
            if ((!search_string.empty()) && (ustat.uri.find(search_string) != string::npos)) {
               if (selected_index > result.size() - 1)
                  increment = -1;
               else
                  increment = 1;
               selected_index = result.size() - 1;
               search_string.erase();
            }
            y += coef;
         }
      }
      if ((not pGlobalOpts->brief) || (Utils::MemberOf(collapsed, scon.peer))) {
         result.push_back( formattedline_t(y) );
         if (selected_index == result.size() - 1) {
            // we dont want to highligth empty line, so jump to next/prev (depends on increment) line
            int j = 1;
            if (increment < 0) j = -1;
            selected_index += j;
         }
         y++;
      }
   }
   if ((not pGlobalOpts->brief) && (result.size() > 0))
      result.pop_back();
   if ((result.size() > 0) && (result[result.size()-1].new_line))
      result.pop_back();
   if (selected_index > result.size() - 1) {
      selected_index = result.size() - 1;
   }
   if (result.size() > 0)
      result[selected_index].highlighted = true;

   if (!search_string.empty()) {
      ShowHelpHint("Failed to find '" + search_string + "'");
      search_string.erase();
   }
   return result;
}

void ncui::Print() {
   if (pGlobalOpts->freeze) return;
   clear();

   std::stringstream header_r, header_l, active_conn, active_ips, average_speed, status;
   vector<formattedline_t> to_print;

   int offset = 3;

   Opts = *pGlobalOpts;
   // FormatConnections can set helphintmsg so it should run before header formatting
   if (pGlobalOpts->Hosts.size() != 0) {
      string hosts = "Filtering by: " + Utils::JoinVector(pGlobalOpts->Hosts, ", ");
      int coef = CompactLongLine(hosts);
      int x = COLS/2 - hosts.size()/2;
      if (coef > 1) x = 0;
      mvaddstr(offset-2, x, hosts.c_str());
      offset += coef;
   }
   if (pGlobalOpts->Users.size() != 0) {
      string users = "Filtering by users: " + Utils::JoinVector(pGlobalOpts->Users, ", ");
      int coef = CompactLongLine(users);
      int x = COLS/2 - users.size()/2;
      if (coef > 1) x = 0;
      mvaddstr(offset-2, x, users.c_str());
      offset += coef;
   }
   if (!pGlobalOpts->do_refresh) {
      string msg = "Statistics refreshing disabled";
      mvaddstr(offset-2, COLS/2 - msg.size()/2, msg.c_str());
      offset++;
   }

   vector<SquidConnection> sqconns_filtered = FilterConns(sqstats.connections);

   if (pGlobalOpts->compactsameurls)
      sqstat::CompactSameUrls(sqconns_filtered);
   to_print = FormatConnections(sqconns_filtered, offset);

   // HEADER: print help hint
   if (pGlobalOpts->showhelphint) {
      std::stringstream helpstr;
      helpstr << " " << helphintmsg << " ";
      mvaddstr(0, 0, helpstr.str().c_str());
      mvchgat(0, 0, helpstr.str().size(), A_REVERSE, 0, NULL);
   // or print any error
   } else if (!error.empty()) {
      error = Utils::replace(error, "\n", " ");
      mvaddstr(0, COLS/2 - error.size()/2, error.c_str());
      mvchgat(0, COLS/2 - error.size()/2, error.size(), A_REVERSE, 0, NULL);
   // or print some info
   } else {
      header_r << "Connected to " << pGlobalOpts->host << ":" << pGlobalOpts->port;
      header_l << PACKAGE_NAME << "-" << VERSION;
      mvaddstr(0, 0, header_r.str().c_str());
      mvaddstr(0, COLS-header_l.str().size(), header_l.str().c_str());
   }

   mvhline(offset - 2, 0, 0, COLS);

   unsigned int max_y = LINES - 1; // screen height

   // Connections list
   if (!pGlobalOpts->showhelp) {
      if (to_print.size() > 0) {
         // some magic to determine visible part of connections according to selected line
         if (((to_print[selected_index].y) < (y_coef + offset)) && (increment < 0)) {
            start = selected_index;
            y_coef = to_print[selected_index].y - offset;
         } else if ((((to_print[selected_index].y + to_print[selected_index].coef) >  (max_y + y_coef - 2)) && (increment>0)) ||
                    (start > to_print.size())) {
            if (to_print[selected_index].y + to_print[selected_index].coef + 2 > max_y) {
               y_coef = to_print[selected_index].y + to_print[selected_index].coef - max_y + 2;
            }
            else {
               y_coef = 0;
            }
            for (start=0; start < selected_index; start++) {
               if (to_print[start].new_line) continue;
               if (to_print[start].y > y_coef+offset-1) break;
            }
         }

         unsigned int y;
         for (vector<formattedline_t>::iterator it = to_print.begin()+start; it != to_print.end(); ++it) {
            formattedline_t fline = *it;
            if (fline.new_line) {
               continue;
            }

            y = fline.y - y_coef;

            if ((y + fline.coef - 1) > (max_y - 2)) break;

            mvaddstr(y, 0, fline.str.c_str());

            if (fline.highlighted) {
               //AddWatch("id", fline.id);
               //AddWatch("peer", fline.sconn.peer);
               std::string::size_type found;
               string temps;
               for (unsigned int st = 0; st < fline.coef; st++) {
                  temps = fline.str.substr(st*COLS, st*COLS + COLS);
                  found = temps.find_first_not_of(" ");
                  mvchgat(y+st, found, temps.size()-found, A_REVERSE, 0, NULL);
               }
               /*AddWatch("found", Utils::itos(found));
               AddWatch("size", Utils::itos(fline.str.size()));
               AddWatch("h_coef", Utils::itos(fline.coef));*/
               selected_t = fline;
            }
         }
         /*AddWatch("incr", Utils::itos(increment));
         AddWatch("max_y", Utils::itos(max_y));
         AddWatch("y_coef", Utils::itos(y_coef));
         AddWatch("start", Utils::itos(start));
         AddWatch("selec_idx", Utils::itos(selected_index));
         AddWatch("h_y", Utils::itos(to_print[selected_index].y));
         AddWatch("to_p[start].y", Utils::itos(to_print[start].y));*/
      }
   } else {
      mvaddstr(offset, 0, helpmsg().c_str());
   }

   // FOOTER
   string speed = sqstat::SpeedsFormat(pGlobalOpts->speed_mode, sqstats.av_speed, sqstats.curr_speed);
   speed[0] = toupper(speed[0]);
   status << speed << "\t\t";
   status << "Active hosts: " << sqstats.connections.size() << "\t\t";
   status << "Active connections: " << sqstats.total_connections << "\t\t";
   //status << "Get time: " << sqstats.get_time << "\t";
   //status << "Process time: " << sqstats.process_time << "\t";

   mvhline(max_y-1, 0, 0, COLS);

   if (debug.empty())
      mvaddstr(max_y, 0, status.str().c_str());
   else
      mvaddstr(max_y, COLS - 1 - debug.size(), debug.c_str());

   move(LINES - 1, COLS - 1);
   refresh();
}

void ncui::ToggleAction() {
   if (selected_t.id != "") {
      if (Utils::MemberOf(detailed, selected_t.id))
         Utils::VectorDeleteStr(detailed, selected_t.id);
      else
         detailed.push_back(selected_t.id);
      return;
   }
   else {
      if (Utils::MemberOf(collapsed, selected_t.sconn.peer))
         Utils::VectorDeleteStr(collapsed, selected_t.sconn.peer);
      else
         collapsed.push_back(selected_t.sconn.peer);
   }
}

void ncui::Loop() {
   string inp;
   int i;
   bool dotick;
   std::stringstream ss;
   while(foad == 0) {
      dotick = true;
      i = getch();
      switch (i) {
         case 'd':
            if (pGlobalOpts->detail) {
               ShowHelpHint("Detailed output OFF");
            } else {
               ShowHelpHint("Detailed output ON");
            }
            pGlobalOpts->detail = !pGlobalOpts->detail;
            break;
         case 'z':
            if (pGlobalOpts->zero) {
               ShowHelpHint("Showing zero values OFF");
            } else {
               ShowHelpHint("Showing zero values ON");
            }
            pGlobalOpts->zero = !pGlobalOpts->zero;
            break;
         case 'f':
            if (pGlobalOpts->full) {
               ShowHelpHint("Showing full details OFF");
            } else {
               ShowHelpHint("Showing full details ON");
               pGlobalOpts->detail = true;
            }
            pGlobalOpts->full = !pGlobalOpts->full;
            break;
        case 'P':
            pGlobalOpts->freeze = true;
            try {
               inp = EdLine(0, "Cachemgr password", pGlobalOpts->pass);
               pGlobalOpts->pass = inp;
               sqstats.connections.clear();
            } catch (const std::invalid_argument& error) {
               ShowHelpHint(error.what());
            }
            pGlobalOpts->freeze = false;
            break;
         case 'H':
            pGlobalOpts->freeze = true;
            try {
               inp = EdLine(0, "Hosts to show", Utils::JoinVector(pGlobalOpts->Hosts, ","));
               pGlobalOpts->Hosts = Utils::SplitString(inp, ",");
            } catch (const std::invalid_argument& error) {
               ShowHelpHint(error.what());
            }
            pGlobalOpts->freeze = false;
            break;
         case 'u':
            pGlobalOpts->freeze = true;
            try {
               inp = EdLine(0, "Users to show", Utils::JoinVector(pGlobalOpts->Users, ","));
               pGlobalOpts->Users = Utils::SplitString(inp, ",");
            } catch (const std::invalid_argument& error) {
               ShowHelpHint(error.what());
            }
            pGlobalOpts->freeze = false;
            break;
         case 'q':
            foad = 1;
            break;
         case ' ':
            pGlobalOpts->do_refresh = !pGlobalOpts->do_refresh;
            break;
         case 'b':
            collapsed.clear();
            if (pGlobalOpts->brief) {
               ShowHelpHint("Brief output OFF");
            } else {
               ShowHelpHint("Brief output ON");
            }
            pGlobalOpts->brief = !pGlobalOpts->brief;
            break;
         case 'C':
            if (pGlobalOpts->compactlongurls) {
               ShowHelpHint("Compacting long urls OFF");
            } else {
               ShowHelpHint("Compacting long urls ON");
            }
            pGlobalOpts->compactlongurls = !pGlobalOpts->compactlongurls;
            break;
         case 'c':
            if (pGlobalOpts->compactsameurls) {
               ShowHelpHint("Compacting same urls OFF");
            } else {
               ShowHelpHint("Compacting same urls ON");
            }
            pGlobalOpts->compactsameurls = !pGlobalOpts->compactsameurls;
            break;
         case 'Z':
            if (pGlobalOpts->strip_user_domain) {
               ShowHelpHint("Username domain part stripping OFF");
            } else {
               ShowHelpHint("Username domain part stripping ON");
            }
            pGlobalOpts->strip_user_domain = !pGlobalOpts->strip_user_domain;
            break;
#ifdef WITH_RESOLVER
         case 'n':
            if (pGlobalOpts->dns_resolution) {
               ShowHelpHint("Hostname lookups OFF");
            } else {
               ShowHelpHint("Hostname lookups ON");
            }
            pGlobalOpts->dns_resolution = !pGlobalOpts->dns_resolution;
            break;
         case 'S':
            if (pGlobalOpts->strip_host_domain) {
               ShowHelpHint("Hostname domain part stripping OFF");
            } else {
               ShowHelpHint("Hostname domain part stripping ON");
            }
            pGlobalOpts->strip_host_domain = !pGlobalOpts->strip_host_domain;
            break;
#endif
         case 's':
            pGlobalOpts->speed_mode++;
            ss.str("");
            ss << "Speed showing mode - " << pGlobalOpts->speed_mode;
            ShowHelpHint(ss.str());
            break;
         case 'o':
            pGlobalOpts->sort_order++;
            ss.str("");
            ss << "Connections sort order - " << pGlobalOpts->sort_order;
            ShowHelpHint(ss.str());
            break;
#ifdef WITH_RESOLVER
         case 'R':
            pGlobalOpts->resolve_mode++;
            ss.str("");
            ss << "Hosts showing mode - " << pGlobalOpts->resolve_mode;
            ShowHelpHint(ss.str());
            break;
#endif
         case 'h':
            pGlobalOpts->freeze = true;
            try {
               inp = EdLine(0, "Squid host", pGlobalOpts->host);
               if (inp != "") pGlobalOpts->host = inp;
               sqstats.connections.clear();
            } catch (const std::invalid_argument& error) {
               ShowHelpHint(error.what());
            }
            pGlobalOpts->freeze = false;
            break;
         case '/':
            pGlobalOpts->freeze = true;
            try {
               inp = EdLine(0, "Search for", "");
               if (inp != "") search_string = inp;
            } catch (const std::invalid_argument& error) {
               ShowHelpHint(error.what());
            }
            pGlobalOpts->freeze = false;
            break;
         case 'p':
            pGlobalOpts->freeze = true;
            try {
               inp = EdLine(0, "Squid port", Utils::itos(pGlobalOpts->port));
               if (inp != "") {
                  long int port = Utils::stol(inp);
                  pGlobalOpts->port = port;
                  sqstats.connections.clear();
               }
            } catch (const std::exception& error) {
               ShowHelpHint(error.what());
            }
            pGlobalOpts->freeze = false;
            break;
         case 'r':
            pGlobalOpts->freeze = true;
            try {
               inp = EdLine(0, "Refresh interval (sec)", Utils::itos(pGlobalOpts->sleep_sec));
               long int sec = Utils::stol(inp);
               if (sec > 0)
                  pGlobalOpts->sleep_sec = sec;
               else
                  ShowHelpHint("Invalid refresh interval");
            } catch (const std::exception& error) {
               ShowHelpHint(error.what());
            }
            pGlobalOpts->freeze = false;
            break;
         case KEY_DOWN:
            selected_index++;
            increment = 1;
            break;
         case KEY_UP:
            if (selected_index > 0) {
               selected_index--;
               increment = -1;
            }
            break;
         case KEY_ENTER:
         case '\r':
            ToggleAction();
            break;
         case KEY_NPAGE:
            selected_index += page_size;
            increment = page_size;
            break;
         case KEY_PPAGE:
            if (selected_index > page_size) {
               selected_index -= page_size;
               increment = -page_size;
            }
            else {
               selected_index = 0;
            }
            break;
         case KEY_HOME:
            selected_index = 0;
            increment = -1;
            break;
         case KEY_END:
            selected_index = UINT_MAX;
            increment = 1;
            break;
         case ERR:
            dotick = false;
            break;
         case '?':
            pGlobalOpts->showhelp = !pGlobalOpts->showhelp;
            break;
         default:
            ShowHelpHint("Press ? for help");
            break;
      }
      if (dotick) Tick();
   }
}

int ncui::min(const int a, const int b) {
    return a < b ? a : b;
}

string ncui::EdLine(int linenum, string prompt, string initial) {
    int c;
    unsigned int pos, xstart, off=0;
    string str = "";

    xstart = prompt.size() + 2;

    if (!initial.empty()) {
        str = initial;
    }

    pos = str.size();

    do {
        c = getch();
        switch (c) {
            case KEY_DL:
            case 21:    /* ^U */
                str = "";
                pos = 0;
                break;

            case KEY_LEFT:
                if (pos == 0)
                  beep();
                else
                  --pos;
                break;

            case KEY_RIGHT:
                ++pos;
                if (pos > str.size()) {
                    beep();
                    pos = str.size();
                }
                break;

            case KEY_HOME:
            case 1:         /* ^A */
                pos = 0;
                break;

            case KEY_END:
            case 5:         /* ^E */
                pos = str.size();
                break;

            case KEY_DC:
                if (pos == str.size())
                    beep();
                else
                    str.erase(pos, 1);
                break;

            case KEY_BACKSPACE:
                if (pos == 0)
                    beep();
                else {
                    str.erase(pos-1, 1);
                    --pos;
                }
                break;

            case ERR:
                break;

            default:
                if (isprint(c) && c != '\t') {
                    str.insert(pos++, 1, (char)c);
                } else
                    beep();
                break;
        }
        /* figure out the offset to use for the string */
        off = 0;
        if (pos > COLS - xstart - 1)
            off = pos - (COLS - xstart - 1);

        /* display the string */
        mvaddstr(linenum, 0, prompt.c_str());
        addstr("> ");
        addnstr(str.c_str() + off, min(str.size() + off, COLS - xstart - 1));
        clrtoeol();
        move(linenum, xstart + pos - off);
        refresh();

    } while (c != KEY_ENTER && c != '\r' && c != '\x1b' && c != 7 /* ^G */);

    if (c == KEY_ENTER || c == '\r') {
        /* Success */
        return str;
    } else if (c == '\x1b') {
       throw std::invalid_argument("Input was canceled");
    } else {
        throw std::invalid_argument("Invalid input");
    }
}

}
// vim: ai ts=3 sts=3 et sw=3 expandtab
