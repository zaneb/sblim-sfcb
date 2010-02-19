
/*
 * indCIMXMLHandler.c
 *
 * © Copyright IBM Corp. 2005, 2007
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:        Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * CIM XML handler for indications.
 *
*/



#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "trace.h"
#include "fileRepository.h"
#include "providerMgr.h"
#include "internalProvider.h" 
#include "cimXmlRequest.h" 
#include "native.h"

extern void closeProviderContext(BinRequestContext* ctx);
extern int exportIndication(char *url, char *payload, char **resp, char **msg);
extern void dumpSegments(void*);
extern UtilStringBuffer *segments2stringBuffer(RespSegment *rs);
extern UtilStringBuffer *newStringBuffer(int);
extern void setStatus(CMPIStatus *st, CMPIrc rc, char *msg);

extern ExpSegments exportIndicationReq(CMPIInstance *ci, char *id);

static const CMPIBroker *_broker;

static int interOpNameSpace(const CMPIObjectPath *cop, CMPIStatus *st) 
 {   
   char *ns=(char*)CMGetNameSpace(cop,NULL)->hdl;   
   if (strcasecmp(ns,"root/interop") && strcasecmp(ns,"root/pg_interop")) {
      setStatus(st,CMPI_RC_ERR_FAILED,"Object must reside in root/interop");
      return 0;
   }
   return 1;
}
   
/* ------------------------------------------------------------------ *
 * Instance MI Cleanup
 * ------------------------------------------------------------------ */

CMPIStatus IndCIMXMLHandlerCleanup(CMPIInstanceMI * mi, const CMPIContext * ctx, CMPIBoolean terminating)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "IndCIMXMLHandlerCleanup");
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------ *
 * Instance MI Functions
 * ------------------------------------------------------------------ */

static CMPIContext* prepareUpcall(CMPIContext *ctx)
{
    /* used to invoke the internal provider in upcalls, otherwise we will
 *      * be routed here (interOpProvider) again*/
    CMPIContext *ctxLocal;
    ctxLocal = native_clone_CMPIContext(ctx);
    CMPIValue val;
    val.string = sfcb_native_new_CMPIString("$DefaultProvider$", NULL,0);
    ctxLocal->ft->addEntry(ctxLocal, "rerouteToProvider", &val, CMPI_string);
    return ctxLocal;
}


CMPIStatus IndCIMXMLHandlerEnumInstanceNames(CMPIInstanceMI * mi,
                                             const CMPIContext * ctx,
                                             const CMPIResult * rslt,
                                             const CMPIObjectPath * ref)
{
   CMPIStatus st;
   CMPIEnumeration *enm;
   CMPIContext *ctxLocal;

   _SFCB_ENTER(TRACE_INDPROVIDER, "IndCIMXMLHandlerEnumInstanceNames");
   if (interOpNameSpace(ref,NULL)!=1) _SFCB_RETURN(st);
   ctxLocal = prepareUpcall((CMPIContext *)ctx);

#ifdef HAVE_OPTIMIZED_ENUMERATION
   CMPIString* cn;
   CMPIObjectPath* refLocal;
   cn = CMGetClassName(ref, &st);

   if (strcasecmp(CMGetCharPtr(cn), "cim_listenerdestination") == 0) {
     enm = _broker->bft->enumerateInstanceNames(_broker, ctxLocal, ref, &st);
     while(enm && enm->ft->hasNext(enm, &st)) {
       CMReturnObjectPath(rslt, (enm->ft->getNext(enm, &st)).value.ref);
     }
     refLocal = CMNewObjectPath(_broker,"root/interop","cim_listenerdestinationcimxml",&st);
     enm = _broker->bft->enumerateInstanceNames(_broker, ctxLocal, refLocal, &st);
     while(enm && enm->ft->hasNext(enm, &st)) {
       CMReturnObjectPath(rslt, (enm->ft->getNext(enm, &st)).value.ref);
     }
     refLocal = CMNewObjectPath(_broker,"root/interop","cim_indicationhandlercimxml",&st);
     enm = _broker->bft->enumerateInstanceNames(_broker, ctxLocal, refLocal, &st);
     while(enm && enm->ft->hasNext(enm, &st)) {
       CMReturnObjectPath(rslt, (enm->ft->getNext(enm, &st)).value.ref);
     }
     CMRelease(refLocal);
   }
   else {
     enm = _broker->bft->enumerateInstanceNames(_broker, ctxLocal, ref, &st);
     while(enm && enm->ft->hasNext(enm, &st)) {
       CMReturnObjectPath(rslt, (enm->ft->getNext(enm, &st)).value.ref);
     }
   }
#else
   enm = _broker->bft->enumerateInstanceNames(_broker, ctxLocal, ref, &st);

   while(enm && enm->ft->hasNext(enm, &st)) {
       CMReturnObjectPath(rslt, (enm->ft->getNext(enm, &st)).value.ref);
   }
#endif

   CMRelease(ctxLocal);
   if(enm) CMRelease(enm);

   _SFCB_RETURN(st);
}

