
/*
 * msgqueue.c
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
 * Author:     Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Intra process communication support for sfcb.
 *
*/



#include <string.h>
#include <errno.h>
#include "msgqueue.h"
#include "trace.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


extern unsigned long exFlags;

ComSockets resultSockets;
ComSockets sfcbSockets;
ComSockets providerSockets;

ComSockets *sPairs;
int ptBase,htBase,stBase,htMax,stMax;
int httpProcId;
int currentProc=0;
int noProvPause=0;
char *provPauseStr=NULL;
int noHttpPause=0;
char *httpPauseStr=NULL;

key_t sfcbSemKey;
int sfcbSem;


/*
 *              semaphore services
 */

int semAcquireUnDo(int semid, int semnum)
{
   int rc;
   
   struct sembuf semBuf = {semnum,-1,SEM_UNDO};
   rc=semop(semid,&semBuf,1);
   return rc;
}

int semAcquire(int semid, int semnum)
{
   int rc;
   
   struct sembuf semBuf = {semnum,-1,0};
   rc=semop(semid,&semBuf,1);
   return rc;
}

int semRelease(int semid, int semnum)
{
   int rc;
   struct sembuf semBuf = {semnum,1,0};
  
   rc=semop(semid,&semBuf,1);
   return rc;
}

int semReleaseUnDo(int semid, int semnum)
{
   int rc;
   struct sembuf semBuf = {semnum,1,SEM_UNDO};
  
   rc=semop(semid,&semBuf,1);
   return rc;
}

int semMultiRelease(int semid, int semnum, int n)
{
    struct sembuf semBuf = {semnum,1,0};
  
    return semop(semid,&semBuf,n);
}

int semGetValue(int semid, int semnum)
{
   union semun sun;
   return semctl(semid,semnum,GETVAL,sun);
}

int semSetValue(int semid, int semnum, int value)
{
   union semun sun;
   sun.val=value;
   return semctl(semid,semnum,SETVAL,sun);
}

int initSem(int https, int shttps, int provs)
{
   union semun sun;
   char emsg[256];
   int i;
   
   sfcbSemKey=ftok(".",'S');

   if ((sfcbSem=semget(sfcbSemKey,1, 0666))!=-1) 
      semctl(sfcbSem,0,IPC_RMID,sun);
   
   if ((sfcbSem=semget(sfcbSemKey,4+(provs*3)+3,IPC_CREAT | IPC_EXCL | 0666))==-1) {
      sprintf(emsg,"SFCB semaphore create %d",currentProc);
      perror(emsg);
      abort();
   }

   sun.val=1;
   semctl(sfcbSem,httpGuardId,SETVAL,sun);
   sun.val=https;
   semctl(sfcbSem,httpProcsId,SETVAL,sun);
   sun.val=1;
   semctl(sfcbSem,shttpGuardId,SETVAL,sun);
   sun.val=shttps;
   semctl(sfcbSem,shttpProcsId,SETVAL,sun);
   
   for (i=0; i<provs; i++) {
      sun.val=1;
      semctl(sfcbSem,(i*3)+provProcGuardId+provProcBaseId,SETVAL,sun);
      sun.val=0;
      semctl(sfcbSem,(i*3)+provProcInuseId+provProcBaseId,SETVAL,sun);
      semctl(sfcbSem,(i*3)+provProcAliveId+provProcBaseId,SETVAL,sun);
   }      
   return 0;   
}

MsgSegment setCharsMsgSegment(char *str)
{
   MsgSegment s;
   s.data = str;
   if (str)
      s.length = strlen(str) + 1;
   else
      s.length = 0;
   s.type = MSG_SEG_CHARS;
   return s;
}

static int spHandleError(int *s, char *m)
{
   _SFCB_ENTER(TRACE_MSGQUEUE, "handleError");
   char emsg[256];

   sprintf(emsg, "%s %d %d-%d", m, *s, currentProc, errno);
   perror(emsg);
   _SFCB_ABORT();
   return errno;
}


