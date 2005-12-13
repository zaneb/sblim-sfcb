/*
 * brokerUpc.c
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
 * Author:       Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * This module implements CMPI up-call functions.
 *
*/

 
#include <stdio.h>
#include <stdlib.h>
#include <error.h>

#include "support.h"
#include "native.h"
#include "utilft.h"
#include "providerMgr.h"
#include "objectImpl.h"
#include "msgqueue.h"
#include "utilft.h"
#include "config.h"

#ifdef HAVE_INDICATIONS
#define SFCB_INCL_INDICATION_SUPPORT 1
#endif

#ifdef SFCB_INCL_INDICATION_SUPPORT 
#include "selectexp.h"
extern int indicationEnabled;
#endif

extern MsgSegment setArgsMsgSegment(const CMPIArgs * args);
extern CMPIArgs *relocateSerializedArgs(void *area);
extern UtilStringBuffer *instanceToString(CMPIInstance * ci, char **props);
extern MsgSegment setInstanceMsgSegment(const CMPIInstance * ci);
extern void memLinkObjectPath(CMPIObjectPath *op);

extern ProviderInfo *activProvs;
#ifdef SFCB_INCL_INDICATION_SUPPORT
extern NativeSelectExp *activFilters;
#endif
extern CMPIObjectPath *TrackedCMPIObjectPath(const char *nameSpace,
                                      const char *className, CMPIStatus * rc);
extern void setStatus(CMPIStatus *st, CMPIrc rc, char *msg);

void closeProviderContext(BinRequestContext * ctx);

//---------------------------------------------------
//---
//-     Thread support
//---
//---------------------------------------------------


static CMPI_MUTEX_TYPE mtx=NULL;

void lockUpCall(const CMPIBroker* mb)
{
   if (mtx==NULL) mtx=mb->xft->newMutex(0);
   mb->xft->lockMutex(mtx);
}

void unlockUpCall(const CMPIBroker* mb)
{
   mb->xft->unlockMutex(mtx);
}

static CMPIContext* prepareAttachThread(const CMPIBroker* mb, const CMPIContext* ctx) 
{
   CMPIContext *nctx;
   _SFCB_ENTER(TRACE_INDPROVIDER | TRACE_UPCALLS, "prepareAttachThread");
   nctx=native_clone_CMPIContext(ctx); 
   _SFCB_RETURN(nctx);
}

static CMPIStatus attachThread(const CMPIBroker* mb, const CMPIContext* ctx) 
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER | TRACE_UPCALLS, "attachThread");
   _SFCB_RETURN(st);
}

static CMPIStatus detachThread(const CMPIBroker* mb, const CMPIContext* ctx) 
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER | TRACE_UPCALLS, "detachThread");
    ctx->ft->release((CMPIContext*)ctx);
   _SFCB_RETURN(st);
}


static CMPIStatus deliverIndication(const CMPIBroker* mb, const CMPIContext* ctx,
       const char *ns, const CMPIInstance* ind) 
{
#ifdef SFCB_INCL_INDICATION_SUPPORT 
   
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIArgs *in=NULL;
   CMPIObjectPath *op=NULL;
   
   _SFCB_ENTER(TRACE_INDPROVIDER | TRACE_UPCALLS, "deliverIndication");
   
   if (indicationEnabled==0) {      
      _SFCB_TRACE(1,("--- Provider not enabled for indications"));
      printf("Provider not enabled for indications\n");
      setStatus(&st,CMPI_RC_ERR_FAILED, "Provider not enabled for indications");
      _SFCB_RETURN(st);
   }

   NativeSelectExp *se=activFilters;
   
   while (se) {
     if (se->exp.ft->evaluate(&se->exp,ind,&st)) {
         if (in==NULL) {
            op=CMNewObjectPath(mb,"root/interop","cim_indicationsubscription",NULL);
            in=CMNewArgs(mb,NULL);
            CMAddArg(in,"nameSpace",ns,CMPI_chars);
            CMAddArg(in,"indication",&ind,CMPI_instance);
            CMAddArg(in,"filterid",&se->filterId,CMPI_uint32);
         }  
         CBInvokeMethod(mb,ctx,op,"_deliver",in,NULL,&st);
      }
      se=se->next;
   }   
   
   _SFCB_RETURN(st);
#else 

   _SFCB_ENTER(TRACE_INDPROVIDER | TRACE_UPCALLS, "deliverIndication");
   CMPIStatus rci = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   _SFCB_RETURN(rci);
   
#endif
}

