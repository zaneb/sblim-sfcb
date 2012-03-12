
/*
 * httpAdapter.c
 *
 * © Copyright IBM Corp. 2005, 2007
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
 * httpAdapter implementation.
 *
*/
 

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <dlfcn.h>

#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>

#include "cmpidt.h"
#include "msgqueue.h"
#include "utilft.h"
#include "trace.h"
#include "cimXmlRequest.h"
#include "support.h"

#include <pthread.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_FSUID_H
#include <sys/fsuid.h>
#endif

#include "httpComm.h"
#include "sfcVersion.h"
#include "control.h"

#ifdef HAVE_UDS
#include <grp.h>
#endif

unsigned long exFlags = 0;
static char *name;
static int debug;
static int doBa;
#ifdef HAVE_UDS
static int doUdsAuth;
#endif
static int doFork = 0;
int noChunking = 0;
int sfcbSSLMode = 0;
int httpLocalOnly = 0;  /* 1 = only listen on loopback interface */
static int hMax;
static int httpProcIdX;
static int stopAccepting=0;
static int running=0;
static long keepaliveTimeout=15;
static long keepaliveMaxRequest=10;
static long numRequest;
static long     selectTimeout = 5; /* default 5 sec. timeout for select() before read() */
struct timeval httpSelectTimeout = {0, 0};

#if defined USE_SSL
static SSL_CTX *ctx;
static X509    *x509 = NULL;
#define CC_VERIFY_IGNORE  0
#define CC_VERIFY_ACCEPT  1
#define CC_VERIFY_REQUIRE 2
int ccVerifyMode = CC_VERIFY_IGNORE;
static int get_cert(int,X509_STORE_CTX*);
static int ccValidate(X509*, char**, int);
#endif

/* return codes used by baValidate */
#define AUTH_PASS 1
#define AUTH_FAIL 0
#define AUTH_EXPIRED -1

static key_t httpProcSemKey;
static key_t httpWorkSemKey;
static int httpProcSem;
static int httpWorkSem;

extern char *decode64(char *data);
extern void libraryName(const char* dir, const char *location, char *fullName, int buf_size);
extern void *loadLibib(const char *libname);
extern int getControlChars(char *id, char **val);
extern void *loadLibib(const char *libname);

extern RespSegments genFirstChunkResponses(BinRequestContext*,BinResponseHdr**,int,int);
extern RespSegments genLastChunkResponses(BinRequestContext*, BinResponseHdr**, int);
extern RespSegments genChunkResponses(BinRequestContext*, BinResponseHdr**, int);
extern RespSegments genFirstChunkErrorResponse(BinRequestContext * binCtx, int rc, char *msg);
extern char *getErrTrailer(int id, int rc, char *m);
extern void dumpTiming(int pid);
extern char * configfile;
extern int inet_aton(const char *cp, struct in_addr *inp);

static unsigned int sessionId;
extern char *opsName[];

struct auth_extras {
  void* (*release)(void*);
  char* clientIp;
  void* authHandle;
  const char* role;
};
typedef struct auth_extras AuthExtras;

typedef int     (*Authenticate) (char *principal, char *pwd);
typedef int     (*Authenticate2) (char *principal, char *pwd, AuthExtras *extras);

typedef struct _buffer {
   char *data, *content;
   int length, size, ptr;
   unsigned int content_length;
   int trailers;
   char *httpHdr, *authorization, *content_type, *host, *useragent;
   char *principal;
   char *protocol;
#if defined USE_SSL
   X509 *certificate;
#endif
} Buffer;

#ifdef HAVE_IPV6
 #define USE_INET6
#endif

#ifdef USE_INET6
static int fallback_ipv4;
#endif

void initHttpProcCtl(int p)
{
   httpProcSemKey=ftok(SFCB_BINARY,'H');
   httpWorkSemKey=ftok(SFCB_BINARY,'W');
   union semun sun;
   int i;

   mlogf(M_INFO,M_SHOW,"--- Max Http procs: %d\n", p);
   if ((httpProcSem=semget(httpProcSemKey,1,0600))!=-1) 
      semctl(httpProcSem,0,IPC_RMID,sun);
      
   if ((httpProcSem=semget(httpProcSemKey,1+p,IPC_CREAT | IPC_EXCL | 0600))==-1) {
      char *emsg=strerror(errno);
      mlogf(M_ERROR,M_SHOW,"\n--- Http Proc semaphore create key: 0x%x failed: %s\n",httpProcSemKey,emsg);
      mlogf(M_ERROR,M_SHOW,"     use \"ipcrm -S 0x%x\" to remove semaphore\n\n",httpProcSemKey);
      abort();
   }
   sun.val=p;
   semctl(httpProcSem,0,SETVAL,sun);
   
   sun.val=0;
   for (i=1; i<=p; i++)
      semctl(httpProcSem,i,SETVAL,sun);

   if ((httpWorkSem=semget(httpWorkSemKey,1,0600))!=-1) 
      semctl(httpWorkSem,0,IPC_RMID,sun);
      
   if ((httpWorkSem=semget(httpWorkSemKey,1,IPC_CREAT | IPC_EXCL | 0600))==-1) {
      char *emsg=strerror(errno);
      mlogf(M_ERROR,M_SHOW,"\n--- Http ProcWork semaphore create key: 0x%x failed: %s\n",httpWorkSemKey,emsg);
      mlogf(M_ERROR,M_SHOW,"     use \"ipcrm -S 0x%x\" to remove semaphore\n\n",httpProcSemKey);
      abort();
   }
   sun.val=1;
   semctl(httpWorkSem,0,SETVAL,sun);
}

int remProcCtl()
{
   semctl(httpProcSem,0,IPC_RMID,0);
   semctl(httpWorkSem,0,IPC_RMID,0);
   return 0;
}

/*
 * Call the authentication library
 * Return 1 on success, 0 on fail, -1 on expired
 */
int baValidate(char *cred, char **principal, AuthExtras* extras)
{
   char *auth,*pw=NULL;
   int i;
   static void *authLib=NULL;
   static Authenticate authenticate=NULL;
   static Authenticate2 authenticate2=NULL;
   char dlName[512];
   int ret = AUTH_FAIL;
   char *entry;

   if (strncasecmp(cred,"basic ",6)) return AUTH_FAIL;
   auth=decode64(cred+6);
   for (i=0; i<strlen(auth); i++) {
      if (auth[i]==':') {
          auth[i]=0;
          pw=&auth[i+1];
          break;
      }
   }

   if (authLib==NULL) {
      char *ln;
      if (getControlChars("basicAuthlib", &ln)==0) {
         libraryName(NULL,ln,dlName, 512);
	 if ((authLib = dlopen(dlName, RTLD_LAZY)) && (getControlChars("basicAuthEntry", &entry) == 0)) {
	   if (strcmp(entry, "_sfcBasicAuthenticate2") == 0)
	     authenticate2 = dlsym(authLib, entry);
	   else 
	     authenticate = dlsym(authLib, entry);

         }
      }
   }

  if (authenticate2 == NULL && authenticate == NULL) {
    mlogf(M_ERROR, M_SHOW, "--- Authentication exit %s not found\n",
	  dlName);
    ret = AUTH_FAIL;
  }
  else {
     *principal=strdup(auth);
     if (authenticate2)
       ret = authenticate2(auth, pw, extras);
     else 
       ret = authenticate(auth, pw);

     if (ret == AUTH_PASS)  ret = AUTH_PASS;
     else if (ret == AUTH_EXPIRED)  ret = AUTH_EXPIRED;
     else  ret = AUTH_FAIL;
   }

   free(auth);
   return ret;
}

static void handleSigChld(int sig)
{
   const int oerrno = errno;
   pid_t pid;
   int status;

   for (;;) {
      pid = wait4(0, &status, WNOHANG, NULL);
      if ((int) pid == 0)
         break;
      if ((int) pid < 0) {
         if (errno == EINTR || errno == EAGAIN) {
    //        fprintf(stderr, "pid: %d continue \n", pid);
            continue;
         }
         if (errno != ECHILD)
            perror("child wait");
         break;
      }
      else {
         running--;
         // fprintf(stderr, "%s: SIGCHLD signal %d - %s(%d)\n", name, pid,
         //        __FILE__, __LINE__);
      }
   }
   errno = oerrno;
}

