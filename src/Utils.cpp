/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

//stringstream
#include <sstream>
//setprecision
#include <iomanip>
//inet_ntoa
#include <arpa/inet.h>
//transform
#include <algorithm>
//errno
#include <cerrno>
//LONG_MIN, LONG_MAX
#include <climits>

#include "Utils.hpp"

using std::string;
using std::vector;

vector<string> Utils::SplitString(string str, string delim) {
   vector<string> result;
   std::string::size_type found = str.find_first_of(delim);
   while (found != string::npos) {
      result.push_back(str.substr(0, found));
      str = str.substr(found+1);
      found = str.find_first_of(delim);
   }
   if (str.size() != 0) {
      result.push_back(str);
   }
   return result;
}

string Utils::JoinVector(vector<string> inv, string delim) {
   string result = "";
   for (vector<string>::iterator it = inv.begin(); it != inv.end(); ++it)
      result += (*it + delim);
   if (!result.empty())
      result.erase(result.size()-delim.size(), delim.size());
   return result;
}

long int Utils::stol(string s) {
   errno = 0;
   char *endptr;
   int base = 10;
   long int val = strtol(s.c_str(), &endptr, base);
   if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
      || (errno != 0 && val == 0)) {
      throw("Unacceptable input");
   }
   string end = string(endptr);
   if (end.size() != 0) {
      throw("Invalid input '"+end+"'");
   }
   return val;
}

string Utils::itos(long long num) {
    std::stringstream ss;
    ss << num;
    return (ss.str());
}

string Utils::ftos(double num, int prec) {
    std::stringstream ss;
    ss << std::setiosflags(std::ios::fixed) << std::setprecision(prec) << num;
    return (ss.str());
}

string Utils::UsernamesToStr(vector<string>& in) {
   string result = "";
   for (vector<string>::iterator it = in.begin(); it != in.end(); ++it)
        result += *it + ", ";
   result.resize(result.size()-2);
   return result;
}

string Utils::ConvertTime(long etime) {
    string result="";
    long hours   = etime/3600;
    long minutes = (etime/60) % 60;
    long seconds = etime % 60;
    if (hours != 0) {
        result = itos(hours) + "h ";
    }
    if (minutes != 0) {
        result += itos(minutes) + "m ";
    }
    if (seconds != 0) {
        result += itos(seconds) + "s";
    } else if ((minutes == 0) && (hours == 0)) {
        result = "0s";
    }
    return result;
}

string Utils::ConvertSize(long long esize) {
    string result="";
    long long gb = esize/1024/1024/1024;
    long long mb = esize/1024/1024 - gb*1024;
    long long kb = (esize/1024) % 1024;
    if (gb != 0) {
        result += itos(gb) + "GB ";
    }
    if (mb != 0) {
        result += itos(mb) + "MB ";
    }
    result += itos(kb) + "KB";
    return result;
}

std::pair <string, string> Utils::ConvertSpeedPair(long long speed) {
   std::pair <string, string> result;
   long long mb = speed/1024/1024;
   //long kb = speed/1024;
   if (mb != 0) {
       result.first = ftos(speed/1024.0/1024.0, 2);
       result.second = "MB/s";
   } else {
       result.first = ftos(speed/1024.0, 1);
       result.second = "KB/s";
   }
   return result;
}

string Utils::ConvertSpeed(long long speed) {
   std::pair <string, string> result = Utils::ConvertSpeedPair(speed);
   return result.first+" "+result.second;
}

bool Utils::VectorFindSubstr(vector<string>& v, string& str) {
   for(vector<string>::iterator it = v.begin(); it != v.end(); ++it) {
      if ((*it).find(str) != string::npos) return true;
   }
   return false;
}

bool Utils::MemberOf(vector<string>& v, string& str) {
     if (find(v.begin(), v.end(), str) == v.end())
        return false;
     else return true;
}

void Utils::VectorDeleteStr(vector<string>& v, string& str)
{
   vector<string>::iterator vItr = v.begin();
   while ( vItr != v.end() ) {
      if ( (*vItr) == str ) {
         vItr = v.erase( vItr ); // Will return next valid iterator
         break;
      } else {
         vItr++;
      }
   }
}

bool Utils::IPMemberOf(vector<string>& v, string& ip_in) {
     for (vector<string>::iterator it = v.begin(); it != v.end(); ++it) {
        vector<string> ip_mask = SplitString(*it, "/");
        unsigned long int ip = inet_addr(ip_mask[0].c_str());
        unsigned long int mask;
        if (ip_mask.size() > 1) {
           if (ip_mask[1].size() > 2) {
               mask = inet_addr(ip_mask[1].c_str());
           } else {
               std::stringstream ss(ip_mask[1]);
               int mask_bits;
               ss >> mask_bits;
               mask = mask_bits ? htonl(0xfffffffful << (32 - mask_bits)) : 0;
           }
        } else {
           mask = inet_addr("255.255.255.255");
        }
        if ((inet_addr (ip_in.c_str()) & mask) == ip)
           return true;
    }
    return false;
}

void Utils::ToLower(string& rData) {
     transform(rData.begin(), rData.end(), rData.begin(), ::tolower);
}

bool Utils::UserMemberOf(vector<string>& v, vector<string>& users) {
     for (vector<string>::iterator it = users.begin(); it != users.end(); ++it) {
         if (MemberOf(v, *it))
            return true;
     }
     return false;
}

string Utils::replace(string text, string s, string d)
{
  for(std::string::size_type index=0; index=text.find(s, index), index!=std::string::npos;)
  {
    text.erase(index, s.length());
    text.insert(index, d);
    index+=d.length();
  }
  return text;
}

// vim: ai ts=3 sts=3 et sw=3 expandtab
