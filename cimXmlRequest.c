
/*
 * cimXmlRequest.c
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
 * Author:       Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 * CMPI broker encapsulated functionality.
 *
 * CIM operations request handler .
 *
*/


#define CMPI_VERSION 90

#include "cimXmlRequest.h"
#include "cimXmlParser.h"
#include "msgqueue.h"
#include "constClass.h"

#include "native.h"
#include "trace.h"
#include "utilft.h"
#include "string.h"

#include "queryOperation.h"

typedef struct handler {
   RespSegments(*handler) (CimXmlRequestContext *, RequestHdr * hdr);
} Handler;

extern int noChunking;

extern CMPIBroker *Broker;
extern UtilStringBuffer *instanceToString(CMPIInstance * ci, char **props);
extern const char *getErrorId(int c);
extern const char *instGetClassName(CMPIInstance * ci);
extern int ClClassGetPropertyAt(ClClass * inst, int id, CMPIData * data,
                                char **name, unsigned long *quals);
extern int ClClassGetPropertyCount(ClClass * inst);
extern int ClClassGetQualifierCount(ClClass * cls);
extern int ClClassGetQualifierAt(ClClass * cls, int id, CMPIData * data,
                                 char **name);
extern int ClInstanceGetPropertyCount(ClInstance * inst);
extern int ClInstanceGetPropertyAt(ClInstance * inst, int id, CMPIData * data,
                                   char **name, unsigned long *quals);
extern int ClClassGetPropQualifierCount(ClClass * cls, int p);
extern int ClClassGetPropQualifierAt(ClClass * cls, int p, int id,
                                     CMPIData * data, char **name);

extern CMPIData opGetKeyCharsAt(CMPIObjectPath * cop, unsigned int index,
                                const char **name, CMPIStatus * rc);
extern MsgSegment setObjectPathMsgSegment(CMPIObjectPath * op);
extern void getSerializedObjectPath(CMPIObjectPath * op, void *area);
extern BinResponseHdr *invokeProvider(BinRequestContext * ctx);
extern CMPIArgs *relocateSerializedArgs(void *area);
extern CMPIObjectPath *relocateSerializedObjectPath(void *area);
extern CMPIInstance *relocateSerializedInstance(void *area);
extern CMPIConstClass *relocateSerializedConstClass(void *area);
extern MsgSegment setInstanceMsgSegment(CMPIInstance * ci);
extern MsgSegment setCharsMsgSegment(char *str);
extern MsgSegment setArgsMsgSegment(CMPIArgs * args);
extern void closeProviderContext(BinRequestContext * ctx);
extern CMPIStatus arraySetElementNotTrackedAt(CMPIArray * array,
             CMPICount index, CMPIValue * val, CMPIType type);
extern QLStatement *parseQuery(int mode, char *query, char *lang, int *rc);

const char *opGetClassNameChars(CMPIObjectPath * cop);
char *XMLEscape(char *in);

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

static char iResponseIntro1[] =
  "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
  "<CIM CIMVERSION=\"2.0\" DTDVERSION=\"1.1\">\n" 
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
  "<CIM CIMVERSION=\"2.0\" DTDVERSION=\"1.1\">\n" 
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

static char *getErrorSegment(CMPIStatus rc)
{
   if (rc.msg && rc.msg->hdl) {
      return getErrSegment(rc.rc, (char *) rc.msg->hdl);
   }
   return getErrSegment(rc.rc, NULL);
}

char *getErrTrailer(int id, int rc, char *m)
{
   char msg[1024];
   
   if (m && *m) snprintf(msg, sizeof(msg), "CIMStatusCodeDescription: %s\n",m);
   else if (rc > 0 && rc < 18)
      snprintf(msg, sizeof(msg), "CIMStatusCodeDescription: %s\n",cimMsg[rc]);
   else snprintf(msg, sizeof(msg), "CIMStatusCodeDescription: *Unknown*\n");
   return strdup(msg);
}



CMPIValue *getKeyValueTypePtr(char *type, char *value, XtokValueReference *ref,
                              CMPIValue * val, CMPIType * typ)
{
   if (type) {
      if (strcasecmp(type, "string") == 0);
      else if (strcasecmp(type, "boolean") == 0) {
         *typ = CMPI_boolean;
         if (strcasecmp(type, "true") == 0)
            val->boolean = 1;
         else
            val->boolean = 0;
         return val;
      }
      else if (strcasecmp(type, "numeric") == 0) {
         if (value[0] == '+' || value[0] == '-') {
            *typ = CMPI_sint64;
            sscanf(value, "%llu", &val->uint64);
         }
         else {
            sscanf(value, "%lld", &val->sint64);
            *typ = CMPI_uint64;
         }
         return val;
      }
      else if (strcasecmp(type, "ref") == 0) {
         CMPIObjectPath *op;
         char *hn="",*ns="",*cn;
         CMPIType type;
         CMPIValue v, *valp;
         int i,m;
         XtokInstanceName *in; 
         
         switch(ref->type) {
         case typeValRef_InstancePath: 
            in=&ref->instancePath.instanceName;
            hn=ref->instancePath.path.host.host;
            ns=ref->instancePath.path.nameSpacePath;
            break;   
         case typeValRef_InstanceName: 
            in=&ref->instanceName;
            break;   
         default:
            printf("%s(%d): unexpected reference type %d %x\n", __FILE__, __LINE__, 
               (int) type, (int) type);
            abort();   
         }

         cn=in->className;
         op=NewCMPIObjectPath(ns,cn,NULL);
         CMSetHostname(op,hn);

         for (i = 0, m = in->bindings.next; i < m; i++) {
            valp = getKeyValueTypePtr(
               in->bindings.keyBindings[i].type,
               in->bindings.keyBindings[i].value,
               &in->bindings.keyBindings[i].ref,
               &v, &type);
            CMAddKey(op,in->bindings.keyBindings[i].name,valp,type);
         }
         *typ = CMPI_ref;
         val->ref=op;
         return val;
      }
   }
   
   *typ = CMPI_chars;
   return (CMPIValue *) value;
}

