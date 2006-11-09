
/*
 * cimXmlRequest.c
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
 * CMPI broker encapsulated functionality.
 *
 * CIM operations request handler .
 *
*/



#include "cmpidt.h"
#include "cmpidtx.h"
#include "cimXmlGen.h"
#include "cimXmlRequest.h"
#include "cimXmlParser.h"
#include "msgqueue.h"
#include "cmpidt.h"
#include "constClass.h"

#ifdef HAVE_QUALREP
#include "qualifier.h"
#endif

#include "objectImpl.h"

#include "native.h"
#include "trace.h"
#include "utilft.h"
#include "string.h"

#include "queryOperation.h"
#include "config.h"

#ifdef SFCB_IX86
#define SFCB_ASM(x) asm(x)
#else
#define SFCB_ASM(x)
#endif

typedef struct handler {
   RespSegments(*handler) (CimXmlRequestContext *, RequestHdr * hdr);
} Handler;

extern int noChunking;

extern CMPIBroker *Broker;
extern UtilStringBuffer *newStringBuffer(int s);
extern UtilStringBuffer *instanceToString(CMPIInstance * ci, char **props);
extern const char *getErrorId(int c);
extern const char *instGetClassName(CMPIInstance * ci);

extern CMPIData opGetKeyCharsAt(CMPIObjectPath * cop, unsigned int index,
                                const char **name, CMPIStatus * rc);
extern BinResponseHdr *invokeProvider(BinRequestContext * ctx);
extern CMPIArgs *relocateSerializedArgs(void *area);
extern CMPIObjectPath *relocateSerializedObjectPath(void *area);
extern CMPIInstance *relocateSerializedInstance(void *area);
extern CMPIConstClass *relocateSerializedConstClass(void *area);
extern MsgSegment setInstanceMsgSegment(CMPIInstance * ci);
extern MsgSegment setArgsMsgSegment(CMPIArgs * args);
extern MsgSegment setConstClassMsgSegment(CMPIConstClass * cl);
extern void closeProviderContext(BinRequestContext * ctx);
extern CMPIStatus arraySetElementNotTrackedAt(CMPIArray * array,
             CMPICount index, CMPIValue * val, CMPIType type);
extern CMPIConstClass initConstClass(ClClass *cl);
extern CMPIQualifierDecl initQualifier(ClQualifierDeclaration *qual);

static char *cimMsg[] = {
   "ok",
   "A general error occured that is not covered by a more specific error code",
   "Access to a CIM resource was not available to the client",
   "The target namespace does not exist",
   "One or more parameter values passed to the method were invalid",
   "The specified Class does not exist",
   "The requested object could not be found",
   "The requested operation is not supported",
   "Operation cannot be carried out on this class since it has subclasses",
   "Operation cannot be carried out on this class since it has instances",
   "Operation cannot be carried out since the specified superclass does not exist",
   "Operation cannot be carried out because an object already exists",
   "The specified Property does not exist",
   "The value supplied is incompatible with the type",
   "The query language is not recognized or supported",
   "The query is not valid for the specified query language",
   "The extrinsic Method could not be executed",
   "The specified extrinsic Method does not exist"
};
/*
static char *cimMsgId[] = {
   "",
   "CIM_ERR_FAILED",
   "CIM_ERR_ACCESS_DENIED",
   "CIM_ERR_INVALID_NAMESPACE",
   "CIM_ERR_INVALID_PARAMETER",
   "CIM_ERR_INVALID_CLASS",
   "CIM_ERR_NOT_FOUND",
   "CIM_ERR_NOT_SUPPORTED",
   "CIM_ERR_CLASS_HAS_CHILDREN",
   "CIM_ERR_CLASS_HAS_INSTANCES",
   "CIM_ERR_INVALID_SUPERCLASS",
   "CIM_ERR_ALREADY_EXISTS",
   "CIM_ERR_NO_SUCH_PROPERTY",
   "CIM_ERR_TYPE_MISMATCH",
   "CIM_ERR_QUERY_LANGUAGE_NOT_SUPPORTED",
   "CIM_ERR_INVALID_QUERY",
   "CIM_ERR_METHOD_NOT_AVAILABLE",
   "CIM_ERR_METHOD_NOT_FOUND",
};
*/
static char iResponseIntro1[] =
  "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
  "<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\">\n" 
   "<MESSAGE ID=\"";
static char iResponseIntro2[] =
               "\" PROTOCOLVERSION=\"1.0\">\n" 
    "<SIMPLERSP>\n" 
     "<IMETHODRESPONSE NAME=\"";
static char iResponseIntro3Error[] = "\">\n";
static char iResponseIntro3[] = "\">\n" "<IRETURNVALUE>\n";
static char iResponseTrailer1Error[] =
     "</IMETHODRESPONSE>\n" 
    "</SIMPLERSP>\n" 
   "</MESSAGE>\n" 
  "</CIM>";
static char iResponseTrailer1[] =
      "</IRETURNVALUE>\n"
     "</IMETHODRESPONSE>\n" 
    "</SIMPLERSP>\n" 
   "</MESSAGE>\n" 
  "</CIM>";
  
  
static char responseIntro1[] =
  "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
  "<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\">\n" 
   "<MESSAGE ID=\"";
static char responseIntro2[] =
               "\" PROTOCOLVERSION=\"1.0\">\n" 
    "<SIMPLERSP>\n" 
     "<METHODRESPONSE NAME=\"";
static char responseIntro3Error[] = "\">\n";
static char responseIntro3[] = "\">\n"; // "<RETURNVALUE>\n";
static char responseTrailer1Error[] =
     "</METHODRESPONSE>\n" 
    "</SIMPLERSP>\n" 
   "</MESSAGE>\n" 
  "</CIM>";
static char responseTrailer1[] =
 //     "</RETURNVALUE>\n"
     "</METHODRESPONSE>\n" 
    "</SIMPLERSP>\n" 
   "</MESSAGE>\n" 
  "</CIM>";

static char exportIndIntro1[] =
  "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
  "<CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\">\n" 
   "<MESSAGE ID=\"";
static char exportIndIntro2[] =
            "\" PROTOCOLVERSION=\"1.0\">\n" 
    "<SIMPLEEXPREQ>\n" 
     "<EXPMETHODCALL NAME=\"ExportIndication\">\n"
      "<EXPPARAMVALUE NAME=\"NewIndication\">\n";
static char exportIndTrailer1[] =
      "</EXPPARAMVALUE>\n"
     "</EXPMETHODCALL>\n"
    "</SIMPLEEXPREQ>\n" 
   "</MESSAGE>\n" 
  "</CIM>";
    
void dumpSegments(RespSegment *rs)
{
   int i;
   if (rs) {
      printf("[");
      for (i = 0; i < 7; i++) {
         if (rs[i].txt) {
            if (rs[i].mode == 2) {
               UtilStringBuffer *sb = (UtilStringBuffer *) rs[i].txt;
               printf("%s", sb->ft->getCharPtr(sb));
            }
            else printf("%s", rs[i].txt);
         }
      }
      printf("]\n");
   }
}

UtilStringBuffer *segments2stringBuffer(RespSegment *rs)
{
   int i;
   UtilStringBuffer *sb=newStringBuffer(4096);
   
   if (rs) {
      for (i = 0; i < 7; i++) {
         if (rs[i].txt) {
            if (rs[i].mode == 2) {
               UtilStringBuffer *sbt = (UtilStringBuffer *) rs[i].txt;
               sb->ft->appendChars(sb,sbt->ft->getCharPtr(sbt));
            }
            else sb->ft->appendChars(sb,rs[i].txt);
         }
      }
   }
   return sb;
}