static void stopProc(void *p)
{
//   printf("--- %s draining %d\n",processName,running);
   for (;;) {
      if (running==0) {
         mlogf(M_INFO,M_SHOW,"--- %s terminating %d\n",processName,getpid());
         exit(0);
      }   
      sleep(1);
   }   
}

static void handleSigUsr1(int sig)
{
   pthread_t t;
   pthread_attr_t tattr;

   if (stopAccepting == 0) {
     stopAccepting=1;
     pthread_attr_init(&tattr);
     pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);      
     pthread_create(&t, &tattr, (void *(*)(void *))stopProc,NULL);
   }
}

static void freeBuffer(Buffer * b)
{
   Buffer emptyBuf = { NULL, NULL, 0, 0, 0, 0, 0 , NULL, NULL, NULL, NULL, NULL, NULL, NULL};
   if (b->data)
      free(b->data);
   if (b->content)
      free(b->content);
   if (b->principal)
      free(b->principal);
   *b=emptyBuf;   
}

static char *getNextHdr(Buffer * b)
{
   int i;
   char c;

   for (i = b->ptr; b->ptr < b->length; ++b->ptr) {
      c = b->data[b->ptr];
      if (c == '\n' || c == '\r') {
         b->data[b->ptr] = 0;
         ++b->ptr;
         if (c == '\r' && b->ptr < b->length && b->data[b->ptr] == '\n') {
            b->data[b->ptr] = 0;
            ++b->ptr;
         }
         return &(b->data[i]);
      }
   }
   return NULL;
}

static void add2buffer(Buffer * b, char *str, size_t len)
{
   if (b->size == 0) {
      b->size = len + 500;
      b->length = 0;
      b->data = (char *) malloc(b->size);
   }
   else if (b->length + len >= b->size) {
      b->size = b->length + len + 500;
      b->data = (char *) realloc((void *) b->data, b->size);
   }
   memmove(&((b->data)[b->length]), str, len);
   b->length += len;
   (b->data)[b->length] = 0;
}

static void genError(CommHndl conn_fd, Buffer * b, int status, char *title,
                     char *more)
{
   char head[1000];
   char server[] = "Server: sfcHttpd\r\n";
   char clength[] = "Content-Length: 0\r\n";
   char cclose[] = "Connection: close\r\n";
   char end[] = "\r\n";

   _SFCB_ENTER(TRACE_HTTPDAEMON, "genError");

   snprintf(head, sizeof(head), "%s %d %s\r\n", b->protocol, status, title);
//   fprintf(stderr,"+++ genError: %s\n",head);
   _SFCB_TRACE(1,("--- genError response: %s",head));
   commWrite(conn_fd, head, strlen(head));
   if (more) {
      commWrite(conn_fd, more, strlen(more));
   }

   commWrite(conn_fd, server, strlen(server));
   commWrite(conn_fd, clength, strlen(clength));
   if (keepaliveTimeout==0 || numRequest >= keepaliveMaxRequest ) {
     _SFCB_TRACE(1,("--- closing after error\n"));
     commWrite(conn_fd, cclose, strlen(cclose));
   }
   commWrite(conn_fd, end, strlen(end));
   commFlush(conn_fd);
}

static int readData(CommHndl conn_fd, char *into, int length)
{
   int c = 0, r, isReady;
   fd_set httpfds;
   FD_ZERO(&httpfds);
   FD_SET(conn_fd.socket,&httpfds);

   while (c < length) {
      isReady = select(conn_fd.socket+1,&httpfds,NULL,NULL,&httpSelectTimeout);
      if (isReady == 0) {
         c = -1;
         break;
      }
      r = commRead(conn_fd, into + c, length - c);
      if (r < 0) {
        if (errno == EINTR || errno == EAGAIN) {
         continue;
        }
        else {
          mlogf(M_INFO,M_SHOW,"--- readData(): read() error %s\n", strerror(errno));
          c = -2;
          break;
        }
      }
      /* r==0 is a success condition for read(), but the loop should complete prior to this */
      else if (r == 0) {
         mlogf(M_INFO,M_SHOW,"--- commRead hit EOF sooner than expected\n");
         c = -2;
	 break;
      }
      c += r;
   }
   return c;
}

static int getPayload(CommHndl conn_fd, Buffer * b)
{
   unsigned int c = b->length - b->ptr;
   int rc = 0;

   if (c > b->content_length) {
     mlogf(M_INFO, M_SHOW,
	   "--- HTTP Content-Length is lying; rejecting %d %d\n", c, b->content_length);
     return -1;
   }

   b->content = (char *) malloc(b->content_length + 8);
   if (c) memcpy(b->content, (b->data) + b->ptr, c);

   rc = readData(conn_fd, (b->content) + c, b->content_length - c);
   *((b->content) + b->content_length) = 0;
   return rc;
}

void dumpResponse(RespSegments * rs)
{
   int i;
   if (rs) {
      for (i = 0; i < 7; i++) {
         if (rs->segments[i].txt) {
            if (rs->segments[i].mode == 2) {
               UtilStringBuffer *sb = (UtilStringBuffer *) rs->segments[i].txt;
               printf("%s", sb->ft->getCharPtr(sb));
            }
            else
               printf("%s", rs->segments[i].txt);
         }
      }
      printf("<\n");
   }
}

static void writeResponse(CommHndl conn_fd, RespSegments rs)
{

   static char head[] = {"HTTP/1.1 200 OK\r\n"};
   static char cont[] = {"Content-Type: application/xml; charset=\"utf-8\"\r\n"};
   static char cach[] = {"Cache-Control: no-cache\r\n"};
   static char op[]   = {"CIMOperation: MethodResponse\r\n"};
   static char cclose[] = "Connection: close\r\n";
   static char end[]  = {"\r\n"};
   char str[256];
   int len, i, ls[8];

   _SFCB_ENTER(TRACE_HTTPDAEMON, "writeResponse");
   
   for (len = 0, i = 0; i < 7; i++) {
      if (rs.segments[i].txt) {
         if (rs.segments[i].mode == 2) {
            UtilStringBuffer *sb = (UtilStringBuffer *) rs.segments[i].txt;
            if (sb == NULL)
               ls[i] = 0;
            else
               len += ls[i] = sb->ft->getSize(sb);
         }
         else
            len += ls[i] = strlen(rs.segments[i].txt);
      }
   }

   commWrite(conn_fd, head, strlen(head));
   commWrite(conn_fd, cont, strlen(cont));
   sprintf(str, "Content-Length: %d\r\n", len);
   commWrite(conn_fd, str, strlen(str));
   commWrite(conn_fd, cach, strlen(cach));
   commWrite(conn_fd, op, strlen(op));
   if (keepaliveTimeout == 0 || numRequest >= keepaliveMaxRequest) {
     commWrite(conn_fd, cclose, strlen(cclose));
   }
   commWrite(conn_fd, end, strlen(end));

   for (len = 0, i = 0; i < 7; i++) {
      if (rs.segments[i].txt) {
         if (rs.segments[i].mode == 2) {
            UtilStringBuffer *sb = (UtilStringBuffer *) rs.segments[i].txt;
            commWrite(conn_fd, (void*)sb->ft->getCharPtr(sb), ls[i]);
            sb->ft->release(sb);
         }
         else {
            commWrite(conn_fd, rs.segments[i].txt, ls[i]);
            if (rs.segments[i].mode == 1)
               free(rs.segments[i].txt);
         }
      }
   }

   commFlush(conn_fd);
   
   _SFCB_EXIT();
}


static void writeChunkHeaders(BinRequestContext *ctx)
{
   static char head[] = {"HTTP/1.1 200 OK\r\n"};
   static char cont[] = {"Content-Type: application/xml; charset=\"utf-8\"\r\n"};
   static char cach[] = {"Cache-Control: no-cache\r\n"};
   static char op[]   = {"CIMOperation: MethodResponse\r\n"};
   static char tenc[] = {"Transfer-encoding: chunked\r\n"};
   static char trls[] = {"Trailer: CIMError, CIMStatusCode, CIMStatusCodeDescription\r\n"};
   static char cclose[] = "Connection: close\r\n";

   _SFCB_ENTER(TRACE_HTTPDAEMON, "writeChunkHeaders");
   
   commWrite(*(ctx->commHndl), head, strlen(head));
   commWrite(*(ctx->commHndl), cont, strlen(cont));
   commWrite(*(ctx->commHndl), cach, strlen(cach));
   commWrite(*(ctx->commHndl), op, strlen(op));
   commWrite(*(ctx->commHndl), tenc, strlen(tenc));
   commWrite(*(ctx->commHndl), trls, strlen(trls));
   if (keepaliveTimeout == 0 || numRequest >= keepaliveMaxRequest) {
     commWrite(*(ctx->commHndl), cclose, strlen(cclose));
   }
   commFlush(*(ctx->commHndl));
   
   _SFCB_EXIT();
}

