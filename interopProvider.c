
/*
 * interopProvider.c
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
 * Author:     Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * InternalProvider for sfcb.
 *
*/


#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "fileRepository.h"
#include "utilft.h"
#include "trace.h"
#include "queryOperation.h"
#include "providerMgr.h"
#include "internalProvider.h"
#include "native.h"
#include <time.h>

#define LOCALCLASSNAME "InteropProvider"

/* ------------------------------------------------------------------------- */

extern CMPIInstance *relocateSerializedInstance(void *area);
extern char *value2Chars(CMPIType type, CMPIValue * value);
extern CMPIString *__oft_toString(CMPIObjectPath * cop, CMPIStatus * rc);
extern CMPIObjectPath *getObjectPath(char *path, char **msg);
extern CMPIString *native_new_CMPIString(const char *ptr, CMPIStatus * rc);


extern void closeProviderContext(BinRequestContext* ctx);
extern void setStatus(CMPIStatus *st, CMPIrc rc, char *msg);
extern int testNameSpace(char *ns, CMPIStatus *st);

/* ------------------------------------------------------------------------- */

static const CMPIBroker *_broker;
static int firstTime=1;


typedef struct filter {
   CMPIInstance *fci;
   QLStatement *qs;
   int useCount;
   char *query;
   char *lang;
   char *type;
   char *sns;
} Filter;

typedef struct handler {
   CMPIInstance *hci;
   CMPIObjectPath *hop;
   int useCount;
} Handler;

typedef struct subscription {
   CMPIInstance *sci;
   Filter *fi;
   Handler *ha;
} Subscription;

static UtilHashTable *filterHt = NULL;
static UtilHashTable *handlerHt = NULL;
static UtilHashTable *subscriptionHt = NULL;

/* ------------------------------------------------------------------------- */

static int interOpNameSpace(
	const CMPIObjectPath * cop,
	CMPIStatus * st) 
 {   
   char *ns = (char*)CMGetNameSpace(cop,NULL)->hdl;   
   if (strcasecmp(ns,"root/interop") && strcasecmp(ns,"root/pg_interop")) {
      if (st) setStatus(st, CMPI_RC_ERR_FAILED, "Object must reside in root/interop");
      return 0;
   }
   return 1;
}
   
/* ------------------------------------------------------------------------- */

static Subscription *addSubscription(
	const CMPIInstance * ci,
	const char * key,
	Filter * fi,
	Handler * ha)
{
   Subscription *su;
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "addSubscription");
   
   if (subscriptionHt == NULL) {
      subscriptionHt=UtilFactory->newHashTable(61,UtilHashTable_charKey);
   }

   _SFCB_TRACE(1,("-- Subscription: %s\n",key));
   
   su=subscriptionHt->ft->get(subscriptionHt,key);
   if (su) _SFCB_RETURN(NULL);
   
   su=(Subscription*)malloc(sizeof(Subscription));
   su->sci=CMClone(ci,NULL);
   su->fi=fi;
   fi->useCount++;
   su->ha=ha;
   ha->useCount++;
   subscriptionHt->ft->put(subscriptionHt,key,su);
   
   _SFCB_RETURN(su);
}

/* ------------------------------------------------------------------------- */

static Subscription *getSubscription(
	char * key)
{
   Subscription *su;

   _SFCB_ENTER(TRACE_INDPROVIDER, "getSubscription");

   if (subscriptionHt==NULL) return NULL;
   su = subscriptionHt->ft->get(subscriptionHt,key);

   _SFCB_RETURN(su);
}

/* ------------------------------------------------------------------------- */

static void removeSubscription(
	Subscription * su,
	char * key)
{
   _SFCB_ENTER(TRACE_INDPROVIDER, "removeSubscription");

   if (subscriptionHt) {
      subscriptionHt->ft->remove(subscriptionHt,key);
      if (su) {
        if (su->fi) su->fi->useCount--;
        if (su->ha) su->ha->useCount--;
      }
   }
   if (su) {
      free (su);
   }

   _SFCB_EXIT();
}

/* ------------------------------------------------------------------------- */

