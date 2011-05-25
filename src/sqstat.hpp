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

#include "options.hpp"

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
   Uri_Stats() : id(""), count(0), uri(""), oldsize(0), size(0), etime(0), delay_pool(-1), username("") {};
};

struct SQUID_Connection {
   std::string peer;
   long long sum_size;
   long max_etime;
   long av_speed;
   long curr_speed;
   std::vector <Uri_Stats> stats;
   std::vector <std::string> usernames;
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

      const char *what() const throw() { return userMessage.c_str(); }
    private:
      std::string userMessage;
      int code;
};

class sqstat {
   public:
      sqstat();
      std::vector <SQUID_Connection> getinfo(std::string host, int port, std::string passwd);

      std::string squid_version;
      int active_conn;
      long av_speed;
      long curr_speed;

      static void compactSameUrls(std::vector <SQUID_Connection> &scon);

      static std::string head_format(options_c *opts, int active_conn, int active_ips, long av_speed);
      static std::string conn_format(options_c *opts, SQUID_Connection &scon);
      static std::string stat_format(options_c *opts, SQUID_Connection &scon, Uri_Stats &ustat);
      static std::string speeds_format(SPEED_MODE mode, long int av_speed, long int curr_speed);

      static bool CompareSIZE(SQUID_Connection a, SQUID_Connection b);
      static bool CompareTIME(SQUID_Connection a, SQUID_Connection b);
      static bool CompareAVSPEED(SQUID_Connection a, SQUID_Connection b);
      static bool CompareCURRSPEED(SQUID_Connection a, SQUID_Connection b);

   private:
      std::vector <SQUID_Connection> Connections;
      std::vector <SQUID_Connection> OldConnections;

      time_t lastruntime;

      void format_changed(std::string line);
      std::vector<SQUID_Connection>::iterator findConnByPeer(std::string Host);
      std::vector<Uri_Stats>::iterator findStatById(std::vector<SQUID_Connection>::iterator conn, std::string id);
      Uri_Stats findUriStatsById(std::vector <SQUID_Connection> conns, std::string id);
};

#endif /* __SQSTAT_H */

// vim: ai ts=3 sts=3 et sw=3 expandtab