CMPIStatus IndCIMXMLHandlerEnumInstances(CMPIInstanceMI * mi,
                                         const CMPIContext * ctx,
                                         const CMPIResult * rslt,
                                         const CMPIObjectPath * ref,
                                         const char **properties)
{
   CMPIStatus st;
   CMPIEnumeration *enm;
   CMPIContext *ctxLocal;

   _SFCB_ENTER(TRACE_INDPROVIDER, "IndCIMXMLHandlerEnumInstances");
   if (interOpNameSpace(ref,NULL)!=1) _SFCB_RETURN(st);
   ctxLocal = prepareUpcall((CMPIContext *)ctx);

#ifdef HAVE_OPTIMIZED_ENUMERATION
   CMPIString* cn;
   CMPIObjectPath* refLocal;
   cn = CMGetClassName(ref, &st);

   if (strcasecmp(CMGetCharPtr(cn), "cim_listenerdestination") == 0) {
     enm = _broker->bft->enumerateInstances(_broker, ctxLocal, ref, properties, &st);
     while(enm && enm->ft->hasNext(enm, &st)) {
       CMReturnInstance(rslt, (enm->ft->getNext(enm, &st)).value.inst);
     }
     refLocal = CMNewObjectPath(_broker,"root/interop","cim_listenerdestinationcimxml",&st);
     enm = _broker->bft->enumerateInstances(_broker, ctxLocal, refLocal, properties, &st);
     while(enm && enm->ft->hasNext(enm, &st)) {
       CMReturnInstance(rslt, (enm->ft->getNext(enm, &st)).value.inst);
     }
     refLocal = CMNewObjectPath(_broker,"root/interop","cim_indicationhandlercimxml",&st);
     enm = _broker->bft->enumerateInstances(_broker, ctxLocal, refLocal, properties, &st);
     while(enm && enm->ft->hasNext(enm, &st)) {
       CMReturnInstance(rslt, (enm->ft->getNext(enm, &st)).value.inst);
     }
     CMRelease(refLocal);
   }
   else {
     enm = _broker->bft->enumerateInstances(_broker, ctxLocal, ref, properties, &st);
     while(enm && enm->ft->hasNext(enm, &st)) {
       CMReturnInstance(rslt, (enm->ft->getNext(enm, &st)).value.inst);
     }
   }
#else
   enm = _broker->bft->enumerateInstances(_broker, ctxLocal, ref, properties, &st);
   while(enm && enm->ft->hasNext(enm, &st)) {
       CMReturnInstance(rslt, (enm->ft->getNext(enm, &st)).value.inst);
   }
#endif

   CMRelease(ctxLocal);
   if(enm) CMRelease(enm);

   _SFCB_RETURN(st);
}


CMPIStatus IndCIMXMLHandlerGetInstance(CMPIInstanceMI * mi,
                                       const CMPIContext * ctx,
                                       const CMPIResult * rslt,
                                       const CMPIObjectPath * cop,
                                       const char **properties)
{
   CMPIStatus st;
   _SFCB_ENTER(TRACE_INDPROVIDER, "IndCIMXMLHandlerGetInstance");
   st=InternalProviderGetInstance(NULL,ctx,rslt,cop,properties);
   _SFCB_RETURN(st);
}