static char *dataType(CMPIType type)
{
   switch (type & ~CMPI_ARRAY) {
   case CMPI_chars:
   case CMPI_string:
      return "string";
   case CMPI_sint64:
      return "sint64";
   case CMPI_uint64:
      return "uint64";
   case CMPI_sint32:
      return "sint32";
   case CMPI_uint32:
      return "uint32";
   case CMPI_sint16:
      return "sint16";
   case CMPI_uint16:
      return "uint16";
   case CMPI_uint8:
      return "uint8";
   case CMPI_sint8:
      return "sint8";
   case CMPI_boolean:
      return "boolean";
   case CMPI_char16:
      return "char16";
   case CMPI_real32:
      return "real32";
   case CMPI_real64:
      return "real64";
   case CMPI_dateTime:
      return "datetime"; 
   case CMPI_ref:
      return "*";
   }
   printf("%s(%d): invalid data type %d %x\n", __FILE__, __LINE__, (int) type,
          (int) type);
   asm("int $3");
   abort();
   return "*??*";
}

static char *keyType(CMPIType type)
{
   switch (type) {
   case CMPI_chars:
   case CMPI_string:
      return "string";
   case CMPI_sint64:
   case CMPI_uint64:
   case CMPI_sint32:
   case CMPI_uint32:
   case CMPI_sint16:
   case CMPI_uint16:
   case CMPI_uint8:
   case CMPI_sint8:
      return "numeric";
   case CMPI_boolean:
      return "boolean";
   case CMPI_ref:
      return "*";
   }
   printf("%s(%d): invalid key data type %d %x\n", __FILE__, __LINE__,
          (int) type, (int) type);
   asm("int $3");
   abort();
   return "*??*";
}

CMPIType guessType(char *val)
{
   if (((*val=='-' || *val=='+') && strlen(val)>1) || isdigit(*val)) {
      char *c;
      for (c=val+1; ; c++) {
         if (*c==0) {
            if (!isdigit(*val)) return CMPI_sint64;
            return CMPI_uint64;
         }
         if (!isdigit(*c)) break;
      }
   }
   else if (strcasecmp(val,"true")) return CMPI_boolean;
   else if (strcasecmp(val,"false")) return CMPI_boolean;
   return CMPI_string;
}

static CMPIValue str2CMPIValue(CMPIType type, char *val, XtokValueReference *ref)
{
   CMPIValue value,*valp;
 //  char *val=p->value;
   CMPIType t;

   if (type==0) {
      type=guessType(val);
   }
   
   switch (type) {
   case CMPI_chars:
      value.char16 = *val;
      break;
   case CMPI_string:
      value.string = native_new_CMPIString(val, NULL);
      break;
   case CMPI_sint64:
      sscanf(val, "%lld", &value.sint64);
      break;
   case CMPI_uint64:
      sscanf(val, "%llu", &value.uint64);
      break;
   case CMPI_sint32:
      sscanf(val, "%ld", &value.sint32);
      break;
   case CMPI_uint32:
      sscanf(val, "%lu", &value.uint32);
      break;
   case CMPI_sint16:
      sscanf(val, "%hd", &value.sint16);
      break;
   case CMPI_uint16:
      sscanf(val, "%hu", &value.uint16);
      break;
   case CMPI_uint8:
      sscanf(val, "%lu", &value.uint32);
      value.uint8 = value.uint32;
      break;
   case CMPI_sint8:
      sscanf(val, "%ld", &value.sint32);
      value.sint8 = value.sint32;
      break;
   case CMPI_boolean:
      value.boolean = strcasecmp(val, "false");
      break;
   case CMPI_real32:
      sscanf(val, "%f", &value.real32);
      break;
   case CMPI_real64:
      sscanf(val, "%lf", &value.real64);
      break;
   case CMPI_dateTime:
      value.dateTime = native_new_CMPIDateTime_fromChars(val, NULL);
      break;
   case CMPI_ref:
      valp=getKeyValueTypePtr("ref", NULL, ref, &value, &t);
      break;
  default:
      printf("%s(%d): invalid value %d-%s\n", __FILE__, __LINE__, (int) type, val);
      abort();
   }
   return value;
}