static char *getErrSegment(int rc, char *m)
{
   char msg[1024];
   
   if (m && *m) snprintf(msg, sizeof(msg), "<ERROR CODE=\"%d\" DESCRIPTION=\"%s\"/>\n",
       rc, m);
   else if (rc > 0 && rc < 18) snprintf(msg, sizeof(msg), 
       "<ERROR CODE=\"%d\" DESCRIPTION=\"%s\"/>\n", rc, cimMsg[rc]);
   else  snprintf(msg, sizeof(msg), "<ERROR CODE=\"%d\"/>\n", rc);
   return strdup(msg);
}
/*
static char *getErrorSegment(CMPIStatus rc)
{
   if (rc.msg && rc.msg->hdl) {
      return getErrSegment(rc.rc, (char *) rc.msg->hdl);
   }
   return getErrSegment(rc.rc, NULL);
}
*/
char *getErrTrailer(int id, int rc, char *m)
{
   char msg[1024];
   
   if (m && *m) snprintf(msg, sizeof(msg), "CIMStatusCodeDescription: %s\r\n",m);
   else if (rc > 0 && rc < 18)
      snprintf(msg, sizeof(msg), "CIMStatusCodeDescription: %s\r\n",cimMsg[rc]);
   else snprintf(msg, sizeof(msg), "CIMStatusCodeDescription: *Unknown*\r\n");
   return strdup(msg);
}



static RespSegments iMethodErrResponse(RequestHdr * hdr, char *error)
{
   RespSegments rs = {
      NULL,0,0,NULL,
      {{0, iResponseIntro1},
       {0, hdr->id},
       {0, iResponseIntro2},
       {0, hdr->iMethod},
       {0, iResponseIntro3Error},
       {1, error},
       {0, iResponseTrailer1Error},
       }
   };

   return rs;
};

static RespSegments methodErrResponse(RequestHdr * hdr, char *error)
{
   RespSegments rs = {
      NULL,0,0,NULL,
      {{0, responseIntro1},
       {0, hdr->id},
       {0, responseIntro2},
       {0, hdr->iMethod},
       {0, responseIntro3Error},
       {1, error},
       {0, responseTrailer1Error},
       }
   };

   return rs;
};

static RespSegments ctxErrResponse(RequestHdr * hdr,
                                          BinRequestContext * ctx, int meth)
{
   MsgXctl *xd = ctx->ctlXdata;
   char msg[256];

   switch (ctx->rc) {
   case MSG_X_NOT_SUPPORTED:
      hdr->errMsg = strdup("Operation not supported yy");
      break;
   case MSG_X_INVALID_CLASS:
      hdr->errMsg = strdup("Class not found");
      break;
   case MSG_X_INVALID_NAMESPACE:
      hdr->errMsg = strdup("Invalid namespace");
      break;
   case MSG_X_PROVIDER_NOT_FOUND:
      hdr->errMsg = strdup("Provider not found or not loadable");
      break;
   case MSG_X_FAILED:
      hdr->errMsg = strdup(xd->data);
      break;
   default:
      sprintf(msg, "Internal error - %d\n", ctx->rc);
      hdr->errMsg = strdup(msg);
   }
   if (meth) return
      methodErrResponse(hdr,getErrSegment(CMPI_RC_ERR_INVALID_CLASS,hdr->errMsg));
   return
      iMethodErrResponse(hdr,getErrSegment(CMPI_RC_ERR_INVALID_CLASS,hdr->errMsg));
};




static RespSegments iMethodGetTrailer(UtilStringBuffer * sb)
{
   RespSegments rs = { NULL,0,0,NULL,
      {{2, (char *) sb},
       {0, iResponseTrailer1},
       {0, NULL},
       {0, NULL},
       {0, NULL},
       {0, NULL},
       {0, NULL}} };

   _SFCB_ENTER(TRACE_CIMXMLPROC, "iMethodGetTrailer");
   _SFCB_RETURN(rs);
}

static RespSegments iMethodResponse(RequestHdr * hdr, UtilStringBuffer * sb)
{
   RespSegments rs = { NULL,0,0,NULL,
      {{0, iResponseIntro1},
       {0, hdr->id},
       {0, iResponseIntro2},
       {0, hdr->iMethod},
       {0, iResponseIntro3},
       {2, (char*)sb},
       {0, iResponseTrailer1}} };
   _SFCB_ENTER(TRACE_CIMXMLPROC, "iMethodResponse");
   _SFCB_RETURN(rs);
};

static RespSegments methodResponse(RequestHdr * hdr, UtilStringBuffer * sb)
{
   RespSegments rs = { NULL,0,0,NULL,
      {{0, responseIntro1},
       {0, hdr->id},
       {0, responseIntro2},
       {0, hdr->iMethod},
       {0, responseIntro3},
       {2, (char*)sb},
       {0, responseTrailer1}} };

   _SFCB_ENTER(TRACE_CIMXMLPROC, "methodResponse");
   _SFCB_RETURN(rs);
};

ExpSegments exportIndicationReq(CMPIInstance *ci, char *id)
{
   UtilStringBuffer *sb = UtilFactory->newStrinBuffer(1024);
   ExpSegments xs = {
      {{0, exportIndIntro1},
       {0, id},
       {0, exportIndIntro2},
       {0, NULL},
       {0, NULL},
       {2, (char*)sb},
       {0, exportIndTrailer1}} };
       
   _SFCB_ENTER(TRACE_CIMXMLPROC, "exportIndicationReq");
   instance2xml(ci, sb, 0);
  _SFCB_RETURN(xs);
};
 

static UtilStringBuffer *genEnumResponses(BinRequestContext * binCtx,
                                 BinResponseHdr ** resp,
                                 int arrLen)
{
   int i, c, j;
   void *object;
   CMPIArray *ar;
   UtilStringBuffer *sb;
   CMPIEnumeration *enm;
   CMPIStatus rc;

   _SFCB_ENTER(TRACE_CIMXMLPROC, "genEnumResponses");

   ar = NewCMPIArray(arrLen, binCtx->type, NULL);
//SFCB_ASM("int $3");
   for (c = 0, i = 0; i < binCtx->rCount; i++) {
      for (j = 0; j < resp[i]->count; c++, j++) {
         if (binCtx->type == CMPI_ref)
            object = relocateSerializedObjectPath(resp[i]->object[j].data);
         else if (binCtx->type == CMPI_instance)
            object = relocateSerializedInstance(resp[i]->object[j].data);
         else if (binCtx->type == CMPI_class) {
            object=relocateSerializedConstClass(resp[i]->object[j].data);
         }   
//         rc=CMSetArrayElementAt(ar, c, &object, binCtx->type);
         rc=arraySetElementNotTrackedAt(ar,c, (CMPIValue*)&object, binCtx->type);
      }
   }

   enm = sfcb_native_new_CMPIEnumeration(ar, NULL);
   sb = UtilFactory->newStrinBuffer(1024);
   
   if (binCtx->oHdr->type==OPS_EnumerateClassNames)
      enum2xml(enm, sb, binCtx->type, XML_asClassName, binCtx->bHdr->flags);
   else if (binCtx->oHdr->type==OPS_EnumerateClasses)
      enum2xml(enm, sb, binCtx->type, XML_asClass, binCtx->bHdr->flags);
   else enum2xml(enm, sb, binCtx->type, binCtx->xmlAs,binCtx->bHdr->flags);
   
   //   ar->ft->release(ar);
   free(ar);

   _SFCB_RETURN(sb);
}

static RespSegments genResponses(BinRequestContext * binCtx,
                                 BinResponseHdr ** resp,
                                 int arrlen)
{
   RespSegments rs;
   UtilStringBuffer *sb;

   _SFCB_ENTER(TRACE_CIMXMLPROC, "genResponses");

   sb=genEnumResponses(binCtx,resp,arrlen);

   rs=iMethodResponse(binCtx->rHdr, sb);
   if (binCtx->pDone<binCtx->pCount) rs.segments[6].txt=NULL;
   _SFCB_RETURN(rs);
//   _SFCB_RETURN(iMethodResponse(binCtx->rHdr, sb));
}