static void writeChunkResponse(BinRequestContext *ctx, BinResponseHdr *rh)
{
   int i,len,ls[8];
   char str[256];
   RespSegments rs;
   _SFCB_ENTER(TRACE_HTTPDAEMON, "writeChunkResponse");
   switch (ctx->chunkedMode) {
   case 1:
      _SFCB_TRACE(1,("---  writeChunkResponse case 1"));
      if (rh->rc!=1) {
         RespSegments rs;
         rs=genFirstChunkErrorResponse(ctx, rh->rc-1, NULL);
         writeResponse(*ctx->commHndl, rs);
         _SFCB_EXIT();
     }  
      writeChunkHeaders(ctx);
  /*    if (rh->rc!=1) {
         _SFCB_TRACE(1,("---  writeChunkResponse case 1 error"));
         rh->moreChunks=0;
         break;
      } */     
      rs=genFirstChunkResponses(ctx, &rh, rh->count,rh->moreChunks);
      ctx->chunkedMode=2;
      break;
   case 2:
      _SFCB_TRACE(1,("---  writeChunkResponse case 2"));
      if (rh->rc!=1) {
         _SFCB_TRACE(1,("---  writeChunkResponse case 2 error"));
         rh->moreChunks=0;
         break;
      }   
      if (rh->moreChunks || ctx->pDone<ctx->pCount)
         rs=genChunkResponses(ctx, &rh, rh->count);
      else {
         rs=genLastChunkResponses(ctx, &rh, rh->count);
      }
      break;
   }

   if (rh->rc==1) {
   
      for (len = 0, i = 0; i < 7; i++) {
         if (rs.segments[i].txt) {
            if (rs.segments[i].mode == 2) {
               UtilStringBuffer *sb = (UtilStringBuffer *) rs.segments[i].txt;
               if (sb == NULL) ls[i] = 0;
               else len += ls[i] = sb->ft->getSize(sb);
            }
            else len += ls[i] = strlen(rs.segments[i].txt);
         }
      }
      /*  
       * make sure we do not have a 0 len , this would 
       * indicate the end of the chunk data. 
       */
      if(len != 0) {
        sprintf(str, "\r\n%x\r\n",len);     
        commWrite(*(ctx->commHndl), str, strlen(str));
        _SFCB_TRACE(1,("---  writeChunkResponse chunk amount %x ",len));
      }
      
      for (len = 0, i = 0; i < 7; i++) {
         if (rs.segments[i].txt) {
            if (rs.segments[i].mode == 2) {
               UtilStringBuffer *sb = (UtilStringBuffer *) rs.segments[i].txt;
               commWrite(*(ctx->commHndl), (void*)sb->ft->getCharPtr(sb), ls[i]);
               sb->ft->release(sb);
            }
            else {
               commWrite(*(ctx->commHndl), rs.segments[i].txt, ls[i]);
               if (rs.segments[i].mode == 1)
                  free(rs.segments[i].txt);
            }

         }
      }
   }
    
   if (rh->moreChunks==0 && ctx->pDone>=ctx->pCount) {
      char *eStr = "\r\n0\r\n";
      char status[512];
      char *desc=NULL;
      
      _SFCB_TRACE(1,("---  writing trailers"));
      
      sprintf(status,"CIMStatusCode: %d\r\n",(int)(rh->rc-1));
      if (rh->rc!=1) desc=getErrTrailer(73,rh->rc-1,NULL);

      commWrite(*(ctx->commHndl), eStr, strlen(eStr));
      commWrite(*(ctx->commHndl), status, strlen(status));
      if (desc) {
         commWrite(*(ctx->commHndl), desc, strlen(desc));
         free(desc);
      }   
      eStr="\r\n";
      commWrite(*(ctx->commHndl), eStr, strlen(eStr));
   }
   commFlush(*(ctx->commHndl));
   _SFCB_EXIT();
}

static ChunkFunctions httpChunkFunctions = {
   writeChunkResponse,
};


#define hdrBufsize 5000
#define hdrLimmit 5000

static int getHdrs(CommHndl conn_fd, Buffer * b, char *cmd)
{
   int first=1,total=0,isReady;
   fd_set httpfds;
   int state=0;
   
   FD_ZERO(&httpfds);
   FD_SET(conn_fd.socket,&httpfds);
	
   for (;;) {
      isReady = select(conn_fd.socket+1,&httpfds,NULL,NULL,&httpSelectTimeout);
      if (isReady == 0) return 3;

      char buf[hdrBufsize];
      int r = commRead(conn_fd, buf, sizeof(buf));
      
      if (r < 0) {
        if (errno == EINTR || errno == EAGAIN) {
          continue;
        }
        else {
          mlogf(M_INFO,M_SHOW,"--- getHdrs: read() error %s\n", strerror(errno));
          state = 3;
          break;
        }
      }
      if (r == 0) {
        if (b->size == 0) break;
        if (strstr(b->data, "\r\n\r\n") == NULL &&
            strstr(b->data, "\n\n") == NULL) {
	        mlogf(M_ERROR,M_SHOW,"-#- HTTP header ended prematurely\n");
      	    state = 3;
	        break; 
	    }
      }
      
      add2buffer(b, buf, r);
      total+=r;

      /* on first run through, ensure that this is a POST req. */
      if (r && first) {
         if (strncasecmp(buf,cmd,strlen(cmd)) != 0) { 
	   /* not what we expected - still continue to read to
	      not confuse the client */
	   state = 1;
	 }
         first=0;
      }

      /* success condition: end of header */
      if (strstr(b->data, "\r\n\r\n") != NULL ||
          strstr(b->data, "\n\n") != NULL) {
         break;
      }
      
      if (total>=hdrLimmit) {
         mlogf(M_ERROR,M_SHOW,"-#- Possible DOS attempt detected\n");
	 state = 2;
         break;
      }
   }
   return state;
}

int pauseCodec(char *name)
{
   int rc=0;
   char *n;
  
   if (noHttpPause) return 0;
   if (httpPauseStr==NULL) {
      if (httpPauseStr) {
         char *p;
         p=httpPauseStr=strdup(httpPauseStr);
         while (*p) { *p=tolower(*p); p++; }
      }
   }   
   if (httpPauseStr) {
      char *p;
      int l=strlen(name);
      p=n=strdup(name);      
      while (*p) { *p=tolower(*p); p++; }
      if ((p=strstr(httpPauseStr,n))!=NULL) {
         if ((p==httpPauseStr || *(p-1)==',') && (p[l]==',' || p[l]==0)) rc=1;
      }
      free(n);
      return rc;
  }
   noHttpPause=1;
   return 0;
}

