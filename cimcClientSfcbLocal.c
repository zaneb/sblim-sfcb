
/*
 * cmpic.h
 *
 * (C) Copyright IBM Corp. 2005
 * (C) Copyright Intel Corp. 2005
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
 * CMPI Client function tables.
 *
 */

#include "cimcClientSfcbLocal.h"
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include "providerMgr.h"
#include "queryOperation.h"

 
#define CMCISetStatusWithChars(st,rcp,chars) \
      { if (st) { (st)->rc=(rcp); \
        (st)->msg=NewCMPIString((chars),NULL); }}
        
#define NewCMPIString sfcb_native_new_CMPIString
   
extern unsigned long _sfcb_trace_mask; 

extern void closeProviderContext(BinRequestContext* binCtx);
extern MsgSegment setObjectPathMsgSegment(const CMPIObjectPath * op);
extern MsgSegment setInstanceMsgSegment(const CMPIInstance * inst);
extern CMPIConstClass *relocateSerializedConstClass(void *area);
extern CMPIObjectPath *relocateSerializedObjectPath(void *area);
extern CMPIInstance *relocateSerializedInstance(void *area);

extern CMPIString *NewCMPIString(const char*, CMPIStatus*);
extern CMPIObjectPath *NewCMPIObjectPath(const char*ns, const char* cn, CMPIStatus*);
extern CMPIInstance *NewCMPIInstance(CMPIObjectPath * cop, CMPIStatus * rc);
extern CMPIArray *NewCMPIArray(CMPICount size, CMPIType type, CMPIStatus * rc);
extern CMPIEnumeration *sfcb_native_new_CMPIEnumeration(CMPIArray * array, CMPIStatus * rc);
extern CMPIString *NewCMPIString(const char *ptr, CMPIStatus * rc);
extern CMPIEnumeration *NewCMPIEnumeration(CMPIArray * array, CMPIStatus * rc);

extern CMPIArgs* NewCMPIArgs(CMPIStatus* rc);
extern CMPIDateTime *NewCMPIDateTime(CMPIStatus *rc);
extern CMPIDateTime *NewCMPIDateTimeFromBinary(CMPIUint64 binTime, CMPIBoolean interval, CMPIStatus *rc);
extern CMPIDateTime *NewCMPIDateTimeFromChars(const char *utcTime, CMPIStatus *rc);

extern int localClientMode;

#include <stdlib.h>
#include <string.h>

#ifdef DEBUG
#undef DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#if DEBUG
int do_debug = 0;	/* simple enable debug messages flag */

static void set_debug()
{
    static int firsttime = 1;
    char *dbg;
    if (firsttime == 1) {
	firsttime--;
        do_debug = ((dbg = getenv("CMPISFCC_DEBUG")) != NULL &&
		    strcasecmp(dbg, "true") == 0);
    }
}
#endif

typedef const struct _ClientConnectionFT {
    CMPIStatus (*release) (ClientConnection *);
} ClientConnectionFT;

struct _ClientEnc {
   Client          enc;
   ClientData      data;
   CredentialData  certData;
   ClientConnection *connection;
};

struct _ClientConnection {
    ClientConnectionFT *ft;        // The handle to the curl object
};

char *getResponse(ClientConnection *con, CMPIObjectPath *cop);
ClientConnection *initConnection(ClientData *cld);

/* --------------------------------------------------------------------------*/


static CMPIStatus releaseConnection(ClientConnection *con)
{
  CMPIStatus rc = {CMPI_RC_OK,NULL};
  free(con);
  return rc;
}


static ClientConnectionFT conFt={
  releaseConnection
};

/* --------------------------------------------------------------------------*/

ClientConnection *initConnection(ClientData *cld)
{
   ClientConnection *c=(ClientConnection*)calloc(1,sizeof(ClientConnection));
   c->ft=&conFt;
   return c;
}

/*--------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/

static Client * cloneClient ( Client * cl, CMPIStatus * st )
{
  CMPIStatus rc;
  CMCISetStatusWithChars(&rc, CMPI_RC_ERR_NOT_SUPPORTED, "Clone function not supported");
  if (st) *st=rc;
  return NULL;
}  

static CMPIStatus releaseClient(Client * mb)
{
  CMPIStatus rc={CMPI_RC_OK,NULL};
  ClientEnc		* cl = (ClientEnc*)mb;
  
  if (cl->data.hostName) {
    free(cl->data.hostName);
  }
  if (cl->data.user) {
    free(cl->data.user);
  }
  if (cl->data.pwd) {
    free(cl->data.pwd);
  }
  if (cl->data.scheme) {
    free(cl->data.scheme);
  }
  if (cl->data.port) {
    free(cl->data.port);
  }
  if (cl->certData.trustStore) {
    free(cl->certData.trustStore);
  }
  if (cl->certData.certFile) {
    free(cl->certData.certFile);
  }
  if (cl->certData.keyFile) {
    free(cl->certData.keyFile);
  }
 
  if (cl->connection) CMRelease(cl->connection);

  free(cl);
  return rc;
}

void freeResps(BinResponseHdr **resp, int count)
{
   if (resp && count) while (count)
      free(resp[(count--)-1]);
   if (resp) free(resp);
}

extern void setEnumArray(CMPIEnumeration * enumeration,CMPIArray * array);

static CMPIEnumeration *cpyEnumResponses(BinRequestContext * binCtx,
                                 BinResponseHdr ** resp,
                                 int arrLen)
{
   int i, c, j;
   union o {
     CMPIInstance *inst;
     CMPIObjectPath *path;
     CMPIConstClass *cls;
   } object;
   CMPIArray *ar,*art;
   CMPIEnumeration *enm;
   CMPIStatus rc;

   _SFCB_ENTER(TRACE_CIMXMLPROC, "genEnumResponses");

   ar = NewCMPIArray(arrLen, binCtx->type, NULL);
   art = NewCMPIArray(0, binCtx->type, NULL);

   for (c = 0, i = 0; i < binCtx->rCount; i++) {
      for (j = 0; j < resp[i]->count; c++, j++) {
         if (binCtx->type == CMPI_ref)
            object.path = relocateSerializedObjectPath(resp[i]->object[j].data);
         else if (binCtx->type == CMPI_instance)
            object.inst = relocateSerializedInstance(resp[i]->object[j].data);
         else if (binCtx->type == CMPI_class) {
            object.cls = relocateSerializedConstClass(resp[i]->object[j].data);
         }
    //     object.inst=CMClone(object.inst,NULL);
         rc=CMSetArrayElementAt(ar,c, (CMPIValue*)&object.inst, binCtx->type);
      }
   }

   enm = NewCMPIEnumeration(art, NULL);
   setEnumArray(enm,ar);
   art->ft->release(art);

   _SFCB_RETURN(enm);
}

static void ctxErrResponse( BinRequestContext * ctx, CMPIStatus *rc)
{
   MsgXctl *xd = ctx->ctlXdata;
   char msg[256],*m;
   int r=CMPI_RC_ERR_FAILED;

   switch (ctx->rc) {
   case MSG_X_NOT_SUPPORTED:
      m = "Operation not supported yy";
      break;
   case MSG_X_INVALID_CLASS:
      m = "Class not found";
      r = CMPI_RC_ERR_INVALID_CLASS;
      break;
   case MSG_X_INVALID_NAMESPACE:
      m = "Invalid namespace";
      r = CMPI_RC_ERR_INVALID_NAMESPACE;
      break;
   case MSG_X_PROVIDER_NOT_FOUND:
      m = "Provider not found or not loadable";
      r = CMPI_RC_ERR_NOT_FOUND;
      break;
   case MSG_X_FAILED:
      m = xd->data;
      break;
   default:
      sprintf(msg, "Internal error - %d\n", ctx->rc);
      m = msg;
   }
   
   if (rc) CMCISetStatusWithChars(rc,r,m);
}

static CMPIEnumeration * enumInstanceNames(
	Client * mb,
	CMPIObjectPath * cop,
	CMPIStatus * rc)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "enumInstanceNames");
   EnumInstanceNamesReq sreq = BINREQ(OPS_EnumerateInstanceNames, 2);
   int irc, l = 0, err = 0;
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   OperationHdr oHdr={OPS_EnumerateInstanceNames,0,2};
   CMPIEnumeration *enm = NULL;

   if (rc) CMSetStatus(rc, CMPI_RC_OK);

   CMPIString *ns=cop->ft->getNameSpace(cop,NULL);
   CMPIString *cn=cop->ft->getClassName(cop,NULL);
   
   oHdr.nameSpace=setCharsMsgSegment((char*)ns->hdl);
   oHdr.className=setCharsMsgSegment((char*)cn->hdl);

   memset(&binCtx,0,sizeof(BinRequestContext));

   sreq.objectPath = setObjectPathMsgSegment(cop);
   sreq.principal = setCharsMsgSegment("principal");

   binCtx.oHdr = (OperationHdr *) &oHdr;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = 0;
   binCtx.type=CMPI_ref;
   binCtx.rHdr = NULL;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) &oHdr);

   CMRelease(ns);
   CMRelease(cn);
   
   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      _SFCB_TRACE(1, ("--- Back from Provider"));

      closeProviderContext(&binCtx);
      if (err == 0) {
         enm=cpyEnumResponses(&binCtx,resp,l);
         freeResps(resp,binCtx.pCount);
         _SFCB_RETURN(enm);
      }
      if (rc) CMCISetStatusWithChars(rc, resp[err-1]->rc, 
                  (char*)resp[err-1]->object[0].data);
      if (resp) freeResps(resp,binCtx.pCount);
      _SFCB_RETURN(NULL);
   }
   else ctxErrResponse(&binCtx,rc);
   closeProviderContext(&binCtx);
   
   _SFCB_RETURN(NULL);
}


static CMPIEnumeration * enumInstances(
	Client * mb,
	CMPIObjectPath * cop,
	CMPIFlags flags,
	char ** properties,
	CMPIStatus * rc)
{
   EnumInstancesReq *sreq;
   int pCount=0, irc, l = 0, err = 0,sreqSize=sizeof(EnumInstancesReq)-sizeof(MsgSegment);
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   OperationHdr oHdr={OPS_EnumerateInstances,0,2};
   CMPIEnumeration *enm = NULL;
   
   _SFCB_ENTER(TRACE_CIMXMLPROC, "enumInstances");
   
   if (rc) CMSetStatus(rc, CMPI_RC_OK);

   CMPIString *ns=cop->ft->getNameSpace(cop,NULL);
   CMPIString *cn=cop->ft->getClassName(cop,NULL);
   
   oHdr.nameSpace=setCharsMsgSegment((char*)ns->hdl);
   oHdr.className=setCharsMsgSegment((char*)cn->hdl);

   memset(&binCtx,0,sizeof(BinRequestContext));

   if (properties) {
      char **p;
      for (p=properties; *p; p++) pCount++;
   }
   
   sreqSize+=pCount*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->hdr.operation=OPS_EnumerateInstances;
   sreq->hdr.count=pCount+2;

   sreq->objectPath = setObjectPathMsgSegment(cop);
   sreq->principal = setCharsMsgSegment("principal");

   binCtx.oHdr = (OperationHdr *) &oHdr;
   binCtx.bHdr = &sreq->hdr;
   binCtx.bHdr->flags = flags;
   binCtx.type=CMPI_instance;
   binCtx.rHdr = NULL;
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) &oHdr);
   
   CMRelease(ns);
   CMRelease(cn);
   
   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      closeProviderContext(&binCtx);

      if (err == 0) {
         enm=cpyEnumResponses(&binCtx,resp,l);
         freeResps(resp,binCtx.pCount);
         free(sreq);
         _SFCB_RETURN(enm);
      }
      if (rc) CMCISetStatusWithChars(rc, resp[err-1]->rc, 
                  (char*)resp[err-1]->object[0].data);
      if (resp) freeResps(resp,binCtx.pCount);
         free(sreq);
      _SFCB_RETURN(NULL);
   }
   else ctxErrResponse(&binCtx,rc);
   closeProviderContext(&binCtx);
   free(sreq);
   
   _SFCB_RETURN(NULL);

}


/* --------------------------------------------------------------------------*/