static int value2xml(CMPIData d, UtilStringBuffer * sb, int wv)
{
   char str[256];
   char *sp = str;
   int freesp = 0;

   if (d.type & CMPI_ARRAY) {
      sb->ft->appendChars(sb, "**[]**");
      return 1;
   }
   
   else {
      if (wv) sb->ft->appendChars(sb, "<VALUE>");
      if ((d.type & (CMPI_UINT|CMPI_SINT))==CMPI_UINT) {
         unsigned long long ul;
         switch (d.type) {
         case CMPI_uint8:
            ul = d.value.uint8;
            break;
         case CMPI_uint16:
            ul = d.value.uint16;
            break;
         case CMPI_uint32:
            ul = d.value.uint32;
            break;
         case CMPI_uint64:
            ul = d.value.uint64;
            break;
         }
         sprintf(str, "%llu", ul);
      }
      else if (d.type & CMPI_SINT) {
         long long sl;
          switch (d.type) {
         case CMPI_sint8:
            sl = d.value.sint8;
            break;
         case CMPI_sint16:
            sl = d.value.sint16;
            break;
         case CMPI_sint32:
            sl = d.value.sint32;
            break;
         case CMPI_sint64:
            sl = d.value.sint64;
            break;
         }
         sprintf(str, "%lld", sl);
      }

      else if (d.type == CMPI_boolean)
         sprintf(str, "%s", d.value.boolean ? "TRUE" : "FALSE");
      else if (d.type == CMPI_char16)
         sprintf(str, "%c", d.value.char16);
      else if (d.type == CMPI_chars) {
         sp = XMLEscape(d.value.chars);
         if (sp) freesp = 1;
      }
      else if (d.type == CMPI_string) {
         sp = XMLEscape((char *) d.value.string->hdl);
         if (sp) freesp = 1;
      }
      else if (d.type == CMPI_dateTime) {
         if (d.value.dateTime) {
            CMPIString *sdf = CMGetStringFormat(d.value.dateTime, NULL);
            sp = (char *) sdf->hdl;
         }
         else sp = "";
      }
      else {
         printf("%s(%d): invalid value2xml %d-%x\n", __FILE__, __LINE__,
                (int) d.type, (int) d.type);
         abort();
      }
      sb->ft->appendChars(sb, sp);
      if (wv) sb->ft->appendChars(sb, "</VALUE>\n");
   }
   if (freesp) free(sp);
   return 0;
}

static int keyBinding2xml(CMPIObjectPath * op, UtilStringBuffer * sb);

static int nsPath2xml(CMPIObjectPath * ci, UtilStringBuffer * sb)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "nsPath2xml");
   sb->ft->appendChars(sb, "<NAMESPACEPATH>\n");
   sb->ft->appendChars(sb, "<HOST>linux</HOST>\n");
   sb->ft->appendChars(sb, "<LOCALNAMESPACEPATH>\n");
   sb->ft->appendChars(sb, "<NAMESPACE NAME=\"root\"/>\n");  //????
   sb->ft->appendChars(sb, "<NAMESPACE NAME=\"cimv2\"/>\n");
   sb->ft->appendChars(sb, "</LOCALNAMESPACEPATH>\n");
   sb->ft->appendChars(sb, "</NAMESPACEPATH>\n");

   _SFCB_RETURN(0);
}

static int instanceName2xml(CMPIObjectPath * cop, UtilStringBuffer * sb)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "instanceName2xml");
   sb->ft->appendChars(sb, "<INSTANCENAME CLASSNAME=\"");
   sb->ft->appendChars(sb, opGetClassNameChars(cop));
   sb->ft->appendChars(sb, "\">\n");
   keyBinding2xml(cop, sb);
   sb->ft->appendChars(sb, "</INSTANCENAME>\n");

   _SFCB_RETURN(0);
}

static void data2xml(CMPIData * data, CMPIString * name, char *bTag, char *eTag,
                     UtilStringBuffer * sb, UtilStringBuffer * qsb, int inst, int param)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "data2xml");

   char *type;

   if (data->type & CMPI_ARRAY) {
      CMPIArray *ar = data->value.array;
      CMPIData d;
      int j, ac = CMGetArrayCount(ar, NULL);
      sb->ft->appendChars(sb, bTag);
      sb->ft->appendChars(sb, (char *) name->hdl);
      if (param) sb->ft->appendChars(sb, "\" PARAMTYPE=\"");
      else sb->ft->appendChars(sb, "\" TYPE=\"");
      sb->ft->appendChars(sb, dataType(data->type));
      sb->ft->appendChars(sb, "\">\n");
      if (qsb)
         sb->ft->appendChars(sb, (char *) qsb->hdl);
      if (data->state == 0) {
         sb->ft->appendChars(sb, "<VALUE.ARRAY>\n");
         for (j = 0; j < ac; j++) {
            d = CMGetArrayElementAt(ar, j, NULL);
            value2xml(d, sb, 1);
         }
         sb->ft->appendChars(sb, "</VALUE.ARRAY>\n");
      }
      sb->ft->appendChars(sb, eTag);
   }
   else {
      type = dataType(data->type);
      if (*type == '*') {
         sb->ft->appendChars(sb, bTag);
         sb->ft->appendChars(sb, (char *) name->hdl);
         sb->ft->appendChars(sb, "\" REFERENCECLASS=\"");
         sb->ft->appendChars(sb, opGetClassNameChars(data->value.ref));
         sb->ft->appendChars(sb, "\">\n");
         if (qsb)
            sb->ft->appendChars(sb, (char *) qsb->hdl);
         if (inst) {
            sb->ft->appendChars(sb, "<VALUE.REFERENCE>\n");
            sb->ft->appendChars(sb, "<INSTANCEPATH>\n");
            nsPath2xml(data->value.ref, sb);
            instanceName2xml(data->value.ref, sb);
            sb->ft->appendChars(sb, "</INSTANCEPATH>\n");
            sb->ft->appendChars(sb, "</VALUE.REFERENCE>\n");
         }
         sb->ft->appendChars(sb, eTag);
      }
      else {
         sb->ft->appendChars(sb, bTag);
         sb->ft->appendChars(sb, (char *) name->hdl);
         if (param) sb->ft->appendChars(sb, "\" PARAMTYPE=\"");
         else sb->ft->appendChars(sb, "\" TYPE=\"");
         sb->ft->appendChars(sb, type);
         sb->ft->appendChars(sb, "\">\n");
         if (qsb)
            sb->ft->appendChars(sb, (char *) qsb->hdl);
         if (data->state == 0)
            value2xml(*data, sb, 1);
         sb->ft->appendChars(sb, eTag);
      }
   }
}