static Filter *addFilter(
	const CMPIInstance * ci,
	const char * key,
	QLStatement * qs, 
	const char * query,
	const char * lang,
	const char * sns)
{
   Filter * fi;
      
   _SFCB_ENTER(TRACE_INDPROVIDER, "addFilter");
   
   _SFCB_TRACE(1,("--- Filter: >%s<",key));
   _SFCB_TRACE(1,("--- query: >%s<",query));
   
   if (filterHt==NULL)
      filterHt=UtilFactory->newHashTable(61,UtilHashTable_charKey);
      
   fi=filterHt->ft->get(filterHt,key);
   if (fi) _SFCB_RETURN(NULL);
   
   fi=(Filter*)malloc(sizeof(Filter));
   fi->fci=CMClone(ci,NULL);
   fi->useCount=0;
   fi->qs=qs;
   fi->query=strdup(query);
   fi->lang=strdup(lang);
   fi->sns=strdup(sns);
   filterHt->ft->put(filterHt,key,fi);
   
   _SFCB_RETURN(fi);
}

/* ------------------------------------------------------------------------- */

static Filter *getFilter(
	char * key)
{
   Filter * fi;

   _SFCB_ENTER(TRACE_INDPROVIDER, "getFilter");

   if (filterHt==NULL) return NULL;
   fi = filterHt->ft->get(filterHt,key);

   _SFCB_RETURN(fi);
}

/* ------------------------------------------------------------------------- */

static void removeFilter(
	Filter * fi,
	char * key)
{
   _SFCB_ENTER(TRACE_INDPROVIDER, "removeFilter");

   if (filterHt) {
      filterHt->ft->remove(filterHt,key);
   }
   if (fi) {
//      CMRelease(fi->fci);
      CMRelease(fi->qs);
      free(fi->query);
      free(fi->lang);
      free(fi->sns);
      free (fi);
   }   

   _SFCB_EXIT();
}

/* ------------------------------------------------------------------------- */

static Handler *addHandler(
	CMPIInstance *ci,
	CMPIObjectPath * op)
{
   Handler *ha; 
   char *key;
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "addHandler");
   
   if (handlerHt==NULL)
      handlerHt=UtilFactory->newHashTable(61,UtilHashTable_charKey);
      
   key=internalProviderNormalizeObjectPath(op);
      
   _SFCB_TRACE(1,("--- Handler: %s",key));
   
   if ((ha=handlerHt->ft->get(handlerHt,key))!=NULL) {
      _SFCB_TRACE(1,("--- Handler already registered %p",ha));
      _SFCB_RETURN(NULL);
   }

   ha=(Handler*)malloc(sizeof(Handler));
   ha->hci=CMClone(ci,NULL);
   ha->hop=CMClone(op,NULL);
   ha->useCount=0;
   handlerHt->ft->put(handlerHt,key,ha);
   
   _SFCB_RETURN(ha);
}

/* ------------------------------------------------------------------------- */

static Handler *getHandler(
	char * key)
{
   Handler *ha;

   _SFCB_ENTER(TRACE_INDPROVIDER, "getHandler");

   if (handlerHt==NULL) return NULL;
   ha=handlerHt->ft->get(handlerHt,key);

   _SFCB_RETURN(ha);
}

/* ------------------------------------------------------------------------- */

static void removeHandler(
	Handler * ha,
	char * key)
{
   _SFCB_ENTER(TRACE_INDPROVIDER, "removeHandler");

   if (handlerHt) {
      handlerHt->ft->remove(handlerHt,key);
   }
   if (ha) {
      free (ha);
   }

   _SFCB_EXIT();
}

/* ------------------------------------------------------------------------- */

extern int isChild(const char *ns, const char *parent, const char* child);

static int isa(const char *sns, const char *child, const char *parent)
{
   int rv;
   _SFCB_ENTER(TRACE_INDPROVIDER, "isa");
   
   if (strcasecmp(child,parent)==0) return 1;
   rv=isChild(sns,parent,child);
   _SFCB_RETURN(rv);
}

/* ------------------------------------------------------------------------- */