#ifdef HAVE_QUALREP
static RespSegments genQualifierResponses(BinRequestContext * binCtx,
                                 BinResponseHdr * resp)
{
   RespSegments rs;
   UtilStringBuffer *sb;
   CMPIArray *ar;
   int j;
   CMPIEnumeration *enm;
   void *object;
   CMPIStatus rc;
   
   _SFCB_ENTER(TRACE_CIMXMLPROC, "genQualifierResponses");

   ar = NewCMPIArray(resp->count, binCtx->type, NULL);

		for (j = 0; j < resp->count; j++) {
			object = relocateSerializedQualifier(resp->object[j].data);			
			rc=arraySetElementNotTrackedAt(ar, j, (CMPIValue*)&object, binCtx->type);
		}

   enm = sfcb_native_new_CMPIEnumeration(ar, NULL);
   sb = UtilFactory->newStrinBuffer(1024);
   
   qualiEnum2xml(enm, sb);
   free(ar);
   rs=iMethodResponse(binCtx->rHdr, sb);
   _SFCB_RETURN(rs);
}
#endif

RespSegments genFirstChunkResponses(BinRequestContext * binCtx,
                                 BinResponseHdr ** resp,
                                 int arrlen, int moreChunks)
{
   UtilStringBuffer *sb;
   RespSegments rs;

   _SFCB_ENTER(TRACE_CIMXMLPROC, "genResponses");

   sb=genEnumResponses(binCtx,resp,arrlen);

   rs=iMethodResponse(binCtx->rHdr, sb);
   if (moreChunks || binCtx->pDone<binCtx->pCount) rs.segments[6].txt=NULL;
   _SFCB_RETURN(rs);
}

RespSegments genChunkResponses(BinRequestContext * binCtx,
                                 BinResponseHdr ** resp,
                                 int arrlen)
{
   RespSegments rs = { NULL,0,0,NULL,
      {{2, NULL},
       {0, NULL},
       {0, NULL},
       {0, NULL},
       {0, NULL},
       {0, NULL},
       {0, NULL}} };

   _SFCB_ENTER(TRACE_CIMXMLPROC, "genChunkResponses");
   rs.segments[0].txt=(char*)genEnumResponses(binCtx,resp,arrlen);
   _SFCB_RETURN(rs);
}

RespSegments genLastChunkResponses(BinRequestContext * binCtx,
                                 BinResponseHdr ** resp, int arrlen)
{
   UtilStringBuffer *sb; 
   RespSegments rs;

   _SFCB_ENTER(TRACE_CIMXMLPROC, "genResponses");

   sb=genEnumResponses(binCtx,resp,arrlen);

   rs=iMethodGetTrailer(sb);
   _SFCB_RETURN(rs);
}

RespSegments genFirstChunkErrorResponse(BinRequestContext * binCtx, int rc, char *msg)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "genFirstChunkErrorResponse");
   _SFCB_RETURN(iMethodErrResponse(binCtx->rHdr, getErrSegment(rc,msg)));
}