static void quals2xml(unsigned long quals, UtilStringBuffer * sb)
{
   if (quals & ClClass_Q_Abstract)
      sb->ft->appendChars(sb, "<QUALIFIER NAME=\"Abstract\" TYPE=\"boolean\">\n"
                          "<VALUE>TRUE</VALUE>\n</QUALIFIER>\n");
   if (quals & ClClass_Q_Association)
      sb->ft->appendChars(sb,
                          "<QUALIFIER NAME=\"Association\" TYPE=\"boolean\">\n"
                          "<VALUE>TRUE</VALUE>\n</QUALIFIER>\n");
   if (quals & ClClass_Q_Indication)
      sb->ft->appendChars(sb,
                          "<QUALIFIER NAME=\"Indication\" TYPE=\"boolean\">\n"
                          "<VALUE>TRUE</VALUE>\n</QUALIFIER>\n");
   if (quals & ClClass_Q_Deprecated)
      sb->ft->appendChars(sb,
                          "<QUALIFIER NAME=\"Deprecated\" TYPE=\"boolean\">\n"
                          "<VALUE>TRUE</VALUE>\n</QUALIFIER>\n");
   if (quals & ClProperty_Q_Key << 8)
      sb->ft->appendChars(sb, "<QUALIFIER NAME=\"Key\" TYPE=\"boolean\">\n"
                          "<VALUE>TRUE</VALUE>\n</QUALIFIER>\n");
   if (quals & ClProperty_Q_Propagated << 8)
      sb->ft->appendChars(sb,
                          "<QUALIFIER NAME=\"Propagated\" TYPE=\"boolean\">\n"
                          "<VALUE>TRUE</VALUE>\n</QUALIFIER>\n");
   if (quals & ClProperty_Q_Deprecated << 8)
      sb->ft->appendChars(sb,
                          "<QUALIFIER NAME=\"Deprecated\" TYPE=\"boolean\">\n"
                          "<VALUE>TRUE</VALUE>\n</QUALIFIER>\n");
}

static int cls2xml(CMPIConstClass * cls, UtilStringBuffer * sb, unsigned int flags)
{
   ClClass *cl = (ClClass *) cls->hdl;
   int i, q, m, qm;
   char *type, *superCls;
   CMPIString *name, *qname;
   CMPIData data, qdata;
   unsigned long quals;
   UtilStringBuffer *qsb = UtilFactory->newStrinBuffer(1024);

   _SFCB_ENTER(TRACE_CIMXMLPROC, "cls2xml");
   
   sb->ft->appendChars(sb, "<CLASS NAME=\"");
   sb->ft->appendChars(sb, cls->ft->getCharClassName(cls));
   superCls = (char *) cls->ft->getCharSuperClassName(cls);
   if (superCls) {
      sb->ft->appendChars(sb, "\" SUPERCLASS=\"");
      sb->ft->appendChars(sb, superCls);
   }
   sb->ft->appendChars(sb, "\">\n");
   quals2xml(cl->quals, sb);
   if (flags & FL_includeQualifiers)
      for (i = 0, m = ClClassGetQualifierCount(cl); i < m; i++) {
         data = cls->ft->getQualifierAt(cls, i, &name, NULL);
            data2xml(&data,name,"<QUALIFIER NAME=\"","</QUALIFIER>\n",sb,NULL,0,0);
      }

   for (i = 0, m = ClClassGetPropertyCount(cl); i < m; i++) {
      qsb->ft->reset(qsb);
      data = cls->ft->getPropertyAt(cls, i, &name, &quals, NULL);
      quals2xml(quals << 8, qsb);
      if (flags & FL_includeQualifiers)
         for (q = 0, qm = ClClassGetPropQualifierCount(cl, i); q < qm; q++) {
            qdata = cls->ft->getPropQualifierAt(cls, i, q, &qname, NULL);
            data2xml(&qdata,qname,"<QUALIFIER NAME=\"","</QUALIFIER>\n",qsb,NULL,0,0);
            CMRelease(qname);
         }
      if (data.type & CMPI_ARRAY) data2xml(&data, name, "<PROPERTY.ARRAY NAME=\"",
          "</PROPERTY.ARRAY>\n", sb, qsb, 0,0);
      else {
         type = dataType(data.type);
         if (*type == '*') data2xml(&data, name, "<PROPERTY.REFERENCE NAME=\"",
                     "</PROPERTY.REFERENCE>\n", sb, qsb, 0,0);
         else  data2xml(&data, name, "<PROPERTY NAME=\"", "</PROPERTY>\n",
            sb, qsb, 0,0);
      }
      CMRelease(name);
   }
   sb->ft->appendChars(sb, "</CLASS>\n");

   _SFCB_RETURN(0);
}

