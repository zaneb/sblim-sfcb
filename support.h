
/*
 * support.c
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
 * Author:        Frank Scheffler
 * Contributions: Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Various support routines
 *
*/


#ifndef CMPI_TOOL_H
#define CMPI_TOOL_H

#define CMPI_VERSION 90

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpiftx.h"
#include "cmpimacs.h"


CMPIInstanceMI *loadInstanceMI(const char *provider,
                                     void *library,
                                     CMPIBroker * broker, CMPIContext * ctx);
CMPIAssociationMI *loadAssociationMI(const char *provider,
                                           void *library,
                                           CMPIBroker * broker,
                                           CMPIContext * ctx);
CMPIMethodMI *loadMethodMI(const char *provider,
                                 void *library,
                                 CMPIBroker * broker, CMPIContext * ctx);
CMPIPropertyMI *loadPropertyMI(const char *provider,
                                     void *library,
                                     CMPIBroker * broker, CMPIContext * ctx);
CMPIIndicationMI *loadIndicationMI(const char *provider,
                                         void *library,
                                         CMPIBroker * broker,
                                         CMPIContext * ctx);

CMPIClassMI *loadClassMI(const char *provider,
                                         void *library,
                                         CMPIBroker * broker,
                                         CMPIContext * ctx);


/*!
  \file support.h
  \brief Memory Managment system for providers (header file).

  \author Frank Scheffler

  \sa support.h
  \sa native.h
*/


//! States cloned objects, i.e. memory that is not being tracked.
#define TOOL_MM_NO_ADD 0
#define MEM_NOT_TRACKED 0

//! States tracked memory objects.
#define TOOL_MM_ADD    1
#define MEM_TRACKED   1

//! States tracked memory objects but already released.
#define TOOL_MM_RELEASED 2
#define MEM_RELEASED -1

//! The initial size of trackable memory pointers per thread.
/*!
  This size is increased by the same amount, once the limit is reached.
 */
#define MT_SIZE_STEP 100


typedef struct {
   int ftVersion;
    CMPIStatus(*release) (void *obj);
} ObjectFT;

typedef struct {
   void *hdl;
   ObjectFT *ft;
} Object;


typedef struct _managed_thread managed_thread;

#include <dlfcn.h>

//! Per-Thread management structure.
/*!
  This struct is returned using a global pthread_key_t and stores all allocated
  objects that are going to be freed, once the thread is flushed or dies.
 */

typedef struct heapControl {
   unsigned size;               /*!< current maximum number of tracked pointers */
   unsigned used;               /*!< currently tracked pointers */
   void **objs;
   unsigned encUsed;
   unsigned encSize;
   Object **encObjs;
   
   unsigned memSize;               /*!< current maximum number of tracked pointers */
   unsigned memUsed;               /*!< currently tracked pointers */
   void **memObjs;
   unsigned memEncUsed;
   unsigned memEncSize;
   Object **memEncObjs;
} HeapControl;
 

struct _managed_thread {
   void *broker;
   void *ctx;
   void *data;
   HeapControl hc;
};



void *tool_mm_load_lib(const char *libname);

void tool_mm_flush();
void *tool_mm_alloc(int, size_t);
void *tool_mm_realloc(void *, size_t);
int tool_mm_add(void *);
void tool_mm_set_broker(void *, void *);
int tool_mm_remove(void *);
void *tool_mm_get_broker(void **);
void *tool_mm_add_obj(int mode, void *ptr, size_t size);

void *memAlloc(int add, size_t size, int *memId);
void *memAddEncObj(int mode, void *ptr, size_t size, int *memId);
void memUnlinkEncObj(int memId);
void memLinkEncObj(void *ptr, int *memId);
void memLinkInstance(CMPIInstance *ci);


typedef struct cntlVals {
   int type;
   char *id;
   char *val;
} CntlVals;

void cntlSkipws(char **p);
int cntlParseStmt(char *in, CntlVals * rv);
char *cntlGetVal(CntlVals * rv);

#endif
