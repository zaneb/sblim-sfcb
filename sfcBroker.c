
/*
 * sfcBroker.c
 *
 * (C) Copyright IBM Corp. 2005
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:        Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * sfcBroker Main.
 *
*/



#include <stdio.h>
#include "native.h"
#include "utilft.h"
#include "string.h"
#include "cimXmlParser.h"

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include "trace.h"
#include "msgqueue.h"
#include <pthread.h>

#include "cimXmlRequest.h"
#include "sfcVersion.h"
#include "control.h"

#include <getopt.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern void setExFlag(unsigned long f);
extern char *parseTarget(const char *target);
extern UtilStringBuffer *instanceToString(CMPIInstance * ci, char **props);
extern int init_sfcBroker(char *);
extern CMPIBroker *Broker;
extern void initProvProcCtl(int);
extern void processTerminated(int pid);
extern int httpDaemon(int argc, char *argv[], int sslMode, int pid);
extern void processProviderMgrRequests();

extern int stopNextProc();
extern int testStartedProc(int pid, int *left);

extern TraceId traceIds[];
extern int sfcBrokerPid;

extern unsigned long exFlags;
static int startHttp = 0;

#ifdef HAVE_JDBC
static int startDbp = 1;
#endif

char *name;
extern int collectStat;

extern unsigned long provSampleInterval;
extern unsigned long provTimeoutInterval;

extern void dumpTiming(int pid);

static char **restartArgv;
static int restartArgc;
static int adaptersStopped=0,providersStopped=0,restartBroker=0;

extern char * configfile;

typedef struct startedAdapter {
   struct startedAdapter *next;
   int stopped;
   int pid;
} StartedAdapter;

StartedAdapter *lastStartedAdapter=NULL;

static void addStartedAdapter(int pid)
{
   StartedAdapter *sa=(StartedAdapter*)malloc(sizeof(StartedAdapter));

   sa->stopped=0;
   sa->pid=pid;
   sa->next=lastStartedAdapter;
   lastStartedAdapter=sa;
}

static int testStartedAdapter(int pid, int *left) 
{
   StartedAdapter *sa=lastStartedAdapter;
   int stopped=0;
   
   *left=0;
   while (sa) {
      if (sa->pid==pid) stopped=sa->stopped=1;
      if (sa->stopped==0) (*left)++;
      sa=sa->next;
   }
   return stopped;
}         

static int stopNextAdapter()
{
   StartedAdapter *sa=lastStartedAdapter;
   
   while (sa) {
      if (sa->stopped==0) {
         sa->stopped=1;
         kill(sa->pid,SIGUSR1);
         return sa->pid;
      }   
      sa=sa->next;
   }
   return 0;
}

static pthread_mutex_t sdMtx=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  sdCnd=PTHREAD_COND_INITIALIZER;
static int stopping=0;
extern int remSem();   

static void stopBroker(void *p)
{
   struct timespec waitTime;
   int rc,sa=0,sp=0;
   
   stopping=1;
   
   stopLocalConnectServer();
   
   for(;;) {
      waitTime.tv_sec=time(NULL)+5;
      waitTime.tv_nsec=0;

      if (adaptersStopped==0) {
         pthread_mutex_lock(&sdMtx);
         if (sa==0) mlogf(M_INFO,M_SHOW,"--- Stopping adapters\n");
         sa++;
         if (stopNextAdapter()) {
            rc=pthread_cond_timedwait(&sdCnd,&sdMtx,&waitTime);
         }
//         else adaptersStopped=1;
         pthread_mutex_unlock(&sdMtx);
      }
      
      if (adaptersStopped) {
         pthread_mutex_lock(&sdMtx);
         if (sp==0) mlogf(M_INFO,M_SHOW,"--- Stopping providers\n");
         sp++;
         if (stopNextProc()) {
            rc=pthread_cond_timedwait(&sdCnd,&sdMtx,&waitTime);
         }   
         //else providersStopped=1;
         pthread_mutex_unlock(&sdMtx);
      }
      if (providersStopped) break;
   }
   remSem();
   
   uninit_sfcBroker();
   uninitProvProcCtl();
   uninitSocketPairs();
   sunsetControl();
//   uninitGarbageCollector();
   
   if (restartBroker) {
      char *emsg=strerror(errno);
      mlogf(M_INFO,M_SHOW,"---\n");   
      execvp("sfcbd",restartArgv);
      mlogf(M_ERROR,M_SHOW,"--- execv for restart problem: %s\n",emsg);
      abort();
   }

   else exit(0);
}   