static RespSegments getClass(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   CMPIObjectPath *path;
   CMPIConstClass *cls;
   UtilStringBuffer *sb;
   int irc,i,sreqSize=sizeof(GetClassReq); //-sizeof(MsgSegment);
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   GetClassReq *sreq;

   _SFCB_ENTER(TRACE_CIMXMLPROC, "getClass");
   
   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokGetClass *req = (XtokGetClass *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   if (req->properties) sreqSize+=req->properties*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->hdr.operation=OPS_GetClass;
   sreq->hdr.count=req->properties+2;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   sreq->objectPath = setObjectPathMsgSegment(path);
   sreq->principal = setCharsMsgSegment(ctx->principal);
   sreq->hdr.sessionId=ctx->sessionId;

   for (i=0; i<req->properties; i++)
      sreq->properties[i]=setCharsMsgSegment(req->propertyList[i]);

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq->hdr;
   binCtx.bHdr->flags = req->flags;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sreqSize;
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) { 
         cls = relocateSerializedConstClass(resp->object[0].data);
         sb = UtilFactory->newStrinBuffer(1024);
         cls2xml(cls, sb, binCtx.bHdr->flags);
	 free(sreq);
         _SFCB_RETURN(iMethodResponse(hdr, sb));
      }
      free(sreq);
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp->rc, 
        (char*)resp->object[0].data)));
   }
   free(sreq);
   closeProviderContext(&binCtx);

   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments deleteClass(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   CMPIObjectPath *path;
   int irc;
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   DeleteClassReq sreq;

   _SFCB_ENTER(TRACE_CIMXMLPROC, "deleteClass");
   
   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokDeleteClass *req = (XtokDeleteClass *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   memset(&sreq,0,sizeof(sreq));
   sreq.hdr.operation=OPS_DeleteClass;
   sreq.hdr.count=2;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.hdr.sessionId=ctx->sessionId;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = 0;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         _SFCB_RETURN(iMethodResponse(hdr, NULL));
      }
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp->rc, 
        (char*)resp->object[0].data)));
   }
   closeProviderContext(&binCtx);
   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments createClass(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "createClass");
   CMPIObjectPath *path;
   CMPIConstClass cls;
   ClClass *cl;
   int irc;
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   CreateClassReq sreq = BINREQ(OPS_CreateClass, 3);
   
   XtokProperty *p = NULL;
   XtokProperties *ps = NULL;
   XtokQualifier *q = NULL;
   XtokQualifiers *qs = NULL;
   XtokMethod *m = NULL;
   XtokMethods *ms = NULL;
   XtokParam *r = NULL;
   XtokParams *rs = NULL;
   XtokClass *c;
   CMPIData d;
   CMPIParameter pa;
   
   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokCreateClass *req = (XtokCreateClass *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   
   cl = ClClassNew(req->op.className.data, req->superClass ? req->superClass : NULL);
   c=&req->cls;
   
   qs=&c->qualifiers;
   for (q=qs->first; q; q=q->next) {
     if (q->value == NULL) {
       d.state=CMPI_nullValue;
       d.value.uint64=0;
     } else {
       d.state=CMPI_goodValue;
       d.value=str2CMPIValue(q->type,q->value,NULL);
     }
     d.type=q->type;
     ClClassAddQualifier(&cl->hdr, &cl->qualifiers, q->name, d);
   }
                  
   ps=&c->properties;
   for (p=ps->first; p; p=p->next) {
      ClProperty *prop;
      int propId;
      if (p->val.value == NULL) {
	d.state=CMPI_nullValue;
	d.value.uint64=0;
      } else {
	d.state=CMPI_goodValue;
	d.value=str2CMPIValue(p->valueType,p->val.value,&p->val.ref);
      }       
      d.type=p->valueType;
      propId=ClClassAddProperty(cl, p->name, d);
     
      qs=&p->val.qualifiers;
      prop=((ClProperty*)ClObjectGetClSection(&cl->hdr,&cl->properties))+propId-1;
      for (q=qs->first; q; q=q->next) {
	if (q->value == NULL) {
	  d.state=CMPI_nullValue;
	  d.value.uint64=0;
	} else {
	  d.state=CMPI_goodValue;
	  d.value=str2CMPIValue(q->type,q->value,NULL);
	}
	d.type=q->type;
	ClClassAddPropertyQualifier(&cl->hdr, prop, q->name, d);
      }
   }
   
   ms=&c->methods;
   for (m=ms->first; m; m=m->next) {
      ClMethod *meth;
      ClParameter *parm;
      int methId,parmId;
      
      methId=ClClassAddMethod(cl, m->name, m->type);
      meth=((ClMethod*)ClObjectGetClSection(&cl->hdr,&cl->methods))+methId-1;
      
      qs=&m->qualifiers;
      for (q=qs->first; q; q=q->next) {
	if (q->value == NULL) {
	  d.state=CMPI_nullValue;
	  d.value.uint64=0;
	} else {
	  d.state=CMPI_goodValue;
	  d.value=str2CMPIValue(q->type,q->value,NULL);
	}
	d.type=q->type;
	ClClassAddMethodQualifier(&cl->hdr, meth, q->name, d);
      }
      
      rs=&m->params;
      for (r=rs->first; r; r=r->next) {
         pa.type=r->type;
         pa.arraySize=(unsigned int)r->arraySize;
         pa.refName=r->refClass;
         parmId=ClClassAddMethParameter(&cl->hdr, meth, r->name, pa);
         parm=((ClParameter*)ClObjectGetClSection(&cl->hdr,&meth->parameters))+methId-1;
   
         qs=&r->qualifiers;
         for (q=qs->first; q; q=q->next) {
	   if (q->value == NULL) {
	     d.state=CMPI_nullValue;
	     d.value.uint64=0;
	   } else {
	     d.state=CMPI_goodValue;
	     d.value=str2CMPIValue(q->type,q->value,NULL);
	   }
	   d.type=q->type;
	   ClClassAddMethParamQualifier(&cl->hdr, parm, q->name, d);
         }
      }
   }
   
   cl = ClClassRebuildClass(cl,NULL); 
   cls=initConstClass(cl);

   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.path = setObjectPathMsgSegment(path);
   sreq.cls = setConstClassMsgSegment(&cls);
   sreq.hdr.sessionId=ctx->sessionId;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         _SFCB_RETURN(iMethodResponse(hdr, NULL));
      }
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp->rc, 
         (char*)resp->object[0].data)));
   }
   closeProviderContext(&binCtx);
   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments enumClassNames(CimXmlRequestContext * ctx,
                                      RequestHdr * hdr)
{
   CMPIObjectPath *path;
   EnumClassNamesReq sreq = BINREQ(OPS_EnumerateClassNames, 2);
   int irc, l = 0, err = 0;
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   
   _SFCB_ENTER(TRACE_CIMXMLPROC, "enumClassNames");

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokEnumClassNames *req = (XtokEnumClassNames *) hdr->cimRequest;
   hdr->className=req->op.className.data;
   
   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.hdr.flags = req->flags;
   sreq.hdr.sessionId=ctx->sessionId;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = req->flags;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.commHndl = ctx->commHndl;
   binCtx.type=CMPI_ref;
   binCtx.xmlAs=binCtx.noResp=0;
   binCtx.chunkedMode=0;
   binCtx.pAs=NULL;

  _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      _SFCB_TRACE(1, ("--- Back from Provider"));
      closeProviderContext(&binCtx);
      if (err == 0) {
         _SFCB_RETURN(genResponses(&binCtx, resp, l));
      }
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp[err-1]->rc, 
        (char*)resp[err-1]->object[0].data)));
    }
   closeProviderContext(&binCtx);
   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments enumClasses(CimXmlRequestContext * ctx,
                                      RequestHdr * hdr)
{
   CMPIObjectPath *path;
   EnumClassesReq sreq = BINREQ(OPS_EnumerateClasses, 2);
   int irc, l = 0, err = 0;
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   
   _SFCB_ENTER(TRACE_CIMXMLPROC, "enumClasses");

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokEnumClasses *req = (XtokEnumClasses *) hdr->cimRequest;
   hdr->className=req->op.className.data;
   
   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.hdr.flags = req->flags;
   sreq.hdr.sessionId=ctx->sessionId;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = req->flags;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.commHndl = ctx->commHndl;
   binCtx.type=CMPI_class;
   binCtx.xmlAs=binCtx.noResp=0;
   binCtx.chunkFncs=ctx->chunkFncs;
   
   if (noChunking || ctx->teTrailers==0)
      hdr->chunkedMode=binCtx.chunkedMode=0;
   else {
      sreq.hdr.flags|=FL_chunked;
      hdr->chunkedMode=binCtx.chunkedMode=1;
   }
   binCtx.pAs=NULL;

  _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      RespSegments rs;
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      _SFCB_TRACE(1, ("--- Back from Provider"));

      closeProviderContext(&binCtx);
      
      if (noChunking || ctx->teTrailers==0) {
         if (err == 0) _SFCB_RETURN(genResponses(&binCtx, resp, l))
        _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp[err-1]->rc, 
           (char*)resp[err-1]->object[0].data)));
      }
      
      rs.chunkedMode=1;
      rs.rc=err;
      rs.errMsg=NULL; 
     _SFCB_RETURN(rs);
    }
   closeProviderContext(&binCtx);
   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments getInstance(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "getInstance");
   CMPIObjectPath *path;
   CMPIInstance *inst;
   CMPIType type;
   CMPIValue val, *valp;
   UtilStringBuffer *sb;
   int irc, i, m,sreqSize=sizeof(GetInstanceReq); //-sizeof(MsgSegment);
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   RespSegments rsegs;
   GetInstanceReq *sreq;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokGetInstance *req = (XtokGetInstance *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   if (req->properties) sreqSize+=req->properties*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->hdr.operation=OPS_GetInstance;
   sreq->hdr.count=req->properties+2; 

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   for (i = 0, m = req->instanceName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->instanceName.bindings.keyBindings[i].type,
                                req->instanceName.bindings.keyBindings[i].value,
                                &req->instanceName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->instanceName.bindings.keyBindings[i].name, valp, type);
   }
   sreq->objectPath = setObjectPathMsgSegment(path);
   sreq->principal = setCharsMsgSegment(ctx->principal);
   sreq->hdr.sessionId=ctx->sessionId;

   for (i=0; i<req->properties; i++)
      sreq->properties[i]=setCharsMsgSegment(req->propertyList[i]);

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq->hdr;
   binCtx.bHdr->flags = req->flags;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sreqSize;
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         inst = relocateSerializedInstance(resp->object[0].data);
         sb = UtilFactory->newStrinBuffer(1024);
         instance2xml(inst, sb, binCtx.bHdr->flags);
         rsegs=iMethodResponse(hdr, sb);
	 free(sreq);
         _SFCB_RETURN(rsegs);
      }
      free(sreq);
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp->rc, 
        (char*)resp->object[0].data)));
   }
   free(sreq);
   closeProviderContext(&binCtx);

   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments deleteInstance(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "deleteInstance");
   CMPIObjectPath *path;
   CMPIType type;
   CMPIValue val, *valp;
   int irc, i, m;
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   DeleteInstanceReq sreq = BINREQ(OPS_DeleteInstance, 2);

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokDeleteInstance *req = (XtokDeleteInstance *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   for (i = 0, m = req->instanceName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->instanceName.bindings.keyBindings[i].type,
                                req->instanceName.bindings.keyBindings[i].value,
                                &req->instanceName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->instanceName.bindings.keyBindings[i].name, valp,
               type);
   }
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.hdr.sessionId=ctx->sessionId;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         _SFCB_RETURN(iMethodResponse(hdr, NULL));
      }
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp->rc, 
        (char*)resp->object[0].data)));
   }
   closeProviderContext(&binCtx);
   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments createInstance(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "createInst");
   CMPIObjectPath *path;
   CMPIInstance *inst;
   CMPIValue val;
   UtilStringBuffer *sb;
   int irc;
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   CreateInstanceReq sreq = BINREQ(OPS_CreateInstance, 3);
   XtokProperty *p = NULL;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokCreateInstance *req = (XtokCreateInstance *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   inst = NewCMPIInstance(path, NULL);
               
   for (p = req->instance.properties.first; p; p = p->next) {
     if (p->val.value) {
       val = str2CMPIValue(p->valueType, p->val.value, &p->val.ref);
       CMSetProperty(inst, p->name, &val, p->valueType);
     }
   }
                  
   sreq.instance = setInstanceMsgSegment(inst);
   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.hdr.sessionId=ctx->sessionId;

   path = inst->ft->getObjectPath(inst,NULL);
   sreq.path = setObjectPathMsgSegment(path);

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         path = relocateSerializedObjectPath(resp->object[0].data);
         sb = UtilFactory->newStrinBuffer(1024);
         instanceName2xml(path, sb);
         _SFCB_RETURN(iMethodResponse(hdr, sb));
      }
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp->rc, 
         (char*)resp->object[0].data)));
   }
   closeProviderContext(&binCtx);
   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments modifyInstance(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "modifyInstance");
   CMPIObjectPath *path;
   CMPIInstance *inst;
   CMPIType type;
   CMPIValue val, *valp;
   int irc, i, m, sreqSize=sizeof(ModifyInstanceReq); //-sizeof(MsgSegment);
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   ModifyInstanceReq *sreq;
   XtokInstance *xci;
   XtokInstanceName *xco;
   XtokProperty *p = NULL;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokModifyInstance *req = (XtokModifyInstance *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   if (req->properties) sreqSize+=req->properties*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->hdr.operation=OPS_ModifyInstance;
   sreq->hdr.count=req->properties+3;

   for (i=0; i<req->properties; i++){
      sreq->properties[i]=setCharsMsgSegment(req->propertyList[i]);
   }
   xci = &req->namedInstance.instance;
   xco = &req->namedInstance.path;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   for (i = 0, m = xco->bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(xco->bindings.keyBindings[i].type,
                                xco->bindings.keyBindings[i].value,
                                &xco->bindings.keyBindings[i].ref,
                                &val, &type);
                           
      CMAddKey(path, xco->bindings.keyBindings[i].name, valp, type);
   }

   inst = NewCMPIInstance(path, NULL);
   for (p = xci->properties.first; p; p = p->next) {
     if (p->val.value) {
       val = str2CMPIValue(p->valueType, p->val.value, &p->val.ref);
       CMSetProperty(inst, p->name, &val, p->valueType);
     }
   }
   sreq->instance = setInstanceMsgSegment(inst);
   sreq->path = setObjectPathMsgSegment(path);
   sreq->principal = setCharsMsgSegment(ctx->principal);
   sreq->hdr.sessionId=ctx->sessionId;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq->hdr;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sreqSize;
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      free(sreq);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         _SFCB_RETURN(iMethodResponse(hdr, NULL));
      }
     _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp->rc,
        (char*)resp->object[0].data)));
   }
   closeProviderContext(&binCtx);
   free(sreq);

   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}