static int doHttpRequest(CommHndl conn_fd)
{
   char *cp;
   Buffer inBuf = { NULL, NULL, 0, 0, 0, 0, 0 ,0};
   RespSegments response;
   static RespSegments nullResponse = { NULL, 0, 0, NULL, { {0, NULL} } };
   unsigned long len;
   int hl, rc,uset=0;
   char *hdr, *path;
   int discardInput=0;
   MsgSegment msgs[2];
   struct rusage us,ue;
   struct timeval sv, ev;
   CimXmlRequestContext ctx;
   int breakloop;
   int hcrFlags = 0;  /* flags to pass to handleCimRequest() */

   _SFCB_ENTER(TRACE_HTTPDAEMON, "doHttpRequest");

   if (pauseCodec("http")) for (breakloop=0;breakloop==0;) {
      fprintf(stdout,"-#- Pausing for codec http %d\n",currentProc);
      sleep(5);
   }
      
   inBuf.authorization = "";
   inBuf.protocol="HTTP/1.1";
   inBuf.content_type = NULL;
   inBuf.content_length = UINT_MAX;
   inBuf.host = NULL;
   inBuf.useragent = "";
   int badReq = 0;

   rc=getHdrs(conn_fd, &inBuf,"POST ");

   if (rc==1) { 
      genError(conn_fd, &inBuf, 501, "Not Implemented", NULL);
      /* we continue to parse headers and empty the socket
	 to be graceful with the client */
      discardInput=1;
   }   
   else if (rc==2) {
     genError(conn_fd, &inBuf, 400, "Bad Request", NULL);
      discardInput=2;
      _SFCB_TRACE(1, ("--- potential DOS attempt discovered."));      
   }
   else if (rc==3) {
      genError(conn_fd, &inBuf, 400, "Bad Request", NULL);
      _SFCB_TRACE(1, ("--- exiting after request timeout."));
      commClose(conn_fd);
      exit(1);
   }
            
   if (inBuf.size == 0) {
     /* no buffer data - end of file - quit */
     _SFCB_TRACE(1,("--- HTTP connection EOF, quit %d ", currentProc));   
     commClose(conn_fd);
     exit(1);
   }
   
   inBuf.httpHdr = getNextHdr(&inBuf);
   for (badReq = 1;;) {
      if (inBuf.httpHdr == NULL)
         break;
      path = strpbrk(inBuf.httpHdr, " \t\r\n");
      if (path == NULL)
         break;
      *path++ = 0;
      _SFCB_TRACE(1,("--- Header: %s",inBuf.httpHdr));
      path += strspn(path, " \t\r\n");
      inBuf.protocol = strpbrk(path, " \t\r\n");
      if (inBuf.protocol == NULL)
         break;
      *inBuf.protocol++ = 0;
#ifdef SFCB_CANONICAL_URI
      if (strcmp(path, "/cimom") != 0)
         break;
#endif
      badReq = 0;
   }

   if (badReq) {
     genError(conn_fd, &inBuf, 400, "Bad Request", NULL);
      _SFCB_TRACE(1, ("--- exiting after malformed header."));      
      commClose(conn_fd);
      exit(1);
   }

   while ((hdr = getNextHdr(&inBuf)) != NULL) {
      _SFCB_TRACE(1,("--- Header: %s",hdr));
      if (hdr[0] == 0)
         break;
      else if (strncasecmp(hdr, "Authorization:", 14) == 0) {
         cp = &hdr[14];
         cp += strspn(cp, " \t");
         inBuf.authorization = cp;
      }
      else if (strncasecmp(hdr, "Content-Length:", 15) == 0) {
         cp = &hdr[15];
         cp += strspn(cp, " \t");
         if (cp[0] == '-') {
           genError(conn_fd, &inBuf, 400, "Negative Content-Length", NULL);
           _SFCB_TRACE(1, ("--- exiting: content-length too big"));
           commClose(conn_fd);
           exit(1);
         }
         errno = 0; 
         unsigned long clen = strtoul(cp, NULL, 10);
         if (errno != 0) {
           genError(conn_fd, &inBuf, 400, "Error converting Content-Length to a decimal value", NULL);
           _SFCB_TRACE(1, ("--- exiting: content-length conversion error"));
           commClose(conn_fd);
           exit(1);
         }
         unsigned int maxLen;
         if ((getControlUNum("httpMaxContentLength", &maxLen) != 0) || (maxLen == 0)) {
           genError(conn_fd, &inBuf, 501, "Server misconfigured (httpMaxContentLength)", NULL);
           _SFCB_TRACE(1, ("--- exiting: bad config httpMaxContentLength"));
           commClose(conn_fd);
           exit(1);
         }
         if((clen >= UINT_MAX) || ((maxLen) && (clen > maxLen))) {
            genError(conn_fd, &inBuf, 413, "Request Entity Too Large", NULL);
            _SFCB_TRACE(1, ("--- exiting: content-length too big"));      
            commClose(conn_fd);
            exit(1);
         }
         inBuf.content_length = clen;
      }
      else if (strncasecmp(hdr, "Content-Type:", 13) == 0) {
         cp = &hdr[13];
         cp += strspn(cp, " \t");
         inBuf.content_type = cp;
      }
      else if (strncasecmp(hdr, "Host:", 5) == 0) {
         cp = &hdr[5];
         cp += strspn(cp, " \t");
         inBuf.host = cp;
         if (strchr(inBuf.host, '/') != NULL || inBuf.host[0] == '.') {
            if (!discardInput) {
   	         genError(conn_fd, &inBuf, 400, "Bad Request", NULL);
	            discardInput=2;
            }
	      }
      }
      else if (strncasecmp(hdr, "User-Agent:", 11) == 0) {
         cp = &hdr[11];
         cp += strspn(cp, " \t");
         inBuf.useragent = cp;
      }
      else if (strncasecmp(hdr, "TE:", 3) == 0) {
         char *cp = &hdr[3];
         cp += strspn(cp, " \t");
         if (strncasecmp(cp,"trailers",8)==0)
         inBuf.trailers=1;
      }
      else if (strncasecmp(hdr, "Expect:", 7) == 0) {
         if (!discardInput) {
            genError(conn_fd, &inBuf, 417, "Expectation Failed", NULL);  //more);
	         discardInput=2;
         }
      }
#ifdef ALLOW_UPDATE_EXPIRED_PW
      else if (strncasecmp(hdr, "Pragma: UpdateExpiredPassword", 29) == 0) {
	hcrFlags |= HCR_UPDATE_PW;
      }
#endif
   }

#if defined USE_SSL
   if (doBa && sfcbSSLMode) {
     if (ccVerifyMode != CC_VERIFY_IGNORE ) {
       if (x509) {
	 inBuf.certificate = x509;
	 if (ccValidate(inBuf.certificate,&inBuf.principal,0)) {
	   /* successful certificate validation overrides basic auth */
	   doBa = 0;
	 }
       } else if (ccVerifyMode == CC_VERIFY_REQUIRE) {
	 /* this should never happen ;-) famous last words */
	 mlogf(M_ERROR,M_SHOW,
	       "\n--- Client certificate not accessible - closing connection\n");
	 commClose(conn_fd);
	 exit(1);
       }
     }     
   }
#endif

   int authorized = 0; 
   int barc = 0;
#ifdef HAVE_UDS
   if (!discardInput && doUdsAuth) {
       struct sockaddr_un sun; 
       sun.sun_family = 0; 
       socklen_t cl = sizeof(sun); 
       int rc = getpeername(conn_fd.socket, (struct sockaddr*)&sun, &cl); 
       if (rc == 0 && sun.sun_family == AF_UNIX) {
           /* Already authenticated via permissions on unix socket */
           authorized = 1;
       }
   }
#endif
   AuthExtras      extras = {NULL, NULL, NULL};

   if (!authorized && !discardInput && doBa) {

     if (inBuf.authorization) {

       /* for PAM, client's IP address is used for host-based authentication */
        struct sockaddr_storage from;
        socklen_t from_len = sizeof(from);
        getpeername(conn_fd.socket, (struct sockaddr *)&from, &from_len);
#ifdef USE_INET6
        char ipstr[INET6_ADDRSTRLEN] = {0};
#else
        char ipstr[INET_ADDRSTRLEN] = {0};
#endif
        if (getnameinfo((struct sockaddr*)&from, from_len, ipstr, sizeof(ipstr), NULL, 0, NI_NUMERICHOST) == 0)
          extras.clientIp = ipstr;
	//	fprintf(stderr, "client is: %s\n", ipstr);

       barc = baValidate(inBuf.authorization,&inBuf.principal,&extras);
#ifdef ALLOW_UPDATE_EXPIRED_PW
       if (barc == AUTH_EXPIRED) {
 	 hcrFlags |= HCR_EXPIRED_PW;
       }
       else if (barc == AUTH_PASS) {
 	 hcrFlags = 0; /* clear flags so non-expired user doesn't update pw */
       }
       else if (barc == AUTH_FAIL) {
#else
	 if (barc != AUTH_PASS) {
#endif
	   char more[]="WWW-Authenticate: Basic realm=\"cimom\"\r\n";
	   genError(conn_fd, &inBuf, 401, "Unauthorized", more);
	   /* we continue to parse headers and empty the socket
	      to be graceful with the client */
	   discardInput=1;
	 }
       }

#if defined USE_SSL
     else if (sfcbSSLMode && ccVerifyMode != CC_VERIFY_IGNORE) {
       /* associate certificate with principal for next request */
       ccValidate(inBuf.certificate,&inBuf.principal,1);
     }
#endif
   }

   len = inBuf.content_length;
   if (len == UINT_MAX) {
     if (!discardInput) {
        genError(conn_fd, &inBuf, 411, "Length Required", NULL);
     }
     _SFCB_TRACE(1, ("--- exiting after missing content length."));      
     commClose(conn_fd);
     freeBuffer(&inBuf);
     exit(1);
   }

   hdr = (char *) malloc(strlen(inBuf.authorization) + 64);
   len += hl =
       sprintf(hdr, "<!-- xml -->\n<!-- auth: %s -->\n", inBuf.authorization);

   rc = getPayload(conn_fd, &inBuf);
   if (rc < 0) {
      genError(conn_fd, &inBuf, 400, "Bad Request", NULL);
      _SFCB_TRACE(1, ("--- exiting after request timeout."));
      commClose(conn_fd);
      exit(1);
   }
   if (discardInput) {
     free(hdr);
     freeBuffer(&inBuf);
     _SFCB_RETURN(discardInput-1);
   }

   msgs[0].data = hdr;
   msgs[0].length = hl;
   msgs[1].data = inBuf.content;
   msgs[1].length = len - hl;

   ctx.cimXmlDoc = inBuf.content;
   ctx.principal = inBuf.principal;
   ctx.host = inBuf.host;
   ctx.teTrailers = inBuf.trailers;
   ctx.cimXmlDocLength = len - hl;
   ctx.commHndl = &conn_fd;
   
   if (msgs[1].length > 0) {
      ctx.chunkFncs=&httpChunkFunctions;
      ctx.sessionId=sessionId;
      
#ifdef SFCB_DEBUG
      if ((_sfcb_trace_mask & TRACE_RESPONSETIMING) ) {
	gettimeofday(&sv,NULL);
	getrusage(RUSAGE_SELF,&us);
	uset=1;
      }
      
      if ((_sfcb_trace_mask & TRACE_XMLIN) ) {
	_sfcb_trace(1,__FILE__,__LINE__,
		    _sfcb_format_trace("-#- xmlIn %d bytes:\n%*s",inBuf.content_length,
				       inBuf.content_length,(char*)inBuf.content));
	_sfcb_trace(1,__FILE__,__LINE__,
		    _sfcb_format_trace("-#- xmlIn end\n"));
      }  
#endif 
      
      response = handleCimXmlRequest(&ctx, hcrFlags);
   } 
   else {
     response = nullResponse;
   }
   free(hdr);

   _SFCB_TRACE(1, ("--- Generate http response"));
   if (response.chunkedMode==0) writeResponse(conn_fd, response);
   if (response.buffer != NULL)
     cleanupCimXmlRequest(&response);

  if (extras.release) {
     extras.release(extras.authHandle);
  }

#ifdef SFCB_DEBUG
   if (uset && (_sfcb_trace_mask & TRACE_RESPONSETIMING) ) {
      gettimeofday(&ev,NULL);
      getrusage(RUSAGE_SELF,&ue);
      _sfcb_trace(1,__FILE__,__LINE__,
		  _sfcb_format_trace("-#- Operation %.5u %s-%s real: %f user: %f sys: %f \n",
				     sessionId, opsName[ctx.operation], ctx.className,
				     timevalDiff(&sv,&ev),
				     timevalDiff(&us.ru_utime,&ue.ru_utime),
				     timevalDiff(&us.ru_stime,&ue.ru_stime)));
   }
#endif

   freeBuffer(&inBuf);
   _SFCB_RETURN(0);
}



/**
 * called by httpDaemon
 *
 */
static void handleHttpRequest(int connFd, int sslMode)
{
   int r;
   CommHndl conn_fd;
   int isReady;
   fd_set httpfds;
   struct sembuf procReleaseUnDo = {0,1,SEM_UNDO};
   struct timeval httpTimeout;
   int breakloop;

   _SFCB_ENTER(TRACE_HTTPDAEMON, "handleHttpRequest");

   _SFCB_TRACE(1, ("--- Forking xml handler"));

   if (doFork) {
      semAcquire(httpWorkSem,0);
      semAcquire(httpProcSem,0);
      for (httpProcIdX=0; httpProcIdX<hMax; httpProcIdX++)
         if (semGetValue(httpProcSem,httpProcIdX+1)==0) break;
      procReleaseUnDo.sem_num=httpProcIdX+1; 

      sessionId++;
      r = fork();

      /* child's thread of execution */
      if (r==0) {
         currentProc=getpid();
         processName="CIMXML-Processor";
         semRelease(httpProcSem,0);
         semAcquireUnDo(httpProcSem,0);
         semReleaseUnDo(httpProcSem,httpProcIdX+1);
         semRelease(httpWorkSem,0);
	 atexit(uninitGarbageCollector);
	 atexit(sunsetControl);
      }
      /* parent's thread of execution */
      else if (r>0) {
         running++;
         _SFCB_EXIT();
      }
   }
   else r = 0;

   /* fork() failed */
   if (r < 0) {
      char *emsg=strerror(errno);
      mlogf(M_ERROR,M_SHOW,"--- fork handler: %s\n",emsg);
      exit(1);
   }

   if (r == 0) {
      localMode=0;
      /* child's thread of execution || doFork=0 */
      if (doFork) {
         _SFCB_TRACE(1,("--- Forked xml handler %d", currentProc))
      }

      _SFCB_TRACE(1,("--- Started xml handler %d %d", currentProc,
                   resultSockets.receive));

      if (getenv("SFCB_PAUSE_HTTP")) for (breakloop=0;breakloop==0;) {
         fprintf(stderr,"-#- Pausing - pid: %d\n",currentProc);
         sleep(5);
      }   

      conn_fd.socket=connFd;
      conn_fd.file=fdopen(connFd,"a");
      conn_fd.buf = NULL;
      if (conn_fd.file == NULL) {
	mlogf(M_ERROR,M_SHOW,"--- failed to create socket stream - continue with raw socket: %s\n",strerror(errno));
      } else {
	conn_fd.buf = malloc(SOCKBUFSZ);
	if (conn_fd.buf) {
	  setbuffer(conn_fd.file,conn_fd.buf,SOCKBUFSZ);
	} else {
	  mlogf(M_ERROR,M_SHOW,"--- failed to create socket buffer - continue unbuffered: %s\n",strerror(errno));
	}
      }
      if (sslMode) {
#if defined USE_SSL
	BIO *sslb;
	BIO *sb;
	long flags = fcntl(connFd,F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(connFd,F_SETFL,flags);
	sb=BIO_new_socket(connFd,BIO_NOCLOSE);
	if (!(conn_fd.ssl = SSL_new(ctx)))
	  intSSLerror("Error creating SSL object");
	SSL_set_bio(conn_fd.ssl, sb, sb);
	char *error_string;
	while(1) {
	  int sslacc, sslerr;
	  sslacc = SSL_accept(conn_fd.ssl);
	  if (sslacc == 1) {
	    /* accepted */
	    _SFCB_TRACE(1, ("--- SSL connection accepted"));
	    break;
	  }
	  sslerr = SSL_get_error(conn_fd.ssl,sslacc);
	  if (sslerr == SSL_ERROR_WANT_WRITE || 
	      sslerr == SSL_ERROR_WANT_READ) {
	    /* still in handshake */
	    FD_ZERO(&httpfds);
	    FD_SET(connFd,&httpfds);
	    if (sslerr == SSL_ERROR_WANT_WRITE) {
	      _SFCB_TRACE(2, (
	          "---  Waiting for SSL handshake (WANT_WRITE): timeout=%ld",
	          httpSelectTimeout.tv_sec));
	      isReady = select(connFd+1,NULL,&httpfds,NULL,&httpSelectTimeout);
	    } else {
	      _SFCB_TRACE(2, (
	          "---  Waiting for SSL handshake (WANT_READ): timeout=%ld",
	          httpSelectTimeout.tv_sec));
	      isReady = select(connFd+1,&httpfds,NULL,NULL,&httpSelectTimeout);
	    }
	    if (isReady == 0) {
	      intSSLerror("Timeout error accepting SSL connection");
	    } else if (isReady < 0) {
	      mlogf(M_ERROR, M_SHOW, "--- Error accepting SSL connection: %s\n",
	            strerror(errno));
	      intSSLerror("Error accepting SSL connection");
	    }
	  // Error determination as follows: First, check the SSL error queue. If
	  // empty, attempt to determine the correct error string some other way.
	  // Finally, if the system errno is nonzero, report that as well.
	  } else if (sslerr == SSL_ERROR_ZERO_RETURN){
	      int syserrno = errno;
	      unsigned long sslqerr = ERR_get_error();
	      if (sslqerr != 0) {
	        error_string = ERR_error_string(sslqerr, NULL);
	      } else {
	        error_string = "TLS/SSL connection has been closed";
	      }
	      mlogf(M_ERROR, M_SHOW,
	          "--- SSL_ERROR_ZERO_RETURN during handshake: %s\n",
	          error_string);
	      if (syserrno)
	        mlogf(M_ERROR, M_SHOW, "--- system errno reports: %s\n",
	            strerror(syserrno));
	      intSSLerror("SSL_ERROR_ZERO_RETURN error during SSL handshake");
	      break;
	  } else if (sslerr == SSL_ERROR_WANT_X509_LOOKUP){
	      int syserrno = errno;
	      unsigned long sslqerr = ERR_get_error();
	      if (sslqerr != 0) {
	        error_string = ERR_error_string(sslqerr, NULL);
	      } else {
	        error_string = "The client_cert_cb function has not completed";
	      }
	      mlogf(M_ERROR, M_SHOW,
	          "--- SSL_ERROR_WANT_X509_LOOKUP during handshake: %s\n",
	          error_string);
	      if (syserrno)
	        mlogf(M_ERROR, M_SHOW, "--- system errno reports: %s\n",
	            strerror(syserrno));
	      intSSLerror("SSL_ERROR_WANT_X509_LOOKUP error during SSL handshake");
	      break;
	  } else if (sslerr == SSL_ERROR_WANT_CONNECT){
	      int syserrno = errno;
	      unsigned long sslqerr = ERR_get_error();
	      if (sslqerr != 0) {
	        error_string = ERR_error_string(sslqerr, NULL);
	      } else {
	        error_string = "The connect operation did not complete";
	      }
	      mlogf(M_ERROR, M_SHOW,
	          "--- SSL_ERROR_WANT_CONNECT during handshake: %s\n",
	          error_string);
	      if (syserrno)
	        mlogf(M_ERROR, M_SHOW, "--- system errno reports: %s\n",
	            strerror(syserrno));
	      intSSLerror("SSL_ERROR_WANT_CONNECT error during SSL handshake");
	      break;
	  } else if (sslerr == SSL_ERROR_SYSCALL){
	      int syserrno = errno;
	      unsigned long sslqerr = ERR_get_error();
	      if (sslqerr != 0) {
	        error_string = ERR_error_string(sslqerr, NULL);
	      } else {
	        if (sslacc == 0) {
	          error_string = "EOF occurred: client may have aborted";
	        } else if (sslacc == -1) {
	          error_string = "BIO reported an I/O error";
	        } else {  /* possible? */
	          error_string = "Unknown I/O error";
	        }
	      }
	      mlogf(M_ERROR, M_SHOW,
	          "--- SSL_ERROR_SYSCALL during handshake: %s\n",
	          error_string);
	      if (syserrno)
	        mlogf(M_ERROR, M_SHOW, "--- system errno reports: %s\n",
	            strerror(syserrno));
	      intSSLerror("SSL_ERROR_SYSCALL error during SSL handshake");
	      break;
	  } else if (sslerr == SSL_ERROR_SSL){
	      /* most certificate verification errors will occur here */
	      int syserrno = errno;
	      unsigned long sslqerr = ERR_get_error();
	      if (sslqerr != 0) {
	        error_string = ERR_error_string(sslqerr, NULL);
	      } else {
	        error_string = "Unknown SSL library error";
	      }
	      mlogf(M_ERROR, M_SHOW,
	          "--- SSL_ERROR_SSL during handshake: %s\n",
	          error_string);
	      if (syserrno)
	        mlogf(M_ERROR, M_SHOW, "--- system errno reports: %s\n",
	            strerror(syserrno));
	      intSSLerror("SSL_ERROR_SSL error during SSL handshake");
	      break;
	  } else {
	      /* unexpected error */
	      int syserrno = errno;
	      unsigned long sslqerr = ERR_get_error();
	      if (sslqerr != 0) {
	        error_string = ERR_error_string(sslqerr, NULL);
	      } else {
	        error_string = "Undefined SSL library error";
	      }
	      mlogf(M_ERROR, M_SHOW,
	          "--- Undefined SSL_ERROR during handshake: %s\n",
	          error_string);
	      if (syserrno)
	        mlogf(M_ERROR, M_SHOW, "--- system errno reports: %s\n",
	            strerror(syserrno));
	      intSSLerror("Undefined error accepting SSL connection");
	      break;
	  }
	}  /* while */
	flags ^= O_NONBLOCK;
	fcntl(connFd,F_SETFL,flags);
	sslb = BIO_new(BIO_f_ssl());
	BIO_set_ssl(sslb,conn_fd.ssl,BIO_CLOSE);
	conn_fd.bio=BIO_new(BIO_f_buffer());
	BIO_push(conn_fd.bio,sslb);
	if (BIO_set_write_buffer_size(conn_fd.bio,SOCKBUFSZ)) { 
	} else {
	  conn_fd.bio=NULL;
	}
#endif
      } else {
#if defined USE_SSL
	conn_fd.bio=NULL;
	conn_fd.ssl=NULL;
#endif
      }
      
      numRequest = 0;
      FD_ZERO(&httpfds);
      FD_SET(conn_fd.socket,&httpfds);
      do {
	numRequest += 1;
        
        if (doHttpRequest(conn_fd)) {
	  /* eof reached - leave */
	  break;
	}
	if (keepaliveTimeout==0 || numRequest >= keepaliveMaxRequest ) {
	  /* no persistence wanted or exceeded - quit */
	  break;
	}
	/* wait for next request or timeout */
	httpTimeout.tv_sec=keepaliveTimeout;
	httpTimeout.tv_usec=keepaliveTimeout;
	isReady = select(conn_fd.socket+1,&httpfds,NULL,NULL,&httpTimeout);
	if (isReady == 0) {
	  _SFCB_TRACE(1,("--- HTTP connection timeout, quit %d ", currentProc));
	  break;
	} else if (isReady < 0) {
	  _SFCB_TRACE(1,("--- HTTP connection error, quit %d ", currentProc));
	  break;
	}
      } while (1);

      commClose(conn_fd);
      if (!doFork) return;

      _SFCB_TRACE(1, ("--- Xml handler exiting %d", currentProc));
      dumpTiming(currentProc);
      exit(0);
   }

}

int isDir(const char *path)
{
#if defined (HAVE_SYS_STAT_H) && defined (USE_SSL)
   struct stat sb;
   int rc;
   rc = stat(path, &sb);
   if (rc == -1)
      intSSLerror("Error opening the client trust store");
   return (sb.st_mode & S_IFMT) == S_IFDIR;
#else
   return 0;
#endif
}


static int
getSocket()
{
  int             fd;
  int             ru = 1;

#ifdef USE_INET6                // need to check
  fd = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
  if (fd < 0) {
    mlogf(M_INFO, M_SHOW, "--- Using IPv4 address\n");
    fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    fallback_ipv4 = 1;
  }
#else
  fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif                          // USE_INET6
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &ru, sizeof(ru));
  return fd;
}


#ifdef USE_INET6
static struct sockaddr *
prepSockAddr6(int port, void *ssin, socklen_t * sin_len)
{
  struct sockaddr_in6 *sin = ssin;

  *sin_len = sizeof(*sin);
  memset(sin, 0, *sin_len);

  sin->sin6_family = AF_INET6;
  if (httpLocalOnly)
    sin->sin6_addr = in6addr_loopback;
  else
    sin->sin6_addr = in6addr_any;
  sin->sin6_port = htons(port);

  return (struct sockaddr *) sin;
}
#endif


static struct sockaddr *
prepSockAddr4(int port, void *ssin, socklen_t * sin_len)
{
  struct sockaddr_in *sin = ssin;

  *sin_len = sizeof(*sin);
  memset(sin, 0, *sin_len);
	
  sin->sin_family = AF_INET;
  if (httpLocalOnly) {
    const char *loopback_int = "127.0.0.1";
    inet_aton(loopback_int, &(sin->sin_addr));
  } else
    sin->sin_addr.s_addr = INADDR_ANY;
  sin->sin_port = htons(port);

  return (struct sockaddr *) sin;
}


static int
bindToPort(int sock, int port, void *ssin, socklen_t * sin_len)
{
  struct sockaddr *sin;

  if (sock < 0)
    return 1;

  if (getControlBool("httpLocalOnly", &httpLocalOnly))
    httpLocalOnly = 0;

#ifdef USE_INET6
  if (!fallback_ipv4)
    sin = prepSockAddr6(port, ssin, sin_len);
  else
#endif
    sin = prepSockAddr4(port, ssin, sin_len);

  if (bind(sock, sin, *sin_len) || listen(sock, 10)) {
    mlogf(M_ERROR, M_SHOW, "--- Cannot listen on port %ld (%s)\n", port,
          strerror(errno));
    sleep(1);
    return 1;
  }

  return 0;
}

#ifdef HAVE_UDS
static int
bindUDS(int fd, char *path, struct sockaddr_un *sun, socklen_t sun_len)
{
  if (fd >= 0) {
    unlink(path);

    size_t          gbuflen = sysconf(_SC_GETGR_R_SIZE_MAX);
    char            gbuf[gbuflen];
    struct group   *pgrp = NULL;
    struct group    grp;
    gid_t           oldfsgid = 0;

    int             rc = getgrnam_r("sfcb", &grp, gbuf, gbuflen, &pgrp);
    if (rc == 0 && pgrp) {
#ifdef HAVE_SYS_FSUID_H
      oldfsgid = setfsgid(pgrp->gr_gid);
#else
      oldfsgid = setegid(pgrp->gr_gid);
#endif
    }
    mode_t          oldmask = umask(0007);
    if (bind(fd, (struct sockaddr *) sun, sun_len) || listen(fd, 10)) {
      mlogf(M_ERROR, M_SHOW,
            "--- Cannot listen on unix socket %s (%s) %d\n", path,
            strerror(errno));
      sleep(1);
      return 1;
    }
    umask(oldmask);
    if (pgrp) {
#ifdef HAVE_SYS_FSUID_H
      setfsgid(oldfsgid);
#else
      setegid(oldfsgid);
#endif
    }

    return 0;
  }

  return 1;
}
#endif                          // HAVE_UDS

static void
acceptRequest(int sock, void *ssin, socklen_t sin_len, int sslMode)
{

#ifdef USE_INET6
  struct sockaddr_in6 *sin = ssin;
#else
  struct sockaddr_in *sin = ssin;
#endif

  int             connFd;
  char           *emsg;

  if ((connFd = accept(sock, (__SOCKADDR_ARG) & *sin, &sin_len)) < 0) {
    if (errno == EINTR || errno == EAGAIN) {
      return;
    }
    emsg = strerror(errno);
    mlogf(M_ERROR, M_SHOW, "--- accept error %s\n", emsg);
    abort();
  }

  handleHttpRequest(connFd, sslMode);
  close(connFd);
}

#ifdef USE_SSL
static void
initSSL()
{

  _SFCB_ENTER(TRACE_HTTPDAEMON, "initSSL");

  char           *fnc,
                 *fnk,
                 *fnt,
                 *fnl,
                 *sslCiphers;
  int             rc;
  ctx = SSL_CTX_new(SSLv23_method());
  getControlChars("sslCertificateFilePath", &fnc);
  _SFCB_TRACE(1, ("---  sslCertificateFilePath = %s", fnc));
  if (SSL_CTX_use_certificate_chain_file(ctx, fnc) != 1)
    intSSLerror("Error loading certificate from file");
  getControlChars("sslKeyFilePath", &fnk);
  _SFCB_TRACE(1, ("---  sslKeyFilePath = %s", fnk));
  if (SSL_CTX_use_PrivateKey_file(ctx, fnk, SSL_FILETYPE_PEM) != 1)
    intSSLerror("Error loading private key from file");
  getControlChars("sslClientCertificate", &fnl);
  _SFCB_TRACE(1, ("---  sslClientCertificate = %s", fnl));
  if (strcasecmp(fnl, "ignore") == 0) {
    ccVerifyMode = CC_VERIFY_IGNORE;
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);
  } else if (strcasecmp(fnl, "accept") == 0) {
    ccVerifyMode = CC_VERIFY_ACCEPT;
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, get_cert);
  } else if (strcasecmp(fnl, "require") == 0) {
    ccVerifyMode = CC_VERIFY_REQUIRE;
    SSL_CTX_set_verify(ctx,
                       SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                       get_cert);
  } else {
    intSSLerror
        ("sslClientCertificate must be one of: ignore, accept or require");
  }
  getControlChars("sslClientTrustStore", &fnt);
  _SFCB_TRACE(1, ("---  sslClientTrustStore = %s", fnt));

  if (ccVerifyMode != CC_VERIFY_IGNORE) {
    if (isDir(fnt))
      rc = SSL_CTX_load_verify_locations(ctx, NULL, fnt);
    else
      rc = SSL_CTX_load_verify_locations(ctx, fnt, NULL);
    if (rc != 1)
      intSSLerror("Error locating the client trust store");
  }

  /*
   * SSLv2 is pretty old; no one should be needing it any more 
   */
  SSL_CTX_set_options(ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 |
                      SSL_OP_SINGLE_DH_USE);
  /*
   * Set valid ciphers
   */
  getControlChars("sslCiphers", &sslCiphers);
  _SFCB_TRACE(1, ("---  sslCiphers = %s", sslCiphers));
  if (SSL_CTX_set_cipher_list(ctx, sslCiphers) != 1)
    intSSLerror("Error setting cipher list (no valid ciphers)");

}
#endif                          // USE_SSL