static int instance2xml(CMPIInstance * ci, UtilStringBuffer * sb)
{
   ClInstance *inst = (ClInstance *) ci->hdl;
   int i, m = ClInstanceGetPropertyCount(inst);
   char *type;
   UtilStringBuffer *qsb = UtilFactory->newStrinBuffer(1024);

   _SFCB_ENTER(TRACE_CIMXMLPROC, "instance2xml");
   
   sb->ft->appendChars(sb, "<INSTANCE CLASSNAME=\"");
   sb->ft->appendChars(sb, instGetClassName(ci));
   sb->ft->appendChars(sb, "\">\n");

   for (i = 0; i < m; i++) {
      CMPIString *name;
      CMPIData data;
      qsb->ft->reset(qsb);
      data = CMGetPropertyAt(ci, i, &name, NULL);
      
      if (data.type & CMPI_ARRAY) {
         data2xml(&data, name, "<PROPERTY.ARRAY NAME=\"", "</PROPERTY.ARRAY>\n",
                  sb, qsb, 1,0);
      }   
      else {
         type = dataType(data.type);
         if (*type == '*')   data2xml(&data, name, "<PROPERTY.REFERENCE NAME=\"",
                     "</PROPERTY.REFERENCE>\n", sb, qsb, 1,0);
         else data2xml(&data, name, "<PROPERTY NAME=\"", "</PROPERTY>\n", sb, qsb, 1,0);
      }
      
      if (data.type & (CMPI_ENC|CMPI_ARRAY)) {// don't get confused using generic release 
         data.value.inst->ft->release(data.value.inst);
      }   
      CMRelease(name);
   }
   sb->ft->appendChars(sb, "</INSTANCE>\n");

   qsb->ft->release(qsb);
   
   _SFCB_RETURN(0);
}

static int args2xml(CMPIArgs * args, UtilStringBuffer * sb)
{
   int i, m;
   char *type;

   _SFCB_ENTER(TRACE_CIMXMLPROC, "args2xml");
   
   if (args==NULL) _SFCB_RETURN(0);
   
   m = CMGetArgCount(args,NULL);
   if (m==0) _SFCB_RETURN(0);

   
   for (i = 0; i < m; i++) {
      CMPIString *name;
      CMPIData data;
      data = CMGetArgAt(args, i, &name, NULL);
      
      if (data.type & CMPI_ARRAY) {
         fprintf(stderr,"-#- args2xml: arrays in CMPIArgs not yet supported\n");
         abort();
//         data2xml(&data, name, "<PROPERTY.ARRAY NAME=\"", "</PROPERTY.ARRAY>\n",
//                  sb, qsb, 1);
      }   
      else {
         type = dataType(data.type);
         if (*type == '*') {
           fprintf(stderr,"-#- args2xml: references in CMPIArgs not yet supported\n");
           abort();
//              data2xml(&data, name, "<PROPERTY.REFERENCE NAME=\"",
//                     "</PROPERTY.REFERENCE>\n", sb, qsb, 1);
         }            
         else data2xml(&data, name, "<PARAMVALUE NAME=\"", "</PARAMVALUE>\n", sb, NULL, 1,1);
      }
      
      if (data.type & (CMPI_ENC|CMPI_ARRAY)) {// don't get confused using generic release 
         data.value.inst->ft->release(data.value.inst);
      }   
      CMRelease(name);
   }
   
   _SFCB_RETURN(0);
}

static int keyBinding2xml(CMPIObjectPath * op, UtilStringBuffer * sb)
{
   int i, m;
   _SFCB_ENTER(TRACE_CIMXMLPROC, "keyBinding2xml");

   for (i = 0, m = CMGetKeyCount(op, NULL); i < m; i++) {
      const char *name;
      char *type;
      CMPIData data;
      data = opGetKeyCharsAt(op, i, &name, NULL);
      sb->ft->appendChars(sb, "<KEYBINDING NAME=\"");
      sb->ft->appendChars(sb, name);
      sb->ft->appendChars(sb, "\">\n");
      type = keyType(data.type);
      if (*type == '*') {
         sb->ft->appendChars(sb, "<VALUE.REFERENCE>\n");
         sb->ft->appendChars(sb, "<INSTANCEPATH>\n");
         nsPath2xml(data.value.ref, sb);
         instanceName2xml(data.value.ref, sb);
         sb->ft->appendChars(sb, "</INSTANCEPATH>\n");
         sb->ft->appendChars(sb, "</VALUE.REFERENCE>\n");
      }
      else {
         sb->ft->appendChars(sb, "<KEYVALUE VALUETYPE=\"");
         sb->ft->appendChars(sb, type);
         sb->ft->appendChars(sb, "\">");
         value2xml(data, sb, 0);
         sb->ft->appendChars(sb, "</KEYVALUE>\n");
      }
      sb->ft->appendChars(sb, "</KEYBINDING>\n");
   }

   _SFCB_RETURN(0);
}

static int className2xml(CMPIObjectPath * op, UtilStringBuffer * sb)
{
   sb->ft->appendChars(sb, "<CLASSNAME NAME=\"");
   sb->ft->appendChars(sb, opGetClassNameChars(op));
   sb->ft->appendChars(sb, "\"/>\n");
   return 0;
}