static RespSegments enumInstanceNames(CimXmlRequestContext * ctx,
                                      RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "enumInstanceNames");
   CMPIObjectPath *path;
   EnumInstanceNamesReq sreq = BINREQ(OPS_EnumerateInstanceNames, 2);
   int irc, l = 0, err = 0;
   BinResponseHdr **resp;
   BinRequestContext binCtx;

   memset(&binCtx,0,sizeof(BinRequestContext));

   XtokEnumInstanceNames *req = (XtokEnumInstanceNames *) hdr->cimRequest;
   hdr->className=req->op.className.data;
   
   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.hdr.sessionId=ctx->sessionId;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = 0;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.commHndl = ctx->commHndl;
   binCtx.type=CMPI_ref;
   binCtx.xmlAs=binCtx.noResp=0;
   binCtx.chunkedMode=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      _SFCB_TRACE(1, ("--- Back from Provider"));

      closeProviderContext(&binCtx);
      if (err == 0) {
         _SFCB_RETURN(genResponses(&binCtx, resp, l));
      }
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp[err-1]->rc, 
        (char*)resp[err-1]->object[0].data)));
    }
   closeProviderContext(&binCtx);
   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments enumInstances(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "enumInstances");

   CMPIObjectPath *path;
   EnumInstancesReq *sreq;
   int irc, l = 0, err = 0,i,sreqSize=sizeof(EnumInstancesReq); //-sizeof(MsgSegment);
   BinResponseHdr **resp;
   BinRequestContext binCtx;

   memset(&binCtx,0,sizeof(BinRequestContext));

   XtokEnumInstances *req = (XtokEnumInstances *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   if (req->properties) sreqSize+=req->properties*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->hdr.operation=OPS_EnumerateInstances;
   sreq->hdr.count=req->properties+2;  

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   sreq->principal = setCharsMsgSegment(ctx->principal);
   sreq->objectPath = setObjectPathMsgSegment(path);
   sreq->hdr.sessionId=ctx->sessionId;

   for (i=0; i<req->properties; i++) {
      sreq->properties[i]=setCharsMsgSegment(req->propertyList[i]);
   }   

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq->hdr;
   binCtx.bHdr->flags = req->flags;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sreqSize;
   binCtx.commHndl = ctx->commHndl;
   
   binCtx.type=CMPI_instance;
   binCtx.xmlAs=binCtx.noResp=0;
   binCtx.chunkFncs=ctx->chunkFncs;
   
   if (noChunking || ctx->teTrailers==0)
      hdr->chunkedMode=binCtx.chunkedMode=0;
   else {
      sreq->hdr.flags|=FL_chunked;
      hdr->chunkedMode=binCtx.chunkedMode=1;
   }
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);
   _SFCB_TRACE(1, ("--- Provider context gotten irc: %d",irc));
   
   if (irc == MSG_X_PROVIDER) {
      RespSegments rs;
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      _SFCB_TRACE(1, ("--- Back from Providers"));
      closeProviderContext(&binCtx);
      
      if (noChunking || ctx->teTrailers==0) {
         if (err == 0) {
	   RespSegments rs = genResponses(&binCtx, resp, l);
	   free(sreq);
	   _SFCB_RETURN(rs);
	 }
	 free(sreq);
        _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp[err-1]->rc, 
           (char*)resp[err-1]->object[0].data)));
      }
      
      free(sreq);
      rs.chunkedMode=1;
      rs.rc=err;
      rs.errMsg=NULL; 
     _SFCB_RETURN(rs);
   }
   closeProviderContext(&binCtx);
   free(sreq);
   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments execQuery(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "execQuery");

   CMPIObjectPath *path;
   ExecQueryReq sreq = BINREQ(OPS_ExecQuery, 4);
   int irc, l = 0, err = 0;
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   QLStatement *qs=NULL;
   char **fCls;
   
   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokExecQuery *req = (XtokExecQuery*)hdr->cimRequest;
   hdr->className=req->op.className.data;

   qs=parseQuery(MEM_TRACKED,(char*)req->op.query.data,
      (char*)req->op.queryLang.data,NULL,&irc);   
   
   fCls=qs->ft->getFromClassList(qs);
   if (fCls==NULL || *fCls==NULL) {
     mlogf(M_ERROR,M_SHOW,"--- from clause missing\n");
     abort();
   }
   req->op.className = setCharsMsgSegment(*fCls);
   
   path = NewCMPIObjectPath(req->op.nameSpace.data, *fCls, NULL);
   
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.query = setCharsMsgSegment((char*)req->op.query.data);
   sreq.queryLang = setCharsMsgSegment((char*)req->op.queryLang.data);
   sreq.hdr.sessionId=ctx->sessionId;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = 0;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.commHndl = ctx->commHndl;
   binCtx.type=CMPI_instance;
   binCtx.xmlAs=XML_asObj; binCtx.noResp=0;
   binCtx.chunkFncs=ctx->chunkFncs;
   
   if (noChunking || ctx->teTrailers==0)
      hdr->chunkedMode=binCtx.chunkedMode=0;
   else {
      sreq.hdr.flags|=FL_chunked;
      hdr->chunkedMode=binCtx.chunkedMode=1;
   }
   binCtx.pAs=NULL;
   
   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   
   if (irc == MSG_X_PROVIDER) {
      RespSegments rs;
      _SFCB_TRACE(1, ("--- Calling Provider"));
      resp = invokeProviders(&binCtx, &err, &l);
      _SFCB_TRACE(1, ("--- Back from Provider"));
      closeProviderContext(&binCtx);
      
      if (noChunking || ctx->teTrailers==0) {
         if (err == 0) _SFCB_RETURN(genResponses(&binCtx, resp, l))
        _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp[err-1]->rc, 
           (char*)resp[err-1]->object[0].data)));
      }
      
      rs.chunkedMode=1;
      rs.rc=err;
      rs.errMsg=NULL; 
      _SFCB_RETURN(rs);
   }
   closeProviderContext(&binCtx);
   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}




