/*
 * Idea taken from iftop's ui.c
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#include <ncurses.h>
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

using std::string;
using std::vector;
using std::endl;

std::ostream& operator<<( std::ostream& os, const SPEED_MODE& mode )
{
   switch (mode) {
      case SPEED_AVERAGE: os << "average only"; break;
      case SPEED_CURRENT: os << "current only (if available)"; break;
      case SPEED_MIXED: os << "both current and average"; break;
   }
   return os;
}

std::ostream& operator<<( std::ostream& os, const SORT_ORDER& order ) {
   switch (order) {
      case SORT_SIZE: os << "by size"; break;
      case SORT_CURRENT_SPEED: os << "by current speed"; break;
      case SORT_AVERAGE_SPEED: os << "by average speed"; break;
      case SORT_MAX_TIME: os << "by max time"; break;
   }
   return os;
}

inline void operator++(SPEED_MODE& mode, int) {
   mode = SPEED_MODE(mode + 1);
}

inline void operator++(SORT_ORDER& order, int) {
   order = SORT_ORDER(order + 1);
}

ncui::ncui(options_c *gopts) {
   pthread_mutexattr_init(&mattr);
   pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK);
   pthread_mutex_init(&tick_mutex, &mattr);
   foad = 0;
   global_opts = gopts;
   global_opts->showhelphint = false;
   helptimer = 0;
   helphint_time = 2;
   selected_index = 0;
   increment = 0;
   page_size = 10;
   y_coef = 0;
   start = 0;
   //ticks = 0;
}

void ncui::curses_init() {
   (void) initscr();      /* initialize the curses library */
   keypad(stdscr, TRUE);  /* enable keyboard mapping */
   (void) nonl();         /* tell curses not to do NL->CR/NL on output */
   (void) cbreak();       /* take input chars one at a time, no wait for \n */
   (void) noecho();       /* don't echo input */
   halfdelay(2);
}

void ncui::finish() {
   clear();
   refresh();
   endwin();
}

ncui::~ncui() {
   pthread_mutexattr_destroy(&mattr); 
   pthread_mutex_destroy(&tick_mutex);
}

void ncui::tick() {
   debug = "";
   //ticks++;
   //addwatch("ticks", Utils::itos(ticks));
   pthread_mutex_lock(&tick_mutex);
   if (global_opts->showhelphint && (time(NULL) - helptimer > helphint_time)) {
      global_opts->showhelphint = false;
   }
   print();
   pthread_mutex_unlock(&tick_mutex);
}

void ncui::showhelphint(string text) {
   if (error.empty()) {
      helphintmsg = text;
      helptimer = time(NULL);
      global_opts->showhelphint = true;
   }
}

void ncui::seterror(string text) {
   error = text;
   tick();
}

void ncui::clearerror() {
   error.erase();
}

void ncui::set_speeds(long aspeed, long cspeed) {
   av_speed = aspeed;
   curr_speed = cspeed;
}

void ncui::set_active_conn(int conn) {
   act_conn = conn;
}

void ncui::set_stat(std::vector <SQUID_Connection> stat) {
   sqconns = stat;
}

string ncui::b2s(bool value) {
   if (value) {
      return "(ON)";
   } else {
      return "(OFF)";
   }
}

void ncui::addwatch(string prefix, string value) {
   if (!debug.empty()) 
      debug += ", ";
   debug += prefix + "=" + value;
}