int httpDaemon(int argc, char *argv[], int sslMode, int sfcbPid)
{

#ifdef USE_INET6
  struct sockaddr_in6 httpSin;
#else
  struct sockaddr_in httpSin;
#endif

  socklen_t       httpSin_len;
  int             i,
                  rc;
  char           *cp;
  long            procs,
                  httpPort;
  int             httpListenFd = -1;
  int             enableHttp = 0;
  fd_set          httpfds;
  int             maxfdp1;      /* highest-numbered fd +1 */

#ifdef USE_SSL
#ifdef USE_INET6
  struct sockaddr_in6 httpsSin;
#else
  struct sockaddr_in httpsSin;
#endif
  socklen_t       httpsSin_len;
  long            httpsPort;
  int             httpsListenFd = -1;
#endif                          // USE_SSL

#ifdef HAVE_UDS
  static char    *udsPath = NULL;
  int             enableUds = 0;
  int             udsListenFd = -1;
  struct sockaddr_un sun;
  socklen_t       sun_len;
#endif

  debug = 1;
  doFork = 1;

  _SFCB_ENTER(TRACE_HTTPDAEMON, "httpDaemon");

  setupControl(configfile);
  sfcbSSLMode = sslMode;
  processName = "HTTP-Daemon";

  if (getControlNum("httpPort", &httpPort))
    httpPort = 5988;            /* 5988 is default port */
#ifdef USE_SSL
  if (sfcbSSLMode) {
    if (getControlNum("httpsPort", &httpsPort))
      httpsPort = 5989;
  }
#endif                          // USE_SSL
#ifdef HAVE_UDS
  if (getControlChars("httpSocketPath", &udsPath))
    udsPath = "/tmp/sfcbHttpSocket";
#endif

  hMax = htMax;                 /* max number of http procs */

  if (getControlNum("httpProcs", &procs))
    procs = 10;
  if (getControlBool("enableHttp", &enableHttp))
    enableHttp = 1;
#ifdef HAVE_UDS
  if (getControlBool("enableUds", &enableUds))
    enableUds = 1;
  if (!enableUds)
    udsPath = NULL;
#endif

  if (!enableHttp)
    httpPort = -1;
  /*
   * note: we check for enableHttps in sfcBroker 
   */

  initHttpProcCtl(procs);

  if (getControlBool("doBasicAuth", &doBa))
    doBa = 0;

#ifdef HAVE_UDS
  if (getControlBool("doUdsAuth", &doUdsAuth))
    doUdsAuth = 0;
#endif

  // Get selectTimeout from the config file, or use 5 if not set
  if (getControlNum("selectTimeout", &selectTimeout))
    selectTimeout = 5;
  httpSelectTimeout.tv_sec=selectTimeout;

  if (getControlNum("keepaliveTimeout", &keepaliveTimeout))
    keepaliveTimeout = 15;

  if (getControlNum("keepaliveMaxRequest", &keepaliveMaxRequest))
    keepaliveMaxRequest = 10;

  if (getControlBool("useChunking", &noChunking))
    noChunking = 0;
  noChunking = noChunking == 0;

  /*
   * grab commandline options 
   */
  i = 1;
  while (i < argc && argv[i][0] == '-') {
    if (strcmp(argv[i], "-D") == 0)
      debug = 1;
    else if (strcmp(argv[i], "-nD") == 0)
      debug = 0;
    else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      ++i;
      httpPort = (unsigned short) atoi(argv[i]);
    } else if (strcmp(argv[i], "-tm") == 0) {
      if (isdigit(*argv[i + 1])) {
        ++i;
      }
    } else if (strcmp(argv[i], "-F") == 0)
      doFork = 1;
    else if (strcmp(argv[i], "-nF") == 0)
      doFork = 0;
    else if (strcmp(argv[i], "-H") == 0);
    ++i;
  }

  name = argv[0];
  cp = strrchr(name, '/');
  if (cp != NULL)
    ++cp;
  else
    cp = name;
  name = cp;

  if (enableHttp) {
    mlogf(M_INFO, M_SHOW,
          "--- %s HTTP Daemon V" sfcHttpDaemonVersion
          " configured for port %ld - %d\n", name, httpPort, currentProc);
  }