static CMPIInstance * getInstance(
	Client * mb,
	CMPIObjectPath * cop,
	CMPIFlags flags,
	char ** properties,
	CMPIStatus * rc)
{
   CMPIInstance *inst;
   int irc, i, sreqSize=sizeof(GetInstanceReq)-sizeof(MsgSegment);
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   GetInstanceReq *sreq;
   int pCount=0;
   OperationHdr oHdr={OPS_GetInstance,0,2};
   
   _SFCB_ENTER(TRACE_CIMXMLPROC, "getInstance");
   
   if (rc) CMSetStatus(rc, CMPI_RC_OK);

   CMPIString *ns=cop->ft->getNameSpace(cop,NULL);
   CMPIString *cn=cop->ft->getClassName(cop,NULL);
   
   oHdr.nameSpace=setCharsMsgSegment((char*)ns->hdl);
   oHdr.className=setCharsMsgSegment((char*)cn->hdl);

   memset(&binCtx,0,sizeof(BinRequestContext));

   if (properties) {
      char **p;
      for (p=properties; *p; p++) pCount++;
   }
   
   sreqSize+=pCount*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->hdr.operation=OPS_GetInstance;
   sreq->hdr.count=pCount+2;

   sreq->objectPath = setObjectPathMsgSegment(cop);
   sreq->principal = setCharsMsgSegment("principal");

   for (i=0; i<pCount; i++)
      sreq->properties[i]=setCharsMsgSegment(properties[i]);

   binCtx.oHdr = (OperationHdr *) &oHdr;
   binCtx.bHdr = &sreq->hdr;
   binCtx.bHdr->flags = flags;
   binCtx.rHdr = NULL;
   binCtx.bHdrSize = sreqSize;
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) &oHdr);

   CMRelease(ns);
   CMRelease(cn);
   
   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Provider"));
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         inst = relocateSerializedInstance(resp->object[0].data);
         inst=inst->ft->clone(inst,NULL);
         free(sreq);
         free(resp);
         _SFCB_RETURN(inst);
      }
      free(sreq);
      if (rc) CMCISetStatusWithChars(rc, resp->rc, (char*)resp->object[0].data);
      free(resp);
      _SFCB_RETURN(NULL);
   }
   else ctxErrResponse(&binCtx,rc);
   free(sreq);
   closeProviderContext(&binCtx);

   _SFCB_RETURN(NULL);
}


/* --------------------------------------------------------------------------*/

static CMPIObjectPath * createInstance(
	Client * mb,
	CMPIObjectPath * cop,
	CMPIInstance * inst,
	CMPIStatus * rc)
{
   int irc;
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   CreateInstanceReq sreq = BINREQ(OPS_CreateInstance, 3);
   CMPIObjectPath *path;
   OperationHdr oHdr={OPS_CreateInstance,0,3};

   _SFCB_ENTER(TRACE_CIMXMLPROC, "createInst");
   
   if (rc) CMSetStatus(rc, CMPI_RC_OK);

   CMPIString *ns=cop->ft->getNameSpace(cop,NULL);
   CMPIString *cn=cop->ft->getClassName(cop,NULL);
   
   oHdr.nameSpace=setCharsMsgSegment((char*)ns->hdl);
   oHdr.className=setCharsMsgSegment((char*)cn->hdl);

   memset(&binCtx,0,sizeof(BinRequestContext));
                  
   sreq.principal = setCharsMsgSegment("principal");
   sreq.path = setObjectPathMsgSegment(cop);
   sreq.instance = setInstanceMsgSegment(inst);

   binCtx.oHdr = (OperationHdr *) &oHdr;
   binCtx.bHdr = &sreq.hdr;
   binCtx.rHdr = NULL;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) &oHdr);

   CMRelease(ns);
   CMRelease(cn);
   
   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Provider"));
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         path = relocateSerializedObjectPath(resp->object[0].data);
         path=path->ft->clone(path,NULL);
         free(resp);
         _SFCB_RETURN(path);
      }
      if (rc) CMCISetStatusWithChars(rc, resp->rc, (char*)resp->object[0].data);
      free(resp);
      _SFCB_RETURN(NULL);
   }
   else ctxErrResponse(&binCtx,rc);
   closeProviderContext(&binCtx);

   _SFCB_RETURN(NULL);
}


