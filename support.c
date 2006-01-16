
/*
 * support.c
 *
 * (C) Copyright IBM Corp. 2005
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:        Frank Scheffler
 * Contributions: Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Various support routines
 *
*/


#include <stdio.h>
#include <dlfcn.h>
#include "support.h"
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include "native.h"
#include "trace.h"
#include "config.h"

#ifdef SFCB_IX86
#define SFCB_ASM(x) asm(x)
#else
#define SFCB_ASM(x)
#endif

int collectStat=0;
unsigned long exFlags = 0;

void *loadLibib(const char *libname)
{
   char filename[255];
   sprintf(filename, "lib%s.so", libname);
   return dlopen(filename, RTLD_LAZY);
}


static void *getGenericEntryPoint(void *library, const char *ptype)
{
   char entry_point[255];
   void *sym;
   sprintf(entry_point, "_Generic_Create_%sMI", ptype);
   sym = dlsym(library, entry_point);
   return sym;
}


static void *getFixedEntryPoint(const char *provider,
                                     void *library, const char *ptype)
{
   char entry_point[255];
   void *sym;
   sprintf(entry_point, "%s_Create_%sMI", provider, ptype);
   sym = dlsym(library, entry_point);
   return sym;
}


typedef CMPIInstanceMI *(*GENERIC_InstanceMI) (CMPIBroker * broker,
                                               CMPIContext * ctx,
                                               const char *provider);
typedef CMPIInstanceMI *(*FIXED_InstanceMI) (CMPIBroker * broker,
                                             CMPIContext * ctx);


CMPIInstanceMI *loadInstanceMI(const char *provider, void *library,
                                     CMPIBroker * broker, CMPIContext * ctx)
{
   CMPIInstanceMI *mi; 
   _SFCB_ENTER(TRACE_PROVIDERDRV, "loadInstanceMI");
   
   GENERIC_InstanceMI g = (GENERIC_InstanceMI) 
      getGenericEntryPoint(library,"Instance");
   if (g == NULL) {
      FIXED_InstanceMI f = (FIXED_InstanceMI) 
         getFixedEntryPoint(provider, library, "Instance");
      if (f == NULL) _SFCB_RETURN(NULL);
      if (broker) {
         mi=(f) (broker, ctx);
         _SFCB_RETURN(mi);
      }   
      _SFCB_RETURN((CMPIInstanceMI*)(void*)-1);
   }
   if (broker) {
      mi=(g) (broker, ctx, provider);
      _SFCB_RETURN(mi);
   }
   _SFCB_RETURN((CMPIInstanceMI*)(void*)-1);
};


typedef CMPIAssociationMI *(*GENERIC_AssociationMI) (CMPIBroker * broker,
                                                     CMPIContext * ctx,
                                                     const char *provider);
typedef CMPIAssociationMI *(*FIXED_AssociationMI) (CMPIBroker * broker,
                                                   CMPIContext * ctx);


CMPIAssociationMI *loadAssociationMI(const char *provider,
                                           void *library,
                                           CMPIBroker * broker,
                                           CMPIContext * ctx)
{
   CMPIAssociationMI *mi; 
   _SFCB_ENTER(TRACE_PROVIDERDRV, "loadAssociationMI");
   
   GENERIC_AssociationMI g = (GENERIC_AssociationMI) 
      getGenericEntryPoint(library, "Association");
      
   if (g == NULL) {
      FIXED_AssociationMI f = (FIXED_AssociationMI) 
         getFixedEntryPoint(provider, library, "Association");
      if (f == NULL) _SFCB_RETURN(NULL);
      if (broker) {
         mi=(f) (broker, ctx);
         _SFCB_RETURN(mi);
      }   
      _SFCB_RETURN((CMPIAssociationMI*)(void*)-1);
   }
   if (broker) {
      mi=(g) (broker, ctx, provider);
      _SFCB_RETURN(mi);
   }
   _SFCB_RETURN((CMPIAssociationMI*)(void*)-1);
};


typedef CMPIMethodMI *(*GENERIC_MethodMI) (CMPIBroker * broker,
                                           CMPIContext * ctelsex,
                                           const char *provider);
typedef CMPIMethodMI *(*FIXED_MethodMI) (CMPIBroker * broker,
                                         CMPIContext * ctx);


