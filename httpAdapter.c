
/*
 * httpAdapter.c
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

#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>

#include "msgqueue.h"
#include "utilft.h"
#include "trace.h"
#include "cimXmlRequest.h"

#include <pthread.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "httpComm.h"
#include "sfcVersion.h"
#include "control.h"

unsigned long exFlags = 0;
static char *name;
static int debug;
static int doBa;
static int doFork = 0;
int noChunking = 0;
int sfcbSSLMode = 0;
static int hBase;
static int hMax;
static int httpProcId;
static int stopAccepting=0;
static int running=0;

#if defined USE_SSL
SSL_CTX *ctx;
#endif

static key_t httpProcSemKey;
static key_t httpWorkSemKey;
static int httpProcSem;
static int httpWorkSem;

extern char *decode64(char *data);
extern void libraryName(const char* dir, const char *location, char *fullName);
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

int sfcBrokerPid=0;

typedef int (*Authenticate)(char* principle, char* pwd);

typedef struct _buffer {
   char *data, *content;
   int length, size, ptr, content_length,trailers;
   char *httpHdr, *authorization, *content_type, *host, *useragent;
   char *principle;
   char *protocol;
} Buffer;

void initHttpProcCtl(int p)
{
   httpProcSemKey=ftok(".",'H');
   httpWorkSemKey=ftok(".",'W');
   union semun sun;
   int i;

   mlogf(M_INFO,M_SHOW,"--- Max Http procs: %d\n",p);
   if ((httpProcSem=semget(httpProcSemKey,1,0666))!=-1) 
      semctl(httpProcSem,0,IPC_RMID,sun);
      
   if ((httpProcSem=semget(httpProcSemKey,1+p,IPC_CREAT | 0666))==-1) {
      char *emsg=strerror(errno);
      mlogf(M_ERROR,M_SHOW,"--- Http Proc semaphore create %d: %s\n",currentProc,emsg);
      abort();
   }
   sun.val=p;
   semctl(httpProcSem,0,SETVAL,sun);
   
   sun.val=0;
   for (i=1; i<=p; i++)
      semctl(httpProcSem,p,SETVAL,sun);

   if ((httpWorkSem=semget(httpWorkSemKey,1,0666))!=-1) 
      semctl(httpWorkSem,0,IPC_RMID,sun);
      
   if ((httpWorkSem=semget(httpWorkSemKey,1,IPC_CREAT | 0666))==-1) {
      char *emsg=strerror(errno);
      mlogf(M_ERROR,M_SHOW,"--- Http ProcWork semaphore create %d: %s\n",currentProc,emsg);
      abort();
   }
   sun.val=1;
   semctl(httpWorkSem,0,SETVAL,sun);
}

int baValidate(char *cred, char **principle)
{
   char *auth,*pw;
   int i,err=0;
   static void *authLib=NULL;
   static Authenticate authenticate=NULL;
   char dlName[512];

   if (strncasecmp(cred,"basic ",6)) return 0;
   auth=decode64(cred+6);
   for (i=0; i<strlen(auth); i++)
      if (auth[i]==':') {
          auth[i]=0;
          pw=&auth[i+1];
          break;
      }

   if (err==0 && authLib==NULL) {
      char *ln;
      int err=1;
      if (getControlChars("basicAuthlib", &ln)==0) {
         libraryName(NULL,ln,dlName);
        if ((authLib=dlopen(dlName, RTLD_LAZY))) {
            authenticate= dlsym(authLib, "_sfcBasicAuthenticate");
            if (authenticate) err=0;
         }
      }
      if (err) mlogf(M_ERROR,M_SHOW,"--- Authentication exit %s not found\n",dlName);
   }

   *principle=strdup(auth);
   if (authenticate(auth,pw)) return 1;
   return 0;
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

static void handleSigUsr1(int sig)
{
   stopAccepting=1;
}

static void freeBuffer(Buffer * b)
{
   if (b->data)
      free(b->data);
   if (b->content)
      free(b->content);
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
   char cclose[] = "Connection: close\r\n\r\n";

   snprintf(head, sizeof(head), "%s %d %s\r\n", b->protocol, status, title);
   commWrite(conn_fd, head, strlen(head));
   if (more) {
      commWrite(conn_fd, more, strlen(more));
      exit(1);
   }

   commWrite(conn_fd, server, strlen(server));
   commWrite(conn_fd, clength, strlen(clength));
   commWrite(conn_fd, cclose, strlen(cclose));
   exit(1);
}

static int readData(CommHndl conn_fd, char *into, int length)
{
   int c = 0, r;

   while (c < length) {
      r = commRead(conn_fd, into + c, length - c);
      if (r < 0 && (errno == EINTR || errno == EAGAIN)) {
         continue;
      }
      c += r;
   }
   return c;
}

static void getPayload(CommHndl conn_fd, Buffer * b)
{
   int c = b->length - b->ptr;
   b->content = (char *) malloc(b->content_length + 8);
   if (c) memcpy(b->content, (b->data) + b->ptr, c);

   readData(conn_fd, (b->content) + c, b->content_length - c);
   *((b->content) + b->content_length) = 0;
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

//#define PrintF(s,d) fprintf(stderr,s,d)
#define PrintF(s,d)

static void writeResponse(CommHndl conn_fd, RespSegments rs)
{

   static char head[] = {"HTTP/1.1 200 OK\r\n"};
   static char cont[] = {"Content-Type: application/xml; charset=\"utf-8\"\r\n"};
   static char cach[] = {"Cache-Control: no-cache\r\n"};
   static char op[]   = {"CIMOperation: MethodResponse\r\n"};
   static char end[]  = {"\r\n\r\n"};
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
               len += ls[i] = strlen(sb->ft->getCharPtr(sb));
         }
         else
            len += ls[i] = strlen(rs.segments[i].txt);
      }
   }

   PrintF("%s","-----------------\n");
   commWrite(conn_fd, head, strlen(head));
   PrintF("%s",head);
   commWrite(conn_fd, cont, strlen(cont));
   PrintF("%s",cont);
   sprintf(str, "Content-Length: %d\r\n", len + 2);
   commWrite(conn_fd, str, strlen(str));
   PrintF("%s",str);
//   commWrite(conn_fd, ext, strlen(ext));
//   PrintF("%s",ext);
   commWrite(conn_fd, cach, strlen(cach));
   PrintF("%s",cach);
//   commWrite(conn_fd, man, strlen(man));
//   PrintF("%s",man);
   commWrite(conn_fd, op, strlen(op));
   PrintF("%s",op);
   commWrite(conn_fd, end, strlen(end));
   PrintF("%s",end);

   for (len = 0, i = 0; i < 7; i++) {
      if (rs.segments[i].txt) {
         if (rs.segments[i].mode == 2) {
            UtilStringBuffer *sb = (UtilStringBuffer *) rs.segments[i].txt;
            if (sb) {
               commWrite(conn_fd, (void*)sb->ft->getCharPtr(sb), ls[i]);
               PrintF("%s",sb->ft->getCharPtr(sb));
               sb->ft->release(sb);
            }
         }
         else {
            commWrite(conn_fd, rs.segments[i].txt, ls[i]);
               PrintF("%s",rs.segments[i].txt);
            if (rs.segments[i].mode == 1)
               free(rs.segments[i].txt);
         }
      }
   }
   PrintF("%s","-----------------\n");
   
   _SFCB_EXIT();
}


static void writeChunkHeaders(BinRequestContext *ctx)
{
   static char head[] = {"HTTP/1.1 200 OK\r\n"};
   static char cont[] = {"Content-Type: application/xml; charset=\"utf-8\"\r\n"};
   static char cach[] = {"Cache-Control: no-cache\r\n"};
   static char op[]   = {"CIMOperation: MethodResponse\r\n"};
   static char tenc[] = {"Transfer-encoding: chunked\r\n"};
   static char trls[] = {"Trailers: CIMError, CIMStatusCode, CIMStatusCodeDescription\r\n"};

   _SFCB_ENTER(TRACE_HTTPDAEMON, "writeChunkHeaders");
   
   commWrite(*(ctx->commHndl), head, strlen(head));
   PrintF("%s",head);
   commWrite(*(ctx->commHndl), cont, strlen(cont));
   PrintF("%s",cont);
   commWrite(*(ctx->commHndl), cach, strlen(cach));
   PrintF("%s",cach);
   commWrite(*(ctx->commHndl), op, strlen(op));
   PrintF("%s",op);
   commWrite(*(ctx->commHndl), tenc, strlen(tenc));
   PrintF("%s",tenc);
   commWrite(*(ctx->commHndl), trls, strlen(trls));
   PrintF("%s",trls);

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
               else len += ls[i] = strlen(sb->ft->getCharPtr(sb));
            }
            else len += ls[i] = strlen(rs.segments[i].txt);
         }
      }

      sprintf(str, "\r\n%x\r\n",len);
      commWrite(*(ctx->commHndl), str, strlen(str));
      PrintF("%s",str);

      for (len = 0, i = 0; i < 7; i++) {
         if (rs.segments[i].txt) {
            if (rs.segments[i].mode == 2) {
               UtilStringBuffer *sb = (UtilStringBuffer *) rs.segments[i].txt;
               if (sb) {
                  commWrite(*(ctx->commHndl), (void*)sb->ft->getCharPtr(sb), ls[i]);
                  PrintF("%s",sb->ft->getCharPtr(sb));
                  sb->ft->release(sb);
               }         
            }
            else {
               commWrite(*(ctx->commHndl), rs.segments[i].txt, ls[i]);
                  PrintF("%s",rs.segments[i].txt);
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
      PrintF("%s",eStr);
      commWrite(*(ctx->commHndl), status, strlen(status));
      PrintF("%s",status);
      if (desc) {
         commWrite(*(ctx->commHndl), desc, strlen(desc));
         PrintF("%s",desc);
         free(desc);
      }   
      eStr="\r\n";
      commWrite(*(ctx->commHndl), eStr, strlen(eStr));
      PrintF("%s",eStr);
   }
   _SFCB_EXIT();
}

static ChunkFunctions httpChunkFunctions = {
   writeChunkResponse,
};


#undef PrintF

static void getHdrs(CommHndl conn_fd, Buffer * b)
{
   for (;;) {
      char buf[5000];
      int r = commRead(conn_fd, buf, sizeof(buf));
      if (r < 0 && (errno == EINTR || errno == EAGAIN)) continue;
      if (r <= 0) break;
      
      add2buffer(b, buf, r);
      if (strstr(b->data, "\r\n\r\n") != NULL ||
          strstr(b->data, "\n\n") != NULL) {
         break;
      }
   }
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
      free(p);
      return rc;
  }
   noHttpPause=1;
   return 0;
}

static void doHttpRequest(CommHndl conn_fd)
{
   char *cp;
   Buffer inBuf = { NULL, NULL, 0, 0, 0, 0, 0 ,0};
   RespSegments response;
   int len, hl;
   char *hdr, *path;
   MsgSegment msgs[2];

   _SFCB_ENTER(TRACE_HTTPDAEMON, "doHttpRequest");

   if (pauseCodec("http")) for (;;) {
      fprintf(stdout,"-#- Pausing for codec http %d\n",currentProc);
      sleep(5);
   }
      
   inBuf.authorization = "";
   inBuf.content_type = NULL;
   inBuf.content_length = -1;
   inBuf.host = NULL;
   inBuf.useragent = "";
   int badReq = 0;

   getHdrs(conn_fd, &inBuf);

   inBuf.httpHdr = getNextHdr(&inBuf);
   for (badReq = 1;;) {
      if (inBuf.httpHdr == NULL)
         break;
      path = strpbrk(inBuf.httpHdr, " \t\r\n");
      if (path == NULL)
         break;
      *path++ = 0;
      _SFCB_TRACE(1,("--- Header: %s",inBuf.httpHdr));
      if (strcasecmp(inBuf.httpHdr, "POST") != 0)
         genError(conn_fd, &inBuf, 501, "Not Implemented", NULL);
      path += strspn(path, " \t\r\n");
      inBuf.protocol = strpbrk(path, " \t\r\n");
      *inBuf.protocol++ = 0;
      if (strcmp(path, "/cimom") != 0)
         break;
      if (inBuf.protocol == NULL)
         break;
      badReq = 0;
   }

   if (badReq) genError(conn_fd, &inBuf, 400, "Bad Request", NULL);

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
         inBuf.content_length = atol(cp);
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
         if (strchr(inBuf.host, '/') != NULL || inBuf.host[0] == '.')
            genError(conn_fd, &inBuf, 400, "Bad Request", NULL);
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
         genError(conn_fd, &inBuf, 417, "Expectation Failed", NULL);  //more);
      }
   }

   if (doBa) {
      if (!(inBuf.authorization && baValidate(inBuf.authorization,&inBuf.principle))) {
         //char more[]="WWW-Authenticate: Basic realm=\"cimom\"\r\n";
         genError(conn_fd, &inBuf, 401, "Unauthorized", NULL);  //more);
      }
   }

   len = inBuf.content_length;
   hdr = (char *) malloc(strlen(inBuf.authorization) + 64);
   len += hl =
       sprintf(hdr, "<!-- xml -->\n<!-- auth: %s -->\n", inBuf.authorization);

   getPayload(conn_fd, &inBuf);

   msgs[0].data = hdr;
   msgs[0].length = hl;
   msgs[1].data = inBuf.content;
   msgs[1].length = len - hl;

   {
     CimXmlRequestContext ctx =
        { inBuf.content, inBuf.principle, inBuf.host, len - hl, &conn_fd };
      ctx.chunkFncs=&httpChunkFunctions;
      response = handleCimXmlRequest(&ctx);
   }
   free(hdr);

   _SFCB_TRACE(1, ("--- Generate http response"));
   if (response.chunkedMode==0) writeResponse(conn_fd, response);

   //commClose(conn_fd);

#if defined USE_SSL
   if (sfcbSSLMode) {
      if ((SSL_get_shutdown(conn_fd.ssl) & SSL_RECEIVED_SHUTDOWN))
         SSL_shutdown(conn_fd.ssl);
      else SSL_clear(conn_fd.ssl);
      SSL_free(conn_fd.ssl);
   }
#endif

   freeBuffer(&inBuf);
   _SFCB_EXIT();
}

static void handleHttpRequest(int connFd)
{
   int r;
   CommHndl conn_fd;
   struct sembuf procReleaseUnDo = {0,1,SEM_UNDO};

   _SFCB_ENTER(TRACE_HTTPDAEMON, "handleHttpRequest");

   _SFCB_TRACE(1, ("--- Forking xml handler"));

   if (doFork) {
      semAcquire(httpWorkSem,0);
      semAcquire(httpProcSem,0);
      for (httpProcId=0; httpProcId<hMax; httpProcId++)
         if (semGetValue(httpProcSem,httpProcId+1)==0) break;
      procReleaseUnDo.sem_num=httpProcId+1; 
         
      r = fork();

      if (r==0) {
         currentProc=getpid();
         processName="CIMXML-Processor";
         semRelease(httpProcSem,0);
         semAcquireUnDo(httpProcSem,0);
         semReleaseUnDo(httpProcSem,httpProcId+1);
         semRelease(httpWorkSem,0);

         if (sfcbSSLMode) {
#if defined USE_SSL
            conn_fd.socket=-2;
            conn_fd.bio=BIO_new(BIO_s_socket());
            BIO_set_fd(conn_fd.bio,connFd,BIO_CLOSE);
            if (!(conn_fd.ssl = SSL_new(ctx)))
               intSSLerror("Error creating SSL context");
            SSL_set_bio(conn_fd.ssl, conn_fd.bio, conn_fd.bio);
            if (SSL_accept(conn_fd.ssl) <= 0)
               intSSLerror("Error accepting SSL connection");
#endif
         }
         else {
            conn_fd.socket=connFd;
#if defined USE_SSL
            conn_fd.bio=NULL;
            conn_fd.ssl=NULL;
#endif
         }
      }
      else if (r>0) {
         running++;
         _SFCB_EXIT();
      }
   }
   else r = 0;

   if (r < 0) {
      char *emsg=strerror(errno);
      mlogf(M_ERROR,M_SHOW,"--- fork handler: %s\n",emsg);
      exit(1);
   }

   if (r == 0) {
      if (doFork) {
         _SFCB_TRACE(1,("--- Forked xml handler %d", currentProc))
         resultSockets=sPairs[hBase+httpProcId];
      }

      _SFCB_TRACE(1,("--- Started xml handler %d %d", currentProc,
                   resultSockets.receive));

      if (getenv("SFCB_PAUSE_HTTP")) for (;;) {
         fprintf(stderr,"-#- Pausing - pid: %d\n",currentProc);
         sleep(5);
      }   
      
      conn_fd.socket=connFd;
#if defined USE_SSL
      conn_fd.bio=NULL;
      conn_fd.ssl=NULL;
#endif

      doHttpRequest(conn_fd);

      if (!doFork) return;

      _SFCB_TRACE(1, ("--- Xml handler exiting %d", currentProc));
      dumpTiming(currentProc);
      exit(0);
   }
}


int httpDaemon(int argc, char *argv[], int sslMode, int sfcbPid)
{
   struct sockaddr_in sin;
   int sz,i,sin_len,ru;
   char *cp;
   long procs, port;
   int listenFd, connFd;

   name = argv[0];
   debug = 1;
   doFork = 1;

   _SFCB_ENTER(TRACE_HTTPDAEMON, "httpDaemon");

   setupControl(configfile);
   sfcbSSLMode=sslMode;
   if (sslMode) processName="HTTPS-Daemon";
   else processName="HTTP-Daemon";

   if (sslMode) {
      if (getControlNum("httpsPort", &port))
         port = 5989;
      hBase=stBase;
      hMax=stMax;
   }
   else {
      if (getControlNum("httpPort", &port))
         port = 5988;
      hBase=htBase;
      hMax=htMax;
   }

   if (getControlNum("httpProcs", &procs))
      procs = 10;
   initHttpProcCtl(procs);

   if (getControlBool("doBasicAuth", &doBa))
      doBa=0;

   i = 1;
   while (i < argc && argv[i][0] == '-') {
      if (strcmp(argv[i], "-D") == 0)
         debug = 1;
      else if (strcmp(argv[i], "-nD") == 0)
         debug = 0;
      else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
         ++i;
         port = (unsigned short) atoi(argv[i]);
      }
      else if (strcmp(argv[i], "-tm") == 0) {
         if (isdigit(*argv[i + 1])) {
            ++i;
         }
      }
      else if (strcmp(argv[i], "-F") == 0)
         doFork = 1;
      else if (strcmp(argv[i], "-nF") == 0)
         doFork = 0;
      else if (strcmp(argv[i], "-H") == 0);
      ++i;
   }

   if (getControlBool("useChunking", &noChunking))
      noChunking=0;
   noChunking=noChunking==0;

   cp = strrchr(name, '/');
   if (cp != NULL)
      ++cp;
   else  cp = name;
   name = cp;

   if (sslMode) mlogf(M_INFO,M_SHOW,"--- %s HTTPS Daemon V" sfcHttpDaemonVersion " started - %d - port %ld\n", 
         name, currentProc,port);
   else mlogf(M_INFO,M_SHOW,"--- %s HTTP  Daemon V" sfcHttpDaemonVersion " started - %d - port %ld\n", 
         name, currentProc,port);


   if (doBa) mlogf(M_INFO,M_SHOW,"--- Using Basic Authentication\n");

   listenFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   sin_len = sizeof(sin);

   ru = 1;
   setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (char *) &ru, sizeof(ru));

   bzero(&sin, sin_len);

   sin.sin_family = AF_INET;
   sin.sin_addr.s_addr = INADDR_ANY;
   sin.sin_port = htons(port);

   if (bind(listenFd, (struct sockaddr *) &sin, sin_len) ||
       listen(listenFd, 0)) {
      mlogf(M_ERROR,M_SHOW,"--- Cannot listen on port %ld\n", port);
      kill(sfcbPid,3);
   }

  if (!debug) {
      int rc = fork();
      if (rc == -1) {
         char *emsg=strerror(errno);
         mlogf(M_ERROR,M_SHOW,"--- fork daemon: %s",emsg);
         exit(1);
      }
      else if (rc)
         exit(0);
   }
//   memInit();
    currentProc=getpid();
    setSignal(SIGCHLD, handleSigChld,0);
    setSignal(SIGUSR1, handleSigUsr1,0);
    setSignal(SIGINT, SIG_IGN,0);
    setSignal(SIGTERM, SIG_IGN,0);
    setSignal(SIGHUP, SIG_IGN,0);

    commInit();

#if defined USE_SSL
    if (sfcbSSLMode) {
       char *fnc,*fnk;
       ctx = SSL_CTX_new(SSLv23_method());
       getControlChars("sslCertificateFilePath", &fnc);
       if (SSL_CTX_use_certificate_chain_file(ctx, fnc) != 1)
           intSSLerror("Error loading certificate from file");
       getControlChars("sslKeyFilePath", &fnk);
       if (SSL_CTX_use_PrivateKey_file(ctx, fnk, SSL_FILETYPE_PEM) != 1)
           intSSLerror("Error loading private key from file");
    }
#endif

   for (;;) {
   char *emsg;
      listen(listenFd, 1);
      sz = sizeof(sin);
      if ((connFd = accept(listenFd, (__SOCKADDR_ARG) & sin, &sz))<0) {
         if (errno == EINTR || errno == EAGAIN) {
            if (stopAccepting) break;
            continue;
         }   
         emsg=strerror(errno);
         mlogf(M_ERROR,M_SHOW,"--- accept error %s\n",emsg);
         _SFCB_ABORT();
      }
      _SFCB_TRACE(1, ("--- Processing http request"));

      handleHttpRequest(connFd);
      close(connFd);
   }
   
//   printf("--- %s draining %d\n",processName,running);
   for (;;) {
      if (running==0) {
         mlogf(M_INFO,M_SHOW,"--- %s terminating %d\n",processName,getpid());
         exit(0);
      }   
      sleep(1);
   }   

}