/* --------------------------------------------------------------------------*/

static CMPIStatus modifyInstance(
	Client * mb,
	CMPIObjectPath * cop,
	CMPIInstance * inst,
	CMPIFlags flags,
	char ** properties)
{
   int pCount=0, irc, i, sreqSize=sizeof(ModifyInstanceReq)-sizeof(MsgSegment);
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   ModifyInstanceReq *sreq;
   OperationHdr oHdr={OPS_ModifyInstance,0,2};
   CMPIStatus rc={0,NULL};
    
   _SFCB_ENTER(TRACE_CIMXMLPROC, "setInstance");
   
   CMPIString *ns=cop->ft->getNameSpace(cop,NULL);
   CMPIString *cn=cop->ft->getClassName(cop,NULL);
   
   oHdr.nameSpace=setCharsMsgSegment((char*)ns->hdl);
   oHdr.className=setCharsMsgSegment((char*)cn->hdl);

   memset(&binCtx,0,sizeof(BinRequestContext));

   if (properties) {
      char **p;
      for (p=properties; *p; p++) pCount++;
   }
   
   sreqSize+=pCount*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   
   for (i=0; i<pCount; i++)
      sreq->properties[i]=setCharsMsgSegment(properties[i]);

   sreq->hdr.operation=OPS_ModifyInstance;
   sreq->hdr.count=pCount+3;

   sreq->instance = setInstanceMsgSegment(inst);
   sreq->path = setObjectPathMsgSegment(cop);
   sreq->principal = setCharsMsgSegment("principal");

   binCtx.oHdr = (OperationHdr *) &oHdr;
   binCtx.bHdr = &sreq->hdr;
   binCtx.rHdr = NULL;
   binCtx.bHdrSize = sreqSize;
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) &oHdr);

   CMRelease(ns);
   CMRelease(cn);
   
   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Provider"));
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         free(sreq);
         free(resp);
         _SFCB_RETURN(rc);
      }
      free(sreq);
      CMCISetStatusWithChars(&rc, resp->rc, (char*)resp->object[0].data);
      free(resp);
      _SFCB_RETURN(rc);
   }
   else ctxErrResponse(&binCtx,&rc);
   free(sreq);
   closeProviderContext(&binCtx);

   _SFCB_RETURN(rc);
}

static CMPIStatus deleteInstance(
	Client * mb,
	CMPIObjectPath * cop)
{
   int irc;
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   DeleteInstanceReq sreq = BINREQ(OPS_DeleteInstance, 2);
   OperationHdr oHdr={OPS_DeleteInstance,0,2};
   CMPIStatus rc={0,NULL};
    
   _SFCB_ENTER(TRACE_CIMXMLPROC, "deleteInstance");
   
   CMPIString *ns=cop->ft->getNameSpace(cop,NULL);
   CMPIString *cn=cop->ft->getClassName(cop,NULL);
   
   oHdr.nameSpace=setCharsMsgSegment((char*)ns->hdl);
   oHdr.className=setCharsMsgSegment((char*)cn->hdl);

   memset(&binCtx,0,sizeof(BinRequestContext));

   sreq.objectPath = setObjectPathMsgSegment(cop);
   sreq.principal = setCharsMsgSegment("principal");

   binCtx.oHdr = (OperationHdr *) &oHdr;
   binCtx.bHdr = &sreq.hdr;
   binCtx.rHdr = NULL;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) &oHdr);

   CMRelease(ns);
   CMRelease(cn);
   
   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Provider"));
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         free(resp);
         _SFCB_RETURN(rc);
      }
      CMCISetStatusWithChars(&rc, resp->rc, (char*)resp->object[0].data);
      free(resp);
      _SFCB_RETURN(rc);
   }
   else ctxErrResponse(&binCtx,&rc);
   closeProviderContext(&binCtx);
   
   _SFCB_RETURN(rc);
}


static CMPIEnumeration * execQuery(
	Client * mb,
	CMPIObjectPath * cop,
	const char * query,
	const char * lang,
	CMPIStatus * rc)
{
   ExecQueryReq sreq = BINREQ(OPS_ExecQuery, 4);
   int irc, l = 0, err = 0;
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   QLStatement *qs=NULL;
   char **fCls;
   OperationHdr oHdr={OPS_ExecQuery,0,4};
	CMPIObjectPath * path;
   CMPIEnumeration *enm;
    
   _SFCB_ENTER(TRACE_CIMXMLPROC, "execQuery");

   CMPIString *ns=cop->ft->getNameSpace(cop,NULL);
   
   oHdr.nameSpace=setCharsMsgSegment((char*)ns->hdl);
   qs=parseQuery(MEM_TRACKED,query,lang,NULL,&irc);
   
   fCls=qs->ft->getFromClassList(qs);
   if (fCls==NULL || *fCls==NULL) {
     mlogf(M_ERROR,M_SHOW,"--- from clause missing\n");
     abort();
   }
   oHdr.className = setCharsMsgSegment(*fCls);

   path = NewCMPIObjectPath((char*)ns->hdl, *fCls, NULL);
   CMRelease(ns);
   
   memset(&binCtx,0,sizeof(BinRequestContext));

   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principal = setCharsMsgSegment("principal");
   sreq.query = setCharsMsgSegment(query);
   sreq.queryLang = setCharsMsgSegment(lang);

   binCtx.oHdr = (OperationHdr *) &oHdr;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = 0;
   binCtx.rHdr = NULL;
   binCtx.type=CMPI_instance;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;
   
   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) &oHdr);

   
   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      closeProviderContext(&binCtx);

      if (err == 0) {
         enm=cpyEnumResponses(&binCtx,resp,l);
         freeResps(resp,binCtx.pCount);
         _SFCB_RETURN(enm);
      }
      if (rc) CMCISetStatusWithChars(rc, resp[err-1]->rc, 
                  (char*)resp[err-1]->object[0].data);
      freeResps(resp,binCtx.pCount);
      _SFCB_RETURN(NULL);
   }
   else ctxErrResponse(&binCtx,rc);
   closeProviderContext(&binCtx);
    
   _SFCB_RETURN(NULL);
}	
	
static CMPIEnumeration * associators(
	Client	* mb,
	CMPIObjectPath	* cop,
	const char	* assocClass,
	const char	* resultClass,
	const char	* role,
	const char	* resultRole,
	CMPIFlags	flags,
	char		** properties,
	CMPIStatus	* rc)
{
   AssociatorsReq *sreq; 
   int irc, pCount=0, i, l = 0, err = 0, sreqSize=sizeof(AssociatorsReq)-sizeof(MsgSegment);
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   OperationHdr oHdr={OPS_Associators,0,6};
   CMPIEnumeration *enm = NULL;
   
   _SFCB_ENTER(TRACE_CIMXMLPROC, "associators");
   
   if (rc) CMSetStatus(rc, CMPI_RC_OK);

   CMPIString *ns=cop->ft->getNameSpace(cop,NULL);
   CMPIString *cn=cop->ft->getClassName(cop,NULL);
   
   oHdr.nameSpace=setCharsMsgSegment((char*)ns->hdl);
   oHdr.className=setCharsMsgSegment((char*)cn->hdl);

   if (properties) {
      char **p;
      for (p=properties; *p; p++) pCount++;
   }
   
   sreqSize+=pCount*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   
   memset(&binCtx,0,sizeof(BinRequestContext));

   if (pCount) sreqSize+=pCount*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->hdr.operation=OPS_Associators;
   sreq->hdr.count=pCount+6;
   
   sreq->objectPath = setObjectPathMsgSegment(cop);
   sreq->resultClass = setCharsMsgSegment(resultClass);
   sreq->role = setCharsMsgSegment(role);
   sreq->assocClass = setCharsMsgSegment(assocClass);
   sreq->resultRole = setCharsMsgSegment(resultRole);
   sreq->hdr.flags = flags;
   sreq->principal = setCharsMsgSegment("principal");

   for (i=0; i<pCount; i++)
      sreq->properties[i]=setCharsMsgSegment(properties[i]);

   oHdr.className = sreq->assocClass;

   binCtx.oHdr = (OperationHdr *) &oHdr;
   binCtx.bHdr = &sreq->hdr;
   binCtx.bHdr->flags = flags;
   binCtx.rHdr = NULL;
   binCtx.bHdrSize = sreqSize;
   binCtx.type=CMPI_instance;
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;
   
   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) &oHdr);

   CMRelease(ns);
   CMRelease(cn);
   
   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Provider"));
      resp = invokeProviders(&binCtx, &err, &l);
      closeProviderContext(&binCtx);
      
      if (err == 0) {
         enm=cpyEnumResponses(&binCtx,resp,l);
         freeResps(resp,binCtx.pCount);
	      free(sreq);
         _SFCB_RETURN(enm);
      }
      if (rc) CMCISetStatusWithChars(rc, resp[err-1]->rc, 
                  (char*)resp[err-1]->object[0].data);
      if (resp) freeResps(resp,binCtx.pCount);
	   free(sreq);
      _SFCB_RETURN(NULL);
   }
   else ctxErrResponse(&binCtx,rc);
   free(sreq);
   closeProviderContext(&binCtx);
    
   _SFCB_RETURN(NULL);
}


