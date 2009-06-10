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

void startLogging(const char *name, int level)
{
  openlog(name,LOG_PID,LOG_DAEMON);
  setlogmask(LOG_UPTO(level));
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
  va_list ap,apc;
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

  semAcquire(sfcbSem,LOG_GUARD_ID);
  va_start(ap,fmt);
  vsnprintf(buf,4096,fmt,ap);
  syslog(priosysl,"%s",buf);

  if (errout) {
    fprintf(stderr,"%s",buf);
  }
  va_end(ap);
  semRelease(sfcbSem,LOG_GUARD_ID);
}