static int enum2xml(CMPIEnumeration * enm, UtilStringBuffer * sb, CMPIType type,
                    int xmlAs, unsigned int flags)
{
   CMPIObjectPath *cop;
   CMPIInstance *ci;
   CMPIConstClass *cl;

   _SFCB_ENTER(TRACE_CIMXMLPROC, "enum2xml");
   
   while (CMHasNext(enm, NULL)) {
      if (type == CMPI_ref) {
         cop = CMGetNext(enm, NULL).value.ref;
         if (xmlAs==XML_asClassName) className2xml(cop,sb);
         else instanceName2xml(cop, sb);
      }
      else if (type == CMPI_class) {
         cl = (CMPIConstClass*)CMGetNext(enm, NULL).value.inst;
         cls2xml(cl,sb,flags); 
      }
      else if (type == CMPI_instance) {
         ci = CMGetNext(enm, NULL).value.inst;
         cop = CMGetObjectPath(ci, NULL);
         if (xmlAs==XML_asObj) {
            sb->ft->appendChars(sb, "<VALUE.OBJECTWITHPATH>\n");
            sb->ft->appendChars(sb, "<INSTANCEPATH>\n");
            nsPath2xml(cop, sb);
         }
         else
            sb->ft->appendChars(sb, "<VALUE.NAMEDINSTANCE>\n");
         instanceName2xml(cop, sb);
         if (xmlAs==XML_asObj)
            sb->ft->appendChars(sb, "</INSTANCEPATH>\n");
         instance2xml(ci, sb);
         if (xmlAs==XML_asObj)
            sb->ft->appendChars(sb, "</VALUE.OBJECTWITHPATH>\n");
         else
            sb->ft->appendChars(sb, "</VALUE.NAMEDINSTANCE>\n");
         cop->ft->release(cop);   
      }
   }
   
   _SFCB_RETURN(0);
}

extern unsigned long getObjectPathSerializedSize(CMPIObjectPath * op);
extern void getSerializedObjectPath(CMPIObjectPath * op, void *area);

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
      hdr->errMsg = strdup("Provider not found");
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




RespSegments iMethodGetTrailer(UtilStringBuffer * sb)
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

RespSegments iMethodResponse(RequestHdr * hdr, UtilStringBuffer * sb)
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

