
/*
 * interopProvider.c
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
#include <time.h>

#define LOCALCLASSNAME "InteropProvider"

extern CMPIInstance *relocateSerializedInstance(void *area);
extern void getSerializedInstance(CMPIInstance * ci, void *area);
extern unsigned long getInstanceSerializedSize(CMPIInstance * ci);
extern char *value2Chars(CMPIType type, CMPIValue * value);
extern CMPIString *__oft_toString(CMPIObjectPath * cop, CMPIStatus * rc);
extern CMPIObjectPath *getObjectPath(char *path, char **msg);
extern CMPIString *native_new_CMPIString(const char *ptr, CMPIStatus * rc);

extern UtilList *SafeInternalProviderEnumInstances(CMPIInstanceMI * mi,
      CMPIContext * ctx, CMPIObjectPath * ref, char **properties, CMPIStatus *st,int ignprov);
extern char *internalProviderNormalizeObjectPath(CMPIObjectPath *cop);

extern CMPIStatus InternalProviderEnumInstanceNames(CMPIInstanceMI * mi,
      CMPIContext * ctx, CMPIResult * rslt, CMPIObjectPath * ref);
extern CMPIStatus InternalProviderEnumInstances(CMPIInstanceMI * mi,
      CMPIContext * ctx, CMPIResult * rslt, CMPIObjectPath * ref, char **properties);
extern CMPIInstance *internalProviderGetInstance(CMPIObjectPath * cop, CMPIStatus *rc);
extern CMPIStatus InternalProviderCreateInstance(CMPIInstanceMI * mi,
       CMPIContext * ctx, CMPIResult * rslt, CMPIObjectPath * cop, CMPIInstance * ci);
extern CMPIStatus InternalProviderGetInstance(CMPIInstanceMI * mi,
       CMPIContext * ctx, CMPIResult * rslt, CMPIObjectPath * cop, char **properties);
extern CMPIStatus InternalProviderDeleteInstance(CMPIInstanceMI * mi,
       CMPIContext * ctx, CMPIResult * rslt, CMPIObjectPath * cop);
extern void closeProviderContext(BinRequestContext* ctx);
extern void setStatus(CMPIStatus *st, CMPIrc rc, char *msg);

extern QLStatement *parseQuery(int mode, char *query, char *lang, int *rc);

static CMPIBroker *_broker;
static int firstTime=1;


typedef struct filter {
   CMPIInstance *fci;
   QLStatement *qs;
   int useCount;
   char *query;
   char *lang;
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


static int interOpNameSpace(CMPIObjectPath *cop, CMPIStatus *st) 
 {   
   char *ns=(char*)CMGetNameSpace(cop,NULL)->hdl;   
   if (strcasecmp(ns,"root/interop") && strcasecmp(ns,"root/pg_interop")) {
      if (st) setStatus(st,CMPI_RC_ERR_FAILED,"Object must reside in root/interop");
      return 0;
   }
   return 1;
}
   

static Subscription *addSubscription(CMPIInstance *ci,  char *key, Filter *fi, Handler *ha)
{
   Subscription *su;
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "addSubscription");
   
   if (subscriptionHt==NULL)
      subscriptionHt=UtilFactory->newHashTable(61,UtilHashTable_charKey);
      
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

static Subscription *getSubscription(char *key)
{
   Subscription *su;
   if (subscriptionHt==NULL) return NULL;
      
   su=subscriptionHt->ft->get(subscriptionHt,key);
   return su;
}

static void removeSubscription(Subscription *su,  char *key)
{
   if (subscriptionHt) {
      filterHt->ft->remove(filterHt,key);
      if (su) {
        if (su->fi) su->fi->useCount--;
        if (su->ha) su->ha->useCount--;
      }
   }
   if (su) {
      free (su);
   }
}


static Filter *addFilter(CMPIInstance *ci,  char *key, QLStatement *qs, 
   char *query, char *lang)
{
   Filter *fi;
      
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
   filterHt->ft->put(filterHt,key,fi);
   
   _SFCB_RETURN(fi);
}

static Filter *getFilter(char *key)
{
   Filter *fi;
   if (filterHt==NULL) return NULL;
      
   fi=filterHt->ft->get(filterHt,key);
   return fi;
}

static void removeFilter(Filter *fi,  char *key)
{
   if (filterHt) {
      filterHt->ft->remove(filterHt,key);
   }
   if (fi) {
      free (fi);
      CMRelease(fi->fci);
      CMRelease(fi->qs);
      free(fi->query);
      free(fi->lang);
   }   
}

static Handler *addHandler(CMPIInstance *ci, CMPIObjectPath *op)
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

static Handler *getHandler(char *key)
{
   Handler *ha;
   if (handlerHt==NULL) return NULL;
      
   ha=handlerHt->ft->get(handlerHt,key);
   return ha;
}

static void removeHandler(Handler *ha,  char *key)
{
   if (handlerHt) {
      handlerHt->ft->remove(handlerHt,key);
   }
   if (ha) {
      free (ha);
   }
}

extern int isChild(const char *ns, const char *parent, const char* child);

static int isa(const char *child, const char *parent)
{
   int rv;
   _SFCB_ENTER(TRACE_INDPROVIDER, "isa");
   if (strcasecmp(child,parent)==0) return 1;
   rv=isChild("root/interop",parent,child);
   _SFCB_RETURN(rv);
}


CMPIStatus deactivateFilter(CMPIContext * ctx, char *ns, char *cn, Filter *fi)
{
   CMPIObjectPath *path;
   CMPIStatus st={CMPI_RC_OK,NULL};
   IndicationReq sreq = BINREQ(OPS_DeactivateFilter, 4);
   BinResponseHdr *resp=NULL;
   BinRequestContext binCtx;
   OperationHdr req = {OPS_IndicationLookup, 2};
   char *principle=ctx->ft->getEntry(ctx,CMPIPrinciple,NULL).value.string->hdl;;
   int irc;
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "deactivateFilter");
   
   path = TrackedCMPIObjectPath(ns, cn, &st);
   
   sreq.principle = setCharsMsgSegment(principle);
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.query = setCharsMsgSegment(NULL);
   sreq.language = setCharsMsgSegment(NULL);
   sreq.filterId=fi;

   req.nameSpace = setCharsMsgSegment((char*) ns);
   req.className = setCharsMsgSegment((char*) cn);

   memset(&binCtx,0,sizeof(BinRequestContext));
   binCtx.oHdr = &req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=0;

   irc = getProviderContext(&binCtx, &req);
 
   if (irc == MSG_X_PROVIDER) {      
      _SFCB_TRACE(1, ("--- Invoking Provider"));
      resp = invokeProvider(&binCtx);
      resp->rc--;
      setStatus(&st,resp->rc,(char*)resp->object[0].data);
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


#define CREATE_INST 1
extern CMPISelectExp *TempCMPISelectExp(QLStatement *qs);

CMPIStatus activateSubscription(char *principle, const char *ns, const char *cn,
             Filter *fi, int *rrc)
{
   CMPIObjectPath *path;
   CMPIStatus st={CMPI_RC_OK,NULL},rc;
   IndicationReq sreq = BINREQ(OPS_ActivateFilter, 4);
   BinResponseHdr *resp=NULL;
   BinRequestContext binCtx;
   OperationHdr req = {OPS_IndicationLookup, 2};
   int irc=0;
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "activateSubscription");
   
   if (rrc) *rrc=0;
   path = TrackedCMPIObjectPath(ns, cn, &rc);
   
   sreq.principle = setCharsMsgSegment(principle);
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.query = setCharsMsgSegment(fi->query);
   sreq.language = setCharsMsgSegment(fi->lang);
   sreq.filterId=fi;

   req.nameSpace = setCharsMsgSegment((char*) ns);
   req.className = setCharsMsgSegment((char*) cn);

   memset(&binCtx,0,sizeof(BinRequestContext));
   binCtx.oHdr = &req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=0;

   irc = getProviderContext(&binCtx, &req);
 
   if (irc == MSG_X_PROVIDER) {      
      _SFCB_TRACE(1, ("--- Invoking Provider"));
      resp = invokeProvider(&binCtx);
      resp->rc--;
      setStatus(&st,resp->rc,(char*)resp->object[0].data);
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

CMPIStatus activateLifeCycleSubscription(char *principle, const char *ns, const char *cn,
             Filter *fi, int type)
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
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "activateLifeCycleSubscription");
   
   if (cond) { 
      cm=CMGetSubCondCountAndType(cond,NULL,NULL);
      if (cm) for (c=0; c<cm; c++) {
         printf("subcondition %d\n",c);
         sc=CMGetSubCondAt(cond,c,NULL);
         int ok=0;
         if (sc) {
            for (s=0, sm=CMGetPredicateCount(sc,NULL); s<sm; s++) {
               pr=CMGetPredicateAt(sc,s,NULL);
               if (pr) {
                  CMGetPredicateData(pr,NULL,&predOp,&lhs,&rhs);
                  if (predOp==CMPI_PredOp_Isa) {
                     ok=1;
                     printf("lhs: %s\n",(char*)lhs->hdl); 
                     printf("rhs: %s\n",(char*)rhs->hdl);
                     st=activateSubscription(principle,ns,(char*)rhs->hdl,fi,&irc);
                     if (irc==MSG_X_INVALID_CLASS) 
                        st.rc=CMPI_RC_ERR_INVALID_CLASS;
                     if (st.rc!=CMPI_RC_OK) break;
                  }
               }
            }
            if (st.rc!=CMPI_RC_OK) break;
         }
         if (ok==0) {
            setStatus(&st,CMPI_RC_ERR_INVALID_QUERY,"'ISA' predicate missing in sub-condition");
            break;
         }
      }
      else setStatus(&st,CMPI_RC_ERR_INVALID_QUERY,"No sub-condition found");
   }
   else setStatus(&st,CMPI_RC_ERR_INVALID_QUERY,"CMGetDoc failed"); 
     
   free(exp);
   return st;
}


int fowardSubscription(CMPIContext * ctx, Filter *fi, char *ns, CMPIStatus *st)
{
   CMPIStatus rc;
   char *principle=NULL;
   char **fClasses=fi->qs->ft->getFromClassList(fi->qs);
   CMPIData principleP=ctx->ft->getEntry(ctx,CMPIPrinciple,&rc);
   int irc;
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "fowardSubscription");
   
   if (rc.rc==CMPI_RC_OK) 
      principle=(char*)principleP.value.string->hdl;

   for ( ; *fClasses; fClasses++) {
      if (isa(*fClasses,"cim_processindication")) {
         *st=activateSubscription(principle, ns, *fClasses, fi, &irc);
      }
      else if (isa(*fClasses,"CIM_InstCreation")) {
          printf("--- CIM_InstCreation\n");
         *st=activateLifeCycleSubscription(principle, ns, *fClasses, fi,CREATE_INST);
      }
      else {
         setStatus(st,CMPI_RC_ERR_NOT_SUPPORTED,"Lifecycle indications not supported");
        _SFCB_RETURN(-1);
     }
   }
   _SFCB_RETURN(0);
}

extern UtilStringBuffer *instanceToString(CMPIInstance * ci, char **props);

static CMPIStatus processSubscription(CMPIBroker *broker, CMPIContext *ctx, 
         CMPIInstance *ci, CMPIObjectPath *cop)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   Handler *ha;
   Filter *fi;
   Subscription *su;
   CMPIObjectPath *op;
   char *key,*skey;
   char *ns;
   CMPIDateTime *dt;
    
   _SFCB_ENTER(TRACE_INDPROVIDER, "processSubscription");
   
   ns=(char*)CMGetNameSpace(cop,NULL)->hdl;
   
   skey=internalProviderNormalizeObjectPath(cop);
   if (getSubscription(skey)) {
      free(skey);
      setStatus(&st,CMPI_RC_ERR_ALREADY_EXISTS,NULL);
      _SFCB_RETURN(st); 
   }      
      
   op=CMGetProperty(ci,"filter",&st).value.ref;
   key=internalProviderNormalizeObjectPath(op);
   fi=getFilter(key);
   free(key);
      
   op=CMGetProperty(ci,"handler",&st).value.ref;
   key=internalProviderNormalizeObjectPath(op);
   ha=getHandler(key);
   free(key);
      
   if (fi==NULL) {
      setStatus(&st,CMPI_RC_ERR_NOT_FOUND,"Filter not found");
      _SFCB_RETURN(st);
   }
   if (ha==NULL) {
      setStatus(&st,CMPI_RC_ERR_NOT_FOUND,"Handler not found");
      _SFCB_RETURN(st);
   }

   dt=CMNewDateTime(_broker,NULL);
   CMSetProperty(ci,"SubscriptionDuration",&dt,CMPI_dateTime);
         
   su=addSubscription(ci,skey,fi,ha); 
   fowardSubscription(ctx,fi,ns,&st);   
       
   if (st.rc!=CMPI_RC_OK) removeSubscription(su,skey); 
      
   _SFCB_RETURN(st); 
}

/* ------------------------------------------------------------------ *
 * InterOp initialization
 * ------------------------------------------------------------------ */

