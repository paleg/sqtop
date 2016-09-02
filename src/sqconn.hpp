/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#ifndef __SQCONN_H
#define __SQCONN_H

#include <string>
#include <vector>
#include <sstream>

//sockaddr_in
#include <netinet/in.h>
//exception
#include <typeinfo>

namespace sqtop {

class sqconnException: public std::exception {
    public:
       sqconnException(const std::string &message) throw() : userMessage(message) {}
       ~sqconnException() throw() {}

       const char *what() const throw() { return userMessage.c_str(); }

    private:
       std::string userMessage;
};

class sqconn {
    public:
       sqconn();
       ~sqconn();

       void open(std::string, int);
       int operator << (const std::string);
       void get(std::vector<std::string>& header, std::vector<std::string>& response);

    private:
       int m_sock;
       struct sockaddr_in m_addr;
       std::stringstream data;
};

}
#endif /* __SQCONN_H */