CMPIStatus IndCIMXMLHandlerCreateInstance(CMPIInstanceMI * mi,
                                          const CMPIContext * ctx,
                                          const CMPIResult * rslt,
                                          const CMPIObjectPath * cop,
                                          const CMPIInstance * ci)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIArgs *in,*out=NULL;
   CMPIObjectPath *op;
   CMPIData rv;
   unsigned short persistenceType;
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "IndCIMXMLHandlerCreateInstance");
   
   if (interOpNameSpace(cop,&st)==0) _SFCB_RETURN(st);

   internalProviderGetInstance(cop,&st);
   if (st.rc==CMPI_RC_ERR_FAILED) _SFCB_RETURN(st);
   if (st.rc==CMPI_RC_OK) {
      setStatus(&st,CMPI_RC_ERR_ALREADY_EXISTS,NULL);
      _SFCB_RETURN(st); 
   }

   CMPIInstance* ciLocal = CMClone(ci, NULL);

   CMPIString* dest = CMGetProperty(ciLocal, "destination", &st).value.string;
   if (dest == NULL || CMGetCharPtr(dest) == NULL) {
     setStatus(&st,CMPI_RC_ERR_FAILED,"Destination property not found; is required");
     _SFCB_RETURN(st);              
   }
   else { /* if no scheme is given, assume http (as req. for param by mof) */
     char* ds = CMGetCharPtr(dest);
     if (strchr(ds, ':') == NULL) {
       char* prefix = "http:";
       int n = strlen(ds)+strlen(prefix)+1;
       char* newdest = (char*)malloc(n*sizeof(char));
       strcpy(newdest, prefix);
       strcat(newdest, ds);
       CMSetProperty(ciLocal, "destination", newdest, CMPI_chars);
       free(newdest);
     }
   }

   CMPIData persistence = CMGetProperty(ciLocal, "persistencetype", &st);
   if (persistence.state == CMPI_nullValue || persistence.state == CMPI_notFound) {
     persistenceType = 2;  /* default is 2 = permanent */
   }
   else if (persistence.value.uint16 < 1 || persistence.value.uint16 > 3) {
     setStatus(&st,CMPI_RC_ERR_FAILED,"PersistenceType property must be 1, 2, or 3");
     _SFCB_RETURN(st);              
   }
   else {
     persistenceType = persistence.value.uint16;
   }
   CMSetProperty(ciLocal, "persistencetype", &persistenceType, CMPI_uint16);

            CMPIString *str=CDToString(_broker,cop,NULL);
            CMPIString *ns=CMGetNameSpace(cop,NULL);
            _SFCB_TRACE(1,("--- handler %s %s",(char*)ns->hdl,(char*)str->hdl));
            
   in=CMNewArgs(_broker,NULL);
   CMAddArg(in,"handler",&ciLocal,CMPI_instance);
   CMAddArg(in,"key",&cop,CMPI_ref);
   op=CMNewObjectPath(_broker,"root/interop","cim_indicationsubscription",&st);
   rv=CBInvokeMethod(_broker,ctx,op,"_addHandler",in,out,&st);

   if (st.rc==CMPI_RC_OK) 
      st=InternalProviderCreateInstance(NULL,ctx,rslt,cop,ciLocal);

   CMRelease(ciLocal);
   _SFCB_RETURN(st);
}

CMPIStatus IndCIMXMLHandlerModifyInstance(CMPIInstanceMI * mi,
					  const CMPIContext * ctx,
					  const CMPIResult * rslt,
					  const CMPIObjectPath * cop,
					  const CMPIInstance * ci, 
					  const char **properties)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "IndCIMXMLHandlerSetInstance");   
   _SFCB_RETURN(st);
}