static RespSegments associatorNames(CimXmlRequestContext * ctx,
                                    RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "associatorNames");
   CMPIObjectPath *path;
   AssociatorNamesReq sreq = BINREQ(OPS_AssociatorNames, 6);
   int irc, i, m, l = 0, err = 0;
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   CMPIType type;
   CMPIValue val, *valp;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokAssociatorNames *req = (XtokAssociatorNames *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   for (i = 0, m = req->objectName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->objectName.bindings.keyBindings[i].type,
                                req->objectName.bindings.keyBindings[i].value,
                                &req->objectName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->objectName.bindings.keyBindings[i].name, valp, type);
   }
   
   if (req->objectName.bindings.next==0) {
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(CMPI_RC_ERR_NOT_SUPPORTED, 
           "AssociatorNames operation for classes not supported")));
   }
   
   sreq.objectPath = setObjectPathMsgSegment(path);

   sreq.resultClass = req->op.resultClass;
   sreq.role = req->op.role;
   sreq.assocClass = req->op.assocClass;
   sreq.resultRole = req->op.resultRole;
   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.hdr.sessionId=ctx->sessionId;

   req->op.className = req->op.assocClass;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = 0;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.commHndl = ctx->commHndl;
   binCtx.type=CMPI_ref;
   binCtx.xmlAs=XML_asObj; binCtx.noResp=0;
   binCtx.chunkedMode=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);
   _SFCB_TRACE(1, ("--- Provider context gotten"));

   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      _SFCB_TRACE(1, ("--- Back from Providers"));

       closeProviderContext(&binCtx);
       if (err == 0) {
          _SFCB_RETURN(genResponses(&binCtx, resp, l))
       }
       _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp[err-1]->rc, 
           (char*)resp[err-1]->object[0].data)));
   }
   closeProviderContext(&binCtx);
   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments associators(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "associators");

   CMPIObjectPath *path;
   AssociatorsReq *sreq; 
   int irc, i, m, l = 0, err = 0, sreqSize=sizeof(AssociatorsReq); //-sizeof(MsgSegment);
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   CMPIType type;
   CMPIValue val, *valp;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokAssociators *req = (XtokAssociators *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   if (req->properties) sreqSize+=req->properties*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->hdr.operation=OPS_Associators;
   sreq->hdr.count=req->properties+6;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   for (i = 0, m = req->objectName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->objectName.bindings.keyBindings[i].type,
                                req->objectName.bindings.keyBindings[i].value,
                                &req->objectName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->objectName.bindings.keyBindings[i].name, valp, type);
   }
   
   if (req->objectName.bindings.next==0) {
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(CMPI_RC_ERR_NOT_SUPPORTED, 
           "Associator operation for classes not supported")));
   }
   
   sreq->objectPath = setObjectPathMsgSegment(path);

   sreq->resultClass = req->op.resultClass;
   sreq->role = req->op.role;
   sreq->assocClass = req->op.assocClass;
   sreq->resultRole = req->op.resultRole;
   sreq->hdr.flags = req->flags;
   sreq->principal = setCharsMsgSegment(ctx->principal);
   sreq->hdr.sessionId=ctx->sessionId;

   for (i=0; i<req->properties; i++)
      sreq->properties[i]=setCharsMsgSegment(req->propertyList[i]);

   req->op.className = req->op.assocClass;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq->hdr;
   binCtx.bHdr->flags = req->flags;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sreqSize;
   binCtx.commHndl = ctx->commHndl;
   binCtx.type=CMPI_instance;
   binCtx.xmlAs=XML_asObj; binCtx.noResp=0;
   binCtx.pAs=NULL;
   binCtx.chunkFncs=ctx->chunkFncs;
   
   if (noChunking || ctx->teTrailers==0)
      hdr->chunkedMode=binCtx.chunkedMode=0;
   else {
      sreq->hdr.flags|=FL_chunked;
      hdr->chunkedMode=binCtx.chunkedMode=1;
   }

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      RespSegments rs;
      _SFCB_TRACE(1, ("--- Calling Provider"));
      resp = invokeProviders(&binCtx, &err, &l);
      _SFCB_TRACE(1, ("--- Back from Provider"));
      
      closeProviderContext(&binCtx);
      
      if (noChunking || ctx->teTrailers==0) {
         if (err == 0) {
	   RespSegments rs = genResponses(&binCtx, resp, l);
	   free(sreq);
	   _SFCB_RETURN(rs);
	 }
	 free(sreq);
        _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp[err-1]->rc, 
           (char*)resp[err-1]->object[0].data)));
      }
      
      free(sreq);
      rs.chunkedMode=1;
      rs.rc=err;
      rs.errMsg=NULL; 
     _SFCB_RETURN(rs);
   }
   closeProviderContext(&binCtx);
   free(sreq);

   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments referenceNames(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "referenceNames");
   CMPIObjectPath *path;
   ReferenceNamesReq sreq = BINREQ(OPS_ReferenceNames, 4);
   int irc, i, m, l = 0, err = 0;
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   CMPIType type;
   CMPIValue val, *valp;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokReferenceNames *req = (XtokReferenceNames *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   for (i = 0, m = req->objectName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->objectName.bindings.keyBindings[i].type,
                                req->objectName.bindings.keyBindings[i].value,
                                &req->objectName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->objectName.bindings.keyBindings[i].name, valp, type);
   }
   
   if (req->objectName.bindings.next==0) {
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(CMPI_RC_ERR_NOT_SUPPORTED, 
           "ReferenceNames operation for classes not supported")));
   }
   
   sreq.objectPath = setObjectPathMsgSegment(path);

   sreq.resultClass = req->op.resultClass;
   sreq.role = req->op.role;
   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.hdr.sessionId=ctx->sessionId;

   req->op.className = req->op.resultClass;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = 0;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.commHndl = ctx->commHndl;
   binCtx.type=CMPI_ref;
   binCtx.xmlAs=XML_asObj; binCtx.noResp=0;
   binCtx.chunkedMode=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);
   _SFCB_TRACE(1, ("--- Provider context gotten"));

   if (irc == MSG_X_PROVIDER) {
      _SFCB_TRACE(1, ("--- Calling Providers"));
      resp = invokeProviders(&binCtx, &err, &l);
      _SFCB_TRACE(1, ("--- Back from Providers"));

      closeProviderContext(&binCtx);
      if (err == 0) {
         _SFCB_RETURN(genResponses(&binCtx, resp, l));
      }
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp[err-1]->rc, 
           (char*)resp[err-1]->object[0].data)));
   }
   closeProviderContext(&binCtx);
   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}