CMPIMethodMI *loadMethodMI(const char *provider, void *library,
                                 CMPIBroker * broker, CMPIContext * ctx)
{
   CMPIMethodMI *mi; 
   _SFCB_ENTER(TRACE_PROVIDERDRV, "loadMethodMI");
   
   GENERIC_MethodMI g =
       (GENERIC_MethodMI) getGenericEntryPoint(library, "Method");
   if (g == NULL) {
      FIXED_MethodMI f =
          (FIXED_MethodMI) getFixedEntryPoint(provider, library,"Method");
      if (f == NULL) _SFCB_RETURN(NULL);
      if (broker) {
         mi=(f)(broker, ctx);
         _SFCB_RETURN(mi);
      }   
      _SFCB_RETURN((CMPIMethodMI*)(void*)-1);
   }
   if (broker)  {
      mi=(g)(broker,ctx, provider);
      _SFCB_RETURN(mi);
   }
   _SFCB_RETURN((CMPIMethodMI*)(void*)-1);
}


typedef CMPIPropertyMI *(*GENERIC_PropertyMI) (CMPIBroker * broker,
                                               CMPIContext * ctx,
                                               const char *provider);
typedef CMPIPropertyMI *(*FIXED_PropertyMI) (CMPIBroker * broker,
                                             CMPIContext * ctx);

CMPIPropertyMI *loadPropertyMI(const char *provider, void *library,
                                     CMPIBroker * broker, CMPIContext * ctx)
{
   CMPIPropertyMI *mi; 

   _SFCB_ENTER(TRACE_PROVIDERDRV, "loadPropertyMI");
   GENERIC_PropertyMI g =
       (GENERIC_PropertyMI) getGenericEntryPoint(library,"Property");
   if (g == NULL) {
      FIXED_PropertyMI f =
          (FIXED_PropertyMI) getFixedEntryPoint(provider, library,
                                                     "Property");
      if (f == NULL) _SFCB_RETURN(NULL);
      if (broker) {
         mi=(f)(broker, ctx);
         _SFCB_RETURN(mi);
      }   
      _SFCB_RETURN((CMPIPropertyMI*)(void*)-1);
   }

   if (broker)  {
      mi=(g)(broker,ctx, provider);
      _SFCB_RETURN(mi);
   }
   _SFCB_RETURN((CMPIPropertyMI*)(void*)-1);
};


typedef CMPIIndicationMI *(*GENERIC_IndicationMI) (CMPIBroker * broker,
                                                   CMPIContext * ctx,
                                                   const char *provider);
typedef CMPIIndicationMI *(*FIXED_IndicationMI) (CMPIBroker * broker,
                                                 CMPIContext * ctx);


CMPIIndicationMI *loadIndicationMI(const char *provider,
                                         void *library,
                                         CMPIBroker * broker, CMPIContext * ctx)
{
   CMPIIndicationMI *mi; 

   _SFCB_ENTER(TRACE_PROVIDERDRV, "loadIndicationMI");
   GENERIC_IndicationMI g =
       (GENERIC_IndicationMI) getGenericEntryPoint(library,
                                                        "Indication");
   if (g == NULL) {
      FIXED_IndicationMI f =
          (FIXED_IndicationMI) getFixedEntryPoint(provider, library,
                                                       "Indication");
      if (f == NULL) _SFCB_RETURN(NULL);
      if (broker) {
         mi=(f)(broker, ctx);
         _SFCB_RETURN(mi);
      }   
      _SFCB_RETURN((CMPIIndicationMI*)(void*)-1);
   }

   if (broker)  {
      mi=(g)(broker,ctx, provider);
      _SFCB_RETURN(mi);
   }
   _SFCB_RETURN((CMPIIndicationMI*)(void*)-1);
};



typedef CMPIClassMI *(*FIXED_ClassMI) (CMPIBroker * broker,
                                                 CMPIContext * ctx);

CMPIClassMI *loadClassMI(const char *provider,
                                         void *library,
                                         CMPIBroker * broker, CMPIContext * ctx)
{
   CMPIClassMI *mi; 

   _SFCB_ENTER(TRACE_PROVIDERDRV, "loadClassMI");
   FIXED_ClassMI f =
          (FIXED_ClassMI) getFixedEntryPoint(provider, library,
                                                       "Class");
   if (f == NULL) _SFCB_RETURN(NULL);
   
   if (broker)  {
      mi=(f)(broker,ctx);
      _SFCB_RETURN(mi);
   }
   _SFCB_RETURN((CMPIClassMI*)(void*)-1);
};