/*
 *              spGetMsg
 */

static int spGetMsg(int *s, void *data, unsigned length, MqgStat* mqg)
{
   static char *em = "spGetMsg receiving from";
   ssize_t n,ol,r=0;

   _SFCB_ENTER(TRACE_MSGQUEUE, "spGetMsg");
   _SFCB_TRACE(1, ("--- Receiving from %d length %d", *s, length));

   ol = length;
   for (;;) {
      if (mqg) mqg->teintr=0;
      if ((n = recv(*s, data+r, length-r, 0)) < 0) {
         if (errno == EINTR) {
            _SFCB_TRACE(1, (" Receive interrupted %d",currentProc));
            if (mqg) {
               mqg->teintr=1;
               return 0;
            }
            continue;
         }
         return spHandleError(s, em);
      }

      if (n == 0) {
         perror("Warning: fd is closed:");
         break;
      }

      r+=n;      
      if (r < ol) continue;
      break;
   }
   return 0;
}


/*
 *              spRcvMsg
 */

static int spRcvMsg(int *s, int *from, void **data, unsigned long *length, MqgStat* mqg)
{
   SpMessageHdr spMsg;
   static char *em = "rcvMsg receiving from";
   MqgStat imqg;

   _SFCB_ENTER(TRACE_MSGQUEUE, "spRcvMsg");
   _SFCB_TRACE(1, ("--- Receiving from %d", *s));

   if ((spGetMsg(s, &spMsg, sizeof(spMsg), mqg)) == -1)
      return spHandleError(s, em);
      
   if (mqg && mqg->teintr) {
      mqg->eintr=1;    
      mqg->rdone=0;
      return 0;  
   }   
   *from=spMsg.returnS;   

   _SFCB_TRACE(1, ("--- Received info segment %d bytes", sizeof(spMsg)));

   *length = spMsg.totalSize;

   if (mqg==NULL) mqg=&imqg;
   mqg->rdone=1;
   mqg->eintr=0;
   
   if (*length) {
      *data = malloc(spMsg.totalSize + 8);
      do {
         if ((spGetMsg(s, *data, *length, mqg)) == -1)
            return spHandleError(s, em);
         if (mqg->teintr) mqg->eintr=1;    
      } while (mqg->teintr) ;       
      _SFCB_TRACE(1, ("--- Received data segment %d bytes", *length));
   }

   if (spMsg.type.type == MSG_DATA) {
      _SFCB_TRACE(1, ("--- Received %d bytes", *length));
      _SFCB_RETURN(0);
   }

   if (spMsg.type.ctl.xtra == MSG_X_EXTENDED_CTL_MSG) {
      *data = malloc(256);
      *length = 256;
      do {
         if ((spGetMsg(s, *data, *length, mqg)) == -1)
            return spHandleError(s, em);
         if (mqg->teintr) mqg->eintr=1;    
      } while (mqg->teintr) ;       
   }

   switch (spMsg.type.ctl.xtra) {
   case MSG_X_PROVIDER:
      *length = spMsg.segments;
      *data = spMsg.provId;
   case MSG_X_INVALID_NAMESPACE:
   case MSG_X_PROVIDER_NOT_FOUND:
   case MSG_X_INVALID_CLASS:
      _SFCB_RETURN(spMsg.type.ctl.xtra);
   default:
      *data = NULL;
      fprintf(stderr,"### %d ??? %ld-%d\n", currentProc, spMsg.type.type,
             spMsg.type.ctl.xtra);
      abort();
   }
   
   _SFCB_RETURN(spMsg.type.ctl.xtra);
}

int spRecvReq(int *s, int *from, void **data, unsigned long *length, MqgStat *mqg)
{
   int rc;
   _SFCB_ENTER(TRACE_MSGQUEUE, "spRecvReq");
   rc = spRcvMsg(s, from, data, length, mqg);
   _SFCB_RETURN(rc);
}