CMPIStatus IndCIMXMLHandlerDeleteInstance(CMPIInstanceMI * mi,
                                          const CMPIContext * ctx,
                                          const CMPIResult * rslt,
                                          const CMPIObjectPath * cop)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIArgs *in,*out=NULL;
   CMPIObjectPath *op;
   CMPIData rv;
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "IndCIMXMLHandlerDeleteInstance");  
    
   if (interOpNameSpace(cop,&st)==0) _SFCB_RETURN(st);
   
   internalProviderGetInstance(cop,&st);
   if (st.rc) _SFCB_RETURN(st);
   
   in=CMNewArgs(_broker,NULL);
   CMAddArg(in,"key",&cop,CMPI_ref);
   op=CMNewObjectPath(_broker,"root/interop","cim_indicationsubscription",&st);
   rv=CBInvokeMethod(_broker,ctx,op,"_removeHandler",in,out,&st);

   if (st.rc==CMPI_RC_OK) {
      st=InternalProviderDeleteInstance(NULL,ctx,rslt,cop);
   }
   
   _SFCB_RETURN(st);
}

CMPIStatus IndCIMXMLHandlerExecQuery(CMPIInstanceMI * mi,
                                     const CMPIContext * ctx,
                                     const CMPIResult * rslt,
                                     const CMPIObjectPath * cop,
                                     const char *lang, const char *query)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "IndCIMXMLHandlerExecQuery");   
   _SFCB_RETURN(st);
}


/* ---------------------------------------------------------------------------*/
/*                        Method Provider Interface                           */
/* ---------------------------------------------------------------------------*/

CMPIStatus IndCIMXMLHandlerMethodCleanup(CMPIMethodMI * mi, 
					 const CMPIContext * ctx, 
					 CMPIBoolean terminating)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "IndCIMXMLHandlerMethodCleanup");   
   _SFCB_RETURN(st);
}


/** \brief deliverInd - Sends the indication to the destination
 *
 *  Performs the actual delivery of the indication payload to
 *  the target destination
 */

CMPIStatus deliverInd(
                const CMPIObjectPath * ref,
			    const CMPIArgs * in)
{
  _SFCB_ENTER(TRACE_INDPROVIDER, "deliverInd");  
  CMPIInstance *hci,*ind;
  CMPIStatus st = { CMPI_RC_OK, NULL };
  CMPIString *dest;
  char strId[64];
  ExpSegments xs;
  UtilStringBuffer *sb;
  static int id=1;
  char *resp;
  char *msg;

  if ((hci=internalProviderGetInstance(ref,&st))==NULL) {
     setStatus(&st,CMPI_RC_ERR_NOT_FOUND,NULL);
     _SFCB_RETURN(st); 
  }
  dest=CMGetProperty(hci,"destination",NULL).value.string;
  _SFCB_TRACE(1,("--- destination: %s\n",(char*)dest->hdl));
  ind=CMGetArg(in,"indication",NULL).value.inst;

  sprintf(strId,"%d",id++);
  xs=exportIndicationReq(ind,strId);
  sb=segments2stringBuffer(xs.segments);
  if (exportIndication((char*)dest->hdl,(char*)sb->ft->getCharPtr(sb), &resp, &msg)) {
     // change rc
     setStatus(&st,CMPI_RC_ERR_FAILED,NULL);
  }
  RespSegment rs = xs.segments[5];
  UtilStringBuffer* usb = (UtilStringBuffer*)rs.txt;
  CMRelease(usb);
  CMRelease(sb);
  if (resp) free(resp);
  if (msg) free(msg);
  _SFCB_RETURN(st); 
}

// Retry queue element and control vars
typedef struct rtelement {
   CMPIObjectPath * ref;
   CMPIArgs * in;
   CMPIInstance * sub;
   int count;
   time_t lasttry;
   struct rtelement  *next,*prev;
} RTElement;
static RTElement *RQhead,*RQtail;
static int retryRunning=0;
static pthread_mutex_t RQlock=PTHREAD_MUTEX_INITIALIZER;

/** \brief enqRetry - Add to retry queue
 *
 *  Adds the element to the retry queue
 *  Initializes the queue if empty 
 *  Adds the current time as the last retry time.
 */