void initInterOp(CMPIBroker *broker, CMPIContext *ctx)
{
   CMPIObjectPath *op;
   UtilList *ul;
   CMPIInstance *ci;
   CMPIStatus st;
   CMPIObjectPath *cop;
   char *key,*query,*lng;
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
      qs=parseQuery(MEM_NOT_TRACKED,query,lng,&rc);
      key=internalProviderNormalizeObjectPath(cop);
      addFilter(ci,key,qs,query,lng);
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
 
/* ------------------------------------------------------------------ *
 * Instance MI Cleanup
 * ------------------------------------------------------------------ */

CMPIStatus InteropProviderCleanup(CMPIInstanceMI * mi, CMPIContext * ctx)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderCleanup");
   
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------ *
 * Instance MI Functions
 * ------------------------------------------------------------------ */


CMPIStatus InteropProviderEnumInstanceNames(CMPIInstanceMI * mi,
                                             CMPIContext * ctx,
                                             CMPIResult * rslt,
                                             CMPIObjectPath * ref)
{
   CMPIStatus st={CMPI_RC_OK,NULL};
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderEnumInstanceNames");
   if (interOpNameSpace(ref,NULL)!=1) _SFCB_RETURN(st);
   st=InternalProviderEnumInstanceNames(NULL,ctx,rslt,ref);
   _SFCB_RETURN(st);
}

CMPIStatus InteropProviderEnumInstances(CMPIInstanceMI * mi,
                                         CMPIContext * ctx,
                                         CMPIResult * rslt,
                                         CMPIObjectPath * ref,
                                         char **properties)
{
   CMPIStatus st={CMPI_RC_OK,NULL};
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderEnumInstances");
//   if (interOpNameSpace(ref,NULL)!=1) _SFCB_RETURN(st);
   st=InternalProviderEnumInstances(NULL,ctx,rslt,ref,properties);
   _SFCB_RETURN(st);
}


CMPIStatus InteropProviderGetInstance(CMPIInstanceMI * mi,
                                       CMPIContext * ctx,
                                       CMPIResult * rslt,
                                       CMPIObjectPath * cop,
                                       char **properties)
{
   CMPIStatus st;
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderGetInstance");
   st=InternalProviderGetInstance(NULL,ctx,rslt,cop,properties);
   _SFCB_RETURN(st);
}

CMPIStatus InteropProviderCreateInstance(CMPIInstanceMI * mi,
                                          CMPIContext * ctx,
                                          CMPIResult * rslt,
                                          CMPIObjectPath * cop,
                                          CMPIInstance * ci)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIString *cn = CMGetClassName(cop, NULL);
   char *cns=cn->ft->getCharPtr(cn,NULL);

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
       
      if (strcasecmp(lng,"wql")!=0) {
         setStatus(&st,CMPI_RC_ERR_QUERY_LANGUAGE_NOT_SUPPORTED,NULL);
         _SFCB_RETURN(st);  
      }   
              
      key=internalProviderNormalizeObjectPath(cop);
      if (getFilter(key)) {
         free(key);
         setStatus(&st,CMPI_RC_ERR_ALREADY_EXISTS,NULL);
         _SFCB_RETURN(st); 
      }
      
      qs=parseQuery(MEM_NOT_TRACKED,(char*)query->hdl,lng,&rc);
      if (rc) {
         free(key);
         setStatus(&st,CMPI_RC_ERR_INVALID_QUERY,"Query parse error");
         _SFCB_RETURN(st);         
      }

      addFilter(ci,key,qs,(char*)query->hdl,lng);
   }
   
   else {
      setStatus(&st,CMPI_RC_ERR_NOT_SUPPORTED,"Class not supported");
      _SFCB_RETURN(st);         
   }
    
   if (st.rc==CMPI_RC_OK) 
      st=InternalProviderCreateInstance(NULL,ctx,rslt,cop,ci);
    
   _SFCB_RETURN(st);
}

