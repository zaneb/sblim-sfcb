
/*
 * classProvider.c
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
 *
 * Class provider for sfcb .
 *
*/



#include "utilft.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

//#include "classRegister.h"
#include "constClass.h"
#include "providerRegister.h"
#include "trace.h"
#include "control.h"

#define NEW(x) ((x *) malloc(sizeof(x)))

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#define LOCALCLASSNAME "ClassProvider"

static CMPIBroker *_broker;

extern char * configfile;
extern ProviderRegister *pReg;

typedef struct _Class_Register_FT Class_Register_FT;
struct _ClassRegister {
   void *hdl;
   Class_Register_FT *ft;
};
typedef struct _ClassRegister ClassRegister;

typedef struct _ClassBase {
   char *fn;
   UtilHashTable *ht;
   UtilHashTable *it;
} ClassBase;

struct _Class_Register_FT {
   int version;
   void (*release) (ClassRegister * br);
   ClassRegister *(*clone) (ClassRegister * br);
   CMPIConstClass *(*getClass) (ClassRegister * br, const char *clsName);
   int (*putClass) (ClassRegister * br, CMPIConstClass * cls);
   UtilList *(*getChildren) (ClassRegister * br, const char *className);
};

extern Class_Register_FT *ClassRegisterFT;

//static ClassRegister *cReg = NULL;
int assocs = 0, topAssocs = 0;

typedef struct nameSpaces {
   int next,max,blen;
   char *base;
   char *names[1];
} NameSpaces;

static UtilHashTable *nsHt=NULL;
static int nsBaseLen;

void buildInheritanceTable(ClassRegister * cr)
{
   ClassBase *cb = (ClassBase *) (cr + 1);
   UtilHashTable *ct = cb->ht, *it;
   HashTableIterator *i;
   char *cn;
   CMPIConstClass *cc;
   UtilList *ul;

   it = cb->it = UtilFactory->newHashTable(61,
             UtilHashTable_charKey | UtilHashTable_ignoreKeyCase);

   for (i = ct->ft->getFirst(ct, (void **) &cn, (void **) &cc); i;
        i = ct->ft->getNext(ct, i, (void **) &cn, (void **) &cc)) {
      const char *p = cc->ft->getCharSuperClassName(cc);
      if (p == NULL) continue;
      ul = it->ft->get(it, p);
      if (ul == NULL) {
         ul = UtilFactory->newList();
         it->ft->put(it, p, ul);
      }
      ul->ft->prepend(ul, cc->ft->getCharClassName(cc));
   }
}

static void release(ClassRegister * cr)
{
   ClassBase *cb = (ClassBase *) cr->hdl;
   free(cb->fn);
   cb->ht->ft->release(cb->ht);
   free(cr);
}

static ClassRegister *clone(ClassRegister * cr)
{
   return NULL;
}

static UtilList *getChildren(ClassRegister * cr, const char *className)
{
   ClassBase *cb = (ClassBase *) (cr + 1);
   return cb->it->ft->get(cb->it, className);
}