#ifdef USE_SSL
  if (sslMode)
    mlogf(M_INFO, M_SHOW,
          "--- %s HTTP Daemon V" sfcHttpDaemonVersion
          " configured for port %ld - %d\n", name, httpsPort, currentProc);
#endif                          // USE_SSL
#ifdef HAVE_UDS
  mlogf(M_INFO, M_SHOW,
        "--- %s HTTP Daemon V" sfcHttpDaemonVersion
        " configured for socket %s - %d\n", name, udsPath, currentProc);
#endif                          // HAVE_UDS

  if (doBa)
    mlogf(M_INFO, M_SHOW, "--- Using Basic Authentication\n");
#ifdef HAVE_UDS
  if (doUdsAuth)
    mlogf(M_INFO, M_SHOW,
          "--- Using Unix Socket Peer Cred Authentication\n");
#endif

  mlogf(M_INFO, M_SHOW, "--- Select timeout: %ld seconds\n",selectTimeout);

  if (keepaliveTimeout == 0) {
    mlogf(M_INFO, M_SHOW, "--- Keep-alive timeout disabled\n");
  } else {
    mlogf(M_INFO, M_SHOW, "--- Keep-alive timeout: %ld seconds\n",
          keepaliveTimeout);
    mlogf(M_INFO, M_SHOW, "--- Maximum requests per connection: %ld\n",
          keepaliveMaxRequest);
  }

  if (enableHttp) {
    httpListenFd = getSocket();
  }