/****************************************************************************/



/**
 * Exits the program with an error message in case the given condition
 * holds.
 */
#define __ALLOC_ERROR(cond) \
  if ( cond ) { \
    error_at_line ( -1, errno, __FILE__, __LINE__, \
		    "unable to allocate requested memory." ); \
  }


/**
 * flag to ensure MM is initialized only once
 */
static int __once = 0;

/**
 * global key to get access to thread-specific memory management data
 */
static CMPI_THREAD_KEY_TYPE __mm_key;

void tool_mm_set_broker(void *broker, void *ctx);
void *tool_mm_get_broker(void **ctx);
void *memAddEncObj(int mode, void *ptr, size_t size, int *memId);
static int memAdd(void *ptr, int *memId);



void *tool_mm_load_lib(const char *libname)
{
   char filename[255];
   sprintf(filename, "lib%s.so", libname);
   return dlopen(filename, RTLD_LAZY);
}

static void __flush_mt(managed_thread * mt)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "__flush_mt");

   while (mt->hc.used) {
      --mt->hc.used;
      free(mt->hc.objs[mt->hc.used]);
      mt->hc.objs[mt->hc.used] = NULL;
   };

   while (mt->hc.encUsed) {
      --mt->hc.encUsed;
      mt->hc.encObjs[mt->hc.encUsed]->ft->release(mt->hc.encObjs[mt->hc.encUsed]);
      mt->hc.encObjs[mt->hc.encUsed] = NULL;
   };

   while (mt->hc.memUsed) {
      --mt->hc.memUsed;
      if (mt->hc.memObjs[mt->hc.memUsed]) free(mt->hc.memObjs[mt->hc.memUsed]);
      mt->hc.memObjs[mt->hc.memUsed] = NULL;
   };
 
   while (mt->hc.memEncUsed) {
      --mt->hc.memEncUsed;
      _SFCB_TRACE(1,("memEnc %d %d %p\n", currentProc,mt->hc.memEncUsed,mt->hc.memEncObjs[mt->hc.memEncUsed]))
      if (mt->hc.memEncObjs[mt->hc.memEncUsed]) {
         mt->hc.memEncObjs[mt->hc.memEncUsed]->ft->release(mt->hc.memEncObjs[mt->hc.memEncUsed]);
      }   
      mt->hc.memEncObjs[mt->hc.memEncUsed] = NULL;
   };
   _SFCB_EXIT();
}

/**
 * Cleans up a previously initialized thread once it dies/exits.
 */
 
static void __cleanup_mt(void *ptr)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "__cleanup_mt");
   managed_thread *mt = (managed_thread *) ptr;

   __flush_mt(mt);

   free(mt->hc.objs);
   free(mt->hc.encObjs);
   free(mt->hc.memObjs);
   free(mt->hc.memEncObjs);
   free(mt);
   _SFCB_EXIT();
}


/**
 * Initializes the current thread by adding it to the memory management sytem.
 */

static managed_thread *__init_mt()
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "managed_thread");
   managed_thread *mt = (managed_thread *) calloc(1, sizeof(managed_thread)+8);

   __ALLOC_ERROR(!mt);

   mt->hc.encSize = mt->hc.size = MT_SIZE_STEP;
   mt->hc.objs = (void **) malloc(MT_SIZE_STEP * sizeof(void *));
   mt->hc.encObjs = (Object **) malloc(MT_SIZE_STEP * sizeof(void *));
   
   mt->hc.memEncSize = mt->hc.memSize = MT_SIZE_STEP;
   mt->hc.memObjs = (void **) malloc(MT_SIZE_STEP * sizeof(void *));
   mt->hc.memEncObjs = (Object **) malloc(MT_SIZE_STEP * sizeof(void *));
   
   mt->data = NULL;

   CMPI_BrokerExt_Ftab->setThreadSpecific(__mm_key, mt);

   _SFCB_RETURN(mt);
}

/**
 * Initializes the memory mangement system.
 */
 
static void __init_mm()
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "__init_mm");
   CMPI_BrokerExt_Ftab->createThreadKey(&__mm_key, __cleanup_mt);
   _SFCB_EXIT();
}