CMPIStatus InteropProviderSetInstance(CMPIInstanceMI * mi,
                                       CMPIContext * ctx,
                                       CMPIResult * rslt,
                                       CMPIObjectPath * cop,
                                       CMPIInstance * ci, char **properties)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };

   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderSetInstance");
   
   _SFCB_RETURN(st);
}

CMPIStatus InteropProviderDeleteInstance(CMPIInstanceMI * mi,
                                          CMPIContext * ctx,
                                          CMPIResult * rslt,
                                          CMPIObjectPath * cop)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIString *cn = CMGetClassName(cop, NULL);
   char *cns=cn->ft->getCharPtr(cn,NULL);
   char *key=internalProviderNormalizeObjectPath(cop);
   Filter *fi;
   Subscription *su;
   char *ns=(char*)CMGetNameSpace(cop,NULL)->hdl;

   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderDeleteInstance");
   
   if (strcasecmp(cns,"cim_indicationsubscription")==0) {
      _SFCB_TRACE(1,("--- delete cim_indicationsubscription %s",key));
      if ((su=getSubscription(key))) {
         fi=su->fi;
         if (fi->useCount==1) {
            char **fClasses=fi->qs->ft->getFromClassList(fi->qs);
            for ( ; *fClasses; fClasses++) {
               if (isa(*fClasses,"cim_processindication")) {
                  deactivateFilter(ctx, ns, *fClasses, fi);
               }
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
      }
      else setStatus(&st,CMPI_RC_ERR_NOT_FOUND,NULL);
   }
   
   else setStatus(&st,CMPI_RC_ERR_NOT_SUPPORTED,"Class not supported");
      
   if (st.rc==CMPI_RC_OK)
      st=InternalProviderDeleteInstance(NULL,ctx,rslt,cop);
   
   if (key) free(key);
   
   _SFCB_RETURN(st);
}

CMPIStatus InteropProviderExecQuery(CMPIInstanceMI * mi,
                                     CMPIContext * ctx,
                                     CMPIResult * rslt,
                                     CMPIObjectPath * cop,
                                     char *lang, char *query)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   return st;
}