CMPIStatus deactivateFilter(
	const CMPIContext * ctx,
	const char * ns,
	const char * cn,
	Filter * fi)
{
   CMPIObjectPath *path;
   CMPIStatus st={CMPI_RC_OK,NULL};
   IndicationReq sreq = BINREQ(OPS_DeactivateFilter, 6);
   BinResponseHdr **resp=NULL;
   BinRequestContext binCtx;
   OperationHdr req = {OPS_IndicationLookup, 2};
   char *principal=ctx->ft->getEntry(ctx,CMPIPrincipal,NULL).value.string->hdl;;
   int irc,err,cnt,i;
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "deactivateFilter"); 
   _SFCB_TRACE(4, ("class %s",cn));
   
   path = TrackedCMPIObjectPath(ns, cn, &st);
   
   sreq.principal = setCharsMsgSegment(principal);
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.query = setCharsMsgSegment(fi->query);
   sreq.language = setCharsMsgSegment(fi->lang);
   sreq.type = setCharsMsgSegment(fi->type);
   sreq.sns = setCharsMsgSegment(fi->sns);
   sreq.filterId=fi;

   req.nameSpace = setCharsMsgSegment(fi->sns);
   req.className = setCharsMsgSegment((char*) cn);

   memset(&binCtx,0,sizeof(BinRequestContext));
   binCtx.oHdr = &req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=0;

   _SFCB_TRACE(1, ("--- getProviderContext for %s-%s",fi->sns,cn));

   irc = getProviderContext(&binCtx, &req);
 
   if (irc == MSG_X_PROVIDER) {      
      _SFCB_TRACE(1, ("--- Invoking Providers"));
      /* one good provider makes success */
      resp = invokeProviders(&binCtx,&err,&cnt);
      if (err == 0) {
	setStatus(&st,0,NULL);
      } else {
	setStatus(&st,resp[err-1]->rc,NULL);
	for (i=0; i<binCtx.pCount; i++) {
	  if (resp[i]->rc == 0) {
	    setStatus(&st,0,NULL);
	    break;
	  }
	}
      }
      _SFCB_TRACE(1, ("--- Invoking Provider rc: %d",st.rc));
   }
   
   else {  // this should not occur
      if (irc==MSG_X_PROVIDER_NOT_FOUND) setStatus(&st,CMPI_RC_ERR_FAILED,
         "No eligible indication provider found");
      else {
         char msg[512];
         snprintf(msg,511,"Failing to find eligible indication provider. Rc: %d",irc);
         setStatus(&st,CMPI_RC_ERR_FAILED,msg);
      }   
   }   
   
   if (resp) {
      free(resp);
      closeProviderContext(&binCtx);
   }   
 
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

#define CREATE_INST 1
#define DELETE_INST 2
#define MODIFY_INST 3

extern CMPISelectExp *TempCMPISelectExp(QLStatement *qs);

