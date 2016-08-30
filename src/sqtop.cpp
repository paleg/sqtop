/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#include "config.h"

//sleep
#include <unistd.h>
#include <getopt.h>
#include <cstdlib>
#include <iostream>
//stringstream
#include <sstream>

#ifdef ENABLE_UI
#include <pthread.h>
#endif

#include "strings.hpp"
#include "Utils.hpp"
#include "ncui.hpp"

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;

using namespace sqtop;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
static struct option longopts[] = {
   { "host",               required_argument,   NULL,    'h' },
   { "port",               required_argument,   NULL,    'p' },
   { "hosts",              required_argument,   NULL,    'H' },
   { "users",              required_argument,   NULL,    'u' },
   { "pass",               required_argument,   NULL,    'P' },
   { "help",               no_argument,         NULL,     0  },
   { "brief",              no_argument,         NULL,    'b' },
   { "full",               no_argument,         NULL,    'f' },
   { "zero",               no_argument,         NULL,    'z' },
   { "detail",             no_argument,         NULL,    'd' },
   { NULL,                 no_argument,         NULL,    'Z' },
#ifdef ENABLE_UI
   { "once",               no_argument,         NULL,    'o' },
   { "refreshinterval",    required_argument,   NULL,    'r' },
#endif
   { NULL,                 no_argument,         NULL,    'c' },
#ifdef WITH_RESOLVER
   { NULL,                 no_argument,         NULL,    'n' },
   { NULL,                 no_argument,         NULL,    'S' },
#endif
   { NULL,                 0,                   NULL,     0 }
};
#pragma GCC diagnostic pop

void usage(char* argv) {
   cout << title << endl;
   cout << "version " << VERSION << " " << copyright << " (" << contacts << ")" << endl;
   cout << endl;
   cout << "Usage:";
   cout << "\n" << argv << " [--help] [--host host] [--port port] [--pass password] [--hosts host1,host...] [--users user1,user2] [--brief] [--detail] [--full] [--zero] [-c] [-Z]";
#ifdef ENABLE_UI
   cout << " [--once] [-r seconds]";
#endif
#ifdef WITH_RESOLVER
   cout << " [-n] [-S]";
#endif
   cout << "\n\t--help                       - show this help;";
   cout << "\n\t--host   (-h) host           - " << host_help << ". Default - '127.0.0.1';";
   cout << "\n\t--port   (-p) port           - " << port_help << ". Default - '3128';";
   cout << "\n\t--pass   (-P) password       - " << passwd_help << ";";
   cout << "\n\t--hosts  (-H) host1,host2... - " << hosts_help << ";";
   cout << "\n\t--users  (-u) user1,user2... - " << users_help << ";";
   cout << "\n\t--brief  (-b)                - " << brief_help << ";";
   cout << "\n\t--detail (-d)                - " << detail_help << ";";
   cout << "\n\t--full   (-f)                - " << full_help << ";";
   cout << "\n\t--zero   (-z)                - " << zero_help << ";";
   cout << "\n\t-c                           - do not " << compact_same_help << ";";
   cout << "\n\t-Z                           - do not " << strip_user_domain_help << ";";
#ifdef ENABLE_UI
   cout << "\n\t--once   (-o)                - disable interactive mode, just print statistics once to stdout;";
   cout << "\n\t--refreshinterval (-r) sec   - " << refresh_interval_help << ";";
#endif
#ifdef WITH_RESOLVER
   cout << "\n\t-n                           - do not " << dns_resolution_help << ";";
   cout << "\n\t-S                           - do not " << strip_host_domain_help << ".";
#endif
   cout << endl;
}

/*static void finish(int sig) {
   foad = sig;
}*/

bool DESC(int a, int b) {
   return a > b;
}

