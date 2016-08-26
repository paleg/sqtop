/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#ifndef __SQSTAT_H
#define __SQSTAT_H

#include <string>
#include <vector>
//exception
#include <typeinfo>

#include "config.h"

#include "options.hpp"

namespace sqtop {

struct Uri_Stats {
   std::string id;
   int count;
   std::string uri;
   long long oldsize; // to calculate current speed, while using ncui
   long long size;
   long etime;
   long av_speed;
   long curr_speed;
   int delay_pool;
   std::string username;
   // TODO: Uri_Stats() : Uri_Stats("") {};
   Uri_Stats() : id(""), count(0), uri(""), oldsize(0), size(0), etime(0), delay_pool(-1), username("") {};
   Uri_Stats(std::string id) : id(id), count(0), uri(""), oldsize(0), size(0), etime(0), delay_pool(-1), username("") {};
};

struct SQUID_Connection {
   std::string peer;
#ifdef WITH_RESOLVER
   std::string hostname;
#endif
   long long sum_size;
   long max_etime;
   long av_speed;
   long curr_speed;
   std::vector<Uri_Stats> stats;
   std::vector<std::string> usernames;
   SQUID_Connection() : sum_size(0), max_etime(0), av_speed(0), curr_speed(0) {};
};

#define FAILED_TO_CONNECT 1
#define FORMAT_CHANGED 2
#define ACCESS_DENIED 3
#define UNKNOWN_ERROR 4

class sqstatException: public std::exception {
    public:
      sqstatException() {};
      sqstatException(const std::string message, int incode) throw() {
         userMessage = message;
         code=incode;
      }
      ~sqstatException() throw() {}

      const char* what() const throw() { return userMessage.c_str(); }

    private:
      std::string userMessage;
      int code;
};

class sqstat {
   public:
      sqstat();
      std::vector<SQUID_Connection> GetInfo(Options* pOpts);

      std::string squid_version;
      int active_conn;
      time_t process_time;
      time_t get_time;
      long av_speed;
      long curr_speed;

      static bool CompareURLs(Uri_Stats a, Uri_Stats b);
      static bool CompareIP(SQUID_Connection a, SQUID_Connection b);
      static bool ConnByPeer(SQUID_Connection conn, std::string Host);
      static bool StatByID(Uri_Stats stat, std::string id);
      static void CompactSameUrls(std::vector<SQUID_Connection>& scon);

      static std::string HeadFormat(Options* pOpts, int active_conn, int active_ips, long av_speed);
      static std::string ConnFormat(Options* pOpts, SQUID_Connection& scon);
      static std::string StatFormat(Options* pOpts, SQUID_Connection& scon, Uri_Stats& ustat);
      static std::string SpeedsFormat(Options::SPEED_MODE mode, long int av_speed, long int curr_speed);

      static bool CompareSIZE(SQUID_Connection a, SQUID_Connection b);
      static bool CompareTIME(SQUID_Connection a, SQUID_Connection b);
      static bool CompareAVSPEED(SQUID_Connection a, SQUID_Connection b);
      static bool CompareCURRSPEED(SQUID_Connection a, SQUID_Connection b);

   private:
      //std::vector<SQUID_Connection> connections;
      std::map <std::string, SQUID_Connection> connections;
      //std::vector<SQUID_Connection> oldConnections;
      //std::map <std::string, SQUID_Connection> oldConnections;
      std::map <std::string, int> sizes;

#ifdef WITH_RESOLVER
      std::string DoResolve(Options* pOpts, std::string peer);
#endif

      time_t last_get_time;

      void FormatChanged(std::string line);
      //std::vector<SQUID_Connection>::iterator FindConnByPeer(std::string Host);
      //std::vector<Uri_Stats>::iterator FindStatById(std::vector<SQUID_Connection>::iterator conn, std::string id);
      Uri_Stats FindUriStatsById(std::vector<SQUID_Connection> conns, std::string id);
};

}
#endif /* __SQSTAT_H */

// vim: ai ts=3 sts=3 et sw=3 expandtab