CMPIStatus activateSubscription(
	const char * principal,
	const char * cn,
	const char * type,
	Filter * fi,
	int * rrc)
{
   CMPIObjectPath *path;
   CMPIStatus st={CMPI_RC_OK,NULL},rc;
   IndicationReq sreq = BINREQ(OPS_ActivateFilter, 6);
   BinResponseHdr **resp=NULL;
   BinRequestContext binCtx;
   OperationHdr req = {OPS_IndicationLookup, 2};
   int irc=0,err,cnt,i;
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "activateSubscription");
   _SFCB_TRACE(4, ("principal %s, class %s, type %s",principal, cn, type));
   
   if (rrc) *rrc=0;
   path = TrackedCMPIObjectPath(fi->sns, cn, &rc);
   
   sreq.principal = setCharsMsgSegment(principal);
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.query = setCharsMsgSegment(fi->query);
   sreq.language = setCharsMsgSegment(fi->lang);
   sreq.type = setCharsMsgSegment((char*)type);
   fi->type=strdup(type);
   sreq.sns = setCharsMsgSegment(fi->sns);
   sreq.filterId=fi;

   req.nameSpace = setCharsMsgSegment(fi->sns);
   req.className = setCharsMsgSegment((char*) cn);

   memset(&binCtx,0,sizeof(BinRequestContext));
   binCtx.oHdr = &req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=0;

   _SFCB_TRACE(1, ("--- getProviderContext for %s-%s",fi->sns,cn));
   
   irc = getProviderContext(&binCtx, &req);

   if (irc == MSG_X_PROVIDER) {      
      _SFCB_TRACE(1, ("--- Invoking Providers"));
      /* one good provider makes success */
      resp = invokeProviders(&binCtx,&err,&cnt);
      if (err == 0) {
	setStatus(&st,0,NULL);
      } else {
	setStatus(&st,resp[err-1]->rc,NULL);
	for (i=0; i<binCtx.pCount; i++) {
	  if (resp[i]->rc == 0) {
	    setStatus(&st,0,NULL);
	    break;
	  }
	}
      }
   }
   
   else {
      if (rrc) *rrc=irc;
      if (irc==MSG_X_PROVIDER_NOT_FOUND) setStatus(&st,CMPI_RC_ERR_FAILED,
         "No eligible indication provider found");
      else {
         char msg[512];
         snprintf(msg,511,"Failing to find eligible indication provider. Rc: %d",irc);
         setStatus(&st,CMPI_RC_ERR_FAILED,msg);
      }   
   }   
   
   if (resp) {
      free(resp);
      closeProviderContext(&binCtx);
   }   
 
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus activateLifeCycleSubscription(
	char * principal,
	const char * cn,
	Filter * fi,
	int type)
{
   CMPIStatus st={CMPI_RC_OK,NULL};
   CMPISelectExp *exp=TempCMPISelectExp(fi->qs);
   CMPISelectCond *cond=CMGetDoc(exp,NULL);
   CMPISubCond *sc;
   CMPIPredicate *pr;
   CMPICount c,cm,s,sm;
   CMPIString *lhs,*rhs;
   CMPIPredOp predOp;
   int irc;
   int has_isa=0;

   _SFCB_ENTER(TRACE_INDPROVIDER, "activateLifeCycleSubscription");
   
   if (cond) { 
     _SFCB_TRACE(1,("condition %p",cond));
      cm=CMGetSubCondCountAndType(cond,NULL,NULL);
      _SFCB_TRACE(1,("subcondition count %d",cm));
      if (cm) for (c=0; c<cm; c++) {
         _SFCB_TRACE(1,("subcondition %d",c));
         sc=CMGetSubCondAt(cond,c,NULL);
         if (sc) {
            for (s=0, sm=CMGetPredicateCount(sc,NULL); s<sm; s++) {
               pr=CMGetPredicateAt(sc,s,NULL);
               if (pr) {
                  CMGetPredicateData(pr,NULL,&predOp,&lhs,&rhs);
                  if (predOp==CMPI_PredOp_Isa) {
                     has_isa=1;
                     _SFCB_TRACE(1,("lhs: %s",(char*)lhs->hdl)); 
                     _SFCB_TRACE(1,("rhs: %s\n",(char*)rhs->hdl));
                     st=activateSubscription(principal,(char*)rhs->hdl,cn,fi,&irc);
                     if (irc==MSG_X_INVALID_CLASS) 
                        st.rc=CMPI_RC_ERR_INVALID_CLASS;
                     break;
		  }
               }
            }
            if (st.rc!=CMPI_RC_OK) break;
         }
      }
      if (has_isa==0) {
	/* no ISA predicate -- need to process indication class provider */
	st=activateSubscription(principal,cn,cn,fi,&irc);
	if (irc==MSG_X_INVALID_CLASS) 
	  st.rc=CMPI_RC_ERR_INVALID_CLASS;
      }
   }
   else setStatus(&st,CMPI_RC_ERR_INVALID_QUERY,"CMGetDoc failed"); 
     
   free(exp);
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

int fowardSubscription(
	const CMPIContext * ctx,
	Filter * fi,
	CMPIStatus * st)
{
   CMPIStatus rc;
   char *principal = NULL;
   char **fClasses = fi->qs->ft->getFromClassList(fi->qs);
   CMPIData principalP = ctx->ft->getEntry(ctx,CMPIPrincipal,&rc);
   int irc;
   int activated = 0;
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "fowardSubscription");
   
   if (rc.rc == CMPI_RC_OK) { 
      principal = (char*)principalP.value.string->hdl;
      _SFCB_TRACE(1,("--- principal=\"%s\"", principal));
   }

   /* Go thru all the indication classes specified in the filter query and activate each */
   for ( ; *fClasses; fClasses++) {
      _SFCB_TRACE(1,("--- indication class=\"%s\" namespace=\"%s\"", *fClasses, fi->sns));

      /* Check if this is a process indication */
      if (isa(fi->sns, *fClasses, "CIM_ProcessIndication")) {
         *st = activateSubscription(principal, *fClasses, *fClasses, fi, &irc);
         if (st->rc == CMPI_RC_OK) activated++; 
      }

      /* Check if this is a lifecycle instance creation indication */
      else if (isa("root/interop", *fClasses, "CIM_InstCreation")) {
         *st = activateLifeCycleSubscription(principal, *fClasses, fi, CREATE_INST);
         if (st->rc == CMPI_RC_OK) activated++;
      }

      /* Check if this is a lifecycle instance deletion indication */
      else if (isa("root/interop", *fClasses, "CIM_InstDeletion")) {
         *st = activateLifeCycleSubscription(principal, *fClasses, fi, DELETE_INST);
         if (st->rc == CMPI_RC_OK) activated++;
      }

      /* Check if this is a lifecycle instance modification indication */
      else if (isa("root/interop", *fClasses, "CIM_InstModification")) {
         *st = activateLifeCycleSubscription(principal, *fClasses, fi, MODIFY_INST);
         if (st->rc == CMPI_RC_OK) activated++;
      }

      /* Warn if this indication class is unknown and continue processing the rest, if any */ 
      else {
         _SFCB_TRACE(1,("--- Unsupported/unrecognized indication class"));
      }
   }

   /* Make sure at least one of the indication classes were successfully activated */
   if (!activated) {
      setStatus(st, CMPI_RC_ERR_NOT_SUPPORTED, "No supported indication classes in filter query");
     _SFCB_RETURN(-1);
   }
     
   setStatus(st, CMPI_RC_OK, NULL);
   _SFCB_RETURN(0);
}

