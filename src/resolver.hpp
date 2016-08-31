/*
 * (C) 2011 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#ifndef __RESOLVER_H
#define __RESOLVER_H

#include <string>
#include <list>
#include <vector>
#include <map>
#include <stdexcept>
#include <pthread.h>

#include "config.h"

#define MAX_THREADS 3

// # caching resolver. Sample usage:
//////
// Resolver* resolver = new Resolver();
//////
// # for sync resolve just do:
// cout << resolver.Resolve(string ip);
//////
// # or for async resolve:
// resolver.Start([int max_threads]);
// resolver.resolve_mode = Resolver::RESOLVE_ASYNC;
// cout << resolver.Resolve(string ip);
// # note: first call of Resolve() in async mode always return given ip, 
// #       next calls may return resolved value (if it has been catched)
//////
// delete resolver;
//////
class Resolver {
   public:
      Resolver();
      ~Resolver();

      void Start();
      void Start(int max_threads);

      enum ResolveMode { RESOLVE_SYNC, RESOLVE_ASYNC };
      ResolveMode resolve_mode;
      std::string Resolve(std::string ip);

      static bool IsIP(std::string ip);
      static void StripDomain(std::string& rName);

      std::string ResolveFunc();
      int MaxThreads();
      std::string ResolveMode();
      
   private:
      void init();

      std::string resolve_func;
      int max_threads;

      std::string ResolveSync(std::string ip);
      std::string ResolveAsync(std::string ip);

      std::map <std::string, std::string> resolved;
      std::map <std::string, std::string>::iterator rit;
      std::list <std::string> queue;
      std::list <std::string>::iterator qit;
      static std::string DoResolve(std::string ip);
      static std::string DoRealResolve(struct in_addr* addr);
      static void Worker(void* pThreadArg);

      pthread_mutex_t rMutex;
      pthread_mutexattr_t mAttr;

      pthread_cond_t rCond;
      pthread_condattr_t cAttr;

      struct ThreadArgs {
          pthread_t pthWorker;
          void* pMain;
          int ThreadNum;
      };
      ThreadArgs* pThreadArgs;
};

#endif /* __RESOLVER_H */
