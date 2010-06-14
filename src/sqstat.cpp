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

using std::string;
using std::vector;
using std::endl;


bool CompareURLs(Uri_Stats a, Uri_Stats b) {
     return a.size > b.size;
}

bool CompareIP(SQUID_Connection a, SQUID_Connection b) {
   unsigned long ip1, ip2;
   struct sockaddr_in n;
   inet_aton(a.peer.c_str(), &n.sin_addr);
   ip1 = ntohl(n.sin_addr.s_addr);
   inet_aton(b.peer.c_str(), &n.sin_addr);
   ip2 = ntohl(n.sin_addr.s_addr);
   return ip1 < ip2;
}

/* for std::find_if */
bool ConnByPeer(SQUID_Connection conn, string Host) {
   return conn.peer == Host;
}

/* for std::find_if */
bool StatByID(Uri_Stats stat, string id) {
   return stat.id == id;
}

sqstat::sqstat() {
   lastruntime = 0;
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


/* static */ void sqstat::compactSameUrls(vector <SQUID_Connection> &sqconns) {
   for (vector <SQUID_Connection>::iterator it = sqconns.begin(); it != sqconns.end(); ++it) {
      std::map<string, Uri_Stats> urls;

      for (vector <Uri_Stats>::iterator itu = it->stats.begin(); itu != it->stats.end(); ++itu) {
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

/* static */ string sqstat::head_format(options_c *opts_ptr, int active_conn, int active_ips, long av_speed) {
   std::stringstream result;
   if ((opts_ptr->Hosts.size() == 0) && (opts_ptr->Users.size() == 0)) {
      result << endl << "Active connections: " << active_conn;
      result << ", active IPs: " << active_ips;
      if (opts_ptr->zero || (av_speed > 103))
         result << ", average speed: " << Utils::convert_speed(av_speed);
      result << endl;
   }
   return result.str();
}

/* static */ string sqstat::conn_format(options_c *opts_ptr, SQUID_Connection &scon) {
   std::stringstream result;

   result << "  IP: " << scon.peer;
   if (!scon.usernames.empty()) {
      string head;
      if (scon.usernames.size() == 1)
         head = "User: ";
      else
         head = "Users: ";
      result << "; " + head << Utils::usernames2str(scon.usernames);
   }

   string condetail="";
   if (opts_ptr->full || opts_ptr->brief)
      condetail += "sessions: " + Utils::itos(scon.stats.size()) + ", ";
   if (opts_ptr->zero || (scon.sum_size > 1024))
      condetail += "size: " + Utils::convert_size(scon.sum_size) + ", ";
   if (opts_ptr->zero || (scon.curr_speed > 103) || (scon.av_speed > 103)) {
      condetail += sqstat::speeds_format(opts_ptr->speed_mode, scon.av_speed, scon.curr_speed) + ", ";
   }
   if (opts_ptr->full && (opts_ptr->zero || scon.max_etime > 0))
      condetail += "max time: " + Utils::convert_time(scon.max_etime) + ", ";
   if (condetail.size() > 2) {
      condetail.resize(condetail.size()-2);
      result << " (" << condetail << ")";
   }
   return result.str();
}

string sqstat::speeds_format(SPEED_MODE mode, long av_speed, long curr_speed) {
   std::stringstream result;
   std::pair <string, string> av_speed_pair;
   std::pair <string, string> curr_speed_pair;
   av_speed_pair = Utils::convert_speed_pair(av_speed);
   switch (mode) {
      case SPEED_CURRENT:
         curr_speed_pair = Utils::convert_speed_pair(curr_speed);
         result << "current speed: " << curr_speed_pair.first << curr_speed_pair.second;
         break;
      case SPEED_MIXED:
         if (curr_speed != av_speed) {
            std::pair <string, string> curr_speed_pair;
            curr_speed_pair = Utils::convert_speed_pair(curr_speed);
            result << "current/average speed: ";
            if (av_speed_pair.second == curr_speed_pair.second) {
               result << curr_speed_pair.first << "/" << av_speed_pair.first << " " << av_speed_pair.second;
            } else {
               result << curr_speed_pair.first << curr_speed_pair.second << " / " << av_speed_pair.first << av_speed_pair.second;
            }
            break;
         }
      case SPEED_AVERAGE:
         result << "average speed: " + av_speed_pair.first + av_speed_pair.second;
         break;
   };
   return result.str();
}

/* static */ string sqstat::stat_format(options_c *opts_ptr, SQUID_Connection &scon, Uri_Stats &ustat) {
   std::stringstream result;
   result << "    " << ustat.uri;
   string udetail = "";
   if (ustat.count > 1) {
      udetail += "count: " + Utils::itos(ustat.count) + ", ";
   }
   if (opts_ptr->detail) {
      if (opts_ptr->zero || (ustat.size > 1024))
         udetail += "size: " + Utils::convert_size(ustat.size) + ", ";
      if (opts_ptr->full && ((opts_ptr->zero || (ustat.etime > 0))))
         udetail += "time: " + Utils::convert_time(ustat.etime) + ", ";
      if (scon.usernames.size() > 1)
         udetail += "user: " + ustat.username + ", ";
      if (opts_ptr->zero || (ustat.av_speed > 103) || (ustat.curr_speed > 103))
         udetail += sqstat::speeds_format(opts_ptr->speed_mode, scon.av_speed, scon.curr_speed) + ", ";
      if (opts_ptr->full && (opts_ptr->zero || (ustat.delay_pool != 0)))
         udetail += "delay_pool: " + Utils::itos(ustat.delay_pool) + ", ";
   }
   if (udetail.size() > 2) {
      udetail.resize(udetail.size()-2);
      result << " (" << udetail << ")";
   }
   return result.str();
}

Uri_Stats sqstat::findUriStatsById(vector <SQUID_Connection> conns, string id) {
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

void sqstat::format_changed(string line) {
   std::stringstream result;
   result << "Warning!!! Please send bug report.";
   result << " active_requests format changed - \'" << line << "\'." << endl;
   result << squid_version << endl;
   result << PACKAGE_NAME << "-" << VERSION;
   throw sqstatException(result.str(), FORMAT_CHANGED);
}

vector <SQUID_Connection> sqstat::getinfo(string host, int port, string passwd) {
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
      con.open(host, port);
   }
   catch(sqconnException &e) {
      std::stringstream error;
      error << e.what() << " while connecting to " << host << ":" << port;
      throw sqstatException(error.str(), FAILED_TO_CONNECT);
   }

   Connections.clear();

   time_t timenow = 0;

   try {
      string request = "GET cache_object://localhost/active_requests HTTP/1.0\n";
      if (!passwd.empty()) {
         string encoded = Base64::Encode("admin:" + passwd);
         request += "Authorization: Basic " + encoded + "\n";
      }
      con << request;
      Uri_Stats oldStats;
      while ((con >> temp_str) != 0) {
         if (Connections.size()==0) {
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

         vector <string> result;
         if (temp_str.substr(0,8) == "Server: ") {
            result = Utils::splitString(temp_str, " ");
            if (result.size() == 2) {
               squid_version = result[1];
            } else { format_changed(temp_str); }
         } else if (temp_str.substr(0,12) == "Connection: ") {
            result = Utils::splitString(temp_str, " ");
            if (result.size() == 2) {
               newStats.id = result[1];
               oldStats = findUriStatsById(OldConnections, result[1]);
               newStats.uri = "";
               newStats.username = "";
               newStats.size = 0;
               newStats.count = 0;
               newStats.oldsize = 0;
               newStats.etime = 0;
               newStats.delay_pool = -1;
            } else { format_changed(temp_str); }
         } else if (temp_str.substr(0,6) == "peer: ") {
            result = Utils::splitString(temp_str, ":");
            if (result.size() == 3) {
               string peer = result[1].substr(1);
               Conn_it = std::find_if( Connections.begin(), Connections.end(), std::bind2nd( std::ptr_fun(ConnByPeer) , peer) );
               // if it is new peer, create new SQUID_Connection
               if (Conn_it == Connections.end()) {
                  SQUID_Connection Connection;
                  Connection.peer = peer;
                  Connections.push_back(Connection);
                  Conn_it = Connections.end() - 1;
               }
               Conn_it->stats.push_back(newStats);
               Stat_it = Conn_it->stats.end() - 1;
            } else { format_changed(temp_str); }
         } else if (temp_str.substr(0,4) == "uri ") {
            result = Utils::splitString(temp_str, " ");
            if (result.size() == 2) {
               Stat_it->uri = result[1];
               Stat_it->count = 1;
               Stat_it->curr_speed = 0;
               Stat_it->av_speed = 0;
            } else { format_changed(temp_str); }
         } else if (temp_str.substr(0,11) == "out.offset ") {
            result = Utils::splitString(temp_str, " ");
            if (result.size() == 4) {
               esize = atoll(result[3].c_str());
               Stat_it->size = esize;
               Stat_it->oldsize = oldStats.size;
               Conn_it->sum_size += esize;
            } else { format_changed(temp_str); }
         } else if (temp_str.substr(0,6) == "start ") {
            result = Utils::splitString(temp_str, " ");
            if (result.size() == 5) {
               string temp = result[2].erase(0,1);
               etime = atoi(temp.c_str());
               Stat_it->etime = etime;
               if (etime > Conn_it->max_etime)
                  Conn_it->max_etime = etime;
            } else { format_changed(temp_str); }
         } else if (temp_str.substr(0,11) == "delay_pool ") {
            result = Utils::splitString(temp_str, " ");
            if (result.size() == 2) {
               string temp = result[1];
               delay_pool = atoi(temp.c_str());
               Stat_it->delay_pool = delay_pool;
            } else { format_changed(temp_str); }
         } else if (temp_str.substr(0,9) == "username ") {
            result = Utils::splitString(temp_str, " ");
            if (result.size() == 1)
               result.push_back("-");
            if (result.size() == 2) {
               string username = result[1];
               if (!(username == "-")) {
                  Utils::ToLower(username);
                  Stat_it->username = username;
                  if (!Utils::memberOf(Conn_it->usernames, username))
                     Conn_it->usernames.push_back(username);
               }
            } else { format_changed(temp_str); }
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
   for (vector<SQUID_Connection>::iterator Conn = Connections.begin(); Conn != Connections.end(); ++Conn) {

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
         } else {
            Conn->curr_speed += stat_av_speed;
            curr_speed += stat_av_speed;
         }
      }
   }

   sort(Connections.begin(), Connections.end(), sqstat::CompareSIZE);

   OldConnections = Connections;
   lastruntime = timenow;

   return Connections;
}

// vim: ai ts=3 sts=3 et sw=3 expandtab