/* ------------------------------------------------------------------------- */

extern UtilStringBuffer *instanceToString(CMPIInstance * ci, char **props);

static CMPIStatus processSubscription(
	const CMPIBroker *broker,
	const CMPIContext *ctx, 
	const CMPIInstance *ci,
	const CMPIObjectPath *cop)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   Handler *ha;
   Filter *fi;
   Subscription *su;
   CMPIObjectPath *op;
   char *key,*skey;
   CMPIDateTime *dt;
    
   _SFCB_ENTER(TRACE_INDPROVIDER, "processSubscription()");
   
   _SFCB_TRACE(1,("--- checking for existing subscription"));
   skey = internalProviderNormalizeObjectPath(cop);
   if (getSubscription(skey)) {
      _SFCB_TRACE(1,("--- subscription already exists"));
      free(skey);
      setStatus(&st, CMPI_RC_ERR_ALREADY_EXISTS, NULL);
      _SFCB_RETURN(st); 
   }      
      
   _SFCB_TRACE(1,("--- getting new subscription filter"));
   op = CMGetProperty(ci, "filter", &st).value.ref;
   key = internalProviderNormalizeObjectPath(op);
   fi = getFilter(key);
   free(key);
   
   if (fi == NULL) {
      _SFCB_TRACE(1,("--- cannot find specified subscription filter"));
      setStatus(&st, CMPI_RC_ERR_NOT_FOUND, "Filter not found");
      _SFCB_RETURN(st);
   }
   
   _SFCB_TRACE(1,("--- getting new subscription handle"));
   op = CMGetProperty(ci, "handler", &st).value.ref;
   key = internalProviderNormalizeObjectPath(op);
   ha = getHandler(key);
   free(key);
      
   if (ha == NULL) {
      _SFCB_TRACE(1,("--- cannot find specified subscription handler"));
      setStatus(&st, CMPI_RC_ERR_NOT_FOUND, "Handler not found");
      _SFCB_RETURN(st);
   }

   _SFCB_TRACE(1,("--- setting subscription duration"));
   dt = CMNewDateTime(_broker,NULL);
   CMSetProperty((CMPIInstance *)ci, "SubscriptionDuration", &dt, CMPI_dateTime);
   
   su=addSubscription(ci, skey, fi, ha); 
   fowardSubscription(ctx, fi, &st);   
       
   if (st.rc != CMPI_RC_OK) removeSubscription(su, skey); 
      
   _SFCB_RETURN(st); 
}

/* ------------------------------------------------------------------ *
 * InterOp initialization
 * ------------------------------------------------------------------ */