static void signalBroker(void *p)
{
     pthread_mutex_lock(&sdMtx);
     pthread_cond_signal(&sdCnd);
     pthread_mutex_unlock(&sdMtx);
} 

#define LOCAL_SFCB

static void startLocalConnectServer()
{
#ifdef LOCAL_SFCB
   void localConnectServer();
   pthread_t t;
   pthread_attr_t tattr;
   
   pthread_attr_init(&tattr);
   pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
   pthread_create(&t, &tattr, (void *(*)(void *))localConnectServer,NULL);
#endif
}

static void handleSigquit(int sig)
{
   pthread_t t;
   pthread_attr_t tattr;
   
   if (sfcBrokerPid==currentProc) {    
      mlogf(M_INFO,M_SHOW, "--- Winding down %s\n", processName);
      pthread_attr_init(&tattr);
      pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);      
      pthread_create(&t, &tattr, (void *(*)(void *))stopBroker,NULL);
      uninitGarbageCollector();
   } 
}

static void handleSigHup(int sig)
{
  pthread_t t;
  pthread_attr_t tattr;
  
   if (sfcBrokerPid==currentProc) {    
      restartBroker=1;
      mlogf(M_INFO,M_SHOW, "--- Restarting %s\n", processName);
      pthread_attr_init(&tattr);
      pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);      
      pthread_create(&t, &tattr, (void *(*)(void *))stopBroker,NULL);
//      uninit_sfcBroker();
   } 
}

static void handleSigChld(int sig)
{
   const int oerrno = errno;
   pid_t pid;
   int status,left;
   pthread_t t;
   pthread_attr_t tattr;

   for (;;) {
      pid = wait3(&status, WNOHANG, (struct rusage *) 0);
      if ((int) pid == 0)
         break; 
      if ((int) pid < 0) {
         if (errno == EINTR || errno == EAGAIN) {
       //     mlogf(M_INFO,M_SHOW, "pid: %d continue \n", pid);
            continue;
         }
         if (errno != ECHILD) 
            perror("child wait");
         break;
      } 
      else {
//         mlogf(M_INFO,M_SHOW,"sigchild %d\n",pid);
        if (testStartedAdapter(pid,&left)) { 
            if (left==0) {
               mlogf(M_INFO,M_SHOW,"--- Adapters stopped\n");
               adaptersStopped=1;
            }   
            pthread_attr_init(&tattr);
            pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);      
            pthread_create(&t, &tattr, (void *(*)(void *))signalBroker,NULL);   
         }
         else if (testStartedProc(pid,&left)) {
            if (left==0) {
               mlogf(M_INFO,M_SHOW,"--- Providers stopped\n");
               providersStopped=1;
            }   
            pthread_attr_init(&tattr);
            pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);      
            pthread_create(&t, &tattr, (void *(*)(void *))signalBroker,NULL);
         }
      }
   }
   errno = oerrno;
}

static void handleSigterm(int sig)
{
   if (!terminating) {
      mlogf(M_ERROR,M_SHOW, "--- %s - %d exiting due to signal %d\n", processName, currentProc, sig);
      dumpTiming(currentProc);
   }   
   terminating=1; 
   if (providerProcess) 
      kill(currentProc,SIGKILL);
   exit(1);
}

static void handleSigSegv(int sig)
{
   mlogf(M_ERROR,M_SHOW, "-#- %s - %d exiting due to a SIGSEGV signal\n",
           processName, currentProc);
}
/*
static void handleSigAbort(int sig)
{
   mlogf(M_INFO,M_SHOW, "%s: exiting due to a SIGABRT signal - %d\n", processName, currentProc);
   kill(0, SIGTERM);
}
*/

