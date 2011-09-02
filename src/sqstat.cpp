/*
 * Idea taken from squid_stat by StarSoft Ltd.
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

//atoll,atoi,sort
#include <algorithm>
//stringstream
#include <sstream>
// for "compactsameurls" option
#include <map>
#include <arpa/inet.h>
#include <iostream>

#include "sqconn.hpp"
#include "sqstat.hpp"
#include "Base64.hpp"
#include "Utils.hpp"
#include "strings.hpp"
#include "options.hpp"

namespace sqtop {

using std::string;
using std::vector;
using std::endl;

sqstat::sqstat() {
   lastruntime = 0;
}

/* static */ bool sqstat::CompareURLs(Uri_Stats a, Uri_Stats b) {
     return a.size > b.size;
}

/* static */ bool sqstat::CompareIP(SQUID_Connection a, SQUID_Connection b) {
   unsigned long ip1, ip2;
   struct sockaddr_in n;
   inet_aton(a.peer.c_str(), &n.sin_addr);
   ip1 = ntohl(n.sin_addr.s_addr);
   inet_aton(b.peer.c_str(), &n.sin_addr);
   ip2 = ntohl(n.sin_addr.s_addr);
   return ip1 < ip2;
}

/* for std::find_if */
/* static */ bool sqstat::ConnByPeer(SQUID_Connection conn, string Host) {
   return conn.peer == Host;
}

/* for std::find_if */
/* static */ bool sqstat::StatByID(Uri_Stats stat, string id) {
   return stat.id == id;
}

/* static */ bool sqstat::CompareSIZE(SQUID_Connection a, SQUID_Connection b) {
     return a.sum_size > b.sum_size;
}

/* static */ bool sqstat::CompareTIME(SQUID_Connection a, SQUID_Connection b) {
   return a.max_etime > b.max_etime;
}

/* static */ bool sqstat::CompareAVSPEED(SQUID_Connection a, SQUID_Connection b) {
   return a.av_speed > b.av_speed;
}

/* static */ bool sqstat::CompareCURRSPEED(SQUID_Connection a, SQUID_Connection b) {
   return a.curr_speed > b.curr_speed;
}   

/* static */ void sqstat::CompactSameUrls(vector<SQUID_Connection>& sqconns) {
   for (vector<SQUID_Connection>::iterator it = sqconns.begin(); it != sqconns.end(); ++it) {
      std::map<string, Uri_Stats> urls;

      for (vector<Uri_Stats>::iterator itu = it->stats.begin(); itu != it->stats.end(); ++itu) {
         string url = itu->uri;
         // TODO: check if username is the same ?
         if (urls.find(url) == urls.end()) {
            urls[url] = *itu;
         }
         else {
            urls[url].count += 1;
            urls[url].size += itu->size;
            urls[url].etime += itu->etime;
            // TODO: check this
            if ((urls[url].size !=0) && (urls[url].etime != 0))
               urls[url].av_speed = urls[url].size/urls[url].etime;
         }
      }

      it->stats.clear();
      for (std::map<string, Uri_Stats>::iterator itm=urls.begin(); itm!=urls.end(); itm++) {
         it->stats.push_back(itm->second);
      }
      sort(it->stats.begin(), it->stats.end(), CompareURLs);
   }
}

/* static */ string sqstat::HeadFormat(Options* pOpts, int active_conn, int active_ips, long av_speed) {
   std::stringstream result;
   if ((pOpts->Hosts.size() == 0) && (pOpts->Users.size() == 0)) {
      result << endl << "Active connections: " << active_conn;
      result << ", active hosts: " << active_ips;
      if (pOpts->zero || (av_speed > 103))
         result << ", average speed: " << Utils::ConvertSpeed(av_speed);
      result << endl;
   }
   return result.str();
}