void initInterOp(
	const CMPIBroker *broker,
	const CMPIContext *ctx)
{
   CMPIObjectPath *op;
   UtilList *ul;
   CMPIInstance *ci;
   CMPIStatus st;
   CMPIObjectPath *cop;
   char *key,*query,*lng,*sns;
   QLStatement *qs=NULL;
   int rc;
    
   _SFCB_ENTER(TRACE_INDPROVIDER, "initInterOp");
   
   firstTime=0;
    
   _SFCB_TRACE(1,("--- checking for cim_indicationfilter"));
   op=CMNewObjectPath(broker,"root/interop","cim_indicationfilter",&st);
   ul=SafeInternalProviderEnumInstances(NULL,ctx,op,NULL,&st,0);
   
   if (ul) for (ci = (CMPIInstance*)ul->ft->getFirst(ul); ci;
          ci=(CMPIInstance*)ul->ft->getNext(ul)) {
      cop=CMGetObjectPath(ci,&st);
      query=(char*)CMGetProperty(ci,"query",&st).value.string->hdl;
      lng=(char*)CMGetProperty(ci,"querylanguage",&st).value.string->hdl;
      sns=(char*)CMGetProperty(ci,"SourceNamespace",&st).value.string->hdl;
      qs=parseQuery(MEM_NOT_TRACKED,query,lng,sns,&rc);
      key=internalProviderNormalizeObjectPath(cop);
      addFilter(ci,key,qs,query,lng,sns);
   }   

   _SFCB_TRACE(1,("--- checking for cim_listenerdestination"));
   op=CMNewObjectPath(broker,"root/interop","cim_listenerdestination",&st);
   ul=SafeInternalProviderEnumInstances(NULL,ctx,op,NULL,&st,1);
   
   if (ul) for (ci = (CMPIInstance*)ul->ft->getFirst(ul); ci;
          ci=(CMPIInstance*) ul->ft->getNext(ul)) {
      cop=CMGetObjectPath(ci,&st); 
      addHandler(ci,cop);
   } 
   _SFCB_TRACE(1,("--- checking for cim_indicationsubscription"));
   op=CMNewObjectPath(broker,"root/interop","cim_indicationsubscription",&st);
   ul=SafeInternalProviderEnumInstances(NULL,ctx,op,NULL,&st,0);
   
   if (ul) for (ci = (CMPIInstance*)ul->ft->getFirst(ul); ci;
          ci=(CMPIInstance*)ul->ft->getNext(ul)) {
      CMPIObjectPath *hop;    
      cop=CMGetObjectPath(ci,&st);
      hop=CMGetKey(cop,"handler",NULL).value.ref;
      processSubscription(broker,ctx,ci,cop);
   }
      
   _SFCB_EXIT(); 
}

/* --------------------------------------------------------------------------*/
/*                       Instance Provider Interface                         */
/* --------------------------------------------------------------------------*/
 
CMPIStatus InteropProviderCleanup(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx, CMPIBoolean terminate)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderCleanup");
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus InteropProviderEnumInstanceNames(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * ref)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderEnumInstanceNames");

   if (interOpNameSpace(ref,NULL) != 1) _SFCB_RETURN(st);
   st=InternalProviderEnumInstanceNames(NULL, ctx, rslt, ref);

   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus InteropProviderEnumInstances(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * ref,
	const char ** properties)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderEnumInstances");