static int startHttpd(int argc, char *argv[], int sslMode)
{
   int pid,sfcPid=currentProc;

   pid= fork();
   if (pid < 0) {
      char *emsg=strerror(errno);
      mlogf(M_ERROR,M_SHOW, "-#- http fork: %s",emsg);
      exit(2);
   }
   if (pid == 0) {
      currentProc=getpid();
      httpDaemon(argc, argv, sslMode, sfcPid);
      closeSocket(&sfcbSockets,cRcv,"startHttpd");
      closeSocket(&resultSockets,cAll,"startHttpd");
   }
   else {
      addStartedAdapter(pid);
      return 0;
   }
   return 0;
}

#ifdef HAVE_JDBC

extern int dbpDaemon(int argc, char *argv[], int sslMode, int sfcbPid);
static int startDbpd(int argc, char *argv[], int sslMode)
{
	int pid,sfcPid=currentProc;
	//sleep(2);
    pid= fork();
    if (pid < 0) {
       perror("dbpd fork");
       exit(2);
    }
    if (pid == 0) {
    	currentProc=getpid();
    	dbpDaemon(argc, argv, sslMode, sfcPid);
    	closeSocket(&sfcbSockets,cRcv,"startHttpd");
      	closeSocket(&resultSockets,cAll,"startHttpd");
    }
    else {
    	addStartedAdapter(pid);
       	return 0;
    }
    return 0;
}

#endif

static void usage(int status)
{
    if (status != 0)
	fprintf(stderr, "Try '%s --help' for more information.\n", name);

    else
    {
	static const char * help[] =
	    {
		"",
		"Options:",
		" -c, --config-file=<FILE>        use alternative configuration file",
		" -d, --daemon                    run in the background",
		" -h, --help                      display this message and exit",
		" -s, --collect-stats             turn on runtime statistics collecting",
		" -t, --trace-components=<N|?>    activate component-level tracing messages where",
		"                                 N is an OR-ed bitmask integer defining the",
		"                                 components to trace; ? lists the available",
		"                                 components with their bitmask and exits",
		" -v, --version                   output version information and exit",
		"",
		"For SBLIM package updates and additional information, please see",
		"    the SBLIM homepage at http://sblim.sourceforge.net"
	    };

	int i;

	fprintf(stdout, "Usage: %s [options]\n", name);
	for (i = 0; i < sizeof(help) / sizeof(char *); i++)
	    fprintf(stdout, "%s\n", help[i]);
    }

    exit(status);
}

static void version()
{
    fprintf(stdout, "%s " sfcHttpDaemonVersion "\n", name);

    exit(0);
}

