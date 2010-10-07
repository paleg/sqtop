#include "../config.h"

#ifndef __STRINGS_H
#define __STRINGS_H

#define title PACKAGE_NAME " (shows active connections for squid)"
#define copyright "(C) 2006 Oleg Palij"
#define contacts "mailto,xmpp:" PACKAGE_BUGREPORT

#define detail_help "display detailed information (size, username and average speed) for each URL in each connection"
#define full_help "display full details (size, username, average speed, delay pool and elapsed time) for each URL in each connection"
#define zero_help "display zero values instead of silently omitting them"
#define brief_help "display brief per-connection information, omits URLs"
#define hosts_help "comma-separated list of clients (by ip[/mask]) to show"
#define users_help "comma-separated list of clients (by login) to show"
#define nocompact_same_help "don't compact the display of multiple occurrences of the same URL in a single connection"
#define host_help "address of Squid server"
#define port_help "port of Squid server"
#define passwd_help "manager password"
#define refresh_interval_help "set the refresh-interval for interactive mode"

#endif /* __STRINGS_H */