static RespSegments references(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "references");

   CMPIObjectPath *path;
   AssociatorsReq *sreq; 
   int irc, i, m, l = 0, err = 0, sreqSize=sizeof(AssociatorsReq); //-sizeof(MsgSegment);
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   CMPIType type;
   CMPIValue val, *valp;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokReferences *req = (XtokReferences *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   if (req->properties) sreqSize+=req->properties*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->hdr.operation=OPS_References;
   sreq->hdr.count=req->properties+4;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   for (i = 0, m = req->objectName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->objectName.bindings.keyBindings[i].type,
                                req->objectName.bindings.keyBindings[i].value,
                                &req->objectName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->objectName.bindings.keyBindings[i].name, valp, type);
   }
   
   if (req->objectName.bindings.next==0) {
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(CMPI_RC_ERR_NOT_SUPPORTED, 
           "References operation for classes not supported")));
   }
   
   sreq->objectPath = setObjectPathMsgSegment(path);

   sreq->resultClass = req->op.resultClass;
   sreq->role = req->op.role;
   sreq->hdr.flags = req->flags;
   sreq->principal = setCharsMsgSegment(ctx->principal);
   sreq->hdr.sessionId=ctx->sessionId;

   for (i=0; i<req->properties; i++)
      sreq->properties[i]=setCharsMsgSegment(req->propertyList[i]);

   req->op.className = req->op.resultClass;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq->hdr;
   binCtx.bHdr->flags = req->flags;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sreqSize;
   binCtx.commHndl = ctx->commHndl;
   binCtx.type=CMPI_instance;
   binCtx.xmlAs=XML_asObj; binCtx.noResp=0;
   binCtx.pAs=NULL;
   binCtx.chunkFncs=ctx->chunkFncs;
   
   if (noChunking || ctx->teTrailers==0)
      hdr->chunkedMode=binCtx.chunkedMode=0;
   else {
      sreq->hdr.flags|=FL_chunked;
      hdr->chunkedMode=binCtx.chunkedMode=1;
   }

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      RespSegments rs;
      _SFCB_TRACE(1, ("--- Calling Provider"));
      resp = invokeProviders(&binCtx, &err, &l);
      _SFCB_TRACE(1, ("--- Back from Provider"));
      closeProviderContext(&binCtx);
      
      if (noChunking || ctx->teTrailers==0) {
	if (err == 0) { 
	   RespSegments rs = genResponses(&binCtx, resp, l);
	   free(sreq);
	   _SFCB_RETURN(rs);
	 }
	free(sreq);
        _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp[err-1]->rc, 
           (char*)resp[err-1]->object[0].data)));
      }
      
      free(sreq);
      rs.chunkedMode=1;
      rs.rc=err;
      rs.errMsg=NULL; 
     _SFCB_RETURN(rs);
   }
   closeProviderContext(&binCtx);
   free(sreq);

   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments invokeMethod(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "invokeMethod");
   CMPIObjectPath *path;
   CMPIArgs *out;
   CMPIType type;
   CMPIValue val, *valp;
   UtilStringBuffer *sb;
   int irc, i, m;
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   RespSegments rsegs;
   InvokeMethodReq sreq = BINREQ(OPS_InvokeMethod, 5);
   CMPIArgs *in=TrackedCMPIArgs(NULL);
   XtokParamValue *p;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokMethodCall *req = (XtokMethodCall *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   if (req->instName) for (i = 0, m = req->instanceName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->instanceName.bindings.keyBindings[i].type,
                                req->instanceName.bindings.keyBindings[i].value,
                                &req->instanceName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->instanceName.bindings.keyBindings[i].name, valp, type);
   }
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.hdr.sessionId=ctx->sessionId;

   for (p = req->paramValues.first; p; p = p->next) {
      // this is a problem: - paramvalue without type
      if (p->type==0) p->type=CMPI_string;
      if (p->value.value) {
	CMPIValue val = str2CMPIValue(p->type, p->value.value, &p->valueRef);
	CMAddArg(in, p->name, &val, p->type);
      }
   }   
   sreq.in = setArgsMsgSegment(in);
   sreq.out = setArgsMsgSegment(NULL);
   sreq.method = setCharsMsgSegment(req->method);
   
   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = 0;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(InvokeMethodReq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         out = relocateSerializedArgs(resp->object[0].data);
         sb = UtilFactory->newStrinBuffer(1024);
         sb->ft->appendChars(sb,"<RETURNVALUE>\n");
         value2xml(resp->rv, sb, 1);
         sb->ft->appendChars(sb,"</RETURNVALUE>\n");
         args2xml(out, sb);
         rsegs=methodResponse(hdr, sb);
         _SFCB_RETURN(rsegs);
      }
      _SFCB_RETURN(methodErrResponse(hdr, getErrSegment(resp->rc, 
        (char*)resp->object[0].data)));
   }
   closeProviderContext(&binCtx);

   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,1));
}

#ifdef HAVE_QUALREP
static RespSegments enumQualifiers(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
	CMPIObjectPath *path;
	EnumClassNamesReq sreq = BINREQ(OPS_EnumerateQualifiers, 2);
	int irc;
	BinResponseHdr *resp;
	BinRequestContext binCtx;

	_SFCB_ENTER(TRACE_CIMXMLPROC, "enumQualifiers");

	memset(&binCtx,0,sizeof(BinRequestContext));
	XtokEnumQualifiers *req = (XtokEnumQualifiers *) hdr->cimRequest;

	path = NewCMPIObjectPath(req->op.nameSpace.data, NULL, NULL);
	sreq.objectPath = setObjectPathMsgSegment(path);
	sreq.principal = setCharsMsgSegment(ctx->principal);
	sreq.hdr.sessionId=ctx->sessionId;

	binCtx.oHdr = (OperationHdr *) req;
	binCtx.bHdr = &sreq.hdr;
	binCtx.rHdr = hdr;
	binCtx.bHdrSize = sizeof(sreq);
	binCtx.commHndl = ctx->commHndl;
	binCtx.type=CMPI_qualifierDecl;
	binCtx.xmlAs=binCtx.noResp=0;
	binCtx.chunkedMode=0;
	binCtx.pAs=NULL;

	_SFCB_TRACE(1, ("--- Getting Provider context"));
	irc = getProviderContext(&binCtx, (OperationHdr *) req);

	_SFCB_TRACE(1, ("--- Provider context gotten"));
	if (irc == MSG_X_PROVIDER) {
		_SFCB_TRACE(1, ("--- Calling Providers"));
		resp = invokeProvider(&binCtx);
		_SFCB_TRACE(1, ("--- Back from Provider"));
		closeProviderContext(&binCtx);
		resp->rc--;
		if (resp->rc == CMPI_RC_OK) {
			_SFCB_RETURN(genQualifierResponses(&binCtx, resp));
		}
	_SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp->rc,
		(char*)resp->object[0].data)));
	}
	closeProviderContext(&binCtx);
	_SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments getQualifier(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "getQualifier");
   CMPIObjectPath *path;
   CMPIQualifierDecl *qual;
   CMPIStatus rc;
   UtilStringBuffer *sb;
   int irc;
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   RespSegments rsegs;
   GetQualifierReq sreq = BINREQ(OPS_GetQualifier, 2);

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokGetQualifier *req = (XtokGetQualifier *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->name, &rc); //abuse classname for qualifier name

   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.path = setObjectPathMsgSegment(path);
   sreq.hdr.sessionId=ctx->sessionId;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         qual = relocateSerializedQualifier(resp->object[0].data);
         sb = UtilFactory->newStrinBuffer(1024);
         qualifierDeclaration2xml(qual, sb);
         rsegs=iMethodResponse(hdr, sb);
         _SFCB_RETURN(rsegs);
      }
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp->rc, 
        (char*)resp->object[0].data)));
   }
   closeProviderContext(&binCtx);

   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments deleteQualifier(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "deleteQualifier");
   CMPIObjectPath *path;
   CMPIStatus rc;
   int irc;
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   DeleteQualifierReq sreq = BINREQ(OPS_DeleteQualifier, 2);

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokDeleteQualifier *req = (XtokDeleteQualifier *) hdr->cimRequest;
   hdr->className=req->op.className.data;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->name, &rc); //abuse classname for qualifier name

   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.path = setObjectPathMsgSegment(path);
   sreq.hdr.sessionId=ctx->sessionId;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   if (irc == MSG_X_PROVIDER) {
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
		_SFCB_RETURN(iMethodResponse(hdr, NULL));
      }
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp->rc, 
        (char*)resp->object[0].data)));
   }
   closeProviderContext(&binCtx);

   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}