RespSegments methodResponse(RequestHdr * hdr, UtilStringBuffer * sb)
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
   instance2xml(ci, sb);
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
//asm("int $3");
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
         rc=arraySetElementNotTrackedAt(ar,c, &object, binCtx->type);
      }
   }

   enm = native_new_CMPIEnumeration(ar, NULL);
   sb = UtilFactory->newStrinBuffer(1024);
   
   if (binCtx->oHdr->type==OPS_EnumerateClassNames)
      enum2xml(enm, sb, binCtx->type, XML_asClassName, binCtx->bHdr->flags);
   else if (binCtx->oHdr->type==OPS_EnumerateClasses)
      enum2xml(enm, sb, binCtx->type, XML_asClass, binCtx->bHdr->flags);
   else enum2xml(enm, sb, binCtx->type, binCtx->xmlAs,binCtx->bHdr->flags);
   
   ar->ft->release(ar);

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
   int irc,i,sreqSize=sizeof(GetClassReq)-sizeof(MsgSegment);
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   GetClassReq *sreq;

   _SFCB_ENTER(TRACE_CIMXMLPROC, "getClass");
   
   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokGetClass *req = (XtokGetClass *) hdr->cimRequest;

   if (req->properties) sreqSize+=req->properties*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->operation=OPS_GetClass;
   sreq->count=req->properties+2;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   sreq->objectPath = setObjectPathMsgSegment(path);
   sreq->principle = setCharsMsgSegment(ctx->principle);

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
      free(sreq);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         cls = relocateSerializedConstClass(resp->object[0].data);
         sb = UtilFactory->newStrinBuffer(1024);
         cls2xml(cls, sb,binCtx.bHdr->flags);
         _SFCB_RETURN(iMethodResponse(hdr, sb));
      }
      _SFCB_RETURN(iMethodErrResponse(hdr, getErrSegment(resp->rc, 
        (char*)resp->object[0].data)));
   }
   free(sreq);
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
   
   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principle = setCharsMsgSegment(ctx->principle);
   sreq.flags = req->flags;

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
   
   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principle = setCharsMsgSegment(ctx->principle);
   sreq.flags = req->flags;

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = req->flags;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.commHndl = ctx->commHndl;
   binCtx.type=CMPI_class;
   binCtx.xmlAs=binCtx.noResp=0;
   binCtx.chunkFncs=ctx->chunkFncs;
   if (noChunking)
      hdr->chunkedMode=binCtx.chunkedMode=0;
   else {
      sreq.flags|=FL_chunked;
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
      
      if (noChunking) {
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
   int irc, i, m,sreqSize=sizeof(GetInstanceReq)-sizeof(MsgSegment);
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   RespSegments rsegs;
   GetInstanceReq *sreq;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokGetInstance *req = (XtokGetInstance *) hdr->cimRequest;

   if (req->properties) sreqSize+=req->properties*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->operation=OPS_GetInstance;
   sreq->count=req->properties+2;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   for (i = 0, m = req->instanceName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->instanceName.bindings.keyBindings[i].type,
                                req->instanceName.bindings.keyBindings[i].value,
                                &req->instanceName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->instanceName.bindings.keyBindings[i].name, valp, type);
   }
   sreq->objectPath = setObjectPathMsgSegment(path);
   sreq->principle = setCharsMsgSegment(ctx->principle);

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
      free(sreq);
      resp->rc--;
      if (resp->rc == CMPI_RC_OK) {
         inst = relocateSerializedInstance(resp->object[0].data);
         sb = UtilFactory->newStrinBuffer(1024);
         instance2xml(inst, sb);
         rsegs=iMethodResponse(hdr, sb);
         _SFCB_RETURN(rsegs);
      }
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
   sreq.principle = setCharsMsgSegment(ctx->principle);

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

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   inst = NewCMPIInstance(path, NULL);
               
   for (p = req->instance.properties.first; p; p = p->next) {
      val = str2CMPIValue(p->valueType, p->value, &p->ref);
      CMSetProperty(inst, p->name, &val, p->valueType);
   }
                  
   sreq.instance = setInstanceMsgSegment(inst);
   sreq.principle = setCharsMsgSegment(ctx->principle);

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
   int irc, i, m, sreqSize=sizeof(ModifyInstanceReq)-sizeof(MsgSegment);
   BinRequestContext binCtx;
   BinResponseHdr *resp;
   ModifyInstanceReq *sreq;
   XtokInstance *xci;
   XtokInstanceName *xco;
   XtokProperty *p = NULL;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokModifyInstance *req = (XtokModifyInstance *) hdr->cimRequest;

   if (req->properties) sreqSize+=req->properties*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->operation=OPS_ModifyInstance;
   sreq->count=req->properties+3;

   for (i=0; i<req->properties; i++)
      sreq->properties[i]=setCharsMsgSegment(req->propertyList[i]);

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
      val = str2CMPIValue(p->valueType, p->value, &p->ref);
      CMSetProperty(inst, p->name, &val, p->valueType);
   }
   sreq->instance = setInstanceMsgSegment(inst);
   sreq->path = setObjectPathMsgSegment(path);
   sreq->principle = setCharsMsgSegment(ctx->principle);

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

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principle = setCharsMsgSegment(ctx->principle);

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
   int irc, l = 0, err = 0,i,sreqSize=sizeof(EnumInstancesReq)-sizeof(MsgSegment);
   BinResponseHdr **resp;
   BinRequestContext binCtx;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokEnumInstances *req = (XtokEnumInstances *) hdr->cimRequest;

   if (req->properties) sreqSize+=req->properties*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->operation=OPS_EnumerateInstances;
   sreq->count=req->properties+2;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   sreq->principle = setCharsMsgSegment(ctx->principle);
   sreq->objectPath = setObjectPathMsgSegment(path);

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
   if (noChunking)
      hdr->chunkedMode=binCtx.chunkedMode=0;
   else {
      sreq->flags|=FL_chunked;
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
      free(sreq);
      
      if (noChunking) {
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

   qs=parseQuery(MEM_TRACKED,(char*)req->op.query.data,
      (char*)req->op.queryLang.data,&irc);   
   
   fCls=qs->ft->getFromClassList(qs);
   if (fCls==NULL || *fCls==NULL) {
     fprintf(stderr,"--- from clause\n");
     abort();
   }
   req->op.className = setCharsMsgSegment(*fCls);
   
   path = NewCMPIObjectPath(req->op.nameSpace.data, *fCls, NULL);
   
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principle = setCharsMsgSegment(ctx->principle);
   sreq.query = setCharsMsgSegment((char*)req->op.query.data);
   sreq.queryLang = setCharsMsgSegment((char*)req->op.queryLang.data);

   binCtx.oHdr = (OperationHdr *) req;
   binCtx.bHdr = &sreq.hdr;
   binCtx.bHdr->flags = 0;
   binCtx.rHdr = hdr;
   binCtx.bHdrSize = sizeof(sreq);
   binCtx.commHndl = ctx->commHndl;
   binCtx.type=CMPI_instance;
   binCtx.xmlAs=XML_asObj; binCtx.noResp=0;
   binCtx.chunkFncs=ctx->chunkFncs;
   if (noChunking)
      hdr->chunkedMode=binCtx.chunkedMode=0;
   else {
      sreq.flags|=FL_chunked;
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
      
      if (noChunking) {
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

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   for (i = 0, m = req->objectName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->objectName.bindings.keyBindings[i].type,
                                req->objectName.bindings.keyBindings[i].value,
                                &req->objectName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->objectName.bindings.keyBindings[i].name, valp, type);
   }
   sreq.objectPath = setObjectPathMsgSegment(path);

   sreq.resultClass = req->op.resultClass;
   sreq.role = req->op.role;
   sreq.assocClass = req->op.assocClass;
   sreq.resultRole = req->op.resultRole;
   sreq.principle = setCharsMsgSegment(ctx->principle);

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
   int irc, i, m, l = 0, err = 0, sreqSize=sizeof(AssociatorsReq)-sizeof(MsgSegment);
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   CMPIType type;
   CMPIValue val, *valp;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokAssociators *req = (XtokAssociators *) hdr->cimRequest;

   if (req->properties) sreqSize+=req->properties*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->operation=OPS_Associators;
   sreq->count=req->properties+6;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   for (i = 0, m = req->objectName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->objectName.bindings.keyBindings[i].type,
                                req->objectName.bindings.keyBindings[i].value,
                                &req->objectName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->objectName.bindings.keyBindings[i].name, valp, type);
   }
   sreq->objectPath = setObjectPathMsgSegment(path);

   sreq->resultClass = req->op.resultClass;
   sreq->role = req->op.role;
   sreq->assocClass = req->op.assocClass;
   sreq->resultRole = req->op.resultRole;
   sreq->flags = req->flags;
   sreq->principle = setCharsMsgSegment(ctx->principle);

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
   if (noChunking)
      hdr->chunkedMode=binCtx.chunkedMode=0;
   else {
      sreq->flags|=FL_chunked;
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
      free(sreq);
      
      if (noChunking) {
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

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   for (i = 0, m = req->objectName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->objectName.bindings.keyBindings[i].type,
                                req->objectName.bindings.keyBindings[i].value,
                                &req->objectName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->objectName.bindings.keyBindings[i].name, valp, type);
   }
   sreq.objectPath = setObjectPathMsgSegment(path);

   sreq.resultClass = req->op.resultClass;
   sreq.role = req->op.role;
   sreq.principle = setCharsMsgSegment(ctx->principle);

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
   int irc, i, m, l = 0, err = 0, sreqSize=sizeof(AssociatorsReq)-sizeof(MsgSegment);
   BinResponseHdr **resp;
   BinRequestContext binCtx;
   CMPIType type;
   CMPIValue val, *valp;

   memset(&binCtx,0,sizeof(BinRequestContext));
   XtokReferences *req = (XtokReferences *) hdr->cimRequest;

   if (req->properties) sreqSize+=req->properties*sizeof(MsgSegment);
   sreq=calloc(1,sreqSize);
   sreq->operation=OPS_References;
   sreq->count=req->properties+4;

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   for (i = 0, m = req->objectName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->objectName.bindings.keyBindings[i].type,
                                req->objectName.bindings.keyBindings[i].value,
                                &req->objectName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->objectName.bindings.keyBindings[i].name, valp, type);
   }
   sreq->objectPath = setObjectPathMsgSegment(path);

   sreq->resultClass = req->op.resultClass;
   sreq->role = req->op.role;
   sreq->flags = req->flags;
   sreq->principle = setCharsMsgSegment(ctx->principle);

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
   if (noChunking)
      hdr->chunkedMode=binCtx.chunkedMode=0;
   else {
      sreq->flags|=FL_chunked;
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
      free(sreq);
      
      if (noChunking) {
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

   path = NewCMPIObjectPath(req->op.nameSpace.data, req->op.className.data, NULL);
   if (req->instName) for (i = 0, m = req->instanceName.bindings.next; i < m; i++) {
      valp = getKeyValueTypePtr(req->instanceName.bindings.keyBindings[i].type,
                                req->instanceName.bindings.keyBindings[i].value,
                                &req->instanceName.bindings.keyBindings[i].ref,
                                &val, &type);
      CMAddKey(path, req->instanceName.bindings.keyBindings[i].name, valp, type);
   }
   sreq.objectPath = setObjectPathMsgSegment(path);
   sreq.principle = setCharsMsgSegment(ctx->principle);

   for (p = req->paramValues.first; p; p = p->next) {
      CMPIValue val = str2CMPIValue(p->type, p->value.value, &p->valueRef);
      CMAddArg(in, p->name, &val, p->type);
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


static RespSegments notSupported(CimXmlRequestContext * ctx, RequestHdr * hdr)
{
   return iMethodErrResponse(hdr, strdup
           ("<ERROR CODE=\"7\" DESCRIPTION=\"Operation not supported xx\"/>\n"));
}


static Handler handlers[] = {
   {notSupported},              //dummy
   {getClass},                  //OPS_GetClass 1
   {getInstance},               //OPS_GetInstance 2
   {notSupported},              //OPS_DeleteClass 3
   {deleteInstance},            //OPS_DeleteInstance 4
   {notSupported},              //OPS_CreateClass 5
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
   {notSupported},              //OPS_GetQualifier 20
   {notSupported},              //OPS_SetQualifier 21
   {notSupported},              //OPS_DeleteQualifier 22
   {notSupported},              //OPS_EnumerateQualifiers 23
   {invokeMethod},              //OPS_InvokeMethod 24
};

RespSegments handleCimXmlRequest(CimXmlRequestContext * ctx)
{
   RespSegments rs;
   RequestHdr hdr = scanCimXmlRequest(ctx->cimXmlDoc);

   Handler hdlr = handlers[hdr.opType];
   rs = hdlr.handler(ctx, &hdr);
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

char *XMLEscape(char *in)
{
   int i, l;
   char *out;
   char cs[2] = "*";

   _SFCB_ENTER(TRACE_CIMXMLPROC, "XMLEscape");

   if (in == NULL)
      return (NULL);
   l = strlen(in);
   out = (char *) malloc(l * 5);
   *out = 0;

   for (i = 0; i < l; i++) {
      char ch = in[i];
      switch (ch) {
      case '>':
         strcat(out, "&gt;");
         break;
      case '<':
         strcat(out, "&lt;");
         break;
      case '&':
         strcat(out, "&amp;");
         break;
      case '"':
         strcat(out, "&quot;");
         break;
      case '\'':
         strcat(out, "&apos;");
         break;
      default:
         *cs = ch;
         strcat(out, cs);
      }
   }
   *cs = 0;
   strcat(out, cs);

   _SFCB_RETURN(out);
}
