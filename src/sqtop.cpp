/*
 * (C) 2006 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#ifdef ENABLE_UI
#include <pthread.h>
#endif

//sleep
#include <unistd.h>
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

using namespace sqtop;

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

void usage(char* argv) {
   cout << title << endl;
   cout << "version " << VERSION << " " << copyright << "(" << contacts << ")" << endl;
   cout << endl;
   cout << "Usage:";
   cout << "\n" << argv << " [--host host] [--port port] [--pass password] [--hosts host1,host...] [--users user1,user2] [--brief] [--full] [--zero] [--detail] [--nocompactsameurls] [--once] [--help]";
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

sqtop::Options* pOpts;

#ifdef ENABLE_UI
pthread_t sq_thread;
#endif

/*static void finish(int sig) {
   foad = sig;
}*/

bool DESC(int a, int b) {
   return a > b;
}

string conns_format(vector<SQUID_Connection> conns) {
   std::stringstream result;

   if (!pOpts->nocompactsameurls)
      sqstat::CompactSameUrls(conns);

   for (vector<SQUID_Connection>::iterator it = conns.begin(); it != conns.end(); ++it) {
      if (((pOpts->Hosts.size() == 0) || Utils::IPMemberOf(pOpts->Hosts, it->peer)) &&
         ((pOpts->Users.size() == 0) || Utils::UserMemberOf(pOpts->Users, it->usernames))) {

         result << sqstat::ConnFormat(pOpts, *it);

         if (not pOpts->brief) {
            result << endl;
            for (vector<Uri_Stats>::iterator itu = it->stats.begin(); itu != it->stats.end(); ++itu) {
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
};

void squid_loop(void* threadarg) {
   sqstat sqs;
   std::vector<SQUID_Connection> stat;
   thread_args* pArgs = reinterpret_cast<thread_args*>(threadarg);
   while (true) {
      if (pOpts->do_refresh) {
         try {
            stat = sqs.GetInfo(pOpts->host, pOpts->port, pOpts->pass);
            pArgs->ui->ClearError();
            pArgs->ui->SetSpeeds(sqs.av_speed, sqs.curr_speed);
            pArgs->ui->SetActiveConnCount(sqs.active_conn);
            pArgs->ui->SetStat(stat);
         }
         catch (sqstatException &e) {
            pArgs->ui->SetError(e.what());
         }
         pArgs->ui->Tick();
      }
      for (int i=0; i<pOpts->sleep_sec; ++i) {
         sleep(1);
      }
   }
}
#endif

int main(int argc, char **argv) {
   // TODO: config file ?
   int ch;
   string tempusers;
   pOpts = new Options();

   while ((ch = getopt_long(argc, argv, "r:u:H:h:p:P:dzbfoc", longopts, NULL)) != -1) {
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
#endif
         case 'c':
            pOpts->nocompactsameurls = true;
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

      pthread_create(&sq_thread, NULL, (void *(*) (void *)) &squid_loop, (void *) &args);

      ui->Loop();

      pthread_cancel(sq_thread);
      ui->CursesFinish();
      delete ui;
   } else {
#endif
      sqstat sqs;
      std::vector<SQUID_Connection> stat;
      try {
         stat = sqs.GetInfo(pOpts->host, pOpts->port, pOpts->pass);
      }
      catch (sqstatException &e) {
         cerr << e.what() << endl;
         exit(1);
      }
      cout << sqstat::HeadFormat(pOpts, sqs.active_conn, stat.size(), sqs.av_speed) << endl;
      cout << conns_format(stat) << endl;
#ifdef ENABLE_UI
   }
#endif

   delete pOpts;

   return 0;
}

// vim: ai ts=3 sts=3 et sw=3 expandtab