//   if (interOpNameSpace(ref,NULL)!=1) _SFCB_RETURN(st);
   st=InternalProviderEnumInstances(NULL, ctx, rslt, ref, properties);

   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus InteropProviderGetInstance(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const char ** properties)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderGetInstance");

   st = InternalProviderGetInstance(NULL, ctx, rslt, cop, properties);

   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus InteropProviderCreateInstance(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const CMPIInstance * ci)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIString *cn = CMGetClassName(cop, NULL);
   char *cns = cn->ft->getCharPtr(cn,NULL);

   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderCreateInstance");
  
   if (interOpNameSpace(cop,&st)!=1) _SFCB_RETURN(st);
   
   if (strcasecmp(cns,"cim_indicationsubscription")==0) {
   
      _SFCB_TRACE(1,("--- create cim_indicationsubscription"));
      
      st=processSubscription(_broker,ctx,ci,cop);
   }
   
   else if (strcasecmp(cns,"cim_indicationfilter")==0) {
      QLStatement *qs=NULL;
      int rc,i,n,m;
      char *key=NULL,*ql,lng[16];
      CMPIString *lang=ci->ft->getProperty(ci,"querylanguage",&st).value.string;
      CMPIString *query=ci->ft->getProperty(ci,"query",&st).value.string;
      CMPIString *sns=ci->ft->getProperty(ci,"SourceNamespace",&st).value.string;
      
      _SFCB_TRACE(1,("--- create cim_indicationfilter"));
   
      if (lang==NULL || query==NULL) {
         setStatus(&st,CMPI_RC_ERR_FAILED,"Query and/or Language property not found");
         _SFCB_RETURN(st);         
      }
      
      for (ql=(char*)lang->hdl,i=0,n=0,m=strlen(ql); i<m; i++) {
         if (ql[i]>' ') lng[n++]=ql[i];
         if (n>=15) break;
      }
      lng[n]=0;      
       
      _SFCB_TRACE(2,("--- CIM query language %s %s",lang->hdl,lng));
      if (strcasecmp(lng,"wql") && strcasecmp(lng,"cql") && strcasecmp(lng,"cim:cql")) {
         setStatus(&st,CMPI_RC_ERR_QUERY_LANGUAGE_NOT_SUPPORTED,NULL);
         _SFCB_RETURN(st);  
      }   
              
      key=internalProviderNormalizeObjectPath(cop);
      if (getFilter(key)) {
         free(key);
         setStatus(&st,CMPI_RC_ERR_ALREADY_EXISTS,NULL);
         _SFCB_RETURN(st); 
      }
      
      qs=parseQuery(MEM_NOT_TRACKED,(char*)query->hdl,lng,(char*)sns->hdl,&rc);
      if (rc) {
         free(key);
         setStatus(&st,CMPI_RC_ERR_INVALID_QUERY,"Query parse error");
         _SFCB_RETURN(st);         
      }

      addFilter(ci,key,qs,(char*)query->hdl,lng,(char*)sns->hdl);
   }
   
   else {
      setStatus(&st,CMPI_RC_ERR_NOT_SUPPORTED,"Class not supported");
      _SFCB_RETURN(st);         
   }
    
   if (st.rc==CMPI_RC_OK) 
      st=InternalProviderCreateInstance(NULL,ctx,rslt,cop,ci);
    
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus InteropProviderModifyInstance(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const CMPIInstance * ci,
	const char ** properties)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderSetInstance");
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus InteropProviderDeleteInstance(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIString *cn = CMGetClassName(cop, NULL);
   char *cns = cn->ft->getCharPtr(cn,NULL);
   char *key = internalProviderNormalizeObjectPath(cop);
   Filter *fi;
   Subscription *su;
   char *ns = (char*)CMGetNameSpace(cop,NULL)->hdl;

   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderDeleteInstance");
   
   if (strcasecmp(cns,"cim_indicationsubscription")==0) {
      _SFCB_TRACE(1,("--- delete cim_indicationsubscription %s",key));
      if ((su=getSubscription(key))) {
         fi=su->fi;
         if (fi->useCount==1) {
            char **fClasses=fi->qs->ft->getFromClassList(fi->qs);
            for ( ; *fClasses; fClasses++) {
	      deactivateFilter(ctx, ns, *fClasses, fi);
            }
         }   
         removeSubscription(su,key);
      }
      else setStatus(&st,CMPI_RC_ERR_NOT_FOUND,NULL);
   }
   
   else if (strcasecmp(cns,"cim_indicationfilter")==0) {
      _SFCB_TRACE(1,("--- delete cim_indicationfilter %s",key));
      
      if ((fi=getFilter(key))) {
         if (fi->useCount) setStatus(&st,CMPI_RC_ERR_FAILED,"Filter in use");
         else removeFilter(fi,key);
      }
      else setStatus(&st,CMPI_RC_ERR_NOT_FOUND,NULL);
   }
   
   else setStatus(&st,CMPI_RC_ERR_NOT_SUPPORTED,"Class not supported");
      
   if (st.rc==CMPI_RC_OK)
      st=InternalProviderDeleteInstance(NULL,ctx,rslt,cop);
   
   if (key) free(key);
   
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus InteropProviderExecQuery(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const char * lang,
	const char * query)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderExecQuery");
   _SFCB_RETURN(st);
}

/* --------------------------------------------------------------------------*/
/*                        Method Provider Interface                          */
/* --------------------------------------------------------------------------*/