/* static */ string sqstat::ConnFormat(Options* pOpts, SQUID_Connection& scon) {
   std::stringstream result;

   result << "  Host: ";
#ifdef WITH_RESOLVER
   // TODO: show both ip and hostname
   string resolved;
   if (pOpts->dns_resolution) {
      string tmp = scon.hostname;
      if (pOpts->strip_domain) {
         pOpts->pResolver->StripDomain(tmp);
      }
      switch (pOpts->resolve_mode) {
         case Options::SHOW_NAME:
            resolved = tmp;
            break;
         case Options::SHOW_IP:
            resolved = scon.peer;
            break;
         case Options::SHOW_BOTH:
            if (!tmp.compare(scon.peer)) {
               resolved = scon.peer;
            } else {
               resolved = tmp + " [" + scon.peer + "]";
            }
            break;
      };
   } else {
      resolved = scon.peer;
   }
   result << resolved;
#else
   result << scon.peer;
#endif
   if (!scon.usernames.empty()) {
      string head;
      if (scon.usernames.size() == 1)
         head = "User: ";
      else
         head = "Users: ";
      result << "; " + head << Utils::UsernamesToStr(scon.usernames);
   }

   string condetail="";
   if (pOpts->full || pOpts->brief)
      condetail += "sessions: " + Utils::itos(scon.stats.size()) + ", ";
   if (pOpts->zero || (scon.sum_size > 1024))
      condetail += "size: " + Utils::ConvertSize(scon.sum_size) + ", ";
   if (pOpts->zero || (scon.curr_speed > 103) || (scon.av_speed > 103)) {
      condetail += SpeedsFormat(pOpts->speed_mode, scon.av_speed, scon.curr_speed) + ", ";
   }
   if (pOpts->full && (pOpts->zero || scon.max_etime > 0))
      condetail += "max time: " + Utils::ConvertTime(scon.max_etime) + ", ";
   if (condetail.size() > 2) {
      condetail.resize(condetail.size()-2);
      result << " (" << condetail << ")";
   }
   return result.str();
}

string sqstat::SpeedsFormat(Options::SPEED_MODE mode, long av_speed, long curr_speed) {
   std::stringstream result;
   std::pair <string, string> av_speed_pair;
   std::pair <string, string> curr_speed_pair;
   av_speed_pair = Utils::ConvertSpeedPair(av_speed);
   /*if ((curr_speed == 0) && (mode == Options::SPEED_MIXED)) {
      mode = Options::SPEED_AVERAGE;
   }*/
   switch (mode) {
      case Options::SPEED_CURRENT:
         curr_speed_pair = Utils::ConvertSpeedPair(curr_speed);
         result << "current speed: " << curr_speed_pair.first << curr_speed_pair.second;
         break;
      case Options::SPEED_MIXED:
         if (curr_speed != av_speed) {
            std::pair <string, string> curr_speed_pair;
            curr_speed_pair = Utils::ConvertSpeedPair(curr_speed);
            result << "current/average speed: ";
            if (av_speed_pair.second == curr_speed_pair.second) {
               result << curr_speed_pair.first << "/" << av_speed_pair.first << " " << av_speed_pair.second;
            } else {
               result << curr_speed_pair.first << curr_speed_pair.second << " / " << av_speed_pair.first << av_speed_pair.second;
            }
            break;
         }
      case Options::SPEED_AVERAGE:
         result << "average speed: " + av_speed_pair.first + av_speed_pair.second;
         break;
   };
   return result.str();
}

/* static */ string sqstat::StatFormat(Options* pOpts, SQUID_Connection& scon, Uri_Stats& ustat) {
   std::stringstream result;
   result << "    " << ustat.uri;
   string udetail = "";
   if (ustat.count > 1) {
      udetail += "count: " + Utils::itos(ustat.count) + ", ";
   }
   if (pOpts->detail) {
      if (pOpts->zero || (ustat.size > 1024))
         udetail += "size: " + Utils::ConvertSize(ustat.size) + ", ";
      if (pOpts->full && ((pOpts->zero || (ustat.etime > 0))))
         udetail += "time: " + Utils::ConvertTime(ustat.etime) + ", ";
      if (scon.usernames.size() > 1)
         udetail += "user: " + ustat.username + ", ";
      if (pOpts->zero || (ustat.av_speed > 103) || (ustat.curr_speed > 103))
         udetail += SpeedsFormat(pOpts->speed_mode, ustat.av_speed, ustat.curr_speed) + ", ";
      if (pOpts->full && (pOpts->zero || (ustat.delay_pool != 0)))
         udetail += "delay_pool: " + Utils::itos(ustat.delay_pool) + ", ";
   }
   if (udetail.size() > 2) {
      udetail.resize(udetail.size()-2);
      result << " (" << udetail << ")";
   }
   return result.str();
}

