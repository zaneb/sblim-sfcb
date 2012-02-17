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
#include <signal.h>
#include "trace.h"              /* for setSignal() */
#include <sys/wait.h>

// Macro to open the syslog
#define OPENLOG(level) openlog("sfcb", LOG_PID, LOG_DAEMON); setlogmask(LOG_UPTO(level));


 
FILE           *log_w_stream;
int             logfds[2] = { 0, 0 };

/*
+ * main function for the logger proc. Waits on a pipe and writes to syslog
+ * Will exit when the other side of the pipe closes 
+ */
void runLogger(int listenFd, int level) {

  FILE           *stream;
  int             priosysl;
  char            buf[LOG_MSG_MAX];
 
  OPENLOG(level);
 
  stream = fdopen(listenFd, "r");

  while (!feof(stream)) {
 
    fgets(buf, sizeof(buf), stream);
 
    int             priority = buf[0];
 
    switch (priority) {
    case M_DEBUG:
      priosysl = LOG_DEBUG;
      break;
    case M_INFO:
      priosysl = LOG_INFO;
      break;
    case M_ERROR:
    default:
      priosysl = LOG_ERR;
      break;
    }

    syslog(priosysl, "%s", buf + 1);

  }
  return;
}
 

/*
 * sets up the logging pipe and forks off the logger process 
 */
void startLogging(int level, int thread) {

  // if we're a client, just open the log and
  // don't start a logger.
  if (! thread ) {
     OPENLOG(level);
    return;
  }
    
  pipe(logfds);
  int             lpid;

  lpid = fork();

  if (lpid == 0) {
    close(logfds[1]);           /* close write end */
    setSignal(SIGINT, SIG_IGN, 0);
    setSignal(SIGTERM, SIG_IGN, 0);
    setSignal(SIGHUP, SIG_IGN, 0);

    runLogger(logfds[0], level);

    close(logfds[0]);
    exit(0);
  } else if (lpid > 0) {
    close(logfds[0]);           /* close read end */
    log_w_stream = fdopen(logfds[1], "w");
    return;
  } else {
    fprintf(stderr, "*** fork of logger proc failed\n");
    abort();
  }
}

/** \brief closeLogging - Closes down logging
  *
  * Closes the pipe used for logging and  closes out
  * the syslog services that are created in startLogging.
  */
void closeLogging() {
   int wstat;
   closelog();
   close(logfds[1]);
   wait(&wstat); // wait to prevent zombie
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
void mlogf(int priority, int errout, const char *fmt, ...) {

  va_list         ap;
  char            buf[LOG_MSG_MAX];
 
  va_start(ap, fmt);
  // Leave a space for the /n on the end.
  vsnprintf(buf, LOG_MSG_MAX-1, fmt, ap);
  // Now check to make sure we have a /n at the end
  int buflen=strlen(buf);
  if ( buf[buflen-1] != '\n' ) {
    strcat(buf,"\n");
  } 



  /*
   * we sometimes call mlogf when sfcbd isn't started (i.e. via
   * sfcbinst2mof) 
   */
  if (logfds[1] == 0) {
    fprintf(stderr, "logger not started");
    int             priosysl;
    switch (priority) {
    case M_DEBUG:
      priosysl = LOG_DEBUG;
      break;
    case M_INFO:
      priosysl = LOG_INFO;
      break;
    case M_ERROR:
    default:
      priosysl = LOG_ERR;
      break;
    }
    syslog(priosysl, "%s", buf);
  }
  /*
   * if sfcbd is started, the logger proc will be waiting to recv log msg 
   */
  else {
    fprintf(log_w_stream, "%c%s", priority, buf);
    fflush(log_w_stream);
  }
 
  /*
   * also print out the logg message to stderr if M_SHOW was passed in 
   */
   if (errout) {
     fprintf(stderr, "%s", buf);
   }

   va_end(ap);

}

