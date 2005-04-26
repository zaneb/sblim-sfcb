
/*
 * trace.h
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
 * Based on concepts developed by Heidi Neumann <heidineu@de.ibm.com>
 *
 * Description:
 *
 * Trace support for sfcb.
 *
*/

#ifndef _trace_h
#define _trace_h

#include "mlog.h"

#ifdef SFCB_DEBUG
#define _SFCB_TRACE(LEVEL,STR) \
  if ((_sfcb_trace_mask & __traceMask) && (LEVEL<=_sfcb_debug) && (LEVEL>0) ) \
  _sfcb_trace(LEVEL,__FILE__,__LINE__,_sfcb_format_trace STR);

#define _SFCB_ENTER(n,f) \
   char *__func_=f; \
   unsigned long __traceMask=n; \
   _SFCB_TRACE(1,("Entering: %s",__func_));

#define _SFCB_EXIT() {\
   _SFCB_TRACE(1,("Leaving: %s",__func_)); \
   return; }

#define _SFCB_RETURN(v) {\
   _SFCB_TRACE(1,("Leaving: %s",__func_)); \
   return v; }

#define _SFCB_TRACE_INIT() \
   _sfcb_trace_init();

#define _SFCB_TRACE_START(n) \
   _sfcb_trace_start(n);

#define _SFCB_TRACE_STOP() \
   _sfcb_trace_stop(n);

#define _SFCB_TRACE_FUNCTION(LEVEL,f) \
   _SFCB_TRACE(LEVEL,("Invoking trace function %s",#f)); \
    if ((_sfcb_trace_mask & __traceMask) &&  (LEVEL<=_sfcb_debug) && (LEVEL>0) ) { \
    f;}

#define _SFCB_ABORT() {\
   _SFCB_TRACE(1,("Aborting: %s",__func_)); \
   abort(); }

#define TRAP(n) \
      _sfcb_trap(n);

extern int _sfcb_debug;
extern unsigned long _sfcb_trace_mask;

extern char *_sfcb_format_trace(char *fmt, ...);
extern void _sfcb_trace(int, char *, int, char *);
extern void _sfcb_trace_start(int l);
extern void _sfcb_trace_init();
extern void _sfcb_trace_stop();
extern void _sfcb_set_trace_mask(int n);
extern void _sfcb_trap(int n);

#else
#define _SFCB_TRACE_FUNCTION(n,f)
#define _SFCB_TRACE(LEVEL,STR)
#define _SFCB_ENTER(n,f)
#define _SFCB_EXIT() { return; }
#define _SFCB_RETURN(v) {\
   return (v); }
#define _SFCB_ABORT() {\
     printf("--- %s(%d) Abrted\n",__FILE__,__LINE__); \
     abort(); }
#define _SFCB_TRACE_INIT()
#define _SFCB_TRACE_START(n)
#define _SFCB_TRACE_STOP()
#define TRAP(n)
#endif

extern void _sfcb_set_trace_mask(int n);

typedef struct traceId {
   char *id;
   int code;
} TraceId;

#define TRACE_PROVIDERMGR       1
#define TRACE_PROVIDERDRV       2
#define TRACE_CIMXMLPROC        4
#define TRACE_HTTPDAEMON        8
#define TRACE_UPCALLS           16
#define TRACE_ENCCALLS          32
#define TRACE_PROVIDERINSTMGR   64
#define TRACE_PROVIDERASSOCMGR  128
#define TRACE_PROVIDERS         256
#define TRACE_INDPROVIDER       512
#define TRACE_INTERNALPROVIDER  1024
#define TRACE_OBJECTIMPL        2048
#define TRACE_SOCKETS           4096
#define TRACE_MEMORYMGR         8192
#define TRACE_MSGQUEUE          16384
#define TRACE_XMLPARSING        32768 

typedef void sigHandler(int);

extern sigHandler *setSignal(int sn, sigHandler * sh, int flags);

extern char *processName;
extern int providerProcess;
extern int idleThreadId;
extern int terminating;

#endif