static void buildStatus(BinResponseHdr *resp, CMPIStatus *st)
{
   if (st==NULL) return;
   st->rc=resp->rc;
   if (resp->rc && resp->count==1 &&  resp->object[0].type==MSG_SEG_CHARS && resp->object[0].length) {
      st->msg=native_new_CMPIString((char*)resp->object[0].data,NULL);
   }
   else st->msg=NULL;   
}

//---------------------------------------------------
//---
//-     Instance support
//---
//---------------------------------------------------



static CMPIStatus setErrorStatus(int code)
{
   CMPIStatus st;
   char *msg;
   char m[256];

   switch (code) {
   case MSG_X_NOT_SUPPORTED:
      msg = "Operation not supported";
      code = CMPI_RC_ERR_NOT_SUPPORTED;
      break;
   case MSG_X_INVALID_CLASS:
      msg = "Class not found";
      code = CMPI_RC_ERR_INVALID_CLASS;
      break;
   case MSG_X_INVALID_NAMESPACE:
      msg = "Invalid namespace";
      code = CMPI_RC_ERR_INVALID_NAMESPACE;
      break;
   case MSG_X_PROVIDER_NOT_FOUND:
      msg = "Provider not found";
      code = CMPI_RC_ERR_FAILED;
      break;
   case MSG_X_FAILED:
      msg = "Provider Manager failed";
      code = CMPI_RC_ERR_FAILED;
      break;
   default:
      sprintf(m, "Provider Manager internal error - %d\n", code);
      code = CMPI_RC_ERR_FAILED;
      msg = m;
   }
   st.rc = code;
   st.msg = native_new_CMPIString(msg, NULL);
   return st;
}

static void setContext(BinRequestContext * binCtx, OperationHdr * oHdr,
                       BinRequestHdr * bHdr, int size, const CMPIObjectPath * cop)
{
   memset(binCtx,0,sizeof(BinRequestContext));
   oHdr->nameSpace = setCharsMsgSegment((char *)
                          ClObjectPathGetNameSpace((ClObjectPath *) cop->hdl));
   oHdr->className = setCharsMsgSegment((char *)
                          ClObjectPathGetClassName((ClObjectPath *) cop->hdl));
   bHdr->object[0] = setCharsMsgSegment("$$");
   bHdr->object[1] = setObjectPathMsgSegment(cop);
   bHdr->flags=0;

   binCtx->oHdr = oHdr;
   binCtx->bHdr = bHdr;
   binCtx->bHdrSize = size;
   binCtx->chunkedMode = 0;
}

static void cpyResult(CMPIResult *result, CMPIArray *ar, int *c)
{
   CMPIArray *r;
   int j,m;
   
   r = native_result2array(result);
   for (j=0,m=CMGetArrayCount(r,NULL); j<m; j++, *c=(*c)+1) {
      CMPIData data=CMGetArrayElementAt(r,j,NULL);
      if (*c) native_array_increase_size(ar, 1);
      CMSetArrayElementAt(ar, *c, &data.value, data.type);
   }
}      

static void cpyResponse(BinResponseHdr *resp, CMPIArray *ar, int *c, CMPIType type)
{
   int j; 
   void *tObj;
   
   for (j = 0; j < resp->count; *c=(*c)+1, j++) {
      if (*c) native_array_increase_size(ar, 1);
      if (type==CMPI_ref) {
         CMPIObjectPath *obj = relocateSerializedObjectPath(resp->object[j].data);
         tObj=obj->ft->clone(obj,NULL);
      }
      else {
         CMPIInstance *obj = relocateSerializedInstance(resp->object[j].data);
         tObj=obj->ft->clone(obj,NULL);
      }
      memLinkInstance(tObj);
      CMSetArrayElementAt(ar, *c, &tObj, type);
   }
}   