static CMPIEnumeration * references(
	Client * mb,
	CMPIObjectPath * cop,
	const char * resultClass,
	const char * role ,
	CMPIFlags flags,
	char ** properties,
	CMPIStatus * rc)
{
   AssociatorsReq *sreq; 
   int irc, pCount=0, i,l = 0, err = 0, sreqSize=sizeof(AssociatorsReq)-sizeof(MsgSegment);
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   OperationHdr oHdr={OPS_References,0,4};
   CMPIEnumeration *enm = NULL;

   _SFCB_ENTER(TRACE_CIMXMLPROC, "references");

   if (rc) CMSetStatus(rc, CMPI_RC_OK);

   memset(&binCtx,0,sizeof(BinRequestContext));
   
   CMPIString *ns=cop->ft->getNameSpace(cop,NULL);
   CMPIString *cn=cop->ft->getClassName(cop,NULL);
   
   oHdr.nameSpace=setCharsMsgSegment((char*)ns->hdl);
   oHdr.className=setCharsMsgSegment((char*)cn->hdl);

   if (properties) {
      char **p;
      for (p=properties; *p; p++) pCount++;
   }
   
   sreqSize+=pCount*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->hdr.operation=OPS_References;
   sreq->hdr.count=pCount+4;

   
   sreq->objectPath = setObjectPathMsgSegment(cop);
   sreq->resultClass = setCharsMsgSegment(resultClass);
   sreq->role = setCharsMsgSegment(role);
   sreq->hdr.flags = flags;
   sreq->principal = setCharsMsgSegment("principal");

   for (i=0; i<pCount; i++)
      sreq->properties[i]=setCharsMsgSegment(properties[i]);

   oHdr.className = sreq->resultClass;

   binCtx.oHdr = (OperationHdr *) &oHdr;
   binCtx.bHdr = &sreq->hdr;
   binCtx.bHdr->flags = flags;
   binCtx.rHdr = NULL;
   binCtx.bHdrSize = sreqSize;
   binCtx.type=CMPI_instance;
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;
   
   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) &oHdr);

   CMRelease(ns);
   CMRelease(cn);
   
   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Provider"));
      resp = invokeProviders(&binCtx, &err, &l);
      closeProviderContext(&binCtx);
      
      if (err == 0) {
         enm=cpyEnumResponses(&binCtx,resp,l);
         freeResps(resp,binCtx.pCount);
	      free(sreq);
         _SFCB_RETURN(enm);
      }
      if (rc) CMCISetStatusWithChars(rc, resp[err-1]->rc, 
                  (char*)resp[err-1]->object[0].data);
      if (resp) freeResps(resp,binCtx.pCount);
	   free(sreq);
      _SFCB_RETURN(NULL);
   }
   else ctxErrResponse(&binCtx,rc);
   free(sreq);
   closeProviderContext(&binCtx);
    
   _SFCB_RETURN(NULL);
}


static CMPIEnumeration * associatorNames(
	Client	* mb,
	CMPIObjectPath	* cop,
	const char	* assocClass,
	const char	* resultClass,
	const char	* role,
	const char	* resultRole,
	CMPIStatus	* rc)
{
   AssociatorNamesReq sreq = BINREQ(OPS_AssociatorNames, 6);
   int irc, l = 0, err = 0;
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   OperationHdr oHdr={OPS_AssociatorNames,0,6};
   CMPIEnumeration *enm = NULL;
   
   _SFCB_ENTER(TRACE_CIMXMLPROC, "associatorNames");

   memset(&binCtx,0,sizeof(BinRequestContext));
   
   sreq.objectPath = setObjectPathMsgSegment(cop);

   sreq.resultClass = setCharsMsgSegment(resultClass);
   sreq.role = setCharsMsgSegment(role);
   sreq.assocClass = setCharsMsgSegment(assocClass);
   sreq.resultRole = setCharsMsgSegment(resultRole);
   sreq.principal = setCharsMsgSegment("principal");

   oHdr.className = sreq.assocClass;

   binCtx.oHdr = (OperationHdr *) &oHdr;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = 0;
   binCtx.rHdr = NULL;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.type=CMPI_ref;
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) &oHdr);

   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      
      if (err == 0) {
         enm=cpyEnumResponses(&binCtx,resp,l);
         freeResps(resp,binCtx.pCount);
         _SFCB_RETURN(enm);
      }
      if (rc) CMCISetStatusWithChars(rc, resp[err-1]->rc, 
                  (char*)resp[err-1]->object[0].data);
      freeResps(resp,binCtx.pCount);
      _SFCB_RETURN(NULL);
   }
   else ctxErrResponse(&binCtx,rc);
   closeProviderContext(&binCtx);
    
   _SFCB_RETURN(NULL);
}
	
	
	
static CMPIEnumeration * referenceNames(
	Client * mb,
	CMPIObjectPath * cop,
	const char * resultClass,
	const char * role,
	CMPIStatus * rc)
{
   ReferenceNamesReq sreq = BINREQ(OPS_ReferenceNames, 4);
   int irc, l = 0, err = 0;
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   OperationHdr oHdr={OPS_ReferenceNames,0,4};
   CMPIEnumeration *enm = NULL;
   
   _SFCB_ENTER(TRACE_CIMXMLPROC, "referenceNames");

   memset(&binCtx,0,sizeof(BinRequestContext));
   
   sreq.objectPath = setObjectPathMsgSegment(cop);
   sreq.resultClass = setCharsMsgSegment(resultClass);
   sreq.role = setCharsMsgSegment(role);
   sreq.principal = setCharsMsgSegment("principal");

   oHdr.className = sreq.resultClass;

   binCtx.oHdr = (OperationHdr *) &oHdr;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = 0;
   binCtx.rHdr = NULL;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.type=CMPI_ref;
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) &oHdr);

   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      closeProviderContext(&binCtx);
      
      if (err == 0) {
         enm=cpyEnumResponses(&binCtx,resp,l);
         freeResps(resp,binCtx.pCount);
         _SFCB_RETURN(enm);
      }
      if (rc) CMCISetStatusWithChars(rc, resp[err-1]->rc, 
                  (char*)resp[err-1]->object[0].data);
      freeResps(resp,binCtx.pCount);
      _SFCB_RETURN(NULL);
   }
   else ctxErrResponse(&binCtx,rc);
   closeProviderContext(&binCtx);
    
   _SFCB_RETURN(NULL);
}

static CMPIData invokeMethod(
	Client * mb,
	CMPIObjectPath * cop,
	const char * method,
	CMPIArgs * in,
	CMPIArgs * out,
	CMPIStatus * rc)
{
   CMPIData retval={0,CMPI_notFound,{0l}};
   if (rc) CMSetStatus(rc, CMPI_RC_ERR_NOT_SUPPORTED);
   return retval;
}

