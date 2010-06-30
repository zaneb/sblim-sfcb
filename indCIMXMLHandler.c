
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
     CMRelease(ciLocal);
     _SFCB_RETURN(st);              
   }
   else { /* if no scheme is given, assume http (as req. for param by mof) */
     char* ds = CMGetCharPtr(dest);
     if (strstr(ds, "://") == NULL) {
       char* prefix = "http://";
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
     CMRelease(ciLocal);
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

  dest=CMGetProperty(hci,"destination",&st).value.string;
  _SFCB_TRACE(1,("--- destination: %s\n",(char*)dest->hdl));
  ind=CMGetArg(in,"indication",NULL).value.inst;

  sprintf(strId,"%d",id++);
  xs=exportIndicationReq(ind,strId);
  sb=segments2stringBuffer(xs.segments);
  if (exportIndication((char*)dest->hdl,(char*)sb->ft->getCharPtr(sb), &resp, &msg)) {
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
   CMPIObjectPath * ref; // LD
   CMPIObjectPath * sub;  // Subscription
   CMPIObjectPath * ind;  // indication with key
   CMPIObjectPath * SFCBIndEle;  // SFCB_indicationelement
   int count;
   time_t lasttry;
   unsigned int instanceID;
   struct rtelement  *next,*prev;
} RTElement;
static RTElement *RQhead,*RQtail;
static int retryRunning=0;
static pthread_mutex_t RQlock=PTHREAD_MUTEX_INITIALIZER;
pthread_t t;
pthread_attr_t tattr;

/** \brief enqRetry - Add to retry queue
 *
 *  Adds the element to the retry queue
 *  Initializes the queue if empty 
 *  Adds the current time as the last retry time.
 */

int enqRetry (RTElement * element, const CMPIContext * ctx, int repo)
{

    _SFCB_ENTER(TRACE_INDPROVIDER, "enqRetry");  
    // Put this one on the retry queue
    if (pthread_mutex_lock(&RQlock)!=0)  {
        //lock failed
        return 1;
    }
    if (RQhead==NULL) {
        // Queue is empty
        _SFCB_TRACE(1,("--- Adding indication to new retry queue."));
        RQhead=element;
        RQtail=element;
        RQtail->next=element;
        RQtail->prev=element;
    } else {
        _SFCB_TRACE(1,("--- Adding indication to retry queue."));
        element->next=RQtail->next;
        element->next->prev=element;
        RQtail->next=element;
        element->prev=RQtail;
        RQtail=element;
    }

    if (repo==1) {
        // If this needs to be persisted in the repo 
        // (not the initial fill from refillRetryQ)
        _SFCB_TRACE(1,("--- Creating SFCB_IndicationElement instance."));
        CMPIObjectPath * op=CMNewObjectPath(_broker,"root/interop","SFCB_IndicationElement",NULL);
        // Add the indID as the only key
        CMAddKey(op,"IndicationID",&element->instanceID,CMPI_uint32);
        // Create the instance
        //element->SFCBIndEle=op;
        element->SFCBIndEle=op->ft->clone(op,NULL);
        CMPIInstance * ci=CMNewInstance(_broker,op,NULL);
        // Set all the properties
        CMSetProperty(ci,"IndicationID",&element->instanceID,CMPI_uint32);
        CMSetProperty(ci,"RetryCount",&(RQtail->count),CMPI_uint32);
        CMSetProperty(ci,"LastDelivery",&(RQtail->lasttry),CMPI_sint32);
        CMSetProperty(ci,"ld",&(element->ref),CMPI_ref);
        CMSetProperty(ci,"ind",&element->ind,CMPI_ref);
        CMSetProperty(ci,"sub",&element->sub,CMPI_ref);
        CBCreateInstance(_broker, ctx, op, ci, NULL);
        CMRelease(op);
        CMRelease(ci);
    }

    if (pthread_mutex_unlock(&RQlock)!=0)  {
        //lock failed
        return 1;
    }
    _SFCB_RETURN(0); 
}

/** \brief dqRetry - Remove from the retry queue
 *
 *  Removes the element from the retry queue
 *  Cleans up the queue if empty
 */

int dqRetry (CMPIContext * ctx,RTElement * cur)
{
    _SFCB_ENTER(TRACE_INDPROVIDER, "dqRetry");  
    // Delete the instance in the repo
    CMPIObjectPath * op=CMNewObjectPath(_broker,"root/interop","SFCB_IndicationElement",NULL);
    CMAddKey(op,"IndicationID",&cur->instanceID,CMPI_uint32);
    CBDeleteInstance(_broker,ctx,op); 
    CBDeleteInstance(_broker,ctx,cur->ind); 
    CMRelease(op);
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
        CMRelease(cur->sub);
        if (cur) free(cur);
    }
    _SFCB_RETURN(0); 
}


/** \brief retryExport - Manages retries
 *
 *  Spawned as a thread when a retry queue exists to
 *  manage the retry attempts at the specified intervals
 *  Thread quits when queue is empty.
 */

void * retryExport (void * lctx)
{
    _SFCB_ENTER(TRACE_INDPROVIDER, "retryExport");  

    CMPIObjectPath *ref;
    CMPIArgs * in;
    CMPIInstance *sub;
    CMPIContext *ctx=(CMPIContext *)lctx;
    CMPIContext *ctxLocal;
    RTElement *cur,*purge;
    struct timeval tv;
    struct timezone tz;
    int rint,maxcount,ract,rtint;
    CMPIUint64 sfc=0;
    CMPIObjectPath *op;
    CMPIEnumeration *isenm = NULL;

    CMPIStatus st = { CMPI_RC_OK, NULL };
    ctxLocal = prepareUpcall(ctx);

    // Get the retry params from IndService
    op=CMNewObjectPath(_broker,"root/interop","CIM_IndicationService",NULL);
    isenm = _broker->bft->enumerateInstances(_broker, ctx, op, NULL, NULL);
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
    sleep(5); //Prevent deadlock on startup when localmode is used.
    pthread_mutex_lock(&RQlock);
    cur=RQhead;
    while (RQhead != NULL ) {
        ref=cur->ref;
        // Build the CMPIArgs that deliverInd needs
        CMPIInstance *iinst=internalProviderGetInstance(cur->ind,&st);
        if (st.rc != 0 ) {
            mlogf(M_ERROR,M_SHOW,"Failed to retrieve indication instance from repository, rc:%d\n",st.rc);
            purge=cur;
            cur=cur->next;
            dqRetry(ctx,purge);
            continue;
        }
        in=CMNewArgs(_broker,NULL);
        CMAddArg(in,"indication",&iinst,CMPI_instance);
        sub=internalProviderGetInstance(cur->sub,&st);
        if (st.rc == CMPI_RC_ERR_NOT_FOUND ) {
            //sub got deleted, purge this indication and move on
            _SFCB_TRACE(1,("--- Subscription for indication gone, deleting indication."));
            purge=cur;
            cur=cur->next;
            dqRetry(ctx,purge);
        } else {
            //Still valid, retry
            gettimeofday(&tv, &tz);
            if ((cur->lasttry+rint) > tv.tv_sec) { 
                _SFCB_TRACE(1,("--- sleeping."));
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
                    _SFCB_TRACE(1,("--- Indication succeeded."));
                    sfc=0;
                    CMSetProperty(sub,"DeliveryFailureTime",&sfc,CMPI_uint64);
                    CBModifyInstance(_broker, ctxLocal, cur->sub, sub, NULL);
                }
                // remove from queue in either case
                _SFCB_TRACE(1,("--- Indication removed."));
                purge=cur;
                cur=cur->next;
                dqRetry(ctx,purge);
            } else {
                // still failing, leave on queue 
                _SFCB_TRACE(1,("--- Indication still failing."));
                cur->count++;
                gettimeofday(&tv, &tz);
                cur->lasttry=tv.tv_sec; 

                CMPIInstance * indele=internalProviderGetInstance(cur->SFCBIndEle,&st);
                CMSetProperty(indele,"LastDelivery",&cur->lasttry,CMPI_sint32);
                CMSetProperty(indele,"RetryCount",&cur->count,CMPI_uint32);
                CBModifyInstance(_broker, ctxLocal, cur->SFCBIndEle, indele, NULL);

                CMPIData sfcp=CMGetProperty(sub,"DeliveryFailureTime",NULL);
                sfc=sfcp.value.uint64;
                if (sfc == 0 ) {
                    // if the time isn't set, this is the first failure
                    sfc=tv.tv_sec;
                    cur=cur->next;
                    CMSetProperty(sub,"DeliveryFailureTime",&sfc,CMPI_uint64);
                    CBModifyInstance(_broker, ctxLocal, cur->sub, sub, NULL);
                } else if (sfc+rtint < tv.tv_sec) {
                    // Exceeded subscription removal threshold, if action is:
                    // 2, delete the sub; 3, disable the sub; otherwise, nothing
                    if (ract == 2 ) {
                        _SFCB_TRACE(1,("--- Subscription threshold reached, deleting."));
                        CBDeleteInstance(_broker, ctx, cur->sub);
                        purge=cur;
                        cur=cur->next;
                        dqRetry(ctx,purge);
                    } else if (ract == 3 ) {
                        //Set sub state to disable(4)
                        _SFCB_TRACE(1,("--- Subscription threshold reached, disable."));
                        CMPIUint16 sst=4;
                        CMSetProperty(sub,"SubscriptionState",&sst,CMPI_uint16);
                        CBModifyInstance(_broker, ctx, cur->sub, sub, NULL);
                        purge=cur;
                        cur=cur->next;
                        dqRetry(ctx,purge);
                    }
                } else {
                    cur=cur->next;
                }
            }
        }
    }
    // Queue went dry, cleanup and exit
    _SFCB_TRACE(1,("--- Indication retry queue empty, thread exitting."));
    pthread_mutex_unlock(&RQlock);
    retryRunning=0;
    CMRelease(ctxLocal);
    CMRelease(ctx);
    _SFCB_RETURN(NULL); 
}