//---------------------------------------------------
//---
//-     Operations requests handlers - Instances
//---
//---------------------------------------------------


static CMPIInstance *getInstance(const CMPIBroker * broker,
                                 const CMPIContext * context,
                                 const CMPIObjectPath * cop,
                                 const char **props, CMPIStatus * rc)
{
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   GetInstanceReq sreq = BINREQ(OPS_GetInstance, 2);
   OperationHdr oHdr = { OPS_GetInstance, 2 };
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIInstance *inst = NULL,*tInst=NULL;
   int irc;
   ProviderInfo *pInfo;

   _SFCB_ENTER(TRACE_UPCALLS, "getInstance");
   
   if (cop && cop->hdl) {
   
      lockUpCall(broker);
   
      setContext(&binCtx, &oHdr, &sreq.hdr, sizeof(sreq), cop);
      _SFCB_TRACE(1,("--- for %s %s",(char*)oHdr.nameSpace.data,(char*)oHdr.className.data)); 

      irc = getProviderContext(&binCtx, &oHdr);
      
      for (pInfo = activProvs; pInfo; pInfo = pInfo->next) {
         if (pInfo->provIds.ids == binCtx.provA.ids.ids) {
            CMPIResult *result = native_new_CMPIResult(0,1,NULL);
            CMPIArray *r;
            unlockUpCall(broker);
            st = pInfo->instanceMI->ft->getInstance(pInfo->instanceMI,context,result,cop,props);
            if (rc) *rc=st;
            r = native_result2array(result);
            if (st.rc==0) 
               inst=CMGetArrayElementAt(r, 0, NULL).value.inst;
            return inst;   
         }
      }

      if (irc == MSG_X_PROVIDER) {
         resp = invokeProvider(&binCtx);
         closeProviderContext(&binCtx);
         resp->rc--;
         buildStatus(resp,&st);
         if (resp->rc == CMPI_RC_OK) {
            inst = relocateSerializedInstance(resp->object[0].data);
            tInst=inst->ft->clone(inst,NULL);
            memLinkInstance(tInst);
            free(resp);
         }
      }
      else st = setErrorStatus(irc);
      
      unlockUpCall(broker);
   }
   else st.rc = CMPI_RC_ERR_FAILED;

   if (rc) *rc = st;
   
   _SFCB_TRACE(1,("--- rc: %d",st.rc));
   
   _SFCB_RETURN(tInst);
}

