
/*
 * instance.c
 *
 * (C) Copyright IBM Corp. 2005
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Common Public License from
 * http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
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

extern void setExFlag(unsigned long f);
extern char *parseTarget(const char *target);
extern UtilStringBuffer *instanceToString(CMPIInstance * ci, char **props);
extern int init_sfcBroker(char *);
extern CMPIBroker *Broker;
extern void initHttpProcCtl(int);
extern void initProvProcCtl(int);
extern void processTerminated(int pid);

extern TraceId traceIds[];
extern int sfcBrokerPid;

extern unsigned long exFlags;
int startHttp = 0;
char *name;
extern int collectStat;

extern unsigned long provSampleInterval;
extern unsigned long provTimeoutInterval;

extern void dumpTiming(int pid);

static char **restartArgv;
static int restartArgc;

static char * copyright = "(C) Copyright IBM Corp. 2005";

void clean_up(int sd, const char *the_file)
{
   close(sd);
   unlink(the_file);
}

static void handleSigChld(int sig)
{
   const int oerrno = errno;
   pid_t pid;
   int status;

   for (;;) {
      pid = wait3(&status, WNOHANG, (struct rusage *) 0);
      if ((int) pid == 0)
         break;
      if ((int) pid < 0) {
         if (errno == EINTR || errno == EAGAIN) {
            fprintf(stderr, "pid: %d continue \n", pid);
            continue;
         }
         if (errno != ECHILD)
            perror("child wait");
         break;
      }
      else {
 //        fprintf(stderr,"--- Provider process terminated - %d \n",pid);
      }
   }
   errno = oerrno;
}

static void handleSigterm(int sig)
{
   if (!terminating) {
      fprintf(stderr, "--- %s - %d exiting due to signal %d\n", processName, currentProc, sig);
      dumpTiming(currentProc);
   }   
   terminating=1; 
   if (providerProcess) 
      kill(currentProc,SIGKILL);
   exit(1);
}

static void restartSfcBroker(void *p)
{
   sleep(1);
   printf("\n--- \n--- Restarting sfcBroker\n---\n");
   
   execvp("sfcBroker",restartArgv);
   perror("--- execv for restart problem:");
   abort();
}

//SIGHUP
//static void handleSigUser(int sig)
static void handleSigHup(int sig)
{
  pthread_t t;
  pthread_attr_t tattr;
  
  if (sfcBrokerPid==currentProc) {    
      pthread_attr_init(&tattr);
      pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
      
      pthread_create(&t, &tattr, (void *(*)(void *))
         restartSfcBroker,NULL);

   } 
   else {
      fprintf(stderr, "--- %s - %d exiting due to signal %d\n", processName, currentProc, sig);
      kill(currentProc,SIGKILL);
   }   
}

static void handleSigSegv(int sig)
{
   fprintf(stderr, "-#- %s - %d exiting due to a SIGSEGV signal\n",
           processName, currentProc);
}

static void handleSigAbort(int sig)
{
   fprintf(stderr, "%s: exiting due to a SIGABRT signal - %d\n", processName, currentProc);
   kill(0, SIGTERM);
}

extern int httpDaemon(int argc, char *argv[], int sslMode, int pid);
extern void processProviderMgrRequests();

int startHttpd(int argc, char *argv[], int sslMode)
{
   int pid,sfcPid=currentProc;

   pid= fork();
   if (pid < 0) {
      perror("httpd fork");
      exit(2);
   }
   if (pid == 0) {
      currentProc=getpid();
      httpDaemon(argc, argv, sslMode, sfcPid);
      closeSocket(&sfcbSockets,cRcv,"startHttpd");
      closeSocket(&resultSockets,cAll,"startHttpd");
   }
   else {
      return 0;
   }
   return 0;
}

int main(int argc, char *argv[])
{
   int c, i;
   unsigned long tmask = 0, sslMode=0,sslOMode=0;
   int enableHttp=0,enableHttps=0,useChunking=0,doBa=0;
   long dSockets, pSockets;
   char *pauseStr;

   _SFCB_TRACE_INIT();

   name = strrchr(argv[0], '/');
   if (name != NULL) ++name;
   else name = argv[0];

   startHttp = 1;
   collectStat=0;
   processName="sfcBroker";
   provPauseStr=getenv("SFCB_PAUSE_PROVIDER");
   httpPauseStr=getenv("SFCB_PAUSE_CODEC");
   currentProc=sfcBrokerPid=getpid();
   restartArgc=argc;
   restartArgv=argv;

   printf("--- %s V" sfcHttpDaemonVersion " started - %d\n", name, currentProc);
   printf("--- (C) Copyright IBM Corp. 2004\n");

   for (c = 0, i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-d") == 0)
         daemon(0,0);
      else if (strcmp(argv[i], "-v") == 0)
         exFlags |= 1;
      else if (strcmp(argv[i], "-tm") == 0) {
         if (*argv[i+1]=='?') {
            fprintf(stdout,"--- -tm values:\n");
            for (i=0; traceIds[i].id; i++)  
               fprintf(stdout,"--- \t%18s: %d\n",traceIds[i].id,traceIds[i].code);
            exit(1);
         }
         if (isdigit(*argv[i + 1])) {
            tmask = atoi(argv[++i]);
         }
      }
      else if (strcmp(argv[i], "-F") == 0);
      else if (strcmp(argv[i], "-nF") == 0);
      else if (strcmp(argv[i], "-S") == 0) collectStat=1;
      else if (strcmp(argv[i], "-I") == 0) exFlags|=2;

      else {
         printf("--- Bad parameter: %s\n", argv[i]);
         exit(3);
      }
   }

   if (collectStat) remove("sfcbStat");
   
   _sfcb_set_trace_mask(tmask);

   setupControl(NULL);
//        SFCB_DEBUG
#ifndef SFCB_DEBUG
   if (tmask)
      printf("--- SCFB_DEBUG not configured. -tm %d ignored\n",tmask);
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
   printf("--- SSL not configured\n");
   enableHttps=0;
   sslMode=0;
   sslOMode=0;
#endif

   if (getControlBool("useChunking", &useChunking))
      useChunking=0;
   if (useChunking==0)
         printf("--- Chunking disabled\n");

   if (getControlBool("doBasicAuth", &doBa))
      doBa=0;
   if (!doBa)
      printf("--- User authentication disabled\n");

   if (getControlNum("httpProcs", &dSockets))
      dSockets = 10;
   if (getControlNum("provProcs", &pSockets))
      pSockets = 16;

   if (getControlNum("providerSampleInterval", &provSampleInterval))
      provSampleInterval = 10;
   if (getControlNum("providerTimeoutInterval", &provTimeoutInterval))
      provTimeoutInterval = 10;

   resultSockets=getSocketPair("sfcBroker result");
   sfcbSockets=getSocketPair("sfcBroker sfcb");

   initSem(dSockets,dSockets,pSockets);
   initProvProcCtl(pSockets);
   init_sfcBroker(NULL);
   initSocketPairs(pSockets,dSockets,0);

   setSignal(SIGTERM, handleSigterm,SA_ONESHOT);
   setSignal(SIGINT,  handleSigterm,0);
   setSignal(SIGKILL, handleSigterm,0);
   setSignal(SIGHUP, handleSigHup,SA_NOMASK);
   
   if (startHttp) {
      if (sslMode)
         startHttpd(argc, argv,1);
      if (!sslOMode)
         startHttpd(argc, argv,0);
   }
   
   setSignal(SIGSEGV, handleSigSegv,SA_ONESHOT);
   setSignal(SIGCHLD, handleSigChld,0);
   
   processProviderMgrRequests();

   return 0;
}