static CMPIStatus setProperty(
	Client * mb,
	CMPIObjectPath * cop,
	const char * name,
	CMPIValue * value,
	CMPIType type)
{
   CMPIStatus rc;
   CMSetStatus(&rc, CMPI_RC_ERR_NOT_SUPPORTED);
   return rc;
}

static CMPIData getProperty(
	Client * mb,
	CMPIObjectPath * cop,
	const char * name,
	CMPIStatus * rc)
{
   CMPIData retval={0,CMPI_notFound,{0l}};
   if (rc) CMSetStatus(rc, CMPI_RC_ERR_NOT_SUPPORTED);
   return retval;
}

#ifdef mmmmmmmmm


------------------------------------------------------------------------*/

/* almost finished (need to add output arg support) but scanCimXmlResponse segv's parsing results */
static CMPIData invokeMethod(
	Client * mb,
	CMPIObjectPath * cop,
	const char * method,
	CMPIArgs * in,
	CMPIArgs * out,
	CMPIStatus * rc)
/*
<?xml version="1.0" encoding="utf-8"?>
<CIM CIMVERSION="2.0" DTDVERSION="2.0">
  <MESSAGE ID="4711" PROTOCOLVERSION="1.0">
    <SIMPLEREQ>
      <METHODCALL NAME="IsAuthorized">
        <LOCALINSTANCEPATH>
          <LOCALNAMESPACEPATH>
            <NAMESPACE NAME="root"/>
            <NAMESPACE NAME="cimv2"/>
          </LOCALNAMESPACEPATH>
          <INSTANCENAME CLASSNAME="CWS_Authorization">
            <KEYBINDING NAME="Username">
              <KEYVALUE VALUETYPE="string">schuur</KEYVALUE>
            </KEYBINDING>
            <KEYBINDING NAME="Classname">
              <KEYVALUE VALUETYPE="string">CIM_ComputerSystem</KEYVALUE>
            </KEYBINDING>
          </INSTANCENAME>
        </LOCALINSTANCEPATH>
        <PARAMVALUE NAME="operation">
          <VALUE>Query</VALUE>
        </PARAMVALUE>
      </METHODCALL>
    </SIMPLEREQ>
  </MESSAGE>
</CIM>
*/
{
   ClientEnc		*cl = (ClientEnc*)mb;
   ClientConnection	*con = cl->connection;
   UtilStringBuffer	*sb=newStringBuffer(2048);
   char			*error;
   ResponseHdr		rh;
   CMPIString		*cn;
   CMPIData		retval;
   int			i, numinargs = 0, numoutargs = 0;
   char                 *cv;

#if DEBUG
   set_debug();
#endif

   if (in)
      numinargs = in->ft->getArgCount(in, NULL);
   if (out)
      numoutargs = out->ft->getArgCount(out, NULL);

   con->ft->genRequest(cl, (const char *)method, cop, 1);

   addXmlHeader(sb);

   /* Add the extrinsic method name */
   sb->ft->append3Chars(sb,"<METHODCALL NAME=\"", method, "\">");

   sb->ft->appendChars(sb,"<LOCALINSTANCEPATH>");

   addXmlNamespace(sb, cop);

   /* Add the objectpath */
   cn = cop->ft->getClassName(cop,NULL);
   sb->ft->append3Chars(sb, "<INSTANCENAME CLASSNAME=\"",
					  (char*)cn->hdl,"\">\n");
   pathToXml(sb, cop);
   sb->ft->appendChars(sb,"</INSTANCENAME>\n");
   CMRelease(cn);

   sb->ft->appendChars(sb,"</LOCALINSTANCEPATH>");

   /* Add the input parameters */
   for (i = 0; i < numinargs; i++) {
      CMPIString * argname;
      CMPIData argdata = in->ft->getArgAt(in, i, &argname, NULL);
      sb->ft->append3Chars(sb,"<PARAMVALUE NAME=\"", argname->hdl, "\">\n");
      sb->ft->append3Chars(sb,"<VALUE>", cv=sfcb_value2Chars(argdata.type,&(argdata.value)), "</VALUE>\n");
      sb->ft->appendChars(sb,"</PARAMVALUE>\n");
      if(cv) free(cv);
      if(argname) CMRelease(argname);
   }

   sb->ft->appendChars(sb,"</METHODCALL>\n");
   addXmlFooter(sb);

   error = con->ft->addPayload(con,sb);

   if (error || (error = con->ft->getResponse(con, cop))) {
      CMCISetStatusWithChars(rc,CMPI_RC_ERR_FAILED,error);
      free(error);
      retval.state = CMPI_notFound | CMPI_nullValue;
      CMRelease(sb);
      return retval;
   }

   CMRelease(sb);

   rh = scanCimXmlResponse(CMGetCharPtr(con->mResponse),cop);

   if (rh.errCode != 0) {
      CMCISetStatusWithChars(rc, rh.errCode, rh.description);
      free(rh.description);
      CMRelease(rh.rvArray);
      retval.state = CMPI_notFound | CMPI_nullValue;
      return retval;
   }

   /* TODO: process output args */
#if DEBUG
   if (do_debug && numoutargs > 0)
	printf("TODO: invokeMethod() with output args\n");
#endif

   /* Set good status and return */
#if DEBUG
   if (do_debug)
       printf ("\treturn type %d\n",
		 rh.rvArray->ft->getSimpleType(rh.rvArray, NULL));
#endif

   CMSetStatus(rc, CMPI_RC_OK);
   retval=rh.rvArray->ft->getElementAt(rh.rvArray, 0, NULL);
   retval.value=sfcb_native_clone_CMPIValue(rh.rvArray->ft->getSimpleType(rh.rvArray, NULL),&retval.value,NULL);
   CMRelease(rh.rvArray);
   return retval;
}

/* --------------------------------------------------------------------------*/

/* finished and working */
static CMPIStatus setProperty(
	Client * mb,
	CMPIObjectPath * cop,
	const char * name,
	CMPIValue * value,
	CMPIType type)
/*
<?xml version="1.0" encoding="utf-8"?>
<CIM CIMVERSION="2.0" DTDVERSION="2.0">
  <MESSAGE ID="4711" PROTOCOLVERSION="1.0">
    <SIMPLEREQ>
      <IMETHODCALL NAME="SetProperty">
        <LOCALNAMESPACEPATH>
          <NAMESPACE NAME="root"/>
          <NAMESPACE NAME="cimv2"/>
        </LOCALNAMESPACEPATH>
        <IPARAMVALUE NAME="PropertyName">
          <VALUE>CurrentTimeZone</VALUE>
        </IPARAMVALUE>
        <IPARAMVALUE NAME="NewValue">
          <VALUE>123</VALUE>
        </IPARAMVALUE>
        <IPARAMVALUE NAME="InstanceName">
          <INSTANCENAME CLASSNAME="Linux_OperatingSystem">
            <KEYBINDING NAME="CSCreationClassName">
              <KEYVALUE VALUETYPE="string">Linux_ComputerSystem</KEYVALUE>
            </KEYBINDING>
            <KEYBINDING NAME="CSName">
              <KEYVALUE VALUETYPE="string">bestorga.ibm.com</KEYVALUE>
            </KEYBINDING>
            <KEYBINDING NAME="CreationClassName">
              <KEYVALUE VALUETYPE="string">Linux_OperatingSystem</KEYVALUE>
            </KEYBINDING>
            <KEYBINDING NAME="Name">
              <KEYVALUE VALUETYPE="string">bestorga.ibm.com</KEYVALUE>
            </KEYBINDING>
          </INSTANCENAME>
        </IPARAMVALUE>
      </IMETHODCALL>
    </SIMPLEREQ>
  </MESSAGE>
</CIM>
*/
{
   ClientEnc        *cl = (ClientEnc*)mb;
   ClientConnection   *con = cl->connection;
   UtilStringBuffer *sb=newStringBuffer(2048);
   char		    *error;
   ResponseHdr      rh;
   CMPIString	    *cn;
   CMPIStatus	    rc = {CMPI_RC_OK, NULL};
   char             *cv;

#if DEBUG
   set_debug();
#endif

   con->ft->genRequest(cl, "SetProperty", cop, 0);

   addXmlHeader(sb);
   sb->ft->appendChars(sb,"<IMETHODCALL NAME=\"SetProperty\">");

   addXmlNamespace(sb, cop);

   /* Add the property */
   sb->ft->append3Chars(sb,
        "<IPARAMVALUE NAME=\"PropertyName\">\n<VALUE>",
        name, "</VALUE>\n</IPARAMVALUE>");

   /* Add the new value */
   sb->ft->append3Chars(sb,
        "<IPARAMVALUE NAME=\"NewValue\">\n<VALUE>",
        cv=sfcb_value2Chars(type,value), "</VALUE>\n</IPARAMVALUE>");
   if(cv) free(cv);

   /* Add the objectpath */
   cn = cop->ft->getClassName(cop,NULL);
   sb->ft->append3Chars(sb,"<IPARAMVALUE NAME=\"InstanceName\">\n"
          "<INSTANCENAME CLASSNAME=\"",(char*)cn->hdl,"\">\n");
   pathToXml(sb, cop);
   sb->ft->appendChars(sb,"</INSTANCENAME>\n</IPARAMVALUE>\n");
   CMRelease(cn);

   sb->ft->appendChars(sb,"</IMETHODCALL>\n");
   addXmlFooter(sb);

   error = con->ft->addPayload(con,sb);

   if (error || (error = con->ft->getResponse(con, cop))) {
      CMCISetStatusWithChars(&rc,CMPI_RC_ERR_FAILED,error);
      free(error);
      CMRelease(sb);
      return rc;
   }

   CMRelease(sb);

   rh = scanCimXmlResponse(CMGetCharPtr(con->mResponse),cop);

   if (rh.errCode != 0) {
      CMCISetStatusWithChars(&rc, rh.errCode, rh.description);
      free(rh.description);
   }
   
   CMRelease(rh.rvArray);
   return rc;
}