static ClassRegister *newClassRegister(char *fname)
{
   ClassRegister *cr =
       (ClassRegister *) malloc(sizeof(ClassRegister) + sizeof(ClassBase));
   ClassBase *cb = (ClassBase *) (cr + 1);
   FILE *in;
   char fin[1024];
   long s, size,total=0;
   
   cr->hdl = cb;
   cr->ft = ClassRegisterFT;
   
   strcpy(fin, fname);
   strcat(fin, "/classSchemas");
   in = fopen(fin, "r");
   
   if (in == NULL) {
   //   fprintf(stderr, "--- %s not found\n", fin);
      cb->ht = UtilFactory->newHashTable(61,
               UtilHashTable_charKey | UtilHashTable_ignoreKeyCase);
      cb->it = UtilFactory->newHashTable(61,
               UtilHashTable_charKey | UtilHashTable_ignoreKeyCase);
      return cr;
   }

   cb->fn = strdup(fin);
   cb->ht = UtilFactory->newHashTable(61,
               UtilHashTable_charKey | UtilHashTable_ignoreKeyCase);

   while ((s = fread(&size, 1, 4, in)) == 4) {
      CMPIConstClass *cc=NULL;
      char *buf = (char *) malloc(size);
      total+=size;
      char *cn;
      *((long *) buf) = size;
      if (fread(buf + 4, 1, size - 4, in) == size - 4) {
         cc = NEW(CMPIConstClass);
         cc->hdl = buf;
         cc->ft = CMPIConstClassFT;
         cc->ft->relocate(cc);
         cn=(char*)cc->ft->getCharClassName(cc);
         if (strncmp(cn,"DMY_",4)) {
            cb->ht->ft->put(cb->ht, cn, cc);
            if (cc->ft->isAssociation(cc)) assocs++;
            if (cc->ft->getCharSuperClassName(cc) == NULL) topAssocs++;
         }   
      }
      else {
         printf("--- got a problem ---");
      }
   }
//   printf("--- %d Association classes\n", assocs);
//   printf("--- %d Top level Association classes\n", topAssocs);
   printf("--- ClassProvider for %s using %ld bytes\n", fname, total);

   buildInheritanceTable(cr);
   return cr;
}

static UtilHashTable *gatherNameSpaces(char *dn, UtilHashTable *ns)
{
   DIR *dir;
   struct dirent *de;
   char *n;
   int l;
   ClassRegister *cr;
   
   if (ns==NULL) {
      ns= UtilFactory->newHashTable(61,
             UtilHashTable_charKey | UtilHashTable_ignoreKeyCase);
      nsBaseLen=strlen(dn)+1;       
   }          
       
   dir=opendir(dn);
   while ((de=readdir(dir))!=NULL) {
      if (de->d_type==DT_DIR) {
         if (strcmp(de->d_name,".")==0) continue;
         if (strcmp(de->d_name,"..")==0) continue;
         l=strlen(dn)+strlen(de->d_name)+4;
         n=(char*)malloc(l);
         strcpy(n,dn);
         strcat(n,"/");
         strcat(n,de->d_name);
         ns->ft->put(ns, n+nsBaseLen, cr=newClassRegister(n));
         gatherNameSpaces(n,ns);
      } 
   }
   closedir(dir);  
   return ns;     
} 

static UtilHashTable *buildClassRegisters()
{
   char *dir;
   char *dn;

   setupControl(configfile);

   if (getControlChars("registrationDir",&dir)) {
     dir = "/var/lib/sfcb/registration";
   }
   
   dn=(char*)alloca(strlen(dir)+32);
   strcpy(dn,dir);
   if (dir[strlen(dir)-1]!='/') strcat(dn,"/");
   strcat(dn,"repository");
   return gatherNameSpaces(dn,NULL);   
}    


static ClassRegister *getNsReg(CMPIObjectPath *ref, int *rc)
{
   char *ns;
   CMPIString *nsi=CMGetNameSpace(ref,NULL);
   ClassRegister *cReg;
   *rc=0;
   
   if (nsHt==NULL) nsHt=buildClassRegisters();
   
   if (nsi && nsi->hdl) {
      ns=(char*)nsi->hdl;
      if (strcasecmp(ns,"root/pg_interop")==0)
         cReg=nsHt->ft->get(nsHt,"root/interop");
      else cReg=nsHt->ft->get(nsHt,ns);
      return cReg;
   }
   
   *rc=1;
   return NULL;
}

static int putClass(ClassRegister * cr, CMPIConstClass * cls)
{
   ClassBase *cb = (ClassBase *) cr->hdl;
   return cb->ht->ft->put(cb->ht, cls->ft->getCharClassName(cls), cls);
}


static CMPIConstClass *getClass(ClassRegister * cr, const char *clsName)
{
   _SFCB_ENTER(TRACE_PROVIDERS, "ClassProviderEnumInstanceNames");
   _SFCB_TRACE(1,("--- classname %s cReg %p",clsName,cr));
   ClassBase *cb = (ClassBase *) cr->hdl;
   CMPIConstClass *cls = cb->ht->ft->get(cb->ht, clsName);
   _SFCB_RETURN(cls);
}