static CMPIObjectPath *createInstance(const CMPIBroker * broker,
                                      const CMPIContext * context,
                                      const CMPIObjectPath * cop,
                                      const CMPIInstance * inst, CMPIStatus * rc)
{
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   CreateInstanceReq sreq = BINREQ(OPS_CreateInstance, 3);
   OperationHdr oHdr = { OPS_CreateInstance, 2 };
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIObjectPath *op = NULL,*tOp=NULL;
   int irc;
   ProviderInfo *pInfo;

   _SFCB_ENTER(TRACE_UPCALLS, "createInstance");
   
   if (cop && cop->hdl && inst && inst->hdl) {
   
      lockUpCall(broker);
   
      setContext(&binCtx, &oHdr, &sreq.hdr, sizeof(sreq), cop);
      _SFCB_TRACE(1,("--- for %s %s",(char*)oHdr.nameSpace.data,(char*)oHdr.className.data)); 

      sreq.instance = setInstanceMsgSegment(inst);
      
      irc = getProviderContext(&binCtx, &oHdr);

      for (pInfo = activProvs; pInfo; pInfo = pInfo->next) {
         if (pInfo->provIds.ids == binCtx.provA.ids.ids) {
            CMPIResult *result = native_new_CMPIResult(0,1,NULL);
            CMPIArray *r;
            unlockUpCall(broker);
            st = pInfo->instanceMI->ft->createInstance(pInfo->instanceMI,context,result,cop,inst);
            if (rc) *rc=st;
            r = native_result2array(result);
            if (st.rc==0) 
               op=CMGetArrayElementAt(r, 0, NULL).value.ref;
            return op;   
         }
      }
      
      if (irc == MSG_X_PROVIDER) {
         resp = invokeProvider(&binCtx);
         closeProviderContext(&binCtx);
         resp->rc--;
         buildStatus(resp,&st);
         if (resp->rc == CMPI_RC_OK) {
            op = relocateSerializedObjectPath(resp->object[0].data);
            tOp=op->ft->clone(op,NULL);
            memLinkObjectPath(tOp);
            free(resp);
         }
      }
      else st = setErrorStatus(irc);
      
      unlockUpCall(broker);
   }
   else st.rc = CMPI_RC_ERR_FAILED;

   if (rc) *rc = st;
   
   _SFCB_TRACE(1,("--- rc: %d",st.rc));
   
   _SFCB_RETURN(tOp);
}

static CMPIStatus modifyInstance(const CMPIBroker * broker,
                              const CMPIContext * context,
                              const CMPIObjectPath * cop,
                              const CMPIInstance * inst, const char **props)
{
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   const char **p=props;
   ModifyInstanceReq *sreq=NULL;
   OperationHdr oHdr = { OPS_ModifyInstance, 2 };
   CMPIStatus st = { CMPI_RC_OK, NULL };
   int ps,irc,sreqSize=sizeof(ModifyInstanceReq)-sizeof(MsgSegment);
   ProviderInfo *pInfo;

   _SFCB_ENTER(TRACE_UPCALLS, "modifyInstance");
   
   if (cop && cop->hdl && inst && inst->hdl) {
   
      lockUpCall(broker);
   
      for (ps=0,p=props; p && *p; p++,ps++) 
         sreqSize+=sizeof(MsgSegment);      
      sreq=(ModifyInstanceReq*)calloc(1,sreqSize);
      sreq->count=ps+3;
      sreq->operation=OPS_ModifyInstance;
      
      setContext(&binCtx, &oHdr, &sreq->hdr, sreqSize, cop);
      _SFCB_TRACE(1,("--- for %s %s",(char*)oHdr.nameSpace.data,(char*)oHdr.className.data)); 
      
      sreq->instance = setInstanceMsgSegment(inst);
      for (ps=0,p=props; p && *p; p++,ps++)   
         sreq->properties[ps]=setCharsMsgSegment(*p);

      irc = getProviderContext(&binCtx, &oHdr);

      for (pInfo = activProvs; pInfo; pInfo = pInfo->next) {
         if (pInfo->provIds.ids == binCtx.provA.ids.ids) {
            CMPIResult *result = native_new_CMPIResult(0,1,NULL);
            unlockUpCall(broker);
            st = pInfo->instanceMI->ft->modifyInstance(pInfo->instanceMI,context,result,cop,inst,props);
            if (sreq) free(sreq);
            return st;   
         }
      }
      
      if (irc == MSG_X_PROVIDER) {
         resp = invokeProvider(&binCtx);
         closeProviderContext(&binCtx);
         resp->rc--;
         buildStatus(resp,&st);
         if (resp->rc == CMPI_RC_OK) {
            free(resp);
         }
      }
      else st = setErrorStatus(irc);
      
      unlockUpCall(broker);
   }
   else st.rc = CMPI_RC_ERR_FAILED;
   if (sreq) free(sreq);

   _SFCB_TRACE(1,("--- rc: %d",st.rc));
   _SFCB_RETURN(st);
}