static RespSegments setQualifier(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "setQualifier");
   CMPIObjectPath *path;
   CMPIQualifierDecl qual;
   CMPIData d;
   ClQualifierDeclaration *q;
   int irc;
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   SetQualifierReq sreq = BINREQ(OPS_SetQualifier, 3);

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokSetQualifier *req = (XtokSetQualifier *) hdr->cimRequest;
   
   path = NewCMPIObjectPath(req->op.nameSpace.data, NULL, NULL);   
   q = ClQualifierDeclarationNew(req->op.nameSpace.data, req->qualifierdeclaration.name);
   
	if (req->qualifierdeclaration.overridable) q->flavor |= ClQual_F_Overridable;
	if (req->qualifierdeclaration.tosubclass) q->flavor |= ClQual_F_ToSubclass;
	if (req->qualifierdeclaration.toinstance) q->flavor |= ClQual_F_ToInstance;
	if (req->qualifierdeclaration.translatable) q->flavor |= ClQual_F_Translatable;
	if (req->qualifierdeclaration.isarray) q->type |= CMPI_ARRAY;
   
	if (req->qualifierdeclaration.type) q->type |= req->qualifierdeclaration.type;
   
	if(req->qualifierdeclaration.scope.class) q->scope |= ClQual_S_Class;
	if(req->qualifierdeclaration.scope.association) q->scope |= ClQual_S_Association;
	if(req->qualifierdeclaration.scope.reference) q->scope |= ClQual_S_Reference;
	if(req->qualifierdeclaration.scope.property) q->scope |= ClQual_S_Property;
	if(req->qualifierdeclaration.scope.method) q->scope |= ClQual_S_Method;
	if(req->qualifierdeclaration.scope.parameter) q->scope |= ClQual_S_Parameter;
	if(req->qualifierdeclaration.scope.indication) q->scope |= ClQual_S_Indication;
    q->arraySize = req->qualifierdeclaration.arraySize;

	if (req->qualifierdeclaration.data.value) {//default value is set
		d.state=CMPI_goodValue;
		d.type=q->type; //"specified" type
		d.type|=req->qualifierdeclaration.data.type; //actual type
		
		//default value declared - isarray attribute must match, if set
		if(req->qualifierdeclaration.isarrayIsSet)
			if(!req->qualifierdeclaration.isarray ^ !(req->qualifierdeclaration.data.type & CMPI_ARRAY))
				_SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(CMPI_RC_ERROR, 
           		"ISARRAY attribute and default value conflict")));   			
		
		d.value=str2CMPIValue(d.type, req->qualifierdeclaration.data.value,
			(XtokValueReference *)&req->qualifierdeclaration.data.valueArray);		
		ClQualifierAddQualifier(&q->hdr, &q->qualifierData, req->qualifierdeclaration.name, d);		
     } else { //no default value - rely on ISARRAY attr, check if it's set
     	/*if(!req->qualifierdeclaration.isarrayIsSet)
			_SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(CMPI_RC_ERROR, 
           "ISARRAY attribute MUST be present if the Qualifier declares no default value")));*/
           q->qualifierData.sectionOffset=0;
           q->qualifierData.used=0;
           q->qualifierData.max=0;
	}
     
   qual = initQualifier(q);
               
   sreq.qualifier = setQualifierMsgSegment(&qual);
   sreq.principal = setCharsMsgSegment(ctx->principal);
   sreq.hdr.sessionId=ctx->sessionId;
   sreq.path = setObjectPathMsgSegment(path);

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.chunkedMode=binCtx.xmlAs=binCtx.noResp=0;
   binCtx.pAs=NULL;

   _SFCB_TRACE(1, ("--- Getting Provider context"));
   irc = getProviderContext(&binCtx, (OperationHdr *) req);

   _SFCB_TRACE(1, ("--- Provider context gotten"));
   
   if (irc == MSG_X_PROVIDER) {
      resp = invokeProvider(&binCtx);
      closeProviderContext(&binCtx);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         _SFCB_RETURN(iMethodResponse(hdr, NULL));
      }   
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp->rc, 
         (char*)resp->object[0].data)));
   }
   closeProviderContext(&binCtx); 
   _SFCB_RETURN(ctxErrResponse(hdr, &binCtx,0));
}
#endif

static RespSegments notSupported(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   return iMethodErrResponse(hdr, strdup
           ("<ERROR CODE=\"7\" DESCRIPTION=\"Operation not supported xx\"/>\n"));
}


static Handler handlers[] = {
   {notSupported},              //dummy
   {getClass},                  //OPS_GetClass 1
   {getInstance},               //OPS_GetInstance 2
   {deleteClass},                //OPS_DeleteClass 3
   {deleteInstance},            //OPS_DeleteInstance 4
   {createClass},               //OPS_CreateClass 5
   {createInstance},            //OPS_CreateInstance 6
   {notSupported},              //OPS_ModifyClass 7
   {modifyInstance},            //OPS_ModifyInstance 8
   {enumClasses},               //OPS_EnumerateClasses 9
   {enumClassNames},            //OPS_EnumerateClassNames 10
   {enumInstances},             //OPS_EnumerateInstances 11
   {enumInstanceNames},         //OPS_EnumerateInstanceNames 12
   {execQuery},                 //OPS_ExecQuery 13
   {associators},               //OPS_Associators 14
   {associatorNames},           //OPS_AssociatorNames 15
   {references},                //OPS_References 16
   {referenceNames},            //OPS_ReferenceNames 17
   {notSupported},              //OPS_GetProperty 18
   {notSupported},              //OPS_SetProperty 19
#ifdef HAVE_QUALREP   
   {getQualifier},              //OPS_GetQualifier 20
   {setQualifier},              //OPS_SetQualifier 21
   {deleteQualifier},           //OPS_DeleteQualifier 22
   {enumQualifiers},            //OPS_EnumerateQualifiers 23
#else
   {notSupported},              //OPS_GetQualifier 20
   {notSupported},              //OPS_SetQualifier 21
   {notSupported},              //OPS_DeleteQualifier 22
   {notSupported},              //OPS_EnumerateQualifiers 23
#endif   
   {invokeMethod},              //OPS_InvokeMethod 24
};

RespSegments handleCimXmlRequest(CimXmlRequestContext * ctx)
{
   RespSegments rs;
   RequestHdr hdr = scanCimXmlRequest(ctx->cimXmlDoc);

   Handler hdlr = handlers[hdr.opType];
   rs = hdlr.handler(ctx, &hdr);
   ctx->className=hdr.className;
   ctx->operation=hdr.opType;
   rs.buffer = hdr.xmlBuffer;

   return rs;
}


int cleanupCimXmlRequest(RespSegments * rs)
{
   XmlBuffer *xmb = (XmlBuffer *) rs->buffer;
   free(xmb->base);
   free(xmb);
   return 0;
}
