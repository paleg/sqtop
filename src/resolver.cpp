/*
 * (C) 2011 Oleg V. Palij <o.palij@gmail.com>
 * Released under the GNU GPL, see the COPYING file in the source distribution for its full text.
 */

#include <iostream>
#include <iomanip>
#include <sys/socket.h>
#include <arpa/inet.h>
// sockaddr_in
#include <netinet/in.h>
#include <netdb.h>
#include <algorithm>
#include <unistd.h>
#include <errno.h>

#include "resolver.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::map;
using std::vector;

Resolver::Resolver() {
   resolve_func = "NONE";
   max_threads = 0;
}

void Resolver::Start() {
   max_threads = MAX_THREADS;
   init();
}

void Resolver::Start(int threads_num) {
   max_threads = threads_num;
   init();
}

void Resolver::init() {
#if !defined(USE_GETHOSTBYADDR_R) && !defined(THREADSAFE_GETNAMEINFO)
   /* getnameinfo (on *BSD) and gethostbyaddr is notreentrant */
   max_threads = 1;
#endif
   pthread_mutexattr_init(&mAttr);
   pthread_mutex_init(&rMutex, &mAttr);

   pthread_condattr_init(&cAttr);
   pthread_cond_init(&rCond, &cAttr);

   //cout << "Starting " << max_threads << " threads" << endl;
#if defined(USE_GETNAMEINFO)
   resolve_func = "GETNAMEINFO";
#elif defined(USE_GETHOSTBYADDR_R)
   resolve_func = "GETHOSTBYADDR_R";
#elif defined(USE_GETHOSTBYADDR)
   resolve_func = "GETHOSTBYADDR";
#endif
   //cout << "Using " << resolve_func << " in " << max_threads << " threads"<< endl;

   pThreadArgs = new ThreadArgs[max_threads];
   for(int i = 0; i < max_threads; i++) {
      pThreadArgs[i].pMain = this;
      pThreadArgs[i].ThreadNum = i;
      pthread_create(&pthWorker, NULL, (void *(*) (void *)) &Worker, (void *) &pThreadArgs[i]);
   }
}

Resolver::~Resolver() {
   pthread_mutex_destroy(&rMutex);
   pthread_mutexattr_destroy(&mAttr);

   pthread_cond_destroy(&rCond);
   pthread_condattr_destroy(&cAttr);

   delete [] pThreadArgs;
}

string Resolver::ResolveFunc() {
   return resolve_func;
}

int Resolver::MaxThreads() {
   return max_threads;
}

string Resolver::ResolveMode() {
   string result;
   switch (resolve_mode) {
      case RESOLVE_SYNC: result = "SYNC"; break;
      case RESOLVE_ASYNC: result = "ASYNC"; break;
   }
   return result;
}

/* static */ bool Resolver::IsIP(string ip) {
   struct sockaddr_in sa;
   int result = inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
   return result != 0;
}

/* static */ void Resolver::StripDomain(string& rName) {
   if (!IsIP(rName)) {
      size_t found = rName.find_first_of(".");
      if (found != string::npos) {
         rName.erase(rName.begin()+found, rName.end());
      }
   }
}

/* static */ void Resolver::Worker(void* pThreadArg) {
   ThreadArgs* pThreadArgs = reinterpret_cast<ThreadArgs*>(pThreadArg);
   Resolver* pMain = reinterpret_cast<Resolver*>(pThreadArgs->pMain);
   //int num = pThreadArgs->ThreadNum;
   pthread_mutex_lock(&pMain->rMutex);
   //cout << "------- worker (" << num << "): locked" << endl;
   string name;
   // /etc/hosts
   //sethostent(1);
   while (true) {
      //cout << "q size = " << pMain->queue.size() << endl;
      while (not pMain->queue.empty()) {
         string ip = pMain->queue.front();
         pMain->queue.pop_front();
         // workaround for the condition when ip deleted from queue but not resolved yet can be added again
         pMain->resolved[ip] = ip;
         pthread_mutex_unlock(&pMain->rMutex);
         string name = DoResolve(ip);
         pthread_mutex_lock(&pMain->rMutex);
         pMain->resolved[ip] = name;
         //cout << "------- worker (" << num << "): resolved " << ip << " to " << name << ". " << pMain->queue.size() << " to go." << endl;
      }
      //cout << "------- worker (" << num << "): waiting for cond" << endl;
      pthread_cond_wait(&pMain->rCond, &pMain->rMutex);
      //cout << "------- worker (" << num << "): awaiked " << pMain->queue.size() <<endl;
   }
}