Uri_Stats sqstat::FindUriStatsById(vector<SQUID_Connection> conns, string id) {
   for (vector<SQUID_Connection>::iterator it = conns.begin(); it != conns.end(); ++it) {
      vector<Uri_Stats>::iterator itu = find_if(it->stats.begin(), it->stats.end(), bind2nd(ptr_fun(StatByID), id));
      if (itu != it->stats.end())
         return *itu;
   }
   Uri_Stats newStats;
   return newStats;
}

/*string get_ip() {
     char str[100];
     struct hostent *he;
     if (gethostname(str,100)!=0) {
        throw "Failed to gethostname: " + string(strerror(errno));
     }
     he=gethostbyname(str);
     if (he==NULL) {
        throw "Failed to gethostbyname: " + Utils::itos(h_errno);
     }
     return string(inet_ntoa(*(struct in_addr *) he->h_addr));
}*/

void sqstat::FormatChanged(string line) {
   std::stringstream result;
   result << "Warning!!! Please send bug report.";
   result << " active_requests format changed - \'" << line << "\'." << endl;
   result << squid_version << endl;
   result << PACKAGE_NAME << "-" << VERSION;
   throw sqstatException(result.str(), FORMAT_CHANGED);
}

#ifdef WITH_RESOLVER
string sqstat::DoResolve(Options* pOpts, string peer) {
   string resolved;
   if (pOpts->dns_resolution) {
      resolved = pOpts->pResolver->Resolve(peer);
   } else {
      resolved = peer;
   }
   return resolved;
}
#endif