/* ---------------------------------------------------------------------------*/
/*                        Method Provider Interface                           */
/* ---------------------------------------------------------------------------*/

CMPIStatus InteropProviderMethodCleanup(CMPIMethodMI * mi, CMPIContext * ctx)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   return st;
}

CMPIStatus InteropProviderInvokeMethod(CMPIMethodMI * mi,
                                     CMPIContext * ctx,
                                     CMPIResult * rslt,
                                     CMPIObjectPath * ref,
                                     char *methodName,
                                     CMPIArgs * in, CMPIArgs * out)
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
      if (ha->useCount) {
         setStatus(&st,CMPI_RC_ERR_FAILED,"Handler in use");
      } 
      else removeHandler(ha,key);   
   }

   else if (strcasecmp(methodName, "_startup") == 0) {
      initInterOp(_broker,ctx);  
   }

   else {
      _SFCB_TRACE(1,("--- Invalid request method: %s",methodName));
      st.rc = CMPI_RC_ERR_METHOD_NOT_FOUND;
   }
   
   _SFCB_RETURN(st);
}


/* ------------------------------------------------------------------ *
 * Association MI Functions
 * ------------------------------------------------------------------ */

CMPIStatus InteropProviderAssociationCleanup(CMPIInstanceMI * mi,
                                              CMPIContext * ctx)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderAssociationCleanup");
   
   _SFCB_RETURN(st);
}

