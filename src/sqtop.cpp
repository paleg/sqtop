/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

//sleep
#include <unistd.h>
#include <getopt.h>
#include <cstdlib>
#include <iostream>
//stringstream
#include <sstream>

#include "config.h"

#ifdef ENABLE_UI
#include <thread>
#include <chrono>
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

void usage(char* argv) {
   cout << title << endl;
   cout << "version " << VERSION << " " << copyright << "(" << contacts << ")" << endl;
   cout << endl;
   cout << "Usage:";
   cout << "\n" << argv << " [--help] [--host host] [--port port] [--pass password] [--hosts host1,host...] [--users user1,user2] [--brief] [--detail] [--full] [--zero] [-c]";
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
#ifdef ENABLE_UI
   cout << "\n\t--once   (-o)                - disable interactive mode, just print statistics once to stdout;";
   cout << "\n\t--refreshinterval (-r) sec   - " << refresh_interval_help << ";";
#endif
#ifdef WITH_RESOLVER
   cout << "\n\t-n                           - do not " << dns_resolution_help << ";";
   cout << "\n\t-S                           - do not " << strip_domain_help << ".";
#endif
   cout << endl;
}

/*static void finish(int sig) {
   foad = sig;
}*/

bool DESC(int a, int b) {
   return a > b;
}

string conns_format(Options* pOpts, vector<SQUID_Connection> conns) {
   std::stringstream result;

   if (pOpts->compactsameurls)
      sqstat::CompactSameUrls(conns);

   for (auto & elem : conns) {
      if (((pOpts->Hosts.size() == 0) || Utils::IPMemberOf(pOpts->Hosts, elem.peer)) &&
         ((pOpts->Users.size() == 0) || Utils::UserMemberOf(pOpts->Users, elem.usernames))) {

         result << sqstat::ConnFormat(pOpts, elem);

         if (not pOpts->brief) {
            result << endl;
            for (auto & elem_stats : elem.stats) {
               result << sqstat::StatFormat(pOpts, elem, elem_stats);
               result << endl;
            }
         }
      result << endl;
      }
   }
   return result.str();
}

#ifdef ENABLE_UI
void squid_loop(ncui* ui, Options* pOpts) {
   sqstat sqs;
   vector<SQUID_Connection> stat;
   while (pOpts->quit != true) {
      if (pOpts->do_refresh) {
         try {
            stat = sqs.GetInfo(pOpts);
            ui->ClearError();
            ui->SetSpeeds(sqs.av_speed, sqs.curr_speed);
            ui->SetActiveConnCount(sqs.active_conn);
            ui->SetStat(stat);
         }
         catch (sqstatException &e) {
            ui->SetError(e.what());
         }
         ui->Tick();
      }
      for (int i=0; i < pOpts->sleep_sec * 10; ++i) {
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
         if (pOpts->quit) {
            break;
         }
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
            catch(const std::runtime_error& s) {
               cerr << "Unknown port - " << s.what() << endl;
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
            catch(const std::runtime_error& s) {
               cerr << "Wrong number - " << s.what() << endl;
               exit(1);
            }
            break;
#endif
#ifdef WITH_RESOLVER
         case 'n':
            pOpts->dns_resolution = false;
            break;
         case 'S':
            pOpts->strip_domain = false;
            break;
#endif
         case 'c':
            pOpts->compactsameurls = false;
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
#ifdef WITH_RESOLVER
      pOpts->pResolver->Start(MAX_THREADS);
      pOpts->pResolver->resolve_mode = Resolver::RESOLVE_ASYNC;
#endif

      std::thread sq_thread(squid_loop, ui, pOpts);

      ui->Loop();

      pOpts->quit = true;
      sq_thread.join();
      ui->CursesFinish();
      delete ui;
   } else {
#endif
      sqstat sqs;
      vector<SQUID_Connection> stat;
#ifdef WITH_RESOLVER
      pOpts->pResolver->resolve_mode = Resolver::RESOLVE_SYNC;
#endif
      pOpts->speed_mode = Options::SPEED_AVERAGE;
      try {
         stat = sqs.GetInfo(pOpts);
      }
      catch (sqstatException &e) {
         cerr << e.what() << endl;
         exit(1);
      }
      cout << sqstat::HeadFormat(pOpts, sqs.active_conn, stat.size(), sqs.av_speed) << endl;
      cout << conns_format(pOpts, stat) << endl;
#ifdef ENABLE_UI
   }
#endif

   delete pOpts;

   return 0;
}

// vim: ai ts=3 sts=3 et sw=3 expandtab