vector<SQUID_Connection> sqstat::GetInfo(Options* pOpts) {
   sqconn con;

   string temp_str;

   active_conn = 0;

   long long esize;
   long etime;

   int n=0, delay_pool;

   vector<SQUID_Connection>::iterator Conn_it; // pointer to current peer
   vector<Uri_Stats>::iterator Stat_it; // pointer to current stat
   Uri_Stats newStats;

   try {
      con.open(pOpts->host, pOpts->port);
   }
   catch(sqconnException &e) {
      std::stringstream error;
      error << e.what() << " while connecting to " << pOpts->host << ":" << pOpts->port;
      throw sqstatException(error.str(), FAILED_TO_CONNECT);
   }

   connections.clear();

   time_t timenow = 0;

   try {
      string request = "GET cache_object://localhost/active_requests HTTP/1.0\n";
      if (!pOpts->pass.empty()) {
         string encoded = Base64::Encode("admin:" + pOpts->pass);
         request += "Authorization: Basic " + encoded + "\n";
      }
      con << request;
      Uri_Stats oldStats;
      while ((con >> temp_str) != 0) {
         if (connections.size()==0) {
            if (n==0) {
               if (temp_str != "HTTP/1.0 200 OK") {
                  std::stringstream error;
                  error << "Access to squid statistic denied: " << temp_str << endl << endl;
                  /*string ip;
                  try {
                     ip = get_ip();
                  }
                  catch (string) {
                     ip = "<your_host_ip>";
                  }*/
                  error << "You must enable access to squid statistic in squid.conf by adding strings like:" << endl << endl;
                  error << "\tacl adminhost src <admin_host_ip_here>/255.255.255.255" << endl;
                  error << "\thttp_access allow manager adminhost" << endl;
                  error << "\thttp_access deny manager";
                  throw sqstatException(error.str(), ACCESS_DENIED);
               } else {
                  n=1;
                  timenow = time(NULL);
                  continue;
               }
            }
         }

         vector<string> result;
         if (temp_str.substr(0,8) == "Server: ") {
            result = Utils::SplitString(temp_str, " ");
            if (result.size() == 2) {
               squid_version = result[1];
            } else { FormatChanged(temp_str); }
         } else if (temp_str.substr(0,12) == "Connection: ") {
            result = Utils::SplitString(temp_str, " ");
            if (result.size() == 2) {
               newStats.id = result[1];
               oldStats = FindUriStatsById(oldConnections, result[1]);
               newStats.uri = "";
               newStats.username = "";
               newStats.size = 0;
               newStats.count = 0;
               newStats.oldsize = 0;
               newStats.etime = 0;
               newStats.delay_pool = -1;
            } else { FormatChanged(temp_str); }
         } else if (temp_str.substr(0,6) == "peer: ") {
            result = Utils::SplitString(temp_str, ":");
            if (result.size() == 3) {
               string peer = result[1].substr(1);
               Conn_it = std::find_if( connections.begin(), connections.end(), std::bind2nd( std::ptr_fun(ConnByPeer) , peer) );
               // if it is new peer, create new SQUID_Connection
               if (Conn_it == connections.end()) {
                  SQUID_Connection connection;
                  connection.peer = peer;
#ifdef WITH_RESOLVER
                  connection.hostname = DoResolve(pOpts, peer);
#endif
                  connections.push_back(connection);
                  Conn_it = connections.end() - 1;
               }
               Conn_it->stats.push_back(newStats);
               Stat_it = Conn_it->stats.end() - 1;
            } else { FormatChanged(temp_str); }
         } else if (temp_str.substr(0,4) == "uri ") {
            result = Utils::SplitString(temp_str, " ");
            if (result.size() == 2) {
               Stat_it->uri = result[1];
               Stat_it->count = 1;
               Stat_it->curr_speed = 0;
               Stat_it->av_speed = 0;
            } else { FormatChanged(temp_str); }
         } else if (temp_str.substr(0,11) == "out.offset ") {
            result = Utils::SplitString(temp_str, " ");
            if (result.size() == 4) {
               esize = atoll(result[3].c_str());
               Stat_it->size = esize;
               Stat_it->oldsize = oldStats.size;
               Conn_it->sum_size += esize;
            } else { FormatChanged(temp_str); }
         } else if (temp_str.substr(0,6) == "start ") {
            result = Utils::SplitString(temp_str, " ");
            if (result.size() == 5) {
               string temp = result[2].erase(0,1);
               etime = atoi(temp.c_str());
               Stat_it->etime = etime;
               if (etime > Conn_it->max_etime)
                  Conn_it->max_etime = etime;
            } else { FormatChanged(temp_str); }
         } else if (temp_str.substr(0,11) == "delay_pool ") {
            result = Utils::SplitString(temp_str, " ");
            if (result.size() == 2) {
               string temp = result[1];
               delay_pool = atoi(temp.c_str());
               Stat_it->delay_pool = delay_pool;
            } else { FormatChanged(temp_str); }
         } else if (temp_str.substr(0,9) == "username ") {
            result = Utils::SplitString(temp_str, " ");
            if (result.size() == 1)
               result.push_back("-");
            if (result.size() == 2) {
               string username = result[1];
               if (!(username == "-")) {
                  Utils::ToLower(username);
                  Stat_it->username = username;
                  if (!Utils::MemberOf(Conn_it->usernames, username))
                     Conn_it->usernames.push_back(username);
               }
            } else { FormatChanged(temp_str); }
         }
      }
   }
   catch(sqconnException &e) {
      std::stringstream error;
      error << e.what();
      throw sqstatException(error.str(), UNKNOWN_ERROR);
   }

   av_speed = 0;
   curr_speed = 0;
   for (vector<SQUID_Connection>::iterator Conn = connections.begin(); Conn != connections.end(); ++Conn) {

      sort(Conn->stats.begin(), Conn->stats.end(), CompareURLs);

      active_conn += Conn->stats.size();

      for (vector<Uri_Stats>::iterator Stats = Conn->stats.begin(); Stats != Conn->stats.end(); ++Stats) {
         long stat_av_speed = 0;
         if ((Stats->size != 0) && (Stats->etime != 0))
            stat_av_speed = Stats->size/Stats->etime;
         Stats->av_speed = stat_av_speed;
         Conn->av_speed += stat_av_speed;
         av_speed += stat_av_speed;
         long stat_curr_speed = 0;
         if ((Stats->size != 0) && (Stats->oldsize != 0) && (lastruntime != 0) && (Stats->size > Stats->oldsize)) {
            time_t diff = timenow - lastruntime;
            if (diff < 1) diff = 1;
            stat_curr_speed = (Stats->size - Stats->oldsize)/(timenow - lastruntime);
            /*if ((stat_curr_speed > 10000000) || (stat_curr_speed < 0)) {
               cout << Stats->size << " " <<  Stats->oldsize << " " << timenow << " " << lastruntime << endl;
               throw;
            }*/
            Stats->curr_speed = stat_curr_speed;
            Conn->curr_speed += stat_curr_speed;
            curr_speed += stat_curr_speed;
         } /*else {
            Conn->curr_speed += stat_av_speed;
            curr_speed += stat_av_speed;
         }*/
      }
   }

   sort(connections.begin(), connections.end(), sqstat::CompareSIZE);

   oldConnections = connections;
   lastruntime = timenow;

   return connections;
}

}
// vim: ai ts=3 sts=3 et sw=3 expandtab