static managed_thread *__memInit()
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "__memInit");
   managed_thread *mt;

   CMPI_BrokerExt_Ftab->threadOnce(&__once, __init_mm);

   mt = (managed_thread *)
       CMPI_BrokerExt_Ftab->getThreadSpecific(__mm_key);
   if (mt==NULL) mt=__init_mt();
   return mt;
}
/*
static void memInit(int newProc)
{
   __memInit();
}
*/


/**
 * Allocates zeroed memory and eventually puts it under memory mangement.
 *
 * Description:
 *
 *   Calls calloc to get the requested block size, then adds it to
 *   the control system depending on add, defined as MEM_TRACKED and
 *   MEM_NOT_TRACKED.
 */
void *tool_mm_alloc(int add, size_t size)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "tool_mm_alloc");
   void *result = calloc(1, size);
   if (!result) {
      _SFCB_TRACE(1,("--- tool_mm_alloc error %u %d\n", size, currentProc))
      SFCB_ASM("int $3");
      abort();
   }
   __ALLOC_ERROR(!result);

   if (add != MEM_NOT_TRACKED) {
      tool_mm_add(result);
   }
   _SFCB_TRACE(1, ("--- Area: %p size: %d", result, size));
   _SFCB_RETURN(result);
}

void *memAlloc(int add, size_t size, int *memId)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "mem_alloc");
   void *result = calloc(1, size);
   if (!result) {
      _SFCB_TRACE(1,("--- memAlloc %u %d\n", size, currentProc))
      SFCB_ASM("int $3");
      abort();
   }
   __ALLOC_ERROR(!result);

   if (add != MEM_TRACKED) {
      memAdd(result,memId);
   }
   _SFCB_TRACE(1, ("--- Area: %p size: %d", result, size));
   _SFCB_RETURN(result);
}


/**
 * Reallocates memory.
 *
 * Description:
 *
 *   Reallocates oldptr to the new size, then checks if the new and old
 *   pointer are equal. If not and the old one is successfully removed from
 *   the managed_thread, this means that the new one has to be added as well,
 *   before returning it as result.
 *
 *   The newly allocated memory is being returned as from the realloc()
 *   sys-call, no zeroing is performed as compared to tool_mm_alloc().
 */
void *tool_mm_realloc(void *oldptr, size_t size)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "tool_mm_realloc");
   void *new = realloc(oldptr, size);

   __ALLOC_ERROR(!new);

   if (oldptr != NULL && tool_mm_remove(oldptr)) {
      tool_mm_add(new);
   }

   _SFCB_RETURN(new);
}


/**
 * Adds ptr to the list of managed objects for the current thread.
 *
 * Description:
 *
 *   First checks if the current thread is already under memory management
 *   control, eventually adds it. Then checks if ptr is already stored, if
 *   not finally adds it. Additionally the array size for stored void
 *   pointers may have to be enlarged by MT_SIZE_STEP.
 */
int tool_mm_add(void *ptr)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "tool_mm_add");
   managed_thread *mt;

   CMPI_BrokerExt_Ftab->threadOnce(&__once, __init_mm);

   mt = (managed_thread *)
       CMPI_BrokerExt_Ftab->getThreadSpecific(__mm_key);

   if (mt == NULL) {
      mt = __init_mt();
   }

   mt->hc.objs[mt->hc.used++] = ptr;

   if (mt->hc.used == mt->hc.size) {
      mt->hc.size += MT_SIZE_STEP;
      mt->hc.objs = (void **) realloc(mt->hc.objs, mt->hc.size * sizeof(void *));

      __ALLOC_ERROR(!mt->hc.objs);
   }

   _SFCB_RETURN(1);
}


void *tool_mm_add_obj(int mode, void *ptr, size_t size)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "tool_mm_add_obj");
   managed_thread *mt;
   void *object;

   CMPI_BrokerExt_Ftab->threadOnce(&__once, __init_mm);

   mt = (managed_thread *)
       CMPI_BrokerExt_Ftab->getThreadSpecific(__mm_key);

   if (mt == NULL) {
      mt = __init_mt();
   }

   object = malloc(size);
   memcpy(object, ptr, size);

   if (mode == MEM_NOT_TRACKED)
      _SFCB_RETURN(object);

   mt->hc.encObjs[mt->hc.encUsed++] = (Object *) object;

   if (mt->hc.encUsed == mt->hc.encSize) {
      mt->hc.encSize += MT_SIZE_STEP;
      mt->hc.encObjs =
          (Object **) realloc(mt->hc.encObjs, mt->hc.encSize * sizeof(void *));
      __ALLOC_ERROR(!mt->hc.encObjs);
   }

   _SFCB_RETURN(object);
}

