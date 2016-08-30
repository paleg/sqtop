/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#ifndef __SQSTAT_H
#define __SQSTAT_H

#include <string>
#include <vector>
#include <set>
#include <map>
//exception
#include <typeinfo>

#include "config.h"

#include "options.hpp"

namespace sqtop {

struct UriStats {
   std::string id;
   int count;
   std::string uri;
   long long oldsize; // to calculate current speed, while using ncui
   long long size;
   long oldetime; // to calculate current speed, while using ncui
   long etime;
   long av_speed;
   long curr_speed;
   int delay_pool;
   std::string username;
   // TODO: UriStats() : UriStats("") {};
   UriStats() : id(""), count(0), uri(""), oldsize(0), size(0), oldetime(0), etime(0), delay_pool(-1), username("") {};
   UriStats(std::string id) : id(id), count(0), uri(""), oldsize(0), size(0), oldetime(0), etime(0), delay_pool(-1), username("") {};
};

struct SquidConnection {
   std::string peer;
#ifdef WITH_RESOLVER
   std::string hostname;
#endif
   long long sum_size;
   long max_etime;
   long av_speed;
   long curr_speed;
   std::vector<UriStats> stats;
   std::set<std::string> usernames;
   SquidConnection() : sum_size(0), max_etime(0), av_speed(0), curr_speed(0) {};
};

struct OldStat {
   // out.size from previous stats
   long long size;
   // seconds ago from previous stats
   long etime;
   OldStat() : size(0), etime(0) {};
   OldStat(long long size, long etime) : size(size), etime(etime) {};
};

struct SquidStats {
   std::vector<SquidConnection> connections;

   long av_speed;
   long curr_speed;

   time_t get_time;
   time_t process_time;

   int total_connections;

   SquidStats() : av_speed(0), curr_speed(0), get_time(0), process_time(0), total_connections(0) {};
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
#ifdef WITH_RESOLVER
      sqstat(Options* pgOpts, Resolver* pResolver) : pOpts(pgOpts), pResolver(pResolver) {};
#else
      sqstat(Options* pgOpts) : pOpts(pgOpts) {};
#endif

      SquidStats GetInfo();
      std::string squid_version;

      static bool CompareURLs(UriStats a, UriStats b);
      static bool CompareIP(SquidConnection a, SquidConnection b);
      static bool ConnByPeer(SquidConnection conn, std::string Host);
      static bool StatByID(UriStats stat, std::string id);
      static void CompactSameUrls(std::vector<SquidConnection>& scon);

      static std::string HeadFormat(Options* pOpts, int active_conn, int active_ips, long av_speed);
      static std::string ConnFormat(Options* pOpts, SquidConnection& scon);
      static std::string StatFormat(Options* pOpts, SquidConnection& scon, UriStats& ustat);
      static std::string SpeedsFormat(Options::SPEED_MODE mode, long int av_speed, long int curr_speed);

      static bool CompareSIZE(SquidConnection a, SquidConnection b);
      static bool CompareTIME(SquidConnection a, SquidConnection b);
      static bool CompareAVSPEED(SquidConnection a, SquidConnection b);
      static bool CompareCURRSPEED(SquidConnection a, SquidConnection b);

   private:
      //std::vector<SquidConnection> connections;
      std::map <std::string, SquidConnection> connections;
      //std::vector<SquidConnection> oldConnections;
      //std::map <std::string, SquidConnection> oldConnections;
      std::map <std::string, OldStat> oldstats;
      //
      SquidStats sqstats;

#ifdef WITH_RESOLVER
      std::string DoResolve(std::string peer);
#endif

      void FormatChanged(std::string line);
      //std::vector<SquidConnection>::iterator FindConnByPeer(std::string Host);
      //std::vector<UriStats>::iterator FindStatById(std::vector<SquidConnection>::iterator conn, std::string id);
      UriStats FindUriStatsById(std::vector<SquidConnection> conns, std::string id);

      Options* pOpts;
#ifdef WITH_RESOLVER
      Resolver* pResolver;
#endif
};

}
#endif /* __SQSTAT_H */

// vim: ai ts=3 sts=3 et sw=3 expandtab