CMPIStatus InteropProviderMethodCleanup(
	CMPIMethodMI * mi,
	const CMPIContext * ctx, CMPIBoolean terminate)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderMethodCleanup");
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus InteropProviderInvokeMethod(
	CMPIMethodMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * ref,
	const char * methodName,
	const CMPIArgs * in,
	CMPIArgs * out)
{ 
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char *ns=(char*)CMGetNameSpace(ref,NULL)->hdl;
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderInvokeMethod");
   
   if (strcasecmp(ns,"root/interop") && strcasecmp(ns,"root/pg_interop")) {
      setStatus(&st,CMPI_RC_ERR_FAILED,"Object must reside in root/interop");
      return st;
   }
   
   _SFCB_TRACE(1,("--- Method: %s",methodName)); 

   if (strcasecmp(methodName, "_deliver") == 0) {
      HashTableIterator *i;
      Subscription *su;
      char *suName;
      CMPIArgs *hin=CMNewArgs(_broker,NULL);
      CMPIInstance *ind=CMGetArg(in,"indication",NULL).value.inst;
      void *filterId=(void*)CMGetArg(in,"filterid",NULL).value.uint32;
      char *ns=(char*)CMGetArg(in,"namespace",NULL).value.string->hdl;
      
      CMAddArg(hin,"indication",&ind,CMPI_instance);
      CMAddArg(hin,"nameSpace",ns,CMPI_chars);
      
      if (subscriptionHt) for (i = subscriptionHt->ft->getFirst(subscriptionHt, 
                 (void**)&suName, (void**)&su); i; 
              i = subscriptionHt->ft->getNext(subscriptionHt,i, 
                 (void**)&suName, (void**)&su)) {
         if ((void*)su->fi==filterId) {
            CMPIString *str=CDToString(_broker,su->ha->hop,NULL);
            CMPIString *ns=CMGetNameSpace(su->ha->hop,NULL);
            _SFCB_TRACE(1,("--- invoke handler %s %s",(char*)ns->hdl,(char*)str->hdl));
            
            CBInvokeMethod(_broker,ctx,su->ha->hop,"_deliver",hin,NULL,&st);
            _SFCB_TRACE(1,("--- invoke handler status: %d",st.rc));
         }     
      }   
   }
   
   else if (strcasecmp(methodName, "_addHandler") == 0) {
      CMPIInstance *ci=in->ft->getArg(in,"handler",&st).value.inst;
      CMPIObjectPath *op=in->ft->getArg(in,"key",&st).value.ref;
      CMPIString *str=CDToString(_broker,op,NULL);
      CMPIString *ns=CMGetNameSpace(op,NULL);
      _SFCB_TRACE(1,("--- _addHandler %s %s",(char*)ns->hdl,(char*)str->hdl));
      addHandler(ci,op);     
   }
   
   else if (strcasecmp(methodName, "_removeHandler") == 0) {
      CMPIObjectPath *op=in->ft->getArg(in,"key",&st).value.ref;
      char *key=internalProviderNormalizeObjectPath(op);
      Handler *ha=getHandler(key);
      if (ha) {
         if (ha->useCount) {
            setStatus(&st,CMPI_RC_ERR_FAILED,"Handler in use");
         } 
         else removeHandler(ha,key);   
      }
      else {
         setStatus(&st, CMPI_RC_ERR_NOT_FOUND, "Handler objectnot found");
      }   
   }

   else if (strcasecmp(methodName, "_startup") == 0) {
      initInterOp(_broker,ctx);  
   }

   else {
      _SFCB_TRACE(1,("--- Invalid request method: %s",methodName));
      setStatus(&st, CMPI_RC_ERR_METHOD_NOT_FOUND, "Invalid request method");
   }
   
   _SFCB_RETURN(st);
}

/* --------------------------------------------------------------------------*/
/*                     Association Provider Interface                        */
/* --------------------------------------------------------------------------*/

CMPIStatus InteropProviderAssociationCleanup(
	CMPIAssociationMI * mi,
	const CMPIContext * ctx,
	CMPIBoolean terminate)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderAssociationCleanup");
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus InteropProviderAssociators(
	CMPIAssociationMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const char * assocClass,
	const char * resultClass,
	const char * role,
	const char * resultRole,
	const char ** propertyList)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderAssociators");
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus InteropProviderAssociatorNames(
	CMPIAssociationMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const char * assocClass,
	const char * resultClass,
	const char * role,
	const char * resultRole)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderAssociatorNames");
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus InteropProviderReferences(
	CMPIAssociationMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const char * assocClass,
	const char * role,
	const char ** propertyList)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderReferences");
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus InteropProviderReferenceNames(
	CMPIAssociationMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const char * assocClass,
	const char * role)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderReferenceNames");
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------ *
 * Instance MI Factory
 *
 * NOTE: This is an example using the convenience macros. This is OK
 *       as long as the MI has no special requirements, i.e. to store
 *       data between calls.
 * ------------------------------------------------------------------ */

CMInstanceMIStub(InteropProvider, InteropProvider, _broker, CMNoHook); 

CMAssociationMIStub(InteropProvider, InteropProvider, _broker, CMNoHook);

CMMethodMIStub(InteropProvider, InteropProvider, _broker, CMNoHook);