int spRecvResult(int *s, int *from, void **data, unsigned long *length)
{
   int rc;
   _SFCB_ENTER(TRACE_MSGQUEUE, "spRecvResult");
   rc = spRcvMsg(s, from, data, length, NULL);
   _SFCB_RETURN(rc);
}

int spRecvCtlResult(int *s, int *from, void **data, unsigned long *length)
{
   int rc;
   _SFCB_ENTER(TRACE_MSGQUEUE, "spRecvCtlResult");
   rc = spRcvMsg(s, from, data, length, NULL);
   _SFCB_RETURN(rc);
}

/*
 *              spSendMsg
 */

static int spSendMsg(int *to, int *from, int n, struct iovec *iov, int size)
{
   SpMessageHdr spMsg = { {MSG_DATA}, *from, size };
   spMsg.type.ctl.type = MSG_DATA;
   static char *em = "spSendMsg sending to";
   struct msghdr msg;
   ssize_t rc;

   _SFCB_ENTER(TRACE_MSGQUEUE, "spSendMsg");
   _SFCB_TRACE(1, ("--- Sending %d bytes to %d", size, *to));

   spMsg.returnS=*from;
   msg.msg_control = 0;
   msg.msg_controllen = 0;

   msg.msg_name = NULL;
   msg.msg_namelen = 0;

   msg.msg_iov = iov;
   msg.msg_iovlen = n;

   iov[0].iov_base = &spMsg;
   iov[0].iov_len = sizeof(spMsg);

   if ((rc=sendmsg(*to, &msg, 0)) < 0) {
      if (errno==EBADF) _SFCB_RETURN(-2)
      return spHandleError(to, em);
   }
   _SFCB_TRACE(1, ("--- Sendt %d bytes to %d", rc, *to));

   _SFCB_RETURN(0);
}

int spSendReq(int *to, int *from, void *data, unsigned long size)
{
   int rc,n;
   struct iovec iov[2];
   _SFCB_ENTER(TRACE_MSGQUEUE, "spSendReq");
   
   if (size) {
      n=2;
      iov[1].iov_base = data;
      iov[1].iov_len = size;
   }
   else n=1;
   
   rc = spSendMsg(to, from, n, iov, size);
   _SFCB_RETURN(rc);
}

int spSendResult(int *to, int *from, void *data, unsigned long size)
{
   int rc,n;
   struct iovec iov[2];
   _SFCB_ENTER(TRACE_MSGQUEUE, "spSendResult");
   
   if (size) {
      n=2;
      iov[1].iov_base = data;
      iov[1].iov_len = size;
   }
   else n=1;
   
   rc = spSendMsg(to, from, n, iov, size);
   _SFCB_RETURN(rc);
}

int spSendResult2(int *to, int *from, 
   void *d1, unsigned long s1, void *d2, unsigned long s2)
{
   int rc,n;
   struct iovec iov[4];
   _SFCB_ENTER(TRACE_MSGQUEUE, "spSendResult2");
   
   iov[1].iov_base = d1;
   iov[1].iov_len = s1;
   
   if (s2) {
      n=3;
      iov[2].iov_base = d2;
      iov[2].iov_len = s2;
   }
   else n=2;
   
   rc = spSendMsg(to, from, n, iov, s1+s2);
   _SFCB_RETURN(rc);
}

int spSendAck(int to)
{
   int rc;
   _SFCB_ENTER(TRACE_MSGQUEUE, "spSendAck");
   rc = write(to, "ack",4);
   _SFCB_RETURN(rc);
}

int spRcvAck(int from)
{
   int rc;
   char ack[8];
   _SFCB_ENTER(TRACE_MSGQUEUE, "spRcvAck");
   rc = read(from, ack,4);
   _SFCB_RETURN(rc);
}

/*
 *              sendCtl
 */

