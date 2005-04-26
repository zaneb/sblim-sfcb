
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
#include "cmpidt.h"
#include "constClass.h"
#include "objectImpl.h"

#include "native.h"
#include "trace.h"
#include "utilft.h"
#include "string.h"

#include "queryOperation.h"

extern const char *instGetClassName(CMPIInstance * ci);
extern CMPIData opGetKeyCharsAt(CMPIObjectPath * cop, unsigned int index,
                                const char **name, CMPIStatus * rc);

const char *opGetClassNameChars(CMPIObjectPath * cop);


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
            sscanf(value, "%lld", &val->uint64);
         }
         else {
            sscanf(value, "%llu", &val->sint64);
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
            mlogf(M_ERROR,M_SHOW,"%s(%d): unexpected reference type %d %x\n", __FILE__, __LINE__, 
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
   case CMPI_instance:
      return "%";
   }
   mlogf(M_ERROR,M_SHOW,"%s(%d): invalid data type %d %x\n", __FILE__, __LINE__, (int) type,
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
   mlogf(M_ERROR,M_SHOW,"%s(%d): invalid key data type %d %x\n", __FILE__, __LINE__,
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

CMPIValue str2CMPIValue(CMPIType type, char *val, XtokValueReference *ref)
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
      mlogf(M_ERROR,M_SHOW,"%s(%d): invalid value %d-%s\n", __FILE__, __LINE__, (int) type, val);
      abort();
   }
   return value;
}

int value2xml(CMPIData d, UtilStringBuffer * sb, int wv)
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
      
      else if (d.type==CMPI_real32) {
         sprintf(str, "%.7e", d.value.real32);
      }
      
      else if (d.type==CMPI_real64) {
         sprintf(str, "%.16e", d.value.real64);
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
         mlogf(M_ERROR,M_SHOW,"%s(%d): invalid value2xml %d-%x\n", __FILE__, __LINE__,
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

int instanceName2xml(CMPIObjectPath * cop, UtilStringBuffer * sb)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "instanceName2xml");
   sb->ft->appendChars(sb, "<INSTANCENAME CLASSNAME=\"");
   sb->ft->appendChars(sb, opGetClassNameChars(cop));
   sb->ft->appendChars(sb, "\">\n");
   keyBinding2xml(cop, sb);
   sb->ft->appendChars(sb, "</INSTANCENAME>\n");

   _SFCB_RETURN(0);
}

static void method2xml(CMPIType type, CMPIString *name, char *bTag, char *eTag,
   UtilStringBuffer * sb, UtilStringBuffer * qsb)
{
   _SFCB_ENTER(TRACE_CIMXMLPROC, "method2xml");
   sb->ft->appendChars(sb, bTag);
   sb->ft->appendChars(sb, (char *) name->hdl);
   sb->ft->appendChars(sb, "\" TYPE=\"");
   sb->ft->appendChars(sb, dataType(type));
   sb->ft->appendChars(sb, "\">\n");
   if (qsb) sb->ft->appendChars(sb, (char *) qsb->hdl);
   sb->ft->appendChars(sb, eTag);

   _SFCB_EXIT();
}

static void data2xml(CMPIData * data, void *obj, CMPIString * name, char *bTag, char *eTag,
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
      if (qsb) sb->ft->appendChars(sb, (char *) qsb->hdl);
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
         if (qsb) sb->ft->appendChars(sb, (char *) qsb->hdl);
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
      
      else if (*type == '%') {         
         const char *eo=ClGetStringData((CMPIInstance*)obj,data->value.dataPtr.length);
         char *sp;
         int freesp = 0;
         
         sb->ft->appendChars(sb, bTag);
         sb->ft->appendChars(sb, (char *) name->hdl);
         if (param) sb->ft->appendChars(sb, "\" PARAMTYPE=\"string\">\n");
         else sb->ft->appendChars(sb, "\" TYPE=\"string\">\n");
         sb->ft->appendChars(sb,"<QUALIFIER NAME=\"EmbeddedObject\" TYPE=\"boolean\">\n"
              "<VALUE>TRUE</VALUE>\n</QUALIFIER>\n");
         sb->ft->appendChars(sb, "<VALUE>");
         sp = XMLEscape((char*)eo);
         if (sp) freesp = 1; 
         sb->ft->appendChars(sb, "<![CDATA[");
         sb->ft->appendChars(sb, sp);
         sb->ft->appendChars(sb, "]]>");
         sb->ft->appendChars(sb, "</VALUE>\n");
         if (freesp) free(sp);
     }
      
      else {
         sb->ft->appendChars(sb, bTag);
         sb->ft->appendChars(sb, (char *) name->hdl);
         if (param) sb->ft->appendChars(sb, "\" PARAMTYPE=\"");
         else sb->ft->appendChars(sb, "\" TYPE=\"");
         sb->ft->appendChars(sb, type);
         sb->ft->appendChars(sb, "\">\n");
         if (qsb) sb->ft->appendChars(sb, (char *) qsb->hdl);
         if (data->state == 0) value2xml(*data, sb, 1);
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

static void param2xml(CMPIParameter *pdata, CMPIConstClass * cls, ClParameter *parm, CMPIString *pname, 
      UtilStringBuffer * sb, unsigned int flags)
{
   ClClass *cl = (ClClass *) cls->hdl;
   int i, m;
   CMPIData data;
   CMPIString qname;
   char *etag="</PARAMETER>\n";
   UtilStringBuffer *qsb = NULL;
   
   if (flags & FL_includeQualifiers) {   
      m = ClClassGetMethParamQualifierCount(cl,parm);
      if (m) qsb = UtilFactory->newStrinBuffer(1024);
      for (i = 0; i < m; i++) {
         ClClassGetMethParamQualifierAt(cl, parm, i, &data, (char**)&qname.hdl);
         data2xml(&data,cls,&qname,"<QUALIFIER NAME=\"","</QUALIFIER>\n",qsb,NULL,0,0);
      }
   }
   
   if (pdata->type==CMPI_ref) {
      sb->ft->appendChars(sb, "<PARAMETER.REFERENCE NAME=\"");
      sb->ft->appendChars(sb, (char*)pname->hdl);
      if (pdata->refName) {         
         sb->ft->appendChars(sb, "\" REFERENCECLASS=\"");
         sb->ft->appendChars(sb, pdata->refName);
      }
      sb->ft->appendChars(sb, "\"\n");
      etag="</PARAMETER.REFERENCE>\n";
   }
   else if (pdata->type==CMPI_refA) {
      sb->ft->appendChars(sb, "<PARAMETER.REFARRAY NAME=\"");
      sb->ft->appendChars(sb, (char*)pname->hdl);
      mlogf(M_ERROR,M_SHOW,"*** PARAMETER.REFARRAY not implemenetd\n");
      abort();  
      etag="</PARAMETER.REFARRAY>\n";
   }   
   else {
      if (pdata->type&CMPI_ARRAY) {
      char size[128];
         sb->ft->appendChars(sb, "<PARAMETER.ARRAY NAME=\"");
         sb->ft->appendChars(sb, (char*)pname->hdl);
         sprintf(size,"\" ARRAYSIZE=\"%d\"",pdata->arraySize);
         sb->ft->appendChars(sb, size);
         etag="</PARAMETER.ARRAY>\n";
      }
      else {
         sb->ft->appendChars(sb, "<PARAMETER NAME=\"");
         sb->ft->appendChars(sb, (char*)pname->hdl);
      }
      sb->ft->appendChars(sb, "\" TYPE=\"");
      sb->ft->appendChars(sb, dataType(pdata->type));
      sb->ft->appendChars(sb, "\"\n");
   } 
 
   if (qsb) sb->ft->appendChars(sb, (char *) qsb->hdl);
   sb->ft->appendChars(sb, etag);
}

int cls2xml(CMPIConstClass * cls, UtilStringBuffer * sb, unsigned int flags)
{
   ClClass *cl = (ClClass *) cls->hdl;
   int i, m, q, qm, p, pm;
   char *type, *superCls;
   CMPIString *name, *qname;
   CMPIData data, qdata;
   CMPIType mtype;
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
            data2xml(&data,cls,name,"<QUALIFIER NAME=\"","</QUALIFIER>\n",sb,NULL,0,0);
      }

   for (i = 0, m = ClClassGetPropertyCount(cl); i < m; i++) {
      qsb->ft->reset(qsb);
      data = cls->ft->getPropertyAt(cls, i, &name, &quals, NULL);
      quals2xml(quals << 8, qsb);
      if (flags & FL_includeQualifiers)
         for (q = 0, qm = ClClassGetPropQualifierCount(cl, i); q < qm; q++) {
            qdata = cls->ft->getPropQualifierAt(cls, i, q, &qname, NULL);
            data2xml(&qdata,cls,qname,"<QUALIFIER NAME=\"","</QUALIFIER>\n",qsb,NULL,0,0);
            CMRelease(qname);
         }
      if (data.type & CMPI_ARRAY) data2xml(&data,cls,name,"<PROPERTY.ARRAY NAME=\"",
          "</PROPERTY.ARRAY>\n", sb, qsb, 0,0);
      else {
         type = dataType(data.type);
         if (*type == '*') data2xml(&data,cls,name,"<PROPERTY.REFERENCE NAME=\"",
                     "</PROPERTY.REFERENCE>\n", sb, qsb, 0,0);
         else  data2xml(&data,cls,name,"<PROPERTY NAME=\"", "</PROPERTY>\n",
            sb, qsb, 0,0);
      }
      CMRelease(name);
   }
   
   for (i = 0, m = ClClassGetMethodCount(cl); i < m; i++) {
      ClMethod *meth;
      ClParameter *parm;
      CMPIString name,mname;
      qsb->ft->reset(qsb);
      ClClassGetMethodAt(cl, i, &mtype, (char**)&mname.hdl, &quals);
      meth=((ClMethod*)ClObjectGetClSection(&cl->hdr,&cl->methods))+i;
      
      if (flags & FL_includeQualifiers) {
         for (q = 0, qm = ClClassGetMethQualifierCount(cl, i); q < qm; q++) {
            ClClassGetMethQualifierAt(cl, meth, q, &qdata, (char**)&name.hdl);
            data2xml(&qdata,cls,&name,"<QUALIFIER NAME=\"","</QUALIFIER>\n",qsb,NULL,0,0);
         }
      }   
      
      for (p = 0, pm = ClClassGetMethParameterCount(cl, i); p < pm; p++) {
         CMPIParameter pdata;
         ClClassGetMethParameterAt(cl, meth, p, &pdata, (char**)&name.hdl);
         parm=((ClParameter*)ClObjectGetClSection(&cl->hdr,&meth->parameters))+p;
         param2xml(&pdata,cls,parm,&name,qsb,flags);
      }
 
      method2xml(mtype,&mname,"<METHOD NAME=\"", "</METHOD>\n",sb, qsb);
   }
   
   sb->ft->appendChars(sb, "</CLASS>\n");

   _SFCB_RETURN(0);
}

int instance2xml(CMPIInstance * ci, UtilStringBuffer * sb)
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
         data2xml(&data,ci,name,"<PROPERTY.ARRAY NAME=\"", "</PROPERTY.ARRAY>\n",
                  sb, qsb, 1,0);
      }   
      else {
         type = dataType(data.type);
         if (*type == '*')   data2xml(&data,ci,name,"<PROPERTY.REFERENCE NAME=\"",
                     "</PROPERTY.REFERENCE>\n", sb, qsb, 1,0);
         else data2xml(&data,ci,name,"<PROPERTY NAME=\"", "</PROPERTY>\n", sb, qsb, 1,0);
      }
      
      if (data.type & (CMPI_ENC|CMPI_ARRAY)) {// don't get confused using generic release 
         if (data.type != CMPI_instance) 
            data.value.inst->ft->release(data.value.inst);
      }   
      CMRelease(name);
   }
   sb->ft->appendChars(sb, "</INSTANCE>\n");

   qsb->ft->release(qsb);
   
   _SFCB_RETURN(0);
}

int args2xml(CMPIArgs * args, UtilStringBuffer * sb)
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
         mlogf(M_ERROR,M_SHOW,"-#- args2xml: arrays in CMPIArgs not yet supported\n");
         abort();
//         data2xml(&data,args,name,"<PROPERTY.ARRAY NAME=\"", "</PROPERTY.ARRAY>\n",
//                  sb, qsb, 1);
      }   
      else {
         type = dataType(data.type);
         if (*type == '*') {
           mlogf(M_ERROR,M_SHOW,"-#- args2xml: references in CMPIArgs not yet supported\n");
           abort();
//              data2xml(&data,args,name,"<PROPERTY.REFERENCE NAME=\"",
//                     "</PROPERTY.REFERENCE>\n", sb, qsb, 1);
         }            
         else data2xml(&data,args,name,"<PARAMVALUE NAME=\"", "</PARAMVALUE>\n", sb, NULL, 1,1);
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

int enum2xml(CMPIEnumeration * enm, UtilStringBuffer * sb, CMPIType type,
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