static int memAdd(void *ptr, int *memId)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "memAdd");
   managed_thread *mt=__memInit();

   mt->hc.memObjs[mt->hc.memUsed++] = ptr;
   *memId=mt->hc.memUsed;

   if (mt->hc.memUsed == mt->hc.memSize) {
      mt->hc.memSize += MT_SIZE_STEP;
      mt->hc.memObjs = (void **) realloc(mt->hc.memObjs, mt->hc.memSize * sizeof(void *));

      __ALLOC_ERROR(!mt->hc.memObjs);
   }

   _SFCB_RETURN(1);
}

void *memAddEncObj(int mode, void *ptr, size_t size, int *memId)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "memAddEncObj");
   managed_thread *mt=__memInit();

   void *object = malloc(size);
   memcpy(object, ptr, size);

   if (mode != MEM_TRACKED) {
      *memId=MEM_NOT_TRACKED;
      _SFCB_RETURN(object);
   }   

   mt->hc.memEncObjs[mt->hc.memEncUsed++] = (Object *) object;
   *memId=mt->hc.memEncUsed;
   
   if (mt->hc.memEncUsed == mt->hc.memEncSize) {
      mt->hc.memEncSize += MT_SIZE_STEP;
      mt->hc.memEncObjs =
          (Object **) realloc(mt->hc.memEncObjs, mt->hc.memEncSize * sizeof(void *));
      __ALLOC_ERROR(!mt->hc.memEncObjs);
   }

   _SFCB_RETURN(object); 
}

void memLinkEncObj(void *object, int *memId)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "memLinkEncObj");
   managed_thread *mt=__memInit();

   mt->hc.memEncObjs[mt->hc.memEncUsed++] = (Object *) object;
   *memId=mt->hc.memEncUsed;
   
   if (mt->hc.memEncUsed == mt->hc.memEncSize) {
      mt->hc.memEncSize += MT_SIZE_STEP;
      mt->hc.memEncObjs =
          (Object **) realloc(mt->hc.memEncObjs, mt->hc.memEncSize * sizeof(void *));
      __ALLOC_ERROR(!mt->hc.memEncObjs);
   }

   _SFCB_EXIT(); 
}

void memUnlinkEncObj(int memId)
{
   managed_thread *mt=__memInit();
   
   if (memId!=MEM_RELEASED && memId!=MEM_NOT_TRACKED)
      mt->hc.memEncObjs[memId-1] = NULL;
}



/**
 * Removes ptr from the list of managed objects for the current thread.
 *
 * Description:
 *
 *   The removal is achieved by replacing the stored pointer with NULL, once
 *   found, as this does not disturb a later free() call.
 */

int tool_mm_remove(void *ptr)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "tool_mm_get_broker");
   managed_thread *mt;

   CMPI_BrokerExt_Ftab->threadOnce(&__once, __init_mm);

   mt = (managed_thread *)
       CMPI_BrokerExt_Ftab->getThreadSpecific(__mm_key);

   if (mt != NULL) {
      int i = mt->hc.used;

      while (i--) {
         if (mt->hc.objs[i] == ptr) {
            mt->hc.objs[i] = NULL;
            return 1;
         }
      }
   }
   _SFCB_RETURN(0);
}

int tool_mm_remove_obj(void *ptr)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "tool_mm_remove_obj");
   managed_thread *mt;

   CMPI_BrokerExt_Ftab->threadOnce(&__once, __init_mm);

   mt = (managed_thread *)
       CMPI_BrokerExt_Ftab->getThreadSpecific(__mm_key);

   if (mt != NULL) {
      int i = mt->hc.encUsed;

      while (i--) {
         if (mt->hc.encObjs[i] == ptr) {
            mt->hc.encObjs[i] = NULL;
            return 1;
         }
      }
   }
   _SFCB_RETURN(0);
}