static CMPIStatus deleteInstance(const CMPIBroker * broker,
                                 const CMPIContext * context, const CMPIObjectPath * cop)
{
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   DeleteInstanceReq sreq = BINREQ(OPS_DeleteInstance, 2);
   OperationHdr oHdr = { OPS_DeleteInstance, 2 };
   CMPIStatus st = { CMPI_RC_OK, NULL };
   int irc;
   ProviderInfo *pInfo;

   _SFCB_ENTER(TRACE_UPCALLS, "deleteInstance");
   
   if (cop && cop->hdl) {
   
      lockUpCall(broker);
   
      setContext(&binCtx, &oHdr, &sreq.hdr, sizeof(sreq), cop);
      _SFCB_TRACE(1,("--- for %s %s",(char*)oHdr.nameSpace.data,(char*)oHdr.className.data)); 
      
      irc = getProviderContext(&binCtx, &oHdr);

      for (pInfo = activProvs; pInfo; pInfo = pInfo->next) {
         if (pInfo->provIds.ids == binCtx.provA.ids.ids) {
            CMPIResult *result = native_new_CMPIResult(0,1,NULL);
            unlockUpCall(broker);
            st = pInfo->instanceMI->ft->deleteInstance(pInfo->instanceMI,context,result,cop);
            return st;   
         }
      }
      
      if (irc == MSG_X_PROVIDER) {
         resp = invokeProvider(&binCtx);
         closeProviderContext(&binCtx);
         resp->rc--;
         buildStatus(resp,&st);
         if (resp->rc == CMPI_RC_OK) {
            free(resp);
         }
      }
      else st = setErrorStatus(irc);
      
      unlockUpCall(broker);
   }
   else st.rc = CMPI_RC_ERR_FAILED;
   
   _SFCB_TRACE(1,("--- rc: %d",st.rc));  
   _SFCB_RETURN(st);
}


static CMPIEnumeration *execQuery(const CMPIBroker * broker,
                                  const CMPIContext * context,
                                  const CMPIObjectPath * cop, const char *query,
                                  const char *lang, CMPIStatus * rc)
{
   BinRequestContext binCtx;
   ExecQueryReq sreq = BINREQ(OPS_ExecQuery, 4);
   OperationHdr oHdr = { OPS_ExecQuery, 2 };
   CMPIStatus st = { CMPI_RC_OK, NULL },rci;
   CMPIEnumeration *enm = NULL;
   CMPIArray *ar = NULL;
   int irc, i, c;

   _SFCB_ENTER(TRACE_UPCALLS, "execQuery");
   
   if (cop && cop->hdl) {
   
      lockUpCall(broker);
   
      setContext(&binCtx, &oHdr, &sreq.hdr, sizeof(sreq), cop);
      _SFCB_TRACE(1,("--- for %s %s",(char*)oHdr.nameSpace.data,(char*)oHdr.className.data)); 
      
      sreq.query = setCharsMsgSegment(query);
      sreq.queryLang = setCharsMsgSegment(lang);
      
      irc = getProviderContext(&binCtx, &oHdr);

      if (irc == MSG_X_PROVIDER) {
         BinResponseHdr *resp;
         ProviderInfo *pInfo;
         int local;
         ar = TrackedCMPIArray(1, CMPI_instance, NULL);
         for (resp=NULL, c=0, i = 0; i < binCtx.pCount; i++,binCtx.pDone++) {
            local=0;
            binCtx.provA = binCtx.pAs[i];
            for (pInfo = activProvs; pInfo; pInfo = pInfo->next) {
               if (pInfo->provIds.ids == binCtx.provA.ids.ids) {
                  CMPIResult *result = native_new_CMPIResult(0,1,NULL);
                  local=1;
                  unlockUpCall(broker);
                  rci = pInfo->instanceMI->ft->execQuery(
                     pInfo->instanceMI, context, result, cop, query, lang);
                  lockUpCall(broker);
                  if (rci.rc == CMPI_RC_OK) cpyResult(result, ar, &c);  
                  else st=rci; 
                  break;
               }
            } 
            if (local) continue;  
            
            resp = invokeProvider(&binCtx);
            
            resp->rc--;
            rci.rc = resp->rc;
            if (resp->rc == CMPI_RC_OK) cpyResponse(resp, ar, &c, CMPI_instance);
            else st=rci; 
            free (resp);
         }
         closeProviderContext(&binCtx);
         enm = native_new_CMPIEnumeration(ar, NULL);
      }  
      else st = setErrorStatus(irc);
      
      unlockUpCall(broker);
   }
   else st.rc = CMPI_RC_ERR_FAILED;
   
   if (rc) *rc = st;
   
   _SFCB_TRACE(1,("--- rc: %d",st.rc));
   _SFCB_RETURN(enm);
}