int enqRetry (RTElement * element)
{
    struct timeval tv;
    struct timezone tz;

    // Put this one on the retry queue
    if (pthread_mutex_lock(&RQlock)!=0)  {
        //lock failed
        return 1;
    }
    if (RQhead==NULL) {
        // Queue is empty
        RQhead=element;
        RQtail=element;
        RQtail->next=element;
        RQtail->prev=element;
    } else {
        element->next=RQtail->next;
        element->next->prev=element;
        RQtail->next=element;
        element->prev=RQtail;
        RQtail=element;
    }
    RQtail->count=0;
    gettimeofday(&tv, &tz);
    RQtail->lasttry=tv.tv_sec;
    if (pthread_mutex_unlock(&RQlock)!=0)  {
        //lock failed
        return 1;
    }
    return(0);
}

/** \brief dqRetry - Remove from the retry queue
 *
 *  Removes the element from the retry queue
 *  Cleans up the queue if empty
 */

int dqRetry (RTElement * cur)
{
    // Remove the entry from the queue, closing the hole
    if (cur->next == cur) {
        // queue is empty
        free(cur);
        RQhead=NULL;
    } else {
        //not last
        cur->prev->next=cur->next;
        cur->next->prev=cur->prev;
        CMRelease(cur->ref);
        CMRelease(cur->in);
        CMRelease(cur->sub);
        if (cur) free(cur);
    }
    return(0);
}

/** \brief retryExport - Manages retries
 *
 *  Spawned as a thread when a retry queue exists to
 *  manage the retry attempts at the specified intervals
 *  Thread quits when queue is empty.
 */

void * retryExport (void * lctx)
{
    CMPIObjectPath *ref;
    CMPIArgs * in;
    CMPIInstance *sub;
    CMPIContext *ctx=(CMPIContext *)lctx;
    RTElement *cur,*purge;
    struct timeval tv;
    struct timezone tz;
    int rint,maxcount,ract,rtint;
    CMPIUint64 sfc=0;
    CMPIObjectPath *op;
    CMPIEnumeration *isenm = NULL;

    CMPIStatus st = { CMPI_RC_OK, NULL };

    // Get the retry params from IndService
    op=CMNewObjectPath(_broker,"root/interop","CIM_IndicationService",NULL);
    isenm = _broker->bft->enumerateInstances(_broker, ctx, op, NULL, &st);
    CMPIData isinst=CMGetNext(isenm,NULL);
    CMPIData mc=CMGetProperty(isinst.value.inst,"DeliveryRetryAttempts",NULL);
    CMPIData ri=CMGetProperty(isinst.value.inst,"DeliveryRetryInterval",NULL);
    CMPIData ra=CMGetProperty(isinst.value.inst,"SubscriptionRemovalAction",NULL);
    CMPIData rti=CMGetProperty(isinst.value.inst,"SubscriptionRemovalTimeInterval",NULL);
    maxcount= mc.value.uint16;  // Number of times to retry delivery
    rint=ri.value.uint32;       // Interval between retries
    rtint=rti.value.uint32;     // Time to allow sub to keep failing until ...
    ract= ra.value.uint16;      // ... this action is taken

    // Now, run the queue
    pthread_mutex_lock(&RQlock);
    cur=RQhead;
    while (RQhead != NULL ) {
        ref=cur->ref;
        in=cur->in;
        sub=cur->sub;
        CMPIObjectPath *subop=sub->ft->getObjectPath(sub,&st);
        if (st.rc == CMPI_RC_ERR_NOT_FOUND ) {
            //sub got deleted, purge this indication and move on
            purge=cur;
            cur=cur->next;
            dqRetry(purge);
        } else {
            //Still valid, retry
            gettimeofday(&tv, &tz);
            if ((cur->lasttry+rint) > tv.tv_sec) { 
                // no retries are ready, release the lock
                // and sleep for an interval, then relock
                pthread_mutex_unlock(&RQlock);
                sleep(rint);
                pthread_mutex_lock(&RQlock);
            }
            st=deliverInd(ref,in);
            if ( (st.rc == 0) || (cur->count >= maxcount-1) ){
                // either it worked, or we maxed out on retries
                // If it succeeded, clear the failtime
                if (st.rc == 0) {
                    sfc=0;
                    CMSetProperty(sub,"DeliveryFailureTime",&sfc,CMPI_uint64);
                    CBModifyInstance(_broker, ctx, subop, sub, NULL);
                }
                // remove from queue in either case
                purge=cur;
                cur=cur->next;
                dqRetry(purge);
            } else {
                // still failing, leave on queue 
                cur->count++;
                gettimeofday(&tv, &tz);
                cur->lasttry=tv.tv_sec; 
                CMPIData sfcp=CMGetProperty(sub,"DeliveryFailureTime",NULL);
                sfc=sfcp.value.uint64;
                if (sfc == 0 ) {
                    // if the time isn't set, this is the first failure
                    sfc=tv.tv_sec;
                    cur=cur->next;
                    CMSetProperty(sub,"DeliveryFailureTime",&sfc,CMPI_uint64);
                    CBModifyInstance(_broker, ctx, subop, sub, NULL);
                } else if (sfc+rtint < tv.tv_sec) {
                    // Exceeded subscription removal threshold, if action is:
                    // 2, delete the sub; 3, disable the sub; otherwise, nothing
                    if (ract == 2 ) {
                        CBDeleteInstance(_broker, ctx, subop);
                        purge=cur;
                        cur=cur->next;
                        dqRetry(purge);
                    } else if (ract == 3 ) {
                        //Set sub state to disable(4)
                        CMPIUint16 sst=4;
                        CMSetProperty(sub,"SubscriptionState",&sst,CMPI_uint16);
                        CBModifyInstance(_broker, ctx, subop, sub, NULL);
                        purge=cur;
                        cur=cur->next;
                        dqRetry(purge);
                    }
                } else {
                    cur=cur->next;
                }
            }
        }
    }
    // Queue went dry, cleanup and exit
    pthread_mutex_unlock(&RQlock);
    retryRunning=0;
    ctx->ft->release(ctx);
    return(NULL);
}