int main(int argc, char *argv[])
{
   int c, i;
   unsigned long tmask = 0, sslMode=0,sslOMode=0;
   int enableHttp=0,enableHttps=0,useChunking=0,doBa=0,enableInterOp=0;
   long dSockets,sSockets,pSockets;
   char *pauseStr;

   _SFCB_TRACE_INIT();

   name = strrchr(argv[0], '/');
   if (name != NULL) ++name;
   else name = argv[0];

   startHttp = 1;
   collectStat=0;
   processName="sfcbd";
   provPauseStr=getenv("SFCB_PAUSE_PROVIDER");
   httpPauseStr=getenv("SFCB_PAUSE_CODEC");
   currentProc=sfcBrokerPid=getpid();
   restartArgc=argc;
   restartArgv=argv;

   startLogging("sfcb");

   exFlags = 0;

   static struct option const long_options[] =
       {
	   { "config-file",      required_argument, 0,        'c' },
	   { "daemon",           no_argument,       0,        'd' },
	   { "help",             no_argument,       0,        'h' },
	   { "collect-stats",    no_argument,       0,        's' },
	   { "trace-components", required_argument, 0,        't' },
	   { "version",          no_argument,       0,        'v' },
	   { 0, 0, 0, 0 }
       };

   while ((c = getopt_long(argc, argv, "c:dhst:v", long_options, 0)) != -1)
   {
       switch(c)
       {
	   case 0:
	       break;

	   case 'c':
	       configfile = strdup(optarg);
	       break;

	   case 'd':
	       daemon(0, 0);
	       break;

	   case 'h':
	       usage(0);

	   case 's':
	       collectStat = 1;
	       break;

	   case 't':
	       if (*optarg == '?') {
		   fprintf(stdout, "---   Traceable Components:     Int       Hex\n");
		   for (i = 0; traceIds[i].id; i++)
		       fprintf(stdout, "--- \t%18s:    %d\t0x%05X\n", traceIds[i].id, traceIds[i].code, traceIds[i].code);
		   exit(0);
	       } else if (isdigit(*optarg)) {
		   char *ep;
		   tmask = strtol(optarg, &ep, 0);
	       } else {
		   fprintf(stderr, "Try %s -t ? for a list of the trace components and bitmasks.\n", name);
		   exit(1);
	       }
	       break;

	   case 'v':
	       version();

	   default:
	       usage(3);
       }
   }

   mlogf(M_INFO,M_SHOW,"--- %s V" sfcHttpDaemonVersion " started - %d\n", name, currentProc);

   if (collectStat) {
       mlogf(M_INFO,M_SHOW,"--- Statistics collection enabled\n");
       remove("sfcbStat");
   }
   
   _SFCB_TRACE_START(1,tmask);

   setupControl(configfile);
//        SFCB_DEBUG
#ifndef SFCB_DEBUG
   if (tmask)
      mlogf(M_ERROR,M_SHOW,"--- SCFB_DEBUG not configured. -t %d ignored\n",tmask);
#endif

   if ((pauseStr=getenv("SFCB_PAUSE_PROVIDER"))) {
     printf("--- Provider pausing for: %s\n",pauseStr);
   }
      
   if (getControlBool("enableHttp", &enableHttp))
      enableHttp=1;
#if defined USE_SSL
   if (getControlBool("enableHttps", &enableHttps))
      enableHttps=0;
   sslMode=enableHttps;
   sslOMode=sslMode & !enableHttp;
#else
   mlogf(M_INFO,M_SHOW,"--- SSL not configured\n");
   enableHttps=0;
   sslMode=0;
   sslOMode=0;
#endif

   if (getControlBool("useChunking", &useChunking))
      useChunking=0;
   if (useChunking==0)
         mlogf(M_INFO,M_SHOW,"--- Chunking disabled\n");

   if (getControlBool("doBasicAuth", &doBa))
      doBa=0;
   if (!doBa)
      mlogf(M_INFO,M_SHOW,"--- User authentication disabled\n");

   if (getControlBool("enableInterOp", &enableInterOp))
       enableInterOp=0;

   if (getControlNum("httpProcs", &dSockets))
      dSockets = 10;
   if (getControlNum("httpsProcs", &sSockets))
      sSockets = 10;
   if (getControlNum("provProcs", &pSockets))
      pSockets = 16;

   if (getControlNum("providerSampleInterval", &provSampleInterval))
      provSampleInterval = 10;
   if (getControlNum("providerTimeoutInterval", &provTimeoutInterval))
      provTimeoutInterval = 10;

   resultSockets=getSocketPair("sfcbd result");
   sfcbSockets=getSocketPair("sfcbd sfcb");

   if (enableInterOp==0)
       mlogf(M_INFO,M_SHOW,"--- InterOp namespace disabled\n");
   else
       exFlags = exFlags | 2;

   initSem(dSockets,sSockets,pSockets);
   initProvProcCtl(pSockets);
   init_sfcBroker(NULL);
   initSocketPairs(pSockets,dSockets,sSockets);

   setSignal(SIGQUIT, handleSigquit,0);
   setSignal(SIGINT,  handleSigquit,0);
   
   setSignal(SIGTERM, handleSigterm,0); /* kill cannot be intercepted */
   setSignal(SIGHUP,  handleSigHup,0);
   
   startLocalConnectServer();
   
   if (startHttp) {
      if (sslMode)
         startHttpd(argc, argv,1);
      if (!sslOMode)
         startHttpd(argc, argv,0);
   }
   
#ifdef HAVE_JDBC
   //Start dbProtocol-Daemon
   if (startDbp) {
      if (sslMode)
         startDbpd(argc, argv,1);
      if (!sslOMode)
         startDbpd(argc, argv,0);
   }
#endif	
   
   setSignal(SIGSEGV, handleSigSegv,SA_ONESHOT);
   setSignal(SIGCHLD, handleSigChld,0);
   
   processProviderMgrRequests();

   return 0;
}