int refillRetryQ (const CMPIContext * ctx)
{
    _SFCB_ENTER(TRACE_INDPROVIDER, "refillRetryQ");  
    int qfill=0;
    if (RQhead==NULL) {
        // The queue is empty, check if there are instances to be restored
        CMPIObjectPath * op=CMNewObjectPath(_broker,"root/interop","SFCB_IndicationElement",NULL);
        CMPIEnumeration * enm = _broker->bft->enumerateInstances(_broker, ctx, op, NULL, NULL);
        while(enm && enm->ft->hasNext(enm, NULL)) {
        // get the properties from the repo instance
            CMPIData inst=CMGetNext(enm,NULL);
            CMPIData indID=CMGetProperty(inst.value.inst,"indicationID",NULL);
            CMPIData rcount=CMGetProperty(inst.value.inst,"retryCount",NULL);
            CMPIData last=CMGetProperty(inst.value.inst,"lastDelivery",NULL);
            CMPIData ind=CMGetProperty(inst.value.inst,"ind",NULL);
            CMPIData sub=CMGetProperty(inst.value.inst,"sub",NULL);
            CMPIData ld=CMGetProperty(inst.value.inst,"ld",NULL);
            _SFCB_TRACE(1,("--- Requeueing indication id:%d",indID.value.Int));
            // Rebuild the queue element
            RTElement *element;
            element = (RTElement *) malloc(sizeof(*element));
            element->instanceID=indID.value.Int;
            element->lasttry=last.value.Int;
            element->count=rcount.value.Int;
            element->ind=ind.value.ref->ft->clone(ind.value.ref,NULL);
            element->ref=ld.value.ref->ft->clone(ld.value.ref,NULL);
            element->sub=sub.value.ref->ft->clone(sub.value.ref,NULL);
            CMPIObjectPath * indele=CMGetObjectPath(inst.value.inst,NULL);
            element->SFCBIndEle=indele->ft->clone(indele,NULL);
            // call enq
            enqRetry(element,ctx,0);
            qfill=1;
        }
        // spawn thread if we queued anything
        if ((qfill == 1 ) && (retryRunning == 0)) {
            retryRunning=1;
            _SFCB_TRACE(1,("--- Starting retryExport thread"));
            pthread_attr_init(&tattr);
            pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
            CMPIContext * pctx = native_clone_CMPIContext(ctx);
            pthread_create(&t, &tattr,&retryExport,(void *) pctx);
        }
    }

    _SFCB_RETURN(0); 
}


