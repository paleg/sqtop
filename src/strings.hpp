#include "../config.h"

#ifndef __STRINGS_H
#define __STRINGS_H

#define title PACKAGE_NAME " (shows active connections for squid)"
#define copyright "(C) 2006 Oleg Palij"
#define contacts "mailto,xmpp:" PACKAGE_BUGREPORT

#define detail_help "show details (size, username, average speed) about each url in connection"
#define full_help "show full details (size, username, average speed, delay pool, elapsed time) about each url in connection"
#define zero_help "show zero values"
#define brief_help "show brief (without urls) statistic"
#define hosts_help "comma-separated list of clients (by ip[/mask]) to show"
#define users_help "comma-separated list of clients (by login) to show"
#define nocompact_same_help "show same urls in single connection"
#define host_help "address of Squid server"
#define port_help "port of Squid server"
#define passwd_help "manager password"
#define refresh_interval_help "refresh interval"

#endif /* __STRINGS_H */