static Class_Register_FT ift = {
   1,
   release,
   clone,
   getClass,
   putClass,
   getChildren
};

Class_Register_FT *ClassRegisterFT = &ift;



/* ------------------------------------------------------------------ *
 * Instance MI Cleanup
 * ------------------------------------------------------------------ */

CMPIStatus ClassProviderCleanup(CMPIInstanceMI * mi, CMPIContext * ctx)
{
/* 
   ClassBase *cb;
   UtilHashTable *ct;
   HashTableIterator *i;
   CMPIConstClass *cc;
   char *cn;

      
   if (cReg==NULL) CMReturn(CMPI_RC_OK);
   cb = (ClassBase *) (cReg + 1);
   ct = cb->ht;

   for (i = ct->ft->getFirst(ct, (void **) &cn, (void **) &cc); i;
        i = ct->ft->getNext(ct, i, (void **) &cn, (void **) &cc)) {
       free(cc->hdl);
       free(cc);
   }
*/
   CMReturn(CMPI_RC_OK);
}

/* ------------------------------------------------------------------ *
 * Instance MI Functions
 * ------------------------------------------------------------------ */

static void loopOnChildNames(ClassRegister *cReg, char *cn, CMPIResult * rslt)
{
   CMPIObjectPath *op;
   UtilList *ul = getChildren(cReg,cn);
   char *child;
   if (ul) for (child = (char *) ul->ft->getFirst(ul); child;  child = (char *) ul->ft->getNext(ul)) {
      op=CMNewObjectPath(_broker,NULL,child,NULL);
      CMReturnObjectPath(rslt,op);
      loopOnChildNames(cReg,child,rslt);
   }     
}
 

CMPIStatus ClassProviderEnumInstanceNames(CMPIInstanceMI * mi,
                                          CMPIContext * ctx,
                                          CMPIResult * rslt,
                                          CMPIObjectPath * ref)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char *cn=NULL;
   CMPIFlags flgs=0;
   CMPIString *cni;
   ClassBase *cb;
   HashTableIterator *it;
   char *key;
   int rc,n;
   CMPIConstClass *cls;
   CMPIObjectPath *op;
   ClassRegister *cReg;
   char *ns;

   _SFCB_ENTER(TRACE_PROVIDERS, "ClassProviderEnumInstanceNames");
   
   cReg=getNsReg(ref, &rc);
   if (cReg==NULL) {
      CMPIStatus st = { CMPI_RC_ERR_INVALID_NAMESPACE, NULL };
      _SFCB_RETURN(st);
   }
      
   ns=(char*)CMGetNameSpace(ref,NULL)->hdl;
   flgs=ctx->ft->getEntry(ctx,CMPIInvocationFlags,NULL).value.uint32;
   cni=ref->ft->getClassName(ref,NULL);
   if (cni) cn=(char*)cni->hdl;
   cb = (ClassBase *) cReg->hdl;
   
   if (cn && strcasecmp(cn,"$ClassProvider$")==0) cn=NULL;
   
   if (cn==NULL) {
      n=0;
      for (it = cb->ht->ft->getFirst(cb->ht, (void **) &key, (void **) &cls);
           key && it && cls;
           it = cb->ht->ft->getNext(cb->ht, it, (void **) &key, (void **) &cls)) {
         if ((flgs & CMPI_FLAG_DeepInheritance) || cls->ft->getCharSuperClassName(cls)==NULL) { 
            if (((flgs & FL_assocsOnly)==0) || cls->ft->isAssociation(cls)) {
               op=CMNewObjectPath(_broker,ns,key,NULL);
               CMReturnObjectPath(rslt,op);
            }   
         }   
      }     
   }
   
   else if (cn && ((flgs & CMPI_FLAG_DeepInheritance)==0)) {
      UtilList *ul = getChildren(cReg,cn);
      char *child;
      for (child = (char *) ul->ft->getFirst(ul); child;  child = (char *) ul->ft->getNext(ul)) {
         op=CMNewObjectPath(_broker,ns,child,NULL);
         CMReturnObjectPath(rslt,op);
      }     
   }
   
   else if (cn && (flgs & CMPI_FLAG_DeepInheritance)) {
      if (((flgs & FL_assocsOnly)==0) || cls->ft->isAssociation(cls))
         loopOnChildNames(cReg, cn, rslt);
   }
     
   _SFCB_RETURN(st);
}

