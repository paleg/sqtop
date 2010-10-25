/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#ifdef ENABLE_UI
#include <pthread.h>
#endif

#include <getopt.h>
#include <cstdlib>
#include <iostream>
//stringstream
#include <sstream>

#include "strings.hpp"
#include "Utils.hpp"
#include "ncui.hpp"

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;

static struct option longopts[] = {
   { "host",               required_argument,   NULL,    'h' },
   { "port",               required_argument,   NULL,    'p' },
   { "hosts",              required_argument,   NULL,    'H' },
   { "users",              required_argument,   NULL,    'u' },
   { "pass",               required_argument,   NULL,    'P' },
   { "help",               no_argument,         NULL,    'n' },
   { "brief",              no_argument,         NULL,    'b' },
   { "full",               no_argument,         NULL,    'f' },
   { "zero",               no_argument,         NULL,    'z' },
   { "detail",             no_argument,         NULL,    'd' },
#ifdef ENABLE_UI
   { "once",               no_argument,         NULL,    'o' },
   { "refreshinterval",    required_argument,   NULL,    'r' },
#endif
   { "nocompactsameurls",    no_argument,         NULL,    'c' },
   { NULL,                 0,                   NULL,     0 }
};

void usage(char **argv) {
   cout << title << endl;
   cout << "version " << VERSION << " " << copyright << "(" << contacts << ")" << endl;
   cout << endl;
   cout << "Usage:";
   cout << "\n" << argv[0] << " [--host host] [--port port] [--pass password] [--hosts host1,host...] [--users user1,user2] [--brief] [--full] [--zero] [--detail] [--nocompactsameurls] [--once] [--help]";
   cout << "\n\t--host   (-h) host           - " << host_help << ". Default - '127.0.0.1';";
   cout << "\n\t--port   (-p) port           - " << port_help << ". Default - '3128';";
   cout << "\n\t--pass   (-P) password       - " << passwd_help << ";";
   cout << "\n\t--hosts  (-H) host1,host2... - " << hosts_help << ";";
   cout << "\n\t--users  (-u) user1,user2... - " << users_help << ";";
   cout << "\n\t--brief  (-b)                - " << brief_help << ";";
   cout << "\n\t--detail (-d)                - " << detail_help << ";";
   cout << "\n\t--full   (-f)                - " << full_help << ";";
   cout << "\n\t--zero   (-z)                - " << zero_help << ";";
#ifdef ENABLE_UI
   cout << "\n\t--once   (-o)                - disable interactive mode, just print statistics once to stdout;";
   cout << "\n\t--refreshinterval (-r) sec   - " << refresh_interval_help << ";";
#endif
   cout << "\n\t--nocompactsameurls (-c)     - " << nocompact_same_help << ";";
   cout << "\n\t--help                       - show this help.";
   cout << endl;
}

options_c options;

#ifdef ENABLE_UI
pthread_t sq_thread;
#endif

/*static void finish(int sig) {
   foad = sig;
}*/

bool DESC(int a, int b) {
   return a > b;
}

string conns_format(vector <SQUID_Connection> conns) {
   std::stringstream result;

   if (!options.nocompactsameurls)
      sqstat::compactSameUrls(conns);

   for (vector<SQUID_Connection>::iterator it = conns.begin(); it != conns.end(); ++it) {
      if (((options.Hosts.size() == 0) || Utils::IPmemberOf(options.Hosts, it->peer)) &&
         ((options.Users.size() == 0) || Utils::UserMemberOf(options.Users, it->usernames))) {

         result << sqstat::conn_format(&options, *it);

         if (not options.brief) {
            result << endl;
            for (vector<Uri_Stats>::iterator itu = it->stats.begin(); itu != it->stats.end(); ++itu) {
               result << sqstat::stat_format(&options, *it, *itu);
               result << endl;
            }
         }
      result << endl;
      }
   }
   return result.str();
}

#ifdef ENABLE_UI
struct thread_args {
   ncui *ui;
};

void squid_loop(void* threadarg) {
   sqstat sqs;
   std::vector <SQUID_Connection> stat;
   thread_args *args = reinterpret_cast<thread_args *>(threadarg);
   while (true) {
      if (options.do_refresh) {
         try {
            stat = sqs.getinfo(options.host, options.port, options.pass);
            args->ui->clearerror();
            args->ui->set_speeds(sqs.av_speed, sqs.curr_speed);
            args->ui->set_active_conn(sqs.active_conn);
            args->ui->set_stat(stat);
         }
         catch (sqstatException &e) {
            args->ui->seterror(e.what());
         }
         args->ui->tick();
      }
      for (int i=0; i<options.sleep_sec; ++i) {
         sleep(1);
      }
   }
}
#endif

int main(int argc, char **argv) {
   // TODO: config file ?
   int ch;
   string tempusers;

   while ((ch = getopt_long(argc, argv, "r:u:H:h:p:P:dzbfoc", longopts, NULL)) != -1) {
      switch(ch) {
         case 'd':
            options.detail = true;
            break;
         case 'z':
            options.zero = true;
            break;
         case 'b':
            options.brief = true;
            break;
         case 'f':
            options.detail = true;
            options.full = true;
            break;
         case 'P':
            options.pass = optarg;
            break;
         case 'h':
            options.host = optarg;
            break;
         case 'p':
            try {
               options.port = Utils::stol(optarg);
            }
            catch(string &s) {
               cerr << "Unknown port - " << s << endl;
               exit(1);
            }
            break;
         case 'H':
            options.Hosts = Utils::splitString(optarg, ",");
            break;
         case 'u':
            tempusers = optarg;
            Utils::ToLower(tempusers);
            options.Users = Utils::splitString(tempusers, ",");
            break;
#ifdef ENABLE_UI
         case 'o':
            options.ui = false;
            break;
         case 'r':
            try {
               long int sec = Utils::stol(optarg);
               if (sec > 0)
                  options.sleep_sec = Utils::stol(optarg);
               else
                  cerr << "Refresh interval should be greater than 0, using default - " << options.sleep_sec << endl;
            }
            catch(string &s) {
               cerr << "Wrong number - " << s << endl;
               exit(1);
            }
#endif
         case 'c':
            options.nocompactsameurls = true;
            break;
         default:
            usage(argv);
            exit(0);
      }
   }

   // TODO:
   //struct sigaction sa = {};
   //sa.sa_handler = finish;
   //sigaction(SIGINT, &sa, NULL);

#ifdef ENABLE_UI
   if (options.ui) {
      ncui ui = ncui(&options);
      ui.curses_init();

      thread_args args;
      args.ui = &ui;

      pthread_create(&sq_thread, NULL, (void *(*) (void *)) &squid_loop, (void *) &args);

      ui.loop();

      pthread_cancel(sq_thread);
      ui.finish();
   } else {
#endif
      sqstat sqs;
      std::vector <SQUID_Connection> stat;
      try {
         stat = sqs.getinfo(options.host, options.port, options.pass);
      }
      catch (sqstatException &e) {
         cerr << e.what() << endl;
         exit(1);
      }
      cout << sqstat::head_format(&options, sqs.active_conn, stat.size(), sqs.av_speed) << endl;
      cout << conns_format(stat) << endl;
#ifdef ENABLE_UI
   }
#endif

   return 0;
}

// vim: ai ts=3 sts=3 et sw=3 expandtab