static int spSendCtl(int *to, int *from, short code, unsigned long count,
                     void *data)
{
   SpMessageHdr spMsg = { {MSG_DATA}, *from, 0 };
   static char *em = "spSendCtl sending to";
   struct msghdr msg;
   struct iovec iov[2];

   union {
      struct cmsghdr cm;
      char control[CMSG_SPACE(sizeof(int))];
   } control_un;
   struct cmsghdr *cmptr;

   _SFCB_ENTER(TRACE_MSGQUEUE, "spSendCtl");
   _SFCB_TRACE(1, ("--- Sending %d bytes to %d", sizeof(SpMessageHdr), *to));

   msg.msg_control = control_un.control;
   msg.msg_controllen = sizeof(control_un.control);

   cmptr = CMSG_FIRSTHDR(&msg);
   cmptr->cmsg_len = CMSG_LEN(sizeof(int));
   cmptr->cmsg_level = SOL_SOCKET;
   cmptr->cmsg_type = SCM_RIGHTS;
   *((int *) CMSG_DATA(cmptr)) = *from;

   msg.msg_name = NULL;
   msg.msg_namelen = 0;

   msg.msg_iov = iov;
   msg.msg_iovlen = 1;

   spMsg.type.ctl.type = MSG_CTL;
   spMsg.type.ctl.xtra = code;
   spMsg.segments = count;
   spMsg.provId = data;

   iov[0].iov_base = &spMsg;
   iov[0].iov_len = sizeof(spMsg);

   if (sendmsg(*to, &msg, 0) < 0)
      return spHandleError(to, em);
      
   _SFCB_RETURN(0);
}

int spSendCtlResult(int *to, int *from, short code, unsigned long count,
                    void *data)
{
   int rc;
   _SFCB_ENTER(TRACE_MSGQUEUE, "spSendCtlResult");
   rc = spSendCtl(to, from, code, count, data);
   _SFCB_RETURN(rc);
}


void initSocketPairs(int provs, int https, int shttps)
{
   int i,t=(provs*2)+https; //,shttps;
   
   sPairs=(ComSockets*)malloc(sizeof(ComSockets)*t);
   fprintf(stderr,"--- initSocketPairs: %d\n",t);
   for (i=0; i<t; i++) {
      socketpair(PF_LOCAL, SOCK_STREAM, 0, &sPairs[i].receive);
 //     fprintf(stderr,"--- initSocketPairs sock[%d]: %d\n",i,sPairs[i].receive);
   }   
   ptBase=provs;
   htBase=ptBase+provs;
   stBase=htBase+https;
   htMax=https;
   stMax=shttps;
}

unsigned long getInode(int fd)
{
   struct stat st;
   fstat(fd,&st);
   return st.st_ino;
}

ComSockets getSocketPair(char *by)
{
   ComSockets sp;
   _SFCB_ENTER(TRACE_MSGQUEUE | TRACE_SOCKETS, "getSocketPair");
 
   socketpair(PF_LOCAL, SOCK_STREAM, 0, &sp.receive);
   _SFCB_TRACE(1,("--- %s rcv: %d - %d %d",by,sp.receive,getInode(sp.receive),currentProc));       
   _SFCB_TRACE(1,("--- %s snd: %d - %d %d",by,sp.send,getInode(sp.send),currentProc));       
   
   _SFCB_RETURN(sp);
}

void closeSocket(ComSockets *sp, ComCloseOpt o,char *by)
{
   _SFCB_ENTER(TRACE_MSGQUEUE | TRACE_SOCKETS, "closeSocket");
   
   if ((o==cRcv || o==cAll) && sp->receive!=0) {
     _SFCB_TRACE(1,("--- %s closing: %d - %d %d\n",by,sp->receive,getInode(sp->receive),currentProc));       
      close (sp->receive); 
      sp->receive=0;
   }   
   if ((o==cSnd || o==cAll) && sp->send!=0) {
     _SFCB_TRACE(1,("--- %s closing: %d - %d %d\n",by,sp->send,getInode(sp->send),currentProc));       
      close (sp->send); 
      sp->send=0;
   }   
   _SFCB_EXIT();
}
