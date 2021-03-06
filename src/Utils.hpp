/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#ifndef __UTILS_H
#define __UTILS_H

#include <string>
#include <vector>
#include <set>

#include <stdexcept>

namespace Utils {
   extern std::vector<std::string> SplitString(std::string str, std::string delim);
   extern std::pair <std::string, std::string> SplitIPPort(std::string ipport);
   extern std::string JoinVector(std::vector<std::string> inv, std::string delim);
   extern std::string itos(long long num);
   extern long int stol(std::string s);
   extern std::string ftos(double num, int prec);
   extern std::string StripUserDomain(std::string user);
   extern std::string UsernamesToStr(std::set<std::string>& in);
   extern std::string ConvertTime(long etime);
   extern std::string ConvertSize(long long esize);
   extern std::pair <std::string, std::string> ConvertSpeedPair(long long speed);
   extern std::string ConvertSpeed(long long speed);
   extern bool MemberOf(std::vector<std::string>& v, const std::string& str);
   extern void VectorDeleteStr(std::vector<std::string>& v, std::string& str);
   extern bool SetFindSubstr(std::set<std::string>& v, const std::string& str);
   extern bool IPMemberOf(std::vector<std::string>& v, std::string& ip_in);
   extern void ToLower(std::string& rData);
   extern bool UserMemberOf(std::vector<std::string>& v, std::set<std::string>& users);
   extern std::string replace(std::string text, std::string s, std::string d);
};

#endif /* __UTILS_H */

// vim: ai ts=3 sts=3 et sw=3 expandtab