#ifdef USE_SSL
  if (sslMode) {
    httpsListenFd = getSocket();
  }
#endif                          // USE_SSL
#ifdef HAVE_UDS
  if (enableUds) {
    udsListenFd = socket(PF_UNIX, SOCK_STREAM, 0);
  }
#endif                          // HAVE_UDS

#ifdef HAVE_UDS
  if (udsListenFd >= 0) {
    if (getControlChars("httpSocketPath", &udsPath)) {
      mlogf(M_ERROR, M_SHOW, "--- No unix socket path defined for HTTP\n");
      sleep(1);
      return 1;
    }
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, udsPath);
  }
#endif

  int             bindrc = 0;
  if (enableHttp) {
    bindrc = bindToPort(httpListenFd, httpPort, &httpSin, &httpSin_len);
  }
#ifdef USE_SSL
  if (sslMode) {
    bindrc |=
        bindToPort(httpsListenFd, httpsPort, &httpsSin, &httpsSin_len);
  }
#endif
#ifdef HAVE_UDS
  if (enableUds) {
    sun_len = sizeof(sun);
    bindrc |= bindUDS(udsListenFd, udsPath, &sun, sun_len);
  }
#endif

  if (bindrc > 0)
    return 1;                   /* if can't bind to port, return 1 to
                                 * shutdown sfcb */

  if (!debug) {
    int             rc = fork();
    if (rc == -1) {
      char           *emsg = strerror(errno);
      mlogf(M_ERROR, M_SHOW, "--- fork daemon: %s", emsg);
      exit(1);
    } else if (rc)
      exit(0);
  }

  currentProc = getpid();

  setSignal(SIGCHLD, handleSigChld, 0);
  setSignal(SIGUSR1, handleSigUsr1, 0);
  setSignal(SIGINT, SIG_IGN, 0);
  setSignal(SIGTERM, SIG_IGN, 0);
  setSignal(SIGHUP, SIG_IGN, 0);

