/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

//errno
#include <cerrno>
//strerror
#include <cstring>
//gethostbyname, h_errno
#include <netdb.h>
//socket, connect
#include <sys/socket.h>
//write, read, close
#include <unistd.h>

#include "sqconn.hpp"

using std::string;

sqconn::sqconn() {
    m_sock=0;
}

sqconn::~sqconn() {
    if (m_sock != 0) {
       shutdown(m_sock,0);
       close(m_sock);
    }
}

void sqconn::open(string server, int port) {
    struct hostent *he;

    m_sock = socket(AF_INET, SOCK_STREAM, 6);
    if(m_sock < 0) throw sqconnException(strerror(errno));
    memset(&m_addr, 0, sizeof(struct sockaddr_in));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    he=gethostbyname(server.c_str());
    if (he==NULL) throw sqconnException(hstrerror(h_errno));
    m_addr.sin_addr = *(struct in_addr *) he->h_addr;
    if (connect(m_sock, (struct sockaddr *)&m_addr, sizeof(m_addr)) < 0) {
         close(m_sock);
         throw sqconnException(strerror(errno));
    }
}

int sqconn::operator << (const string str) {
    int f = write(m_sock, str.c_str(), str.size());
    if (f == -1) {
       throw sqconnException(strerror(errno));
    }
    int s = write(m_sock, "\x0D\x0A", 2);
    if (s == -1) {
       throw sqconnException(strerror(errno));
    }
    return f+s;
}

int sqconn::operator >> (string &result) {
    char ch;
    int data;
    result="";
    data = read(m_sock, &ch, 1);
    while ((data!=0) && (data!=-1)) {
          if (ch>30) result.push_back(ch);
          if (ch==10) break;
          data = read(m_sock, &ch, 1);
    }
    if (data == -1) throw sqconnException(strerror(errno));
    return data;
}