CMPIStatus IndCIMXMLHandlerInvokeMethod(CMPIMethodMI * mi,
					const CMPIContext * ctx,
					const CMPIResult * rslt,
					const CMPIObjectPath * ref,
					const char *methodName,
					const CMPIArgs * in, CMPIArgs * out)
{ 
   CMPIStatus st = { CMPI_RC_OK, NULL };
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "IndCIMXMLHandlerInvokeMethod");  
    
   if (interOpNameSpace(ref,&st)==0) _SFCB_RETURN(st);
   
   if (strcasecmp(methodName, "_deliver") == 0) {     
      st=deliverInd(ref,in);
      if (st.rc != 0) {
        // Get the retry params from IndService
        CMPIObjectPath *op=CMNewObjectPath(_broker,"root/interop","CIM_IndicationService",NULL);
        CMPIEnumeration *isenm = _broker->bft->enumerateInstances(_broker, ctx, op, NULL, NULL);
        CMPIData isinst=CMGetNext(isenm,NULL);
        CMPIData mc=CMGetProperty(isinst.value.inst,"DeliveryRetryAttempts",NULL);
        if (mc.value.uint16 > 0) {
            // Indication delivery failed, send to retry queue
            // build an element
            RTElement *element;
            element = (RTElement *) malloc(sizeof(*element));
            element->ref=ref->ft->clone(ref,&st);
            element->in=in->ft->clone(in,&st);
            CMPIInstance *ind=CMGetArg(in,"subscription",NULL).value.inst;
            element->sub=ind->ft->clone(ind,&st);
            // Add it to the retry queue
            enqRetry(element);
            // And launch the thread if it isn't already running
            pthread_t t;
            pthread_attr_t tattr;
            pthread_attr_init(&tattr);
            pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
            if (retryRunning == 0) {
                CMPIContext * pctx = native_clone_CMPIContext(ctx);
                pthread_create(&t, &tattr,&retryExport,(void *) pctx);
                retryRunning=1;
            }
        }
      }
   }
   
   else {
      printf("--- ClassProvider: Invalid request %s\n", methodName);
      st.rc = CMPI_RC_ERR_METHOD_NOT_FOUND;
   }

   return st;
   _SFCB_RETURN(st);
}




CMInstanceMIStub(IndCIMXMLHandler, IndCIMXMLHandler, _broker, CMNoHook);
CMMethodMIStub(IndCIMXMLHandler, IndCIMXMLHandler, _broker, CMNoHook);

