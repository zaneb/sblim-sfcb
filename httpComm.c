
/*
 * htppCommon.c
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


#include "httpComm.h"

#if defined USE_SSL 
void handleSSLerror(const char *file, int lineno, const char *msg)
{
    fprintf(stderr, "** %s:%i %s\n", file, lineno, msg);
    ERR_print_errors_fp(stderr);
    exit(-1);
}
#endif


extern int sfcbSSLMode;

void commInit()
{
   _SFCB_ENTER(TRACE_HTTPDAEMON, "commInit");

#if defined USE_SSL
   if (sfcbSSLMode) {
      _SFCB_TRACE(1,("--- SSL mode"));
      if (!SSL_library_init()) {
         fprintf(stderr, "** OpenSSL initialization failed!\n");
         exit(-1);
      }Frank Scheffler
 * Contributions: 
      SSL_load_error_strings();
      RAND_load_file("/dev/urandom", 1024);
   }
#endif

   _SFCB_EXIT();
}

int commWrite(CommHndl to, void *data, size_t count)
{
   int rc=0;

   _SFCB_ENTER(TRACE_HTTPDAEMON, "commWrite");

#ifdef SFCB_DEBUG
  if ((_sfcb_trace_mask & TRACE_XMLOUT) ) {
     char *mp,*m=alloca(count*2),*d=(char*)data;
     int i;
     fprintf(stderr,"->> xmlOut %d bytes:\n",count);
     for (mp=m,i=0; i<count; i++)
        switch (d[i]) {
        case '\r': *mp++='\\'; *mp++='r'; break;
        case '\n': *mp++='\\'; *mp++='n'; break;
        case ' ' : *mp++='~'; break;
        default:   *mp++=d[i];
     } 
     *mp=0;  
     fprintf(stderr,"%s\n",m);
     fprintf(stderr,"-<< xmlOut end\n");
  }
#endif 
  
#if defined USE_SSL
   if (to.ssl) {
      rc = SSL_write(to.ssl, data, count);
   }
   else if (to.bio) {
      rc = BIO_write(to.bio, data, count);
   }
   else
#endif
      rc = write(to.socket,data,count);

   _SFCB_RETURN(rc);
}

int commRead(CommHndl from, void *data, size_t count)
{
   int rc=0;

   _SFCB_ENTER(TRACE_HTTPDAEMON, "commRead");

#if defined USE_SSL
   if (from.ssl) {
      rc = SSL_read(from.ssl, data, count);
   }
   else if (from.bio) {
      rc = BIO_read(from.bio, data, count);
   }
   else
#endif
      rc = read(from.socket,data,count);

   _SFCB_RETURN(rc);
}