#if defined USE_SSL
  if (sslMode) {
    commInit();
    initSSL();
  }
#endif                          // USE_SSL

  maxfdp1 = httpListenFd + 1;
#ifdef USE_SSL
  maxfdp1 = (maxfdp1 > httpsListenFd) ? maxfdp1 : (httpsListenFd + 1);
#endif
#ifdef HAVE_UDS
  maxfdp1 = (maxfdp1 > udsListenFd) ? maxfdp1 : (udsListenFd + 1);
#endif

  for (;;) {

    /*
     * select() modifies httpfds in-place, so reset after every select() 
     */
    FD_ZERO(&httpfds);
    if (httpListenFd >= 0) {
      FD_SET(httpListenFd, &httpfds);
    }
#ifdef USE_SSL
    if (httpsListenFd >= 0) {
      FD_SET(httpsListenFd, &httpfds);
    }
#endif                          // USE_SSL
#ifdef HAVE_UDS
    if (udsListenFd >= 0) {
      FD_SET(udsListenFd, &httpfds);
    }
#endif                          // USE_UDS

    rc = select(maxfdp1, &httpfds, NULL, NULL, NULL);

    if (stopAccepting)
      break;
    if (rc < 0) {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      }
    }

    if (httpListenFd >= 0 && FD_ISSET(httpListenFd, &httpfds)) {
      _SFCB_TRACE(1, ("--- Processing http request"));
      acceptRequest(httpListenFd, &httpSin, httpSin_len, 0);
    }
#ifdef USE_SSL
    else if (httpsListenFd >= 0 && FD_ISSET(httpsListenFd, &httpfds)) {
      _SFCB_TRACE(1, ("--- Processing https request"));
      acceptRequest(httpsListenFd, &httpsSin, httpsSin_len, 1);
    }
#endif                          // USE_SSL
#ifdef HAVE_UDS
    else if (udsListenFd >= 0 && FD_ISSET(udsListenFd, &httpfds)) {
      _SFCB_TRACE(1, ("--- Processing http UDS request"));
      acceptRequest(udsListenFd, &sun, sun_len, 0);
    }
#endif
  }

  remProcCtl();

  /*
   * patiently wait for sfcBroker to kill us 
   */
  while (1) {
    sleep(5);
  }

}

#if defined USE_SSL
static int get_cert(int preverify_ok,X509_STORE_CTX* x509_ctx)
{
  _SFCB_ENTER(TRACE_HTTPDAEMON, "get_cert");

  char    buf[256];
  int     err, depth;

  x509 = X509_STORE_CTX_get_current_cert(x509_ctx);
  err = X509_STORE_CTX_get_error(x509_ctx);
  depth = X509_STORE_CTX_get_error_depth(x509_ctx);

  _SFCB_TRACE(2, ("--- Verify peer certificate chain: level %d:", depth));
  X509_NAME_oneline(X509_get_subject_name(x509), buf, 256);
  _SFCB_TRACE(2, ("---  subject=%s", buf));
  X509_NAME_oneline(X509_get_issuer_name(x509), buf, 256);
  _SFCB_TRACE(2, ("---  issuer= %s", buf));

  _SFCB_RETURN(preverify_ok);
}

typedef int (*Validate)(X509 *certificate, char ** principal, int mode);

static int ccValidate(X509 *certificate, char ** principal, int mode)
{
  int   result=0;
  char *ln;
  char  dlName[512];
  void *authLib;
  Validate validate;
  _SFCB_ENTER(TRACE_HTTPDAEMON, "ccValidate");
  
  if (getControlChars("certificateAuthlib", &ln)==0) {
    libraryName(NULL,ln,dlName, 512);
    if ((authLib=dlopen(dlName, RTLD_LAZY))) {
      validate= dlsym(authLib, "_sfcCertificateAuthenticate");
      if (validate) {
	result = validate(certificate,principal,mode);
      } else {
	mlogf(M_ERROR,M_SHOW,
	      "--- Certificate authentication exit %s not found\n",dlName);
	result = 0;
      } 
      dlclose(authLib);
    }
  } else {
    mlogf(M_ERROR,M_SHOW,
	  "--- Certificate authentication exit not configured\n");	
  }
  _SFCB_RETURN(result);
}
#endif