void tool_mm_flush()
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "tool_mm_flush");
   managed_thread *mt;

   CMPI_BrokerExt_Ftab->threadOnce(&__once, __init_mm);

   mt = (managed_thread *)
       CMPI_BrokerExt_Ftab->getThreadSpecific(__mm_key);

   if (mt != NULL) {
      __flush_mt(mt);
   }
   _SFCB_EXIT();
}




void tool_mm_set_broker(void *broker, void *ctx)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "tool_mm_set_broker");
   managed_thread *mt=__memInit();

   mt->broker = broker;
   mt->ctx = ctx;
   _SFCB_EXIT();
}

void *tool_mm_get_broker(void **ctx)
{
   _SFCB_ENTER(TRACE_MEMORYMGR, "tool_mm_get_broker");
   managed_thread *mt=__memInit();

   if (ctx)
      *ctx = mt->ctx;
   _SFCB_RETURN(mt->broker);
}


void *getThreadDataSlot()
{
   managed_thread *mt=__memInit();
   return &mt->data;
}

void *markHeap()
{
   managed_thread *mt;
   HeapControl *hc=(HeapControl*)calloc(1,sizeof(HeapControl)+8);
   _SFCB_ENTER(TRACE_MEMORYMGR, "markHeap");
   
   mt=__memInit();
   
   //*hc=mt->hc;
   memcpy(hc,&mt->hc,sizeof(HeapControl));
   
   mt->hc.encUsed = mt->hc.used = 0;
   mt->hc.encSize = mt->hc.size = MT_SIZE_STEP;
   mt->hc.objs = (void **) malloc(MT_SIZE_STEP * sizeof(void *));
   mt->hc.encObjs = (Object **) malloc(MT_SIZE_STEP * sizeof(void *));
   
   mt->hc.memEncUsed = mt->hc.memUsed = 0;
   mt->hc.memEncSize = mt->hc.memSize = MT_SIZE_STEP;
   mt->hc.memObjs = (void **) malloc(MT_SIZE_STEP * sizeof(void *));
   mt->hc.memEncObjs = (Object **) malloc(MT_SIZE_STEP * sizeof(void *));
   
   _SFCB_RETURN(hc);
}

void releaseHeap(void *hc)
{
   managed_thread *mt;
   mt = (managed_thread *) CMPI_BrokerExt_Ftab->getThreadSpecific(__mm_key);
   _SFCB_ENTER(TRACE_MEMORYMGR, "releaseHeap");
   
   mt=__memInit();
   
   __flush_mt(mt);
   
   if (mt->hc.objs) free(mt->hc.objs);
   if (mt->hc.encObjs) free(mt->hc.encObjs);
   if (mt->hc.memObjs) free(mt->hc.memObjs);
   if (mt->hc.memEncObjs) free(mt->hc.memEncObjs);
   
   memcpy(&mt->hc,hc,sizeof(HeapControl));
   
   free(hc);
   _SFCB_EXIT();
}



#include "utilft.h"

ProviderRegister *pReg = NULL;

int init_sfcBroker(char *home)
{
   pReg = UtilFactory->newProviderRegister(home);
   return 0;
}


//*********************************************************************
//*
//*     Copyright (c) 1999, Bob Withers - bwit@pobox.com
//*
//* This code may be freely used for any purpose, either personal
//* or commercial, provided the authors copyright notice remains
//* intact.
//*********************************************************************