static void loopOnChildren(ClassRegister *cReg, char *cn, CMPIResult * rslt)
{
   UtilList *ul = getChildren(cReg,cn);
   char *child;
   if (ul) for (child = (char *) ul->ft->getFirst(ul); child;  child = (char *) ul->ft->getNext(ul)) {
      CMPIConstClass *cl = getClass(cReg,child);
      CMReturnInstance(rslt, (CMPIInstance *) cl);
      loopOnChildren(cReg,child,rslt);
   }     
}
 
 

CMPIStatus ClassProviderEnumInstances(CMPIInstanceMI * mi,
                                      CMPIContext * ctx,
                                      CMPIResult * rslt,
                                      CMPIObjectPath * ref, char **properties)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char *cn=NULL;
   CMPIFlags flgs=0;
   CMPIString *cni;
   ClassBase *cb;
   HashTableIterator *it;
   char *key;
   int rc;
   CMPIConstClass *cls;
   ClassRegister *cReg;

   _SFCB_ENTER(TRACE_PROVIDERS, "ClassProviderEnumInstances");
   
   cReg=getNsReg(ref, &rc);
   if (cReg==NULL) {
      CMPIStatus st = { CMPI_RC_ERR_INVALID_NAMESPACE, NULL };
      _SFCB_RETURN(st);
   }
           
   flgs=ctx->ft->getEntry(ctx,CMPIInvocationFlags,NULL).value.uint32;
   cni=ref->ft->getClassName(ref,NULL);
   if (cni) {
      cn=(char*)cni->hdl;
      if (cn && *cn==0) cn=NULL;
   }   
   cb = (ClassBase *) cReg->hdl;
 
      
   if (cn==NULL) {
      for (it = cb->ht->ft->getFirst(cb->ht, (void **) &key, (void **) &cls);
           key && it && cls;
           it = cb->ht->ft->getNext(cb->ht, it, (void **) &key, (void **) &cls)) {
         if ((flgs & CMPI_FLAG_DeepInheritance) || cls->ft->getCharSuperClassName(cls)==NULL) {  
            CMReturnInstance(rslt, (CMPIInstance *) cls);
         }   
      }     
   }
   else if (cn && ((flgs & CMPI_FLAG_DeepInheritance)==0)) {
      UtilList *ul = getChildren(cReg,cn);
      char *child;
      for (child = (char *) ul->ft->getFirst(ul); child;  child = (char *) ul->ft->getNext(ul)) {
         cls = getClass(cReg,child);
         CMReturnInstance(rslt, (CMPIInstance *) cls);
      }     
   }
   else if (cn && (flgs & CMPI_FLAG_DeepInheritance)) {
      loopOnChildren(cReg, cn, rslt);
   }
     
   _SFCB_RETURN(st);
}


CMPIStatus ClassProviderGetInstance(CMPIInstanceMI * mi,
                                    CMPIContext * ctx,
                                    CMPIResult * rslt,
                                    CMPIObjectPath * ref, char **properties)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIString *cn = CMGetClassName(ref, NULL);
   CMPIConstClass *cl;
   ClassRegister *cReg;
   int rc;

   _SFCB_ENTER(TRACE_PROVIDERS, "ClassProviderGetInstance");
   _SFCB_TRACE(1,("--- ClassName %s",(char *) cn->hdl));
   
   cReg=getNsReg(ref, &rc);
   if (cReg==NULL) {
      CMPIStatus st = { CMPI_RC_ERR_INVALID_NAMESPACE, NULL };
      return st;
   }

   cl = getClass(cReg, (char *) cn->hdl);
   if (cl) CMReturnInstance(rslt, (CMPIInstance *) cl);
   else {
      st.rc = CMPI_RC_ERR_NOT_FOUND;
   }
   _SFCB_RETURN(st);
}