static CMPIEnumeration *enumInstances(const CMPIBroker * broker,
                                      const CMPIContext * context,
                                      const CMPIObjectPath * cop,
                                      const char **props, CMPIStatus * rc)
{
   BinRequestContext binCtx;
   EnumInstancesReq sreq = BINREQ(OPS_EnumerateInstances, 2);
   OperationHdr oHdr = { OPS_EnumerateInstances, 2 };
   CMPIStatus st = { CMPI_RC_OK, NULL },rci;
   CMPIEnumeration *enm = NULL;
   CMPIArray *ar = NULL;
   int irc, c, i;

   _SFCB_ENTER(TRACE_UPCALLS, "enumInstances");
   
   if (cop && cop->hdl) {
   
      lockUpCall(broker);
      
      setContext(&binCtx, &oHdr, &sreq.hdr, sizeof(sreq), cop);

      irc = getProviderContext(&binCtx, &oHdr);

      if (irc == MSG_X_PROVIDER) {
         BinResponseHdr *resp;
         ProviderInfo *pInfo;
         int local;
         ar = TrackedCMPIArray(1, CMPI_instance, NULL);
         for (resp=NULL, c=0, i = 0; i < binCtx.pCount; i++,binCtx.pDone++) {
            local=0;
            binCtx.provA = binCtx.pAs[i];
            for (pInfo = activProvs; pInfo; pInfo = pInfo->next) {
               if (pInfo->provIds.ids == binCtx.provA.ids.ids) {
                  CMPIResult *result = native_new_CMPIResult(0,1,NULL);
                  local=1;
                  unlockUpCall(broker);
                  rci = pInfo->instanceMI->ft->enumInstances(
                     pInfo->instanceMI, context, result, cop, props);
                  lockUpCall(broker);
                  if (rci.rc == CMPI_RC_OK) cpyResult(result, ar, &c);  
                  else st=rci; 
                  break;
               }
            } 
            if (local) continue;  
            
            resp = invokeProvider(&binCtx);
            
            resp->rc--;
            rci.rc = resp->rc;
            if (resp->rc == CMPI_RC_OK) cpyResponse(resp, ar, &c, CMPI_instance);
            else st=rci; 
            free (resp);
         }
         closeProviderContext(&binCtx);
         enm = native_new_CMPIEnumeration(ar, NULL);
      }  
      else st = setErrorStatus(irc);
      
      unlockUpCall(broker);
   }
   else st.rc = CMPI_RC_ERR_FAILED;

   if (rc) *rc = st;
   
   _SFCB_TRACE(1,("--- rc: %d",st.rc));
   _SFCB_RETURN(enm);
}