/* --------------------------------------------------------------------------*/

/* finished and working */
static CMPIData getProperty(
	Client * mb,
	CMPIObjectPath * cop,
	const char * name,
	CMPIStatus * rc)
/*
<?xml version="1.0" encoding="utf-8"?>
<CIM CIMVERSION="2.0" DTDVERSION="2.0">
  <MESSAGE ID="4711" PROTOCOLVERSION="1.0">
    <SIMPLEREQ>
      <IMETHODCALL NAME="GetProperty">
        <LOCALNAMESPACEPATH>
          <NAMESPACE NAME="root"/>
          <NAMESPACE NAME="cimv2"/>
        </LOCALNAMESPACEPATH>
        <IPARAMVALUE NAME="PropertyName">
          <VALUE>CurrentTimeZone</VALUE>
        </IPARAMVALUE>
        <IPARAMVALUE NAME="InstanceName">
          <INSTANCENAME CLASSNAME="Linux_OperatingSystem">
            <KEYBINDING NAME="CSCreationClassName">
              <KEYVALUE VALUETYPE="string">Linux_ComputerSystem</KEYVALUE>
            </KEYBINDING>
            <KEYBINDING NAME="CSName">
              <KEYVALUE VALUETYPE="string">bestorga.ibm.com</KEYVALUE>
            </KEYBINDING>
            <KEYBINDING NAME="CreationClassName">
              <KEYVALUE VALUETYPE="string">Linux_OperatingSystem</KEYVALUE>
            </KEYBINDING>
            <KEYBINDING NAME="Name">
              <KEYVALUE VALUETYPE="string">bestorga.ibm.com</KEYVALUE>
            </KEYBINDING>
          </INSTANCENAME>
        </IPARAMVALUE>
      </IMETHODCALL>
    </SIMPLEREQ>
  </MESSAGE>
</CIM>
*/
{
   ClientEnc		*cl = (ClientEnc*)mb;
   ClientConnection	*con = cl->connection;
   UtilStringBuffer	*sb=newStringBuffer(2048);
   char			*error;
   ResponseHdr		rh;
   CMPIString		*cn;
   CMPIData		retval;

#if DEBUG
   set_debug();
#endif

   con->ft->genRequest(cl, "GetProperty", cop, 0);

   addXmlHeader(sb);
   sb->ft->appendChars(sb,"<IMETHODCALL NAME=\"GetProperty\">");

   addXmlNamespace(sb, cop);

   /* Add the property */
   sb->ft->append3Chars(sb,
        "<IPARAMVALUE NAME=\"PropertyName\">\n<VALUE>",
        name, "</VALUE>\n</IPARAMVALUE>");

   /* Add the objectpath */
   cn = cop->ft->getClassName(cop,NULL);
   sb->ft->append3Chars(sb,"<IPARAMVALUE NAME=\"InstanceName\">\n"
          "<INSTANCENAME CLASSNAME=\"",(char*)cn->hdl,"\">\n");
   pathToXml(sb, cop);
   sb->ft->appendChars(sb,"</INSTANCENAME>\n</IPARAMVALUE>\n");
   CMRelease(cn);

   sb->ft->appendChars(sb,"</IMETHODCALL>\n");
   addXmlFooter(sb);

   error = con->ft->addPayload(con,sb);

   if (error || (error = con->ft->getResponse(con, cop))) {
      CMCISetStatusWithChars(rc,CMPI_RC_ERR_FAILED,error);
      free(error);
      retval.state = CMPI_notFound | CMPI_nullValue;
      CMRelease(sb);
      return retval;
   }

   CMRelease(sb);

   rh = scanCimXmlResponse(CMGetCharPtr(con->mResponse),cop);

   if (rh.errCode != 0) {
      CMCISetStatusWithChars(rc, rh.errCode, rh.description);
      free(rh.description);
      CMRelease(rh.rvArray);
      retval.state = CMPI_notFound | CMPI_nullValue;
      return retval;
   }

   /* Set good return status and return */
#if DEBUG
   if (do_debug)
       printf ("\treturn type %d\n",
	    rh.rvArray->ft->getSimpleType(rh.rvArray, NULL));
#endif

   CMSetStatus(rc, CMPI_RC_OK);
   retval=rh.rvArray->ft->getElementAt(rh.rvArray, 0, NULL);
   retval.value=sfcb_native_clone_CMPIValue(rh.rvArray->ft->getSimpleType(rh.rvArray, NULL),&retval.value,NULL);
   CMRelease(rh.rvArray);
   return retval;
}

#endif

/* --------------------------------------------------------------------------*/
static CMPIConstClass * getClass(
   Client * mb,
	CMPIObjectPath * cop,
	CMPIFlags flags,
	char ** properties,
	CMPIStatus * rc)
{
   CMPIConstClass *cls;   
   int irc,i,sreqSize=sizeof(GetClassReq)-sizeof(MsgSegment);
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   GetClassReq *sreq;
   int pCount=0;
   OperationHdr oHdr={OPS_GetClass,0,2};
   
   _SFCB_ENTER(TRACE_CIMXMLPROC, "getClass");
   
   if (rc) CMSetStatus(rc, CMPI_RC_OK);

   CMPIString *ns=cop->ft->getNameSpace(cop,NULL);
   CMPIString *cn=cop->ft->getClassName(cop,NULL);
   
   oHdr.nameSpace=setCharsMsgSegment((char*)ns->hdl);
   oHdr.className=setCharsMsgSegment((char*)cn->hdl);

   memset(&binCtx,0,sizeof(BinRequestContext));

   if (properties) {
      char **p;
      for (p=properties; *p; p++) pCount++;
   }
   
   sreqSize+=pCount*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->hdr.operation=OPS_GetClass;
   sreq->hdr.count=pCount+2;

   sreq->objectPath = setObjectPathMsgSegment(cop);
   sreq->principal = setCharsMsgSegment("principal");

   for (i=0; i<pCount; i++)
      sreq->properties[i]=setCharsMsgSegment(properties[i]);

   binCtx.oHdr = (OperationHdr *) &oHdr;
   binCtx.bHdr = &sreq->hdr;
   binCtx.bHdr->flags = flags;
   binCtx.rHdr = NULL;
   binCtx.bHdrSize = sreqSize;
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) &oHdr);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) { 
         cls = relocateSerializedConstClass(resp->object[0].data);
         cls = cls->ft->clone(cls,NULL);
         free(resp);
         free(sreq);
         _SFCB_RETURN(cls);
      }
      free(sreq);
      if (rc) CMCISetStatusWithChars(rc, resp->rc, (char*)resp->object[0].data);
      _SFCB_RETURN(NULL);
   }
   else ctxErrResponse(&binCtx,rc);
   free(sreq);
   closeProviderContext(&binCtx);

   _SFCB_RETURN(NULL);
}