CMPIStatus ClassProviderCreateInstance(CMPIInstanceMI * mi,
                                       CMPIContext * ctx,
                                       CMPIResult * rslt,
                                       CMPIObjectPath * cop, CMPIInstance * ci)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   return st;
}

CMPIStatus ClassProviderSetInstance(CMPIInstanceMI * mi,
                                    CMPIContext * ctx,
                                    CMPIResult * rslt,
                                    CMPIObjectPath * cop,
                                    CMPIInstance * ci, char **properties)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   return st;
}

CMPIStatus ClassProviderDeleteInstance(CMPIInstanceMI * mi,
                                       CMPIContext * ctx,
                                       CMPIResult * rslt, CMPIObjectPath * cop)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   return st;
}

CMPIStatus ClassProviderExecQuery(CMPIInstanceMI * mi,
                                  CMPIContext * ctx,
                                  CMPIResult * rslt,
                                  CMPIObjectPath * cop, char *lang, char *query)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   return st;
}

/* ---------------------------------------------------------------------------*/
/*                        Method Provider Interface                           */
/* ---------------------------------------------------------------------------*/

extern CMPIBoolean isAbstract(CMPIConstClass * cc);

static int repCandidate(ClassRegister *cReg, char *cn)
{ 
   CMPIConstClass *cl = getClass(cReg,cn);
   if (isAbstract(cl)) return 0;
   ProviderInfo *info;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "repCandidate");
  
   if (strcasecmp(cn,"cim_indicationfilter")==0 ||  
       strcasecmp(cn,"cim_indicationsubscription")==0) _SFCB_RETURN(0);
         
   while (cn != NULL) {
      info = pReg->ft->getProvider(pReg, cn, INSTANCE_PROVIDER);
      if (info) _SFCB_RETURN(0);
      cn = (char*)cl->ft->getCharSuperClassName(cl);
      if (cn==NULL) break;
      cl = getClass(cReg,cn);
   }
   _SFCB_RETURN(1);
}

static void loopOnChildChars(ClassRegister *cReg, char *cn, CMPIArray *ar, int *i, int ignprov)
{
   UtilList *ul = getChildren(cReg,cn);
   char *child;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "loopOnChildChars");
   _SFCB_TRACE(1,("--- class %s",cn));
   
   if (ul) for (child = (char *) ul->ft->getFirst(ul); child;  
         child=(char*)ul->ft->getNext(ul)) {
      if (ignprov || repCandidate(cReg, child)) {
         CMSetArrayElementAt(ar, *i, child, CMPI_chars);
         *i=(*i)+1;
      }   
      loopOnChildChars(cReg, child,ar,i,ignprov);
   }     
   _SFCB_EXIT();
}

static void loopOnChildCount(ClassRegister *cReg, char *cn, int *i, int ignprov)
{
   UtilList *ul = getChildren(cReg,cn);
   char *child;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "loopOnChildCount");
   
   if (ul) for (child = (char *) ul->ft->getFirst(ul); child;  
         child=(char*)ul->ft->getNext(ul)) {
      if (ignprov || repCandidate(cReg, child)) *i=(*i)+1;
      loopOnChildCount(cReg, child,i,ignprov);
   }     
   _SFCB_EXIT();
}


CMPIStatus ClassProviderMethodCleanup(CMPIMethodMI * mi, CMPIContext * ctx)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   return st;
}

