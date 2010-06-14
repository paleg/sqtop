/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#ifndef __UTILS_H
#define __UTILS_H

#include <string>
#include <vector>

namespace Utils {
   extern std::vector <std::string>  splitString(std::string str, std::string delim);
   extern std::string joinVector(std::vector<std::string> inv, std::string delim);
   extern std::string itos(long long num);
   extern long int stol(std::string s);
   extern std::string ftos(double num, int prec);
   extern std::string usernames2str(std::vector<std::string> &in);
   extern std::string convert_time(long etime);
   extern std::string convert_size(long long esize);
   extern std::pair <std::string, std::string> convert_speed_pair(long long speed);
   extern std::string convert_speed(long long speed);
   extern bool memberOf(std::vector <std::string> &v, std::string &str);
   extern void VectorDeleteStr(std::vector <std::string> &v, std::string &str);
   extern bool VectorFindSubstr(std::vector <std::string> &v, std::string &str);
   extern bool IPmemberOf(std::vector <std::string> &v, std::string &ip_in);
   extern void ToLower(std::string &data);
   extern bool UserMemberOf(std::vector <std::string> &v, std::vector <std::string> &users);
};

#endif /* __UTILS_H */

// vim: ai ts=3 sts=3 et sw=3 expandtab