static CMPIEnumeration* enumClassNames(
	Client * mb,
	CMPIObjectPath * cop,
	CMPIFlags flags,
	CMPIStatus * rc)
{
   EnumClassNamesReq sreq = BINREQ(OPS_EnumerateClassNames, 2);
   int irc, l = 0, err = 0;
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   OperationHdr oHdr={OPS_EnumerateClassNames,0,2};
   CMPIEnumeration *enm = NULL;
   
   _SFCB_ENTER(TRACE_CIMXMLPROC, "enumClassNames");
   
   if (rc) CMSetStatus(rc, CMPI_RC_OK);

   CMPIString *ns=cop->ft->getNameSpace(cop,NULL);
   CMPIString *cn=cop->ft->getClassName(cop,NULL);
   
   oHdr.nameSpace=setCharsMsgSegment((char*)ns->hdl);
   oHdr.className=setCharsMsgSegment((char*)cn->hdl);

   memset(&binCtx,0,sizeof(BinRequestContext));
   
   sreq.objectPath = setObjectPathMsgSegment(cop);
   sreq.principal = setCharsMsgSegment("principal");
   sreq.hdr.flags = flags;

   binCtx.oHdr = (OperationHdr*) &oHdr;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = flags;
   binCtx.rHdr = NULL;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.type=CMPI_ref;
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

  _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx,(OperationHdr*)&oHdr);

   CMRelease(ns);
   CMRelease(cn);

   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      _SFCB_TRACE(1, ("--- Back from Provider"));

      closeProviderContext(&binCtx);
      if (err == 0) {
         enm=cpyEnumResponses(&binCtx,resp,l);
         freeResps(resp,binCtx.pCount);
         _SFCB_RETURN(enm);
      }
      if (rc) CMCISetStatusWithChars(rc, resp[err-1]->rc, 
                  (char*)resp[err-1]->object[0].data);
      freeResps(resp,binCtx.pCount);
      _SFCB_RETURN(NULL);
   }
   else ctxErrResponse(&binCtx,rc);
   closeProviderContext(&binCtx);
   
   _SFCB_RETURN(NULL);
}

static CMPIEnumeration * enumClasses(
	Client * mb,
	CMPIObjectPath * cop,
	CMPIFlags flags,
	CMPIStatus * rc)
{
   EnumClassesReq sreq = BINREQ(OPS_EnumerateClasses, 2);
   int irc, l = 0, err = 0;
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   OperationHdr oHdr={OPS_EnumerateClasses,0,2};
   CMPIEnumeration *enm = NULL;
   
   _SFCB_ENTER(TRACE_CIMXMLPROC, "enumClasses");
   
   if (rc) CMSetStatus(rc, CMPI_RC_OK);

   CMPIString *ns=cop->ft->getNameSpace(cop,NULL);
   CMPIString *cn=cop->ft->getClassName(cop,NULL);
   
   oHdr.nameSpace=setCharsMsgSegment((char*)ns->hdl);
   oHdr.className=setCharsMsgSegment((char*)cn->hdl);

   memset(&binCtx,0,sizeof(BinRequestContext));

   sreq.objectPath = setObjectPathMsgSegment(cop);
   sreq.principal = setCharsMsgSegment("principal");
   sreq.hdr.flags = flags;

   binCtx.oHdr = (OperationHdr*)&oHdr;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = flags;
   binCtx.type=CMPI_class;
   binCtx.rHdr = NULL;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr*)&oHdr);

   CMRelease(ns);
   CMRelease(cn);

   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      closeProviderContext(&binCtx);

      if (err == 0) {
         enm=cpyEnumResponses(&binCtx,resp,l);
         freeResps(resp,binCtx.pCount);
         _SFCB_RETURN(enm);
      }
      if (rc) CMCISetStatusWithChars(rc, resp[err-1]->rc, 
                  (char*)resp[err-1]->object[0].data);
      freeResps(resp,binCtx.pCount);
      _SFCB_RETURN(NULL);
   }
   else ctxErrResponse(&binCtx,rc);
   closeProviderContext(&binCtx);

   _SFCB_RETURN(NULL);

}


#ifdef mmmmmmmmmm
/* --------------------------------------------------------------------------*/

/* finished & working */
static CMPIEnumeration* enumClassNames(
	Client * mb,
	CMPIObjectPath * cop,
	CMPIFlags flags,
	CMPIStatus * rc)
{
   ClientEnc *cl=(ClientEnc*)mb;
   ClientConnection *con=cl->connection;
   UtilStringBuffer *sb=newStringBuffer(2048);
   char *error;
   ResponseHdr       rh;

#if DEBUG
   set_debug();
#endif

   con->ft->genRequest(cl, "EnumerateClassNames", cop, 0);

   /* Construct the CIM-XML request */
   addXmlHeader(sb);
   sb->ft->appendChars(sb,"<IMETHODCALL NAME=\"EnumerateClassNames\">");

   addXmlNamespace(sb, cop);
   emitdeep(sb,flags & CMPI_FLAG_DeepInheritance);
   addXmlClassnameParam(sb, cop);

   sb->ft->appendChars(sb,"</IMETHODCALL>\n");
   addXmlFooter(sb);

   error = con->ft->addPayload(con,sb);

   if (error || (error = con->ft->getResponse(con,cop))) {
      CMCISetStatusWithChars(rc,CMPI_RC_ERR_FAILED,error);
      free(error);
      CMRelease(sb);
      return NULL;
   }

   CMRelease(sb);

   rh=scanCimXmlResponse(CMGetCharPtr(con->mResponse),cop);

   if (rh.errCode!=0) {
      CMCISetStatusWithChars(rc,rh.errCode,rh.description);
      free(rh.description);
      CMRelease(rh.rvArray);
      return NULL;
   }

#if DEBUG
#ifdef CHECK_RETURN_TYPES
    if (CMGetArrayCount(rh.rvArray,NULL)==0 ||
	rh.rvArray->ft->getSimpleType(rh.rvArray,NULL) != CMPI_ref) {
	/* Should not happen in production builds */
	CMCISetStatusWithChars(rc, CMPI_RC_ERR_FAILED,
				 strdup("Unexpected return value"));
	return NULL;
    }
#else
   if (do_debug)
       printf ("\treturn enumeration array type %d expected CMPI_ref %d\n",
	    rh.rvArray->ft->getSimpleType(rh.rvArray, NULL), CMPI_ref);
#endif
#endif

   CMSetStatus(rc,CMPI_RC_OK);
   return newCMPIEnumeration(rh.rvArray,NULL);
}

/* --------------------------------------------------------------------------*/