static CMPIEnumeration *enumInstanceNames(const CMPIBroker * broker,
                                          const CMPIContext * context,
                                          const CMPIObjectPath * cop, CMPIStatus * rc)
{
   BinRequestContext binCtx;
   EnumInstanceNamesReq sreq = BINREQ(OPS_EnumerateInstanceNames, 2);
   OperationHdr oHdr = { OPS_EnumerateInstanceNames, 2 };
   CMPIStatus st = { CMPI_RC_OK, NULL },rci;
   CMPIEnumeration *enm = NULL;
   CMPIArray *ar = NULL;
   int irc, c, i;

   _SFCB_ENTER(TRACE_UPCALLS, "enumInstanceNames");
   
   if (cop && cop->hdl) {
   
      lockUpCall(broker);
      
      setContext(&binCtx, &oHdr, &sreq.hdr, sizeof(sreq), cop);

      irc = getProviderContext(&binCtx, &oHdr);

      if (irc == MSG_X_PROVIDER) {
         BinResponseHdr *resp;
         ProviderInfo *pInfo;
         int local;
         ar = TrackedCMPIArray(1, CMPI_ref, NULL);
         for (resp=NULL, c=0, i = 0; i < binCtx.pCount; i++,binCtx.pDone++) {
            local=0;
            binCtx.provA = binCtx.pAs[i];
            for (pInfo = activProvs; pInfo; pInfo = pInfo->next) {
               if (pInfo->provIds.ids == binCtx.provA.ids.ids) {
                  CMPIResult *result = native_new_CMPIResult(0,1,NULL);
                  local=1;
                  unlockUpCall(broker);
                  rci = pInfo->instanceMI->ft->enumInstanceNames(
                     pInfo->instanceMI, context, result, cop);
                  lockUpCall(broker);
                  if (rci.rc == CMPI_RC_OK) cpyResult(result, ar, &c);  
                  else st=rci; 
                  break;
               }
            } 
            if (local) continue;  
            
            resp = invokeProvider(&binCtx);
            
            resp->rc--;
            rci.rc = resp->rc;
            if (resp->rc == CMPI_RC_OK) cpyResponse(resp, ar, &c, CMPI_ref);
            else st=rci; 
            free (resp);
         }
         closeProviderContext(&binCtx);
         enm = native_new_CMPIEnumeration(ar, NULL);
      }   
      else st = setErrorStatus(irc);
      unlockUpCall(broker);
   }
   else st.rc = CMPI_RC_ERR_FAILED;

   _SFCB_TRACE(1,("--- rc: %d",st.rc));
   _SFCB_RETURN(enm);
}



//---------------------------------------------------
//---
//-     Operations requests handlers - Associations
//---
//---------------------------------------------------



static CMPIEnumeration *associators(const CMPIBroker * broker,
                                    const CMPIContext * context,
                                    const CMPIObjectPath * cop,
                                    const char *assocclass,
                                    const char *resultclass,
                                    const char *role,
                                    const char *resultrole, const char **props,
                                    CMPIStatus * rc)
{
   CMPIStatus rci = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   if (rc)
      *rc = rci;
   return NULL;
}

static CMPIEnumeration *associatorNames(const CMPIBroker * broker,
                                        const CMPIContext * context,
                                        const CMPIObjectPath * cop,
                                        const char *assocclass,
                                        const char *resultclass,
                                        const char *role,
                                        const char *resultrole, CMPIStatus * rc)
{
   CMPIStatus rci = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   if (rc)
      *rc = rci;
   return NULL;
}

static CMPIEnumeration *references(const CMPIBroker * broker,
                                   const CMPIContext * context,
                                   const CMPIObjectPath * cop,
                                   const char *assocclass,
                                   const char *role, const char **props,
                                   CMPIStatus * rc)
{
   CMPIStatus rci = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   if (rc)
      *rc = rci;
   return NULL;
}

static CMPIEnumeration *referenceNames(const CMPIBroker * broker,
                                       const CMPIContext * context,
                                       const CMPIObjectPath * cop,
                                       const char *assocclass,
                                       const char *role, CMPIStatus * rc)
{
   CMPIStatus rci = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   if (rc)
      *rc = rci;
   return NULL;
}



//---------------------------------------------------
//---
//-     Operations requests handlers - Method
//---
//---------------------------------------------------

struct native_args {
   CMPIArgs args;   
   int mem_state;   
};


