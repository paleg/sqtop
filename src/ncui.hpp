/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#ifndef __NCUI_H
#define __NCUI_H

#include <string>
#include <vector>
#include <csignal>

#include "options.hpp"
#include "sqstat.hpp"

struct formattedline_t {
   // text representation of sconn or url
   std::string str;
   // position (height)
   unsigned int y;
   // how much lines takes str
   unsigned int coef;
   // original structure (for toggle_action/0)
   SQUID_Connection sconn;
   // url id (for toggle_action/0)
   std::string id;
   // is it empty line
   bool new_line;
   // line was selected
   bool highlighted;
};

class ncui {
   public:
      ncui();
      ~ncui();
      ncui(options_c *gopts);
      void curses_init(void);
      void print(void);
      void loop(void);
      void finish(void);
      void tick(void);

      bool dontshowdisplay;

      void seterror(std::string);
      void clearerror();

      void set_speeds(long av_speed, long curr_speed);
      void set_active_conn(int);
      void set_stat(std::vector <SQUID_Connection>);
   private:
      int compactlongline(std::string &line, options_c *opts);

      std::string edline(int linenum, std::string prompt, std::string initial);
      int min(const int a, const int b);

      void showhelphint(std::string text);
      int helphint_time;

      std::string helpmsg();

      std::string b2s(bool);

      std::string error;

      unsigned int selected_index;
      formattedline_t selected_t;
      void toggle_action();

      unsigned int page_size;

      //unsigned int ticks;

      std::vector <std::string> collapsed;
      std::vector <std::string> detailed;

      std::string search_string;

      std::string debug;
      void addwatch(std::string prefix, std::string value);

      long av_speed;
      long curr_speed;
      int act_conn;
      std::vector <SQUID_Connection> sqconns;

      std::string helphintmsg;
      time_t helptimer;
      sig_atomic_t foad;
      options_c *global_opts;

      pthread_mutex_t tick_mutex;
      pthread_mutexattr_t mattr;

      std::vector <formattedline_t> format_connections_str(std::vector<SQUID_Connection> conns, int offset);
      std::vector<SQUID_Connection> filter_conns(std::vector<SQUID_Connection> in);
      int increment;
      unsigned int y_coef;
      unsigned int start;
};

#endif /* __NCUI_H */

// vim: ai ts=3 sts=3 et sw=3 expandtab