/* finished and working */
static CMPIEnumeration * enumClasses(
	Client * mb,
	CMPIObjectPath * cop,
	CMPIFlags flags,
	CMPIStatus * rc)
{
   ClientEnc	     *cl  = (ClientEnc *)mb;
   ClientConnection   *con = cl->connection;
   UtilStringBuffer *sb  = newStringBuffer(2048);
   char             *error;
   ResponseHdr	     rh;

#if DEBUG
   set_debug();
#endif

   con->ft->genRequest(cl, "EnumerateClasses", cop, 0);

   /* Construct the CIM-XML request */
   addXmlHeader(sb);
   sb->ft->appendChars(sb, "<IMETHODCALL NAME=\"EnumerateClasses\">");

   addXmlNamespace(sb, cop);
   emitdeep(sb,flags & CMPI_FLAG_DeepInheritance);
   emitlocal(sb,flags & CMPI_FLAG_LocalOnly);
   emitqual(sb,flags & CMPI_FLAG_IncludeQualifiers);
   emitorigin(sb,flags & CMPI_FLAG_IncludeClassOrigin);
   addXmlClassnameParam(sb, cop);

   sb->ft->appendChars(sb,"</IMETHODCALL>\n");
   addXmlFooter(sb);

   error = con->ft->addPayload(con,sb);

   if (error || (error = con->ft->getResponse(con, cop))) {
      CMCISetStatusWithChars(rc,CMPI_RC_ERR_FAILED,error);
      free(error);
      CMRelease(sb);
      return NULL;
   }

   CMRelease(sb);

   rh = scanCimXmlResponse(CMGetCharPtr(con->mResponse), cop);

   if (rh.errCode != 0) {
      CMCISetStatusWithChars(rc, rh.errCode, rh.description);
      free(rh.description);
      CMRelease(rh.rvArray);
      return NULL;
   }

#if DEBUG
#ifdef CHECK_RETURN_TYPES
    if (CMGetArrayCount(rh.rvArray,NULL)==0 ||
	rh.rvArray->ft->getSimpleType(rh.rvArray,NULL) != CMPI_class) {
	/* Should not happen in production builds */
	CMCISetStatusWithChars(rc, CMPI_RC_ERR_FAILED,
				 strdup("Unexpected return value"));
	return NULL;
    }
#else
   if (do_debug)
       printf ("\treturn enumeration array type %d expected CMPI_class %d\n",
	    rh.rvArray->ft->getSimpleType(rh.rvArray, NULL), CMPI_class);
#endif
#endif

   CMSetStatus(rc, CMPI_RC_OK);
   return newCMPIEnumeration(rh.rvArray,NULL);
}

/* --------------------------------------------------------------------------*/

#endif


static ClientFT clientFt = {
   1, //NATIVE_FT_VERSION,
   releaseClient,
   cloneClient,
   getClass,
   enumClassNames,
   enumClasses,
   getInstance,
   createInstance,
   modifyInstance,
   deleteInstance,
   execQuery,
   enumInstanceNames,
   enumInstances,
   associators,
   associatorNames,
   references,
   referenceNames,
   invokeMethod,
   setProperty,
   getProperty
};

/* --------------------------------------------------------------------------*/

extern int getControlChars(char *, char **);
extern int setupControl(char *fn);
extern int errno;

int localConnect(ClientEnv* ce, CMPIStatus *st) 
{
   static struct sockaddr_un serverAddr;
   int sock,rc,sfcbSocket;
   void *idData;
   unsigned long int l;
   char *user;
   static char *socketName=NULL;

   
   struct _msg {
      unsigned int size;
      char oper;
      pid_t pid;
      char id[64];
   } msg;
   
   if ((sock=socket(PF_UNIX, SOCK_STREAM, 0))<0) {
      return -1;
      if (st) {
         st->rc=CMPI_RC_ERR_FAILED;
         st->msg=ce->ft->newString(ce,strerror(errno),NULL);
      }
      return -1;
   }
   
   if (socketName==NULL) {
      setupControl(NULL);
      rc=getControlChars("localSocketPath", &socketName);    
      sunsetControl();
      if (rc) {
         fprintf(stderr,"--- Failed to open sfcb local socket (%d)\n",rc);
         return -2;
      }
   }   

   serverAddr.sun_family=AF_UNIX;
   strcpy(serverAddr.sun_path,socketName);
   
   if (connect(sock,(struct sockAddr*)&serverAddr,
      sizeof(serverAddr.sun_family)+strlen(serverAddr.sun_path))<0) {
      if (st) {
         st->rc=CMPI_RC_ERR_FAILED;
         st->msg=ce->ft->newString(ce,strerror(errno),NULL);
      }
      return -1;
   }
   
   msg.size=sizeof(msg)-sizeof(msg.size);
   msg.oper=1;
   msg.pid=getpid();
   user=getenv("USER");
   strncpy(msg.id, (user) ? user : "", sizeof(msg.id)-1);
   msg.id[sizeof(msg.id)-1]=0;
   
   l=write(sock, &msg, sizeof(msg)); 
   
   rc = spRecvCtlResult(&sock, &sfcbSocket,&idData, &l);
  
   sfcbSockets.send=sfcbSocket;
   localMode=0;
   
   return sfcbSocket;
}

static void* release(ClientEnv* ce)
{
   void *lib=ce->hdl;
   free(ce);
   return lib;
}

static Client *CMPIConnect2(ClientEnv* ce, const char *hn, const char *scheme, const char *port,
			 const char *user, const char *pwd, 
			 int verifyMode, const char * trustStore,
			 const char * certFile, const char * keyFile,
			 CMPIStatus *rc);
                         
static Client *CMPIConnect(ClientEnv* ce, const char *hn, const char *scheme, const char *port,
                        const char *user, const char *pwd, CMPIStatus *rc)
{
  return CMPIConnect2(ce, hn, scheme, port, user, pwd, Client_VERIFY_PEER, NULL,
		      NULL, NULL, rc);
}

static Client *CMPIConnect2(ClientEnv* ce, const char *hn, const char *scheme, const char *port,
			 const char *user, const char *pwd, 
			 int verifyMode, const char * trustStore,
			 const char * certFile, const char * keyFile,
			 CMPIStatus *rc)
{  
   ClientEnc *cc = (ClientEnc*)calloc(1, sizeof(ClientEnc));

   if (rc) rc->rc=0;
   
   cc->enc.hdl		= &cc->data;
   cc->enc.ft		= &clientFt;

   cc->data.hostName	= hn ? strdup(hn) : strdup("localhost");
   cc->data.user	= user ? strdup(user) : NULL;
   cc->data.pwd		= pwd ? strdup(pwd) : NULL;
   cc->data.scheme	= scheme ? strdup(scheme) : strdup("http");

   if (port != NULL)
      cc->data.port = strdup(port); 
   else
      cc->data.port = strcmp(cc->data.scheme, "https") == 0 ? 
	strdup("5989") : strdup("5988");

   cc->certData.verifyMode = verifyMode;
   cc->certData.trustStore = trustStore ? strdup(trustStore) : NULL;
   cc->certData.certFile = certFile ? strdup(certFile) : NULL;
   cc->certData.keyFile = keyFile ? strdup(keyFile) : NULL;

   if (localConnect(ce,rc)<0) return NULL;

   return (Client*)cc;
}

static CMPIInstance* newInstance(ClientEnv* ce, const CMPIObjectPath* op, CMPIStatus* rc)
{
   return NewCMPIInstance((CMPIObjectPath*)op,rc);
}

static CMPIString* newString(ClientEnv* ce, const char *ptr, CMPIStatus * rc)
{
   return NewCMPIString(ptr, rc);
}

static CMPIObjectPath* newObjectPath(ClientEnv* ce, const char *ns, const char *cn, CMPIStatus* rc)
{
   return NewCMPIObjectPath(ns,cn,rc);
}

static CMPIArgs* newArgs(ClientEnv* ce, CMPIStatus* rc)
{
   return NewCMPIArgs(rc);
}

static CMPIArray* newArray(ClientEnv* ce, CMPICount max, CMPIType type, CMPIStatus* rc)
{
   return NewCMPIArray(max, type, rc);
}

static CMPIDateTime *newDateTime(ClientEnv* ce, CMPIStatus *rc) 
{
   return NewCMPIDateTime(rc); 
}

static CMPIDateTime *newDateTimeFromBinary(ClientEnv* ce, CMPIUint64 binTime, CMPIBoolean interval, 
      CMPIStatus *rc) 
{
   return NewCMPIDateTimeFromBinary(binTime,interval,rc); 
}

static CMPIDateTime *newDateTimeFromChars(ClientEnv* ce, const char *utcTime, CMPIStatus *rc) 
{
   return NewCMPIDateTimeFromChars(utcTime,rc); 
}

ClientEnv* _Create_SfcbLocal_Env(char *id)
{
 
   static ClientEnvFT localFT = {
      "SfcbLocal",
      release,
      CMPIConnect,
      CMPIConnect2,     
      newInstance,      
      newObjectPath,
      newArgs,
      newArray,
      newDateTime,
      newDateTimeFromBinary,
      newDateTimeFromChars,
      newString,
    };
   
    localClientMode=1;
    
    ClientEnv *env = (ClientEnv*)malloc(sizeof(ClientEnv));
    env->hdl=NULL;
    env->ft=&localFT;
    
    return env;
 }
 