string ncui::helpmsg() {
   std::stringstream ss;
   ss << "Output:" << endl;
   ss << " d - " << detail_help << " " << b2s(global_opts->detail) << endl;
   ss << " z - " << zero_help << " " << b2s(global_opts->zero) << endl;
   ss << " f - " << full_help << " " << b2s(global_opts->full) << endl;
   ss << " b - " << brief_help << " " << b2s(global_opts->brief) << endl;
   ss << " C - compact long urls to fit one line" << " " << b2s(global_opts->compactlongurls) << endl;
   ss << " c - " << nocompact_same_help << " " << b2s(global_opts->nocompactsameurls) << endl;
   ss << " s - " << "speed showing mode (" << global_opts->speed_mode << ")" << endl;
   ss << " o - " << "connections sort order (" << global_opts->sort_order << ")" << endl;
   ss << " SPACE - stop refreshing " << b2s(!global_opts->do_refresh) << endl;
   ss << " UP/DOWN/PAGE_UP/PAGE_DOWN/HOME/END keys - scroll display" << endl;
   ss << " ENTER - toggle showing/hiding: urls (for connections), full details (for urls)" << endl;
   ss << endl;
   ss << "Filtering:" << endl;
   string hosts = "";
   if (!global_opts->Hosts.empty())
      hosts = " (" + Utils::joinVector(global_opts->Hosts, ",") + ")";
   ss << " H - " << hosts_help << hosts << endl;
   string users = "";
   if (!global_opts->Users.empty())
      users = " (" + Utils::joinVector(global_opts->Users, ",") + ")";
   ss << " u - " << users_help << users << endl;
   ss << endl;
   ss << "Squid connection:" << endl;
   ss << " h - " << host_help << " (" << global_opts->host << ")" << endl;
   ss << " p - " << port_help << " (" << Utils::itos(global_opts->port) << ")" << endl;
   string pass = "";
   if (!global_opts->pass.empty())
      pass = " (" + global_opts->pass + ")";
   ss << " P - " << passwd_help << pass << endl;
   ss << " r - " << refresh_interval_help << " (" << Utils::itos(global_opts->sleep_sec) << ")" << endl;
   ss << endl;
   ss << "General:" << endl;
   ss << " / - search for substring in IPs, usernames and urls" << endl;
   ss << " ? - this help" << endl;
   ss << " q - quit" << endl;
   ss << endl;
   ss << "Press '?' again to return" << endl;
   return ss.str();
}

int ncui::compactlongline(string &line, options_c *opts) {
   int len = line.size();
   int coef = len / COLS;
   if (len > COLS*coef) {
      coef++;
   }
   if (( coef > 1) && opts->compactlongurls) {
      int extra_chars = len - COLS + 3;
      line.erase(len/2-extra_chars/2, extra_chars);
      line.insert(len/2-extra_chars/2, "...");
      //line = line.substr(0, COLS-2);
      return 1;
   }
   return coef;
}

bool toFilter(SQUID_Connection scon, options_c *options) {
   if (((options->Hosts.size() == 0) || Utils::IPmemberOf(options->Hosts, scon.peer)) &&
       ((options->Users.size() == 0) || Utils::UserMemberOf(options->Users, scon.usernames))) {
         return false;
   }
   return true;
}

vector<SQUID_Connection> ncui::filter_conns(vector<SQUID_Connection> in) {
   vector<SQUID_Connection>::iterator it;
   it = std::remove_if( in.begin(), in.end(), std::bind2nd(std::ptr_fun(toFilter), global_opts));
   in.erase(it, in.end());
   return in;
}

formattedline_t make_result(string str, int y, int coef, SQUID_Connection sconn, string id) {
   formattedline_t t;
   t.str = str;
   t.y = y;
   t.coef = coef;
   t.sconn = sconn;
   t.id = id;
   t.new_line = false;
   t.highlighted = false;
   return t;
}

formattedline_t make_new_line(int y) {
   formattedline_t t;
   t.new_line = true;
   t.y = y;
   t.coef = 1;
   t.highlighted = false;
   return t;
}

