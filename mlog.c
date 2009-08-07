/*
 * $Id$
 *
 * (C) Copyright IBM Corp. 2003, 2004
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.cim>
 * Contributors: Michael Schuele <schuelem@de.ibm.com>
 *               Anas Nashif <nashif@planux.com>
 *
 * Description: Logger support.
 *
 */

const char *_mlog_id = "$Id$";


#include "mlog.h"
#include "msgqueue.h"
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>

//semaphore
static key_t logSemKey;
static int logSem = -1;


void startLogging(const char *name, int level)
{
   union semun sun;

   logSemKey=ftok(SFCB_BINARY,getpid());

    // if sem exists, clear it out.
   if ((logSem=semget(logSemKey,1, 0600))!=-1)
      semctl(logSem,0,IPC_RMID,sun);

   if ((logSem=semget(logSemKey,1,IPC_CREAT | IPC_EXCL | 0600))==-1) {
      char *emsg=strerror(errno);
      fprintf(stderr,"\n--- Logging semaphore create key: 0x%x failed: %s\n",logSemKey,emsg);
      abort();
   }

   sun.val=1;
   semctl(logSem,0,SETVAL,sun);

  openlog(name,LOG_PID,LOG_DAEMON);
  setlogmask(LOG_UPTO(level));

}

/** \brief closeLogging - Closes down loggin
 *
 * closeLogging deletes the semaphore and closes out
 * the syslog services that are created in startLogging.
 */
void closeLogging ()
{
    union semun sun;
    semctl(logSem,0,IPC_RMID,sun);
    closelog();
}

/** \brief mlogf - Create syslog entries
 *
 * This should be called with a format string in fmt, with 
 * the variables to be inserted in it as the arguments 
 * following (...)
 * eg
 *  mlogf(M_ERROR,M_SHOW,"--- %s failed rc=%d\n",oper,rc);
 * 
 * Don't allow user input into the format string as it 
 * is not to be trusted. No need to use sprintf to build
 * the string before passing it to mlogf.
 */
void mlogf(int priority, int errout, const char *fmt, ...)
{
  va_list ap;
  int priosysl;

  char buf[4096];

  switch (priority) {
  case M_DEBUG:
    priosysl=LOG_DEBUG;
    break;
  case M_INFO:
    priosysl=LOG_INFO;
    break;
  case M_ERROR:
  default:
    priosysl=LOG_ERR;
    break;
  }

  if (semAcquire(logSem,0)) {
      char *emsg=strerror(errno);
      fprintf(stderr,"\n--- Unable to acquire logging lock: %s\n",emsg);
      // not aborting since that will kill sfcb, so try to continue
  }
  
  va_start(ap,fmt);
  vsnprintf(buf,4096,fmt,ap);
  syslog(priosysl,"%s",buf);

  if (errout) {
    fprintf(stderr,"%s",buf);
  }
  va_end(ap);
  if (semRelease(logSem,0)) {
      char *emsg=strerror(errno);
      fprintf(stderr,"\n--- Unable to release logging lock: %s\n",emsg);
      // not aborting since that will kill sfcb, so try to continue
  }
}