string conns_format(Options* pOpts, vector<SquidConnection> conns) {
   std::stringstream result;

   if (pOpts->compactsameurls)
      sqstat::CompactSameUrls(conns);

   for (vector<SquidConnection>::iterator it = conns.begin(); it != conns.end(); ++it) {
      if (((pOpts->Hosts.size() == 0) || Utils::IPMemberOf(pOpts->Hosts, it->peer)) &&
         ((pOpts->Users.size() == 0) || Utils::UserMemberOf(pOpts->Users, it->usernames))) {

         result << sqstat::ConnFormat(pOpts, *it);

         if (not pOpts->brief) {
            result << endl;
            for (vector<UriStats>::iterator itu = it->stats.begin(); itu != it->stats.end(); ++itu) {
               result << sqstat::StatFormat(pOpts, *it, *itu);
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
   ncui* ui;
   Options* pOpts;
};

void squid_loop(void* threadarg) {
   sqstat sqs;
   thread_args* pArgs = reinterpret_cast<thread_args*>(threadarg);
   while (true) {
      if (pArgs->pOpts->do_refresh) {
         try {
            pArgs->ui->SetStat( sqs.GetInfo(pArgs->pOpts) );
            pArgs->ui->ClearError();
         }
         catch (sqstatException &e) {
            pArgs->ui->SetError(e.what());
         }
         pArgs->ui->Tick();
      }
      for (int i=0; i<pArgs->pOpts->sleep_sec; ++i) {
         sleep(1);
      }
   }
}
#endif

int main(int argc, char **argv) {
   // TODO: config file ?
   int ch;
   string tempusers;

   sqtop::Options* pOpts = new Options();

   string getopt_options = "u:H:h:p:P:dzbfc";
#ifdef ENABLE_UI
   getopt_options += "r:o";
#endif
#ifdef WITH_RESOLVER
   getopt_options += "nS";
#endif
   while ((ch = getopt_long(argc, argv, getopt_options.c_str(), longopts, NULL)) != -1) {
      switch (ch) {
         case 'd':
            pOpts->detail = true;
            break;
         case 'z':
            pOpts->zero = true;
            break;
         case 'b':
            pOpts->brief = true;
            break;
         case 'f':
            pOpts->detail = true;
            pOpts->full = true;
            break;
         case 'P':
            pOpts->pass = optarg;
            break;
         case 'h':
            pOpts->host = optarg;
            break;
         case 'p':
            try {
               pOpts->port = Utils::stol(optarg);
            }
            catch(string &s) {
               cerr << "Unknown port - " << s << endl;
               exit(1);
            }
            break;
         case 'H':
            pOpts->Hosts = Utils::SplitString(optarg, ",");
            break;
         case 'u':
            tempusers = optarg;
            Utils::ToLower(tempusers);
            pOpts->Users = Utils::SplitString(tempusers, ",");
            break;
#ifdef ENABLE_UI
         case 'o':
            pOpts->ui = false;
            break;
         case 'r':
            try {
               long int sec = Utils::stol(optarg);
               if (sec > 0)
                  pOpts->sleep_sec = Utils::stol(optarg);
               else
                  cerr << "Refresh interval should be greater than 0, using default - " << pOpts->sleep_sec << endl;
            }
            catch(string &s) {
               cerr << "Wrong number - " << s << endl;
               exit(1);
            }
            break;
#endif
#ifdef WITH_RESOLVER
         case 'n':
            pOpts->dns_resolution = false;
            break;
         case 'S':
            pOpts->strip_host_domain = false;
            break;
#endif
         case 'c':
            pOpts->compactsameurls = false;
            break;
         case 'Z':
            pOpts->strip_user_domain = false;
            break;
         default:
            usage(argv[0]);
            exit(0);
      }
   }

   // TODO:
   //struct sigaction sa = {};
   //sa.sa_handler = finish;
   //sigaction(SIGINT, &sa, NULL);

#ifdef ENABLE_UI
   if (pOpts->ui) {
      ncui *ui = new ncui(pOpts);
      ui->CursesInit();
      thread_args args;
      args.ui = ui;
      args.pOpts = pOpts;
#ifdef WITH_RESOLVER
      pOpts->pResolver->Start(MAX_THREADS);
      pOpts->pResolver->resolve_mode = Resolver::RESOLVE_ASYNC;
#endif

      pthread_t sq_thread;
      pthread_create(&sq_thread, NULL, (void *(*) (void *)) &squid_loop, (void *) &args);

      ui->Loop();

      pthread_cancel(sq_thread);
      ui->CursesFinish();
      delete ui;
   } else {
#endif
      sqstat sqs;
      SquidStats sqstats;
#ifdef WITH_RESOLVER
      pOpts->pResolver->resolve_mode = Resolver::RESOLVE_SYNC;
#endif
      pOpts->speed_mode = Options::SPEED_AVERAGE;
      try {
         sqstats = sqs.GetInfo(pOpts);
      }
      catch (sqstatException &e) {
         cerr << e.what() << endl;
         exit(1);
      }
      cout << sqstat::HeadFormat(pOpts, sqstats.total_connections, sqstats.connections.size(), sqstats.av_speed) << endl;
      cout << conns_format(pOpts, sqstats.connections) << endl;
#ifdef ENABLE_UI
   }
#endif

   delete pOpts;

   return 0;
}

// vim: ai ts=3 sts=3 et sw=3 expandtab