static char cvt[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz0123456789+/";

char *encode64(char *data)
{
   int i, o = 0;
   char c;
   int len = strlen(data);
   char *ret = (char *) malloc(len * 2);

   for (i = 0; i < len; ++i) {
      c = (data[i] >> 2) & 0x3f;
      ret[o++] = cvt[(int) c];
      c = (data[i] << 4) & 0x3f;
      if (++i < len)
         c |= (data[i] >> 4) & 0x0f;

      ret[o++] = cvt[(int) c];
      if (i < len) {
         c = (data[i] << 2) & 0x3f;
         if (++i < len)
            c |= (data[i] >> 6) & 0x03;

         ret[o++] = cvt[(int) c];
      }
      else {
         ++i;
         ret[o++] = '=';
      }

      if (i < len) {
         c = data[i] & 0x3f;
         ret[o++] = cvt[(int) c];
      }
      else
         ret[o++] = '=';
   }
   ret[o] = 0;
   return (ret);
}

static int find(char *str, char c)
{
   char *p = strchr(str, c);
   return p - str;
}

char *decode64(char *din)
{
   unsigned char *data=(unsigned char*)din;
   int i, o = 0, len = strlen(data);
   unsigned char c, c1;
   unsigned char *ret = (unsigned char *) malloc(len * 2);

 for (i = 0; i < len; ++i) {
      c = (char) find(cvt, data[i]);
      ++i;
      c1 = (char) find(cvt, data[i]);
      c = (c << 2) | ((c1 >> 4) & 0x3);
      ret[o++] = c;
      if (++i < len) {
         c = data[i];
         if ('=' == c)
            break;
         c = (char) find(cvt, c);
         c1 = ((c1 << 4) & 0xf0) | ((c >> 2) & 0xf);
         ret[o++] = c1;
      }

      if (++i < len) {
         c1 = data[i];
         if ('=' == c1)
            break;
         c1 = (char) find(cvt, c1);
         c = ((c << 6) & 0xc0) | c1;
         ret[o++] = c;
      }
   }

   ret[o] = 0;
   return (ret);
}

void dump(char *msg, void *a, int len)
{
   unsigned char *b = (unsigned char *) a, *bb;
   int i, j, k, l;
   printf("(%p-%d) %s\n", a, len, msg);
   static char ht[] = "0123456789ABCDEF";

   for (bb = b, k = 0, j = 1, i = 0; i < len; i++, j++) {
      if (j == 1 && k == 0)
         printf("%p: ", b + i);
      printf("%c%c", ht[b[i] >> 4], ht[b[i] & 15]);
      if (j == 4) {
         j = 0;
         printf(" ");
         k++;
      }
      if (k == 8) {
         printf(" *");
         for (l = 0; l < 32; l++) {
            if (bb[l] >= ' ' && bb[l] <= 'z')
               printf("%c", bb[l]);
            else
               printf(".");
         }
         bb = &bb[32];
         k = 0;
         printf("*\n");
      }
   }
   printf("\n");
}


/* --------------------------------------------
 * ------
 * --    Sfcb control statement scanner support
 * ------
 * --------------------------------------------
 */


void cntlSkipws(char **p)
{
   while (**p && **p <= ' ' && **p != '\n')
      (*p)++;
}

int cntlParseStmt(char *in, CntlVals * rv)
{
   rv->type = 0;
   cntlSkipws(&in);
   if (*in == 0 || *in == '#' || *in == '\n') {
      rv->type = 3;
   }
   else if (*in == '[') {
      char *p = strpbrk(in + 1, "] \t\n");
      if (*p == ']') {
         rv->type = 1;
         *p = 0;
         rv->id = in + 1;
      }
   }
   else {
      char *p = strpbrk(in, ": \t\n");
      if (*p == ':') {
         rv->type = 2;
         *p = 0;
         rv->id = in;
         in = ++p;
         cntlSkipws(&in);
         rv->val = in;
      }
   }
   return rv->type;
}

char *cntlGetVal(CntlVals * rv)
{
   char *p, *v;
   if (rv->val == NULL)
      return NULL;
   cntlSkipws(&rv->val);
   v = rv->val;
   p = strpbrk(rv->val, " \t\n");
   if (p) {
      if (*p != '\n')
         rv->val = p + 1;
      else
         rv->val = NULL;
      *p = 0;
   }
   else rv->val = NULL;
   return v;
}

void dumpTiming(int pid)
{
   char buffer[4096];
   FILE *f;
   int l;
   
   if (collectStat==0) return;
   
   sprintf(buffer,"/proc/%d/stat",pid);
   f=fopen(buffer,"r");
   l=fread(buffer,1,4095,f);
   fclose(f);   
   buffer[l]=0;
   f=fopen("sfcbStat","a");
   fprintf(f,"%s %s",processName,buffer);
   fclose(f);
}

void setStatus(CMPIStatus *st, CMPIrc rc, char *msg)
{
   st->rc=rc;
   if (rc!=0 && msg) st->msg=native_new_CMPIString(msg,NULL);
   else st->msg=NULL;
}   

void showStatus(CMPIStatus *st, char *msg)
{
   char *m=NULL;
   if (st->msg) m=(char*)st->msg->hdl;
   mlogf(M_INFO,M_SHOW,"--- showStatus (%s): %d %s\n",msg,st->rc,m);
}   