CMPIStatus InteropProviderAssociators(CMPIAssociationMI * mi,
                                       CMPIContext * ctx,
                                       CMPIResult * rslt,
                                       CMPIObjectPath * cop,
                                       const char *assocClass,
                                       const char *resultClass,
                                       const char *role,
                                       const char *resultRole,
                                       char **propertyList)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderAssociators");
   
   _SFCB_RETURN(st);
}

CMPIStatus InteropProviderAssociatorNames(CMPIAssociationMI * mi,
                                           CMPIContext * ctx,
                                           CMPIResult * rslt,
                                           CMPIObjectPath * cop,
                                           const char *assocClass,
                                           const char *resultClass,
                                           const char *role,
                                           const char *resultRole)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderAssociatorNames");
   
   _SFCB_RETURN(st);
}

CMPIStatus InteropProviderReferences(CMPIAssociationMI * mi,
                                      CMPIContext * ctx,
                                      CMPIResult * rslt,
                                      CMPIObjectPath * cop,
                                      const char *assocClass,
                                      const char *role, char **propertyList)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "InteropProviderReferences");
   
   _SFCB_RETURN(st);
}


CMPIStatus InteropProviderReferenceNames(CMPIAssociationMI * mi,
                                          CMPIContext * ctx,
                                          CMPIResult * rslt,
                                          CMPIObjectPath * cop,
                                          const char *assocClass,
                                          const char *role)
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