CMPIStatus IndCIMXMLHandlerInvokeMethod(CMPIMethodMI * mi,
					const CMPIContext * ctx,
					const CMPIResult * rslt,
					const CMPIObjectPath * ref,
					const char *methodName,
					const CMPIArgs * in, CMPIArgs * out)
{ 
   _SFCB_ENTER(TRACE_INDPROVIDER, "IndCIMXMLHandlerInvokeMethod");  
   CMPIStatus st = { CMPI_RC_OK, NULL };
   struct timeval tv;
   struct timezone tz;
   static unsigned int indID=1;
    
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
            _SFCB_TRACE(1,("--- Indication delivery failed, adding to retry queue"));
            // Indication delivery failed, send to retry queue
            // build an element
            RTElement *element;
            element = (RTElement *) malloc(sizeof(*element));
            element->ref=ref->ft->clone(ref,NULL);
            // Get the OP of the subscription
            CMPIInstance *sub=CMGetArg(in,"subscription",NULL).value.inst;
            CMPIObjectPath * subop=CMGetObjectPath(sub,NULL);
            element->sub=subop->ft->clone(subop,NULL);
            // Get the OP of the indication
            CMPIInstance *ind=CMGetArg(in,"indication",NULL).value.inst;
            CMPIObjectPath * iop=CMGetObjectPath(ind,NULL);
            // Add the key value
            CMAddKey(iop,"SFCB_IndicationID",&indID,CMPI_uint32);
            CMSetProperty(ind,"SFCB_IndicationID",&indID,CMPI_uint32);
            element->ind=iop->ft->clone(iop,NULL);
            // Store other attrs
            element->instanceID=indID;
            element->count=0;
            gettimeofday(&tv, &tz);
            element->lasttry=tv.tv_sec;
            CBCreateInstance(_broker, ctx, iop, ind, &st);
            if (st.rc != 0) {
                mlogf(M_ERROR,M_SHOW,"Pushing indication instance to repository failed, rc:%d\n",st.rc);
            }
            indID++;
            // Add it to the retry queue
            enqRetry(element,ctx,1);
            // And launch the thread if it isn't already running
            pthread_attr_init(&tattr);
            pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
            if (retryRunning == 0) {
                retryRunning=1;
                _SFCB_TRACE(1,("--- Starting retryExport thread"));
                CMPIContext * pctx = native_clone_CMPIContext(ctx);
                pthread_create(&t, &tattr,&retryExport,(void *) pctx);
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

CMInstanceMIStub(IndCIMXMLHandler, IndCIMXMLHandler, _broker, refillRetryQ(ctx) );
CMMethodMIStub(IndCIMXMLHandler, IndCIMXMLHandler, _broker, CMNoHook);

