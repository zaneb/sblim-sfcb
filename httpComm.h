
/*
 * htppCommon.h
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
 * http common (http(https) support
 *
*/

#ifndef HTTPCONN_H
#define HTTPCONN_H

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

#include "config.h"

#if HAVE_LIBSSL
 #define USE_SSL 1
#endif

#ifdef USE_SSL
 #include <openssl/rand.h>
 #include <openssl/ssl.h>
 #include <openssl/err.h>

 #define intSSLerror(msg)  handleSSLerror(__FILE__, __LINE__, msg)
 void handleSSLerror(const char *file, int lineno, const char *msg);
#endif


typedef struct commHndl {
  int socket;
#if defined USE_SSL
  BIO *bio;
  SSL *ssl;
#endif
  int rc;
} CommHndl;

void commInit();
int commWrite(CommHndl to, void *data, size_t count);
int commRead(CommHndl from, void *data, size_t count);

#endif