string Resolver::Resolve(string ip) {
   string result;
   switch (resolve_mode) {
      case RESOLVE_SYNC: result = ResolveSync(ip); break;
      case RESOLVE_ASYNC: result = ResolveAsync(ip); break;
   }
   return result;
}

string Resolver::ResolveAsync(string ip) {
   //cout << "resolve: got " << ip << endl;
   pthread_mutex_lock(&rMutex);
   rit = resolved.find(ip);
   qit = std::find(queue.begin(), queue.end(), ip);
   string result;
   bool added = false;
   // already resolved
   if (rit != resolved.end()) {
      //cout << "resolve: already resolved" << endl;
      result = rit->second;
   // not resolved and not in queue
   } else if (qit == queue.end()) {
      //cout << "resolve: not resolved and not in queue" << endl;
      queue.push_back(ip);
      result = ip;
      added = true;
   // not resolved and already in queue
   } else {
      //cout << "resolve: not resolved and already in queue" << endl;
      result = ip;
   }
   pthread_mutex_unlock(&rMutex);
   if (added) {
      //cout << "resolve: signaling" << endl;
      pthread_cond_signal(&rCond);
   }
   return result;
}

string Resolver::ResolveSync(string ip) {
   return DoResolve(ip);
}

/* static */ string Resolver::DoResolve(string ip) {
   struct in_addr addr;
   if (!inet_aton(ip.c_str(), &addr)) {
      return ip;
   }
   string result;
   try {
      result = DoRealResolve(&addr);
   }
   catch (int n) {
      //cout << "Error resolving " << ip << ": " << n << endl;
      result = ip;
   }
   return result;
}

#ifdef USE_GETNAMEINFO
/* static */ string Resolver::DoRealResolve(struct in_addr* addr) {
   struct sockaddr_in sin = {0};
   int res;
   sin.sin_family = AF_INET;
   sin.sin_addr = *addr;
   sin.sin_port = 0;

   char buf[NI_MAXHOST];
   res = getnameinfo((struct sockaddr*)&sin, sizeof sin, buf, sizeof buf, NULL, 0, NI_NOFQDN);
   if (res == 0) {
      return buf;
   } else {
      throw res;
   }
}
#elif defined(USE_GETHOSTBYADDR)
/* static */ string Resolver::DoRealResolve(struct in_addr* addr) {
   struct hostent* he;
   he = gethostbyaddr((const void*)addr, sizeof addr, AF_INET);
   if (he) {
      return he->h_name;
   } else {
      throw h_errno;
   }
}
#elif defined(USE_GETHOSTBYADDR_R)
/* static */ string Resolver::DoRealResolve(struct in_addr* addr) {
   struct hostent hostbuf;
   struct hostent* hp;
   size_t hstbuflen = 1024;
   char* tmphstbuf;
   int res;
   int herr;
   bool error;

   tmphstbuf = (char*) malloc(hstbuflen);
   if (!tmphstbuf) abort();
   while ((res = gethostbyaddr_r((const void*)addr, sizeof addr, AF_INET,
                                 &hostbuf, tmphstbuf, hstbuflen,
                                 &hp, &herr)) == ERANGE) {
       /* Enlarge the buffer.  */
       hstbuflen *= 2;
       tmphstbuf = (char*) realloc(tmphstbuf, hstbuflen);
   }

   error = ((res != 0) || (hp == NULL));
   string result;
   if (not error) {
      result = hp->h_name;
   }
   free(tmphstbuf);
   if (error) {
      throw herr;
   }
   return result;
}
#else
/* static */ string Resolver::DoRealResolve(struct in_addr* addr) {
   throw -1;
}
#endif