static CMPIData invokeMethod(const CMPIBroker * broker, const CMPIContext * context,
                             const CMPIObjectPath * cop, const char *method,
                             const CMPIArgs * in, CMPIArgs * out, CMPIStatus * rc)
{
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   InvokeMethodReq *sreq; // = BINREQ(OPS_InvokeMethod, 5);
   OperationHdr oHdr = { OPS_InvokeMethod, 2 };
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIArgs *tOut;
   int irc,i,s,x=0,size,n;
   CMPIString *name;
   CMPIData rv = { 0, CMPI_nullValue, {0} };

   if (cop && cop->hdl) {
   
      for (i=0,s=CMGetArgCount(in,NULL); i<s; i++) {
         CMPIData d=CMGetArgAt(in,i,NULL,NULL);
         if (d.type==CMPI_instance) x++;
      }
   
      size=sizeof(InvokeMethodReq)+(x*sizeof(MsgSegment));
      sreq=(InvokeMethodReq*)calloc(1,size);
      sreq->count=5+x;
      sreq->operation=OPS_InvokeMethod;
      
      lockUpCall(broker);
      
      setContext(&binCtx, &oHdr, &sreq->hdr, size, cop);
      
      sreq->in = setArgsMsgSegment(in);
      sreq->out = setArgsMsgSegment(NULL);
      sreq->method = setCharsMsgSegment(method);
      
      if (x) for (n=5,i=0,s=CMGetArgCount(in,NULL); i<s; i++) {
         CMPIData d=CMGetArgAt(in,i,NULL,NULL);
         BinRequestHdr *req=(BinRequestHdr*)sreq;
         if (d.type==CMPI_instance) {
            req->object[n++]=setInstanceMsgSegment(d.value.inst);
         }
      }

      irc = getProviderContext(&binCtx, &oHdr);

      if (irc == MSG_X_PROVIDER) {

         resp = invokeProvider(&binCtx);
         closeProviderContext(&binCtx);
         
         resp->rc--;
         buildStatus(resp,&st);
         if (resp->rc == CMPI_RC_OK) {
            if (out) {
               tOut = relocateSerializedArgs(resp->object[0].data);
              for (i=0,s=tOut->ft->getArgCount(tOut,NULL); i<s; i++) {
                  CMPIData data=tOut->ft->getArgAt(tOut,i,&name,NULL);
                  out->ft->addArg(out,CMGetCharPtr(name),&data.value,data.type);
               }
            }
            rv=resp->rv;
            free(resp);
         }
      }
      else st = setErrorStatus(irc);
      
      unlockUpCall(broker);
      if (sreq) free(sreq);
   }
   else st.rc = CMPI_RC_ERR_FAILED;

   if (rc) *rc = st;
   return rv;
   
}



//---------------------------------------------------
//---
//-     Operations requests handlers - Property
//---
//---------------------------------------------------



static CMPIStatus setProperty(const CMPIBroker * broker,
                              const CMPIContext * context,
                              const CMPIObjectPath * cop, const char *name,
                              const CMPIValue * value, CMPIType type)
{
   CMPIStatus rci = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   return rci;
}

static CMPIData getProperty(const CMPIBroker * broker, const CMPIContext * context,
                            const CMPIObjectPath * cop, const char *name,
                            CMPIStatus * rc)
{
   CMPIStatus rci = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   CMPIData rv = { 0, CMPI_nullValue, {0} };
   if (rc)
      *rc = rci;
   return rv;
}


static CMPIBrokerFT request_FT = {
   0, 0,
   "RequestHandler",
   prepareAttachThread,
   attachThread,
   detachThread,
   deliverIndication,
   enumInstanceNames,
   getInstance,
   createInstance,
   modifyInstance,
   deleteInstance,
   execQuery,
   enumInstances,
   associators,
   associatorNames,
   references,
   referenceNames,
   invokeMethod,
   setProperty,
   getProperty
};

CMPIBrokerFT *RequestFT = &request_FT;

extern CMPIBrokerExtFT brokerExt_FT;

static CMPIBroker _broker = {
   NULL,
   &request_FT,
   &native_brokerEncFT,
   &brokerExt_FT
};

CMPIBroker *Broker = &_broker;
