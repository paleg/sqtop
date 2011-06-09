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

namespace sqtop {

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
      //ncui();
      ~ncui();
      ncui(Options* pgOpts);

      void CursesInit(void);
      void CursesFinish(void);

      void Print(void);
      void Loop(void);
      void Tick(void);

      void SetError(std::string);
      void ClearError();

      void SetSpeeds(long av_speed, long curr_speed);
      void SetActiveConnCount(int);
      void SetStat(std::vector<SQUID_Connection>);

   private:
      int CompactLongLine(std::string &line);

      std::string EdLine(int linenum, std::string prompt, std::string initial);
      int min(const int a, const int b);

      void ShowHelpHint(std::string text);
      int helphint_time;

      std::string helpmsg(void);

      std::string b2s(bool);

      std::string error;

      unsigned int selected_index;
      void ToggleAction();

      formattedline_t MakeResult(std::string str, int y, int coef, SQUID_Connection sconn, std::string id);
      formattedline_t MakeNewLine(int y);

      unsigned int page_size;

      std::vector<std::string> collapsed;
      std::vector<std::string> detailed;

      std::string search_string;

      std::string debug;
      void AddWatch(std::string prefix, std::string value);

      long av_speed;
      long curr_speed;
      int act_conn;
      std::vector<SQUID_Connection> sqconns;

      std::string helphintmsg;
      time_t helptimer;
      sig_atomic_t foad;
      Options* pGlobalOpts;

      pthread_mutex_t tick_mutex;
      pthread_mutexattr_t mattr;

      formattedline_t selected_t;

      std::vector<formattedline_t> FormatConnections(std::vector<SQUID_Connection> conns, int offset);
      static bool Filter(SQUID_Connection scon, Options* pOpts);
      std::vector<SQUID_Connection> FilterConns(std::vector<SQUID_Connection> in);
      int increment;
      unsigned int y_coef;
      unsigned int start;

      Options* pOpts;
};

}
#endif /* __NCUI_H */

// vim: ai ts=3 sts=3 et sw=3 expandtab