vector <formattedline_t> ncui::format_connections_str(vector<SQUID_Connection> conns, int offset) {
//   addwatch("orig", Utils::itos(sqconns.size()));
   vector <formattedline_t> result;
   int coef = 0;
   unsigned int y = offset;

   options_c opts;

   bool (*compareFunc)(SQUID_Connection, SQUID_Connection) = NULL;
   switch (global_opts->sort_order) {
      case (SORT_SIZE):
         compareFunc = &sqstat::CompareSIZE;
         break;
      case SORT_CURRENT_SPEED:
         compareFunc = &sqstat::CompareCURRSPEED;
         break;
      case SORT_AVERAGE_SPEED:
         compareFunc = &sqstat::CompareAVSPEED;
         break;
      case SORT_MAX_TIME:
         compareFunc = &sqstat::CompareTIME;
         break;
   };

   if (compareFunc != NULL)
      sort(conns.begin(), conns.end(), compareFunc);

   for (vector<SQUID_Connection>::iterator it = conns.begin(); it != conns.end(); ++it) {
      SQUID_Connection scon = *it;
      if ((not global_opts->brief) && (Utils::memberOf(collapsed, scon.peer))) {
         opts = global_opts->copy();
         opts.brief = true;
      } else if ((global_opts->brief) && (Utils::memberOf(collapsed, scon.peer))) {
         opts = global_opts->copy();
         opts.brief = false;
      } else {
         opts = *global_opts;
      }
      string header_str = sqstat::conn_format(&opts, scon);
      coef = compactlongline(header_str, global_opts);
      result.push_back(make_result(header_str, y, coef, scon, ""));
      if ((!search_string.empty()) && 
          ((scon.peer.find(search_string) != string::npos) || (Utils::VectorFindSubstr(scon.usernames, search_string)))) {
         if (selected_index > result.size() - 1)
            increment = -1;
         else
            increment = 1;
         selected_index = result.size() - 1;
         search_string.erase();
      }
      y += coef;

      if (((not global_opts->brief) && (not Utils::memberOf(collapsed, scon.peer))) ||
          ((global_opts->brief) && (Utils::memberOf(collapsed, scon.peer)))) {
         for (vector<Uri_Stats>::iterator itu = scon.stats.begin(); itu != scon.stats.end(); ++itu) {
            Uri_Stats ustat = *itu;
            if (Utils::memberOf(detailed, ustat.id)) {
               opts = global_opts->copy();
               opts.detail = true;
               opts.full = true;
               opts.compactlongurls = false;
            } else {
               opts = *global_opts;
            }
            string url_str = sqstat::stat_format(&opts, scon, ustat);
            coef = compactlongline(url_str, &opts);
            result.push_back(make_result(url_str, y, coef, scon, ustat.id));
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
      if ((not global_opts->brief) || (Utils::memberOf(collapsed, scon.peer))) {
         result.push_back(make_new_line(y));
         if (selected_index == result.size() - 1) {
            // we dont want to highligth empty line, so jump to next/prev (depends on increment) line
            int j = 1;
            if (increment < 0) j = -1;
            selected_index += j;
         }
         y++;
      }
   }
   if ((not global_opts->brief) && (result.size() > 0))
      result.pop_back();
   if ((result.size() > 0) && (result[result.size()-1].new_line))
      result.pop_back();
   if (selected_index > result.size() - 1) {
      selected_index = result.size() - 1;
   } else if (selected_index < 0) {
      selected_index = 0;
   }
   if (result.size() > 0)
      result[selected_index].highlighted = true;

   if (!search_string.empty()) {
      showhelphint("Failed to find '" + search_string + "'");
      search_string.erase();
   }
   return result;
}

void ncui::print() {
   if (global_opts->freeze) return;
   clear();

   std::stringstream header_r, header_l, active_conn, active_ips, average_speed, status;
   vector <formattedline_t> to_print;

   int offset = 3;

   // format_connections_str can set helphintmsg so it should run before header formatting
   if (error.empty()) {
      if (global_opts->Hosts.size() != 0) {
         string hosts = "Filtering by: " + Utils::joinVector(global_opts->Hosts, ", ");
         int coef = compactlongline(hosts, global_opts);
         int x = COLS/2 - hosts.size()/2;
         if (coef > 1) x = 0;
         mvaddstr(offset-2, x, hosts.c_str());
         offset += coef;
      }
      if (global_opts->Users.size() != 0) {
         string users = "Filtering by users: " + Utils::joinVector(global_opts->Users, ", ");
         int coef = compactlongline(users, global_opts);
         int x = COLS/2 - users.size()/2;
         if (coef > 1) x = 0;
         mvaddstr(offset-2, x, users.c_str());
         offset += coef;
      }
      if (!global_opts->do_refresh) {
         string msg = "Statistics refreshing disabled";
         mvaddstr(offset-2, COLS/2 - msg.size()/2, msg.c_str());
         offset++;
      }

      vector<SQUID_Connection> sqconns_filtered = filter_conns(sqconns);

      if (!global_opts->nocompactsameurls)
         sqstat::compactSameUrls(sqconns_filtered);
      to_print = format_connections_str(sqconns_filtered, offset);
   }

   // HEADER: print help hint
   if (global_opts->showhelphint) {
      std::stringstream helpstr;
      helpstr << " " << helphintmsg << " ";
      mvaddstr(0, 0, helpstr.str().c_str());
      mvchgat(0, 0, helpstr.str().size(), A_REVERSE, 0, NULL);
   // or print any error
   } else if (!error.empty()) {
      mvaddstr(0, 0, error.c_str());
      return;
   // or print some info
   } else {
      header_r << "Connected to " << global_opts->host << ":" << global_opts->port;
      header_l << PACKAGE_NAME << "-" << VERSION << " " << copyright;
      mvaddstr(0, 0, header_r.str().c_str());
      mvaddstr(0, COLS-header_l.str().size(), header_l.str().c_str());
   }

   mvhline(offset - 2, 0, 0, COLS);

   if (error.empty()) {
      unsigned int max_y = LINES - 1; // screen height

      // Connections list
      if (!global_opts->showhelp) {
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
                  //addwatch("id", fline.id);
                  //addwatch("peer", fline.sconn.peer);
                  std::string::size_type found;
                  string temps;
                  for (unsigned int st = 0; st < fline.coef; st++) {
                     temps = fline.str.substr(st*COLS, st*COLS + COLS);
                     found = temps.find_first_not_of(" ");
                     mvchgat(y+st, found, temps.size()-found, A_REVERSE, 0, NULL);
                  }
                  /*addwatch("found", Utils::itos(found));
                  addwatch("size", Utils::itos(fline.str.size()));
                  addwatch("h_coef", Utils::itos(fline.coef));*/
                  selected_t = fline;
               }
            }
            /*addwatch("incr", Utils::itos(increment));
            addwatch("max_y", Utils::itos(max_y));
            addwatch("y_coef", Utils::itos(y_coef));
            addwatch("start", Utils::itos(start));
            addwatch("selec_idx", Utils::itos(selected_index));
            addwatch("h_y", Utils::itos(to_print[selected_index].y));
            addwatch("to_p[start].y", Utils::itos(to_print[start].y));*/
         }
      } else {
         mvaddstr(offset, 0, helpmsg().c_str());
      }

      // FOOTER
      string speed = sqstat::speeds_format(global_opts->speed_mode, av_speed, curr_speed);
      speed[0] = toupper(speed[0]);
      status << speed << "\t\t";
      status << "Active IPs: " << sqconns.size() << "\t\t";
      status << "Active connections: " << act_conn << "\t\t";

      mvhline(max_y-1, 0, 0, COLS);

      if (debug.empty())
         mvaddstr(max_y, 0, status.str().c_str());
      else
         mvaddstr(max_y, COLS - 1 - debug.size(), debug.c_str());
   }
   move(LINES - 1, COLS - 1);
   refresh();
}