CMPIStatus ClassProviderInvokeMethod(CMPIMethodMI * mi,
                                     CMPIContext * ctx,
                                     CMPIResult * rslt,
                                     CMPIObjectPath * ref,
                                     char *methodName,
                                     CMPIArgs * in, CMPIArgs * out)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIArray *ar;
   int rc;
   ClassRegister *cReg;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "ClassProviderInvokeMethod");

   CMPIString *nsi=CMGetNameSpace(ref,NULL);
   
   cReg=getNsReg(ref, &rc);
   if (cReg==NULL) {
      CMPIStatus st = { CMPI_RC_ERR_INVALID_NAMESPACE, NULL };
      return st;
   }

   if (strcasecmp(methodName, "getchildren") == 0) {
      CMPIData cn = CMGetArg(in, "class", NULL);
      _SFCB_TRACE(1,("--- getchildren %s",(char*)cn.value.string->hdl));
      if (cn.type == CMPI_string && cn.value.string && cn.value.string->hdl) {
         char *child;
         int l=0, i=0;
         UtilList *ul = getChildren(cReg, (char *) cn.value.string->hdl);
         if (ul) l = ul->ft->size(ul);
         ar = CMNewArray(_broker, l, CMPI_string, NULL);
         if (ul) for (child = (char *) ul->ft->getFirst(ul); child;  child = (char *)
               ul->ft->getNext(ul)) {
            CMSetArrayElementAt(ar, i++, child, CMPI_chars);
         }
         st = CMAddArg(out, "children", &ar, CMPI_stringA);
      }
      else {
      }
   }
   
   else if (strcasecmp(methodName, "getallchildren") == 0) {
      int ignprov=0;
      CMPIStatus st;
      CMPIData cn = CMGetArg(in, "class", &st);
      if (st.rc!=CMPI_RC_OK) {
         cn = CMGetArg(in, "classignoreprov", NULL);
         ignprov=1;
      }
      _SFCB_TRACE(1,("--- getallchildren %s",(char*)cn.value.string->hdl));
      if (cn.type == CMPI_string && cn.value.string && cn.value.string->hdl) {
         int n=0,i=0;
         loopOnChildCount(cReg,(char *)cn.value.string->hdl,&n,ignprov);
         _SFCB_TRACE(1,("--- count %d",n));
         ar = CMNewArray(_broker, n, CMPI_string, NULL);
         if (n) {
            _SFCB_TRACE(1,("--- loop %s",(char*)cn.value.string->hdl));
            loopOnChildChars(cReg, (char *)cn.value.string->hdl,ar,&i,ignprov);         
         }   
         st = CMAddArg(out, "children", &ar, CMPI_stringA);
      }
      else {
      }
   }

   else if (strcasecmp(methodName, "getassocs") == 0) {
      ar = CMNewArray(_broker, topAssocs, CMPI_string, NULL);
      ClassBase *cb = (ClassBase *) (cReg + 1);
      UtilHashTable *ct = cb->ht;
      HashTableIterator *i;
      char *cn;
      CMPIConstClass *cc;
      int n;

      for (n = 0, i = ct->ft->getFirst(ct, (void **) &cn, (void **) &cc); i;
           i = ct->ft->getNext(ct, i, (void **) &cn, (void **) &cc)) {
         if (cc->ft->getCharSuperClassName(cc) == NULL) {
            CMSetArrayElementAt(ar, n++, cn, CMPI_chars);
         }
      }
      CMAddArg(out, "assocs", &ar, CMPI_stringA);
   }

   else if (strcasecmp(methodName, "ischild") == 0) {
      char *parent=(char*)CMGetClassName(ref,NULL)->hdl;
      UtilList *ul = getChildren(cReg, parent);
      char *chldn=(char*)CMGetArg(in, "child", NULL).value.string->hdl;
      char *child;
      
      st.rc = CMPI_RC_ERR_FAILED;
      if (ul) for (child=(char*)ul->ft->getFirst(ul); child; 
                   child=(char*)ul->ft->getNext(ul)) {
         if (strcasecmp(child,chldn)==0 ) {
            st.rc=CMPI_RC_OK;
            break;
         }   
      }
   }
   
   else if (strcasecmp(methodName, "_startup") == 0) {
      st.rc=CMPI_RC_OK;
  }
   
   else {
      fprintf(stderr,"--- ClassProvider: Invalid request %s\n", methodName);
      st.rc = CMPI_RC_ERR_METHOD_NOT_FOUND;
   }
   return st;
}


CMInstanceMIStub(ClassProvider, ClassProvider, _broker, CMNoHook);

CMMethodMIStub(ClassProvider, ClassProvider, _broker, CMNoHook);
//
//