void ncui::toggle_action() {
   if (selected_t.id != "") {
      if (Utils::memberOf(detailed, selected_t.id))
         Utils::VectorDeleteStr(detailed, selected_t.id);
      else
         detailed.push_back(selected_t.id);
      return;
   }
   else {
      if (Utils::memberOf(collapsed, selected_t.sconn.peer))
         Utils::VectorDeleteStr(collapsed, selected_t.sconn.peer);
      else
         collapsed.push_back(selected_t.sconn.peer);
   }
}

void ncui::loop() {
   string inp;
   int i;
   bool dotick;
   std::stringstream ss;
   while(foad == 0) {
      dotick = true;
      i = getch();
      switch (i) {
         case 'd':
            if (global_opts->detail) {
               showhelphint("Detailed output OFF");
            } else {
               showhelphint("Detailed output ON");
            }
            global_opts->detail = !global_opts->detail;
            break;
         case 'z':
            if (global_opts->zero) {
               showhelphint("Showing zero values OFF");
            } else {
               showhelphint("Showing zero values ON");
            }
            global_opts->zero = !global_opts->zero;
            break;
         case 'f':
            if (global_opts->full) {
               showhelphint("Showing full details OFF");
            } else {
               showhelphint("Showing full details ON");
               global_opts->detail = true;
            }
            global_opts->full = !global_opts->full;
            break;
        case 'P':
            global_opts->freeze = true;
            try {
               inp = edline(0, "Cachemgr password", global_opts->pass);
               global_opts->pass = inp;
            }
            catch (string &s) {
               showhelphint(s);
            }
            global_opts->freeze = false;
            break;
         case 'H':
            global_opts->freeze = true;
            try {
               inp = edline(0, "Hosts to show", Utils::joinVector(global_opts->Hosts, ","));
               global_opts->Hosts = Utils::splitString(inp, ",");
            }
            catch (string &s) {
               showhelphint(s);
            }
            global_opts->freeze = false;
            break;
         case 'u':
            global_opts->freeze = true;
            try {
               inp = edline(0, "Users to show", Utils::joinVector(global_opts->Users, ","));
               global_opts->Users = Utils::splitString(inp, ",");
            }
            catch (string &s) {
               showhelphint(s);
            }
            global_opts->freeze = false;
            break;
         case 'q':
            foad = 1;
            break;
         case ' ':
            global_opts->do_refresh = !global_opts->do_refresh;
            break;
         case 'b':
            collapsed.clear();
            if (global_opts->brief) {
               showhelphint("Brief output OFF");
            } else {
               showhelphint("Brief output ON");
            }
            global_opts->brief = !global_opts->brief;
            break;
         case 'C':
            if (global_opts->compactlongurls) {
               showhelphint("Compacting long urls OFF");
            } else {
               showhelphint("Compacting long urls ON");
            }
            global_opts->compactlongurls = !global_opts->compactlongurls;
            break;
         case 'c':
            if (!global_opts->nocompactsameurls) {
               showhelphint("Compacting same urls OFF");
            } else {
               showhelphint("Compacting same urls ON");
            }
            global_opts->nocompactsameurls = !global_opts->nocompactsameurls;
            break;
         case 's':
            if (global_opts->speed_mode == SPEED_CURRENT) {
               global_opts->speed_mode = SPEED_MIXED;
            } else {
               global_opts->speed_mode++;
            }
            ss.str("");
            ss << "Speed showing mode - " << global_opts->speed_mode;
            showhelphint(ss.str());
            break;
         case 'o':
            if (global_opts->sort_order == SORT_MAX_TIME) {
               global_opts->sort_order = SORT_SIZE;
            } else {
               global_opts->sort_order++;
            }
            ss.str("");
            ss << "Connections sort order - " << global_opts->sort_order;
            showhelphint(ss.str());
            break;
         case 'h':
            global_opts->freeze = true;
            try {
               inp = edline(0, "Squid host", global_opts->host);
               if (inp != "") global_opts->host = inp;
            }
            catch (string &s) {
               showhelphint(s);
            }
            global_opts->freeze = false;
            break;
         case '/':
            global_opts->freeze = true;
            try {
               inp = edline(0, "Search for", "");
               if (inp != "") search_string = inp;
            }
            catch (string &s) {
               showhelphint(s);
            }
            global_opts->freeze = false;
            break;
         case 'p':
            global_opts->freeze = true;
            try {
               inp = edline(0, "Squid port", Utils::itos(global_opts->port));
               if (inp != "") {
                  long int port = Utils::stol(inp);
                  global_opts->port = port;
               }
            }
            catch(string &s) {
               showhelphint(s);
            }
            global_opts->freeze = false;
            break;
         case 'r':
            global_opts->freeze = true;
            try {
               inp = edline(0, "Refresh interval (sec)", Utils::itos(global_opts->sleep_sec));
               long int sec = Utils::stol(inp);
               if (sec > 0)
                  global_opts->sleep_sec = sec;
               else
                  showhelphint("Invalid refresh interval");
            }
            catch(string &s) {
               showhelphint(s);
            }
            global_opts->freeze = false;
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
            toggle_action();
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
            global_opts->showhelp = !global_opts->showhelp;
            break;
         default:
            showhelphint("Press ? for help");
            break;
      }
      if (dotick) tick();
   }
}

int ncui::min(const int a, const int b) {
    return a < b ? a : b;
}

string ncui::edline(int linenum, string prompt, string initial) {
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

    if (c == KEY_ENTER || c == '\r')
        /* Success */
        return str;
    else {
        throw(string("invalid input"));
    }
}

// vim: ai ts=3 sts=3 et sw=3 expandtab
