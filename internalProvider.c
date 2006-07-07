
/*
 * internalProvider.c
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
 * Based on concepts developed by Viktor Mihajlovski <mihajlov@de.ibm.com>
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
#include "providerRegister.h"
#include "fileRepository.h"
#include "utilft.h"
#include "trace.h"
#include "constClass.h"
#include "internalProvider.h"
#include "native.h"

#define LOCALCLASSNAME "InternalProvider"

static char * interopNs = "root/interop";
static char * pg_interopNs = "root/pg_interop";

extern ProviderInfo *interOpProvInfoPtr;
extern ProviderInfo *forceNoProvInfoPtr;

extern CMPIInstance *relocateSerializedInstance(void *area);
extern char *value2Chars(CMPIType type, CMPIValue * value);
extern CMPIString *__oft_toString(CMPIObjectPath * cop, CMPIStatus * rc);
extern CMPIObjectPath *getObjectPath(char *path, char **msg);
extern CMPIBroker *Broker;
extern UtilStringBuffer *newStringBuffer(int s);
extern void setStatus(CMPIStatus *st, CMPIrc rc, const char *msg);

static const CMPIBroker *_broker;

typedef struct keyIds {
   CMPIString *key;
   CMPIData data;
} KeyIds;


static char * repositoryNs (char * nss)
{
  if (strcasecmp(nss,pg_interopNs)==0) {
    return interopNs;
  } else {
    return nss;
  }   
}

static int qCompare(const void *arg1, const void *arg2)
{
   return strcasecmp((char *) ((KeyIds *) arg1)->key->hdl,
                     (char *) ((KeyIds *) arg2)->key->hdl);
}
/*
static int cpy2lower(char *in, char *out)
{
   int i = 0;
   while ((out[i] = tolower(in[i++])) != 0);
   return i - 1;
}
*/
static char copKey[8192];

static UtilStringBuffer *normalize_ObjectPath(const CMPIObjectPath * cop)
{
   int c = CMGetKeyCount(cop, NULL);
   int i;
   char pc = 0,*cp;
   UtilStringBuffer *sb=newStringBuffer(512);
      
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "normalize_ObjectPath");

   KeyIds *ids = (KeyIds *) malloc(sizeof(KeyIds) * c);
   
   for (i = 0; i < c; i++) {
      ids[i].data = CMGetKeyAt(cop, i, &ids[i].key, NULL);
      cp=ids[i].key->hdl;
      while (*cp) {
         *cp=tolower(*cp);
         cp++; 
      }
   }
   qsort(ids, c, sizeof(KeyIds), qCompare);

   for (i = 0; i < c; i++) {
      if (pc) sb->ft->appendChars(sb,",");
      sb->ft->appendChars(sb,(char*)ids[i].key->hdl);
      sb->ft->appendChars(sb,"=");
      if (ids[i].data.type==CMPI_ref) {
         CMPIString *cn=CMGetClassName(ids[i].data.value.ref,NULL);
	 CMPIString *ns=CMGetNameSpace(ids[i].data.value.ref,NULL);
         UtilStringBuffer *sbt= normalize_ObjectPath(ids[i].data.value.ref);
	 char *nss;
         cp=(char*)cn->hdl;
         while (*cp) {
            *cp=tolower(*cp);
            cp++; 
         }
	 if (ns==NULL) {
	   nss = CMGetCharPtr(CMGetNameSpace(cop,NULL));
	 } else {
	   nss = CMGetCharPtr(ns);
	 }
         sb->ft->appendChars(sb,nss);
         sb->ft->appendChars(sb,":");
         sb->ft->appendChars(sb,(char*)cn->hdl);
         sb->ft->appendChars(sb,".");
         sb->ft->appendChars(sb,sbt->ft->getCharPtr(sbt));
         sbt->ft->release(sbt);
      }
      else {
         char *v = value2Chars(ids[i].data.type, &ids[i].data.value);
         sb->ft->appendChars(sb,v);
         free(v);
      }   
      pc = ',';
   }
   free(ids);
   
   _SFCB_TRACE(1,("--- key: >%s<",sb->ft->getCharPtr(sb)));

   _SFCB_RETURN(sb);
}
/*
static char *normalizeObjectPath(CMPIObjectPath * cop)
{
   int c = CMGetKeyCount(cop, NULL);
   int i, p;
   char pc = 0;
   copKey[0]=0;
   
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "normalizeObjectPath");

normalize_ObjectPath(cop);
   KeyIds *ids = (KeyIds *) malloc(sizeof(KeyIds) * c);
   for (i = 0; i < c; i++) {
      ids[i].data = CMGetKeyAt(cop, i, &ids[i].key, NULL);
   }
   qsort(ids, c, sizeof(KeyIds), qCompare);

   p=0;
   for (i = 0; i < c; i++) {
      if (pc) copKey[p++] = pc;
      p += cpy2lower(ids[i].key->hdl,copKey+p);
      copKey[p++]='=';
      char *v = value2Chars(ids[i].data.type, &ids[i].data.value);
      p += cpy2lower(v,copKey + p);
      pc = ',';
      free(v);
   }
   free(ids);
   
   _SFCB_TRACE(1,("--- key: >%s<",copKey));

   _SFCB_RETURN(copKey);
}
*/
char *normalizeObjectPath(const CMPIObjectPath *cop)
{
   UtilStringBuffer *sb=normalize_ObjectPath(cop);  
   strcpy(copKey,sb->ft->getCharPtr(sb));
   sb->ft->release(sb);
   return copKey;
}

char *internalProviderNormalizeObjectPath(const CMPIObjectPath *cop)
{
   char *n;
   UtilStringBuffer *sb=normalize_ObjectPath(cop);  
   n=strdup(sb->ft->getCharPtr(sb));
   sb->ft->release(sb);
   return n;
}

static char **nsTab=NULL;
static int nsTabLen=0;

static int testNameSpace(char *ns, CMPIStatus *st)
{
    char **nsp=nsTab;
    
    if (interOpProvInfoPtr==forceNoProvInfoPtr) {
       if (strcasecmp(ns,interopNs)==0) {
          st->msg=native_new_CMPIString("Interop namespace disabled",NULL);
          st->rc=CMPI_RC_ERR_FAILED;
          return 0;
       }   
    }
    
    while (nsTabLen && *nsp) {
       if (strcasecmp(*nsp,ns)==0) return 1;
       nsp++;
    }
    if (existingNameSpace(ns)) {
      nsTab=nsp=realloc(nsTab,sizeof(nsp)*(nsTabLen+2));
      nsp[nsTabLen++]=strdup(ns);
      nsp[nsTabLen]=NULL;
      return 1;
    }
    
    st->rc=CMPI_RC_ERR_INVALID_NAMESPACE;
    return 0;
}

static BlobIndex *_getIndex(char *ns, char *cn)
 {
   BlobIndex *bi;
   if (getIndex(ns,cn,strlen(ns)+strlen(cn)+64,0,&bi))
      return bi;
   else return NULL;
 }

/* ------------------------------------------------------------------ *
 * Instance MI Cleanup
 * ------------------------------------------------------------------ */

CMPIStatus InternalProviderCleanup(CMPIInstanceMI * mi, 
				   const CMPIContext * ctx,
				   CMPIBoolean terminate)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderCleanup");
   
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------ *
 * Instance MI Functions
 * ------------------------------------------------------------------ */ 
 

CMPIStatus InternalProviderEnumInstanceNames(CMPIInstanceMI * mi,
                                             const CMPIContext * ctx,
                                             const CMPIResult * rslt,
                                             const CMPIObjectPath * ref)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIStatus sti = { CMPI_RC_OK, NULL };
   BlobIndex *bi;
   CMPIString *cn = CMGetClassName(ref, NULL);
   CMPIString *ns = CMGetNameSpace(ref, NULL);
   CMPIObjectPath *cop;
   char *nss=ns->ft->getCharPtr(ns,NULL);
   char *cns=cn->ft->getCharPtr(cn,NULL);
   char *bnss=repositoryNs(nss);
   size_t ekl;
   int i,ac=0;
   char copKey[8192]="";
   char *kp;
   char *msg;
   CMPIArgs *in,*out;
   CMPIObjectPath *op;
   CMPIArray *ar;
   CMPIData rv;

   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderEnumInstanceNames");
   _SFCB_TRACE(1,("%s %s",nss,cns));
   
   in=CMNewArgs(Broker,NULL);
   out=CMNewArgs(Broker,NULL);
   CMAddArg(in,"class",cns,CMPI_chars);
   op=CMNewObjectPath(Broker,bnss,"$ClassProvider$",&sti);
   rv=CBInvokeMethod(Broker,ctx,op,"getallchildren",in,out,&sti);     
   ar=CMGetArg(out,"children",NULL).value.array;
   if (ar) ac=CMGetArrayCount(ar,NULL);
   
   for (i=0; cns; i++) {
      if ((bi=_getIndex(bnss,cns))!=NULL) {
	if (getFirst(bi,NULL,&kp,&ekl)) {
	  while(1) {
            strcpy(copKey,nss);
            strcat(copKey,":");
            strcat(copKey,cns);
            strcat(copKey,".");
            strncat(copKey,kp,ekl);
	    
            cop=getObjectPath(copKey,&msg);
            if (cop) CMReturnObjectPath(rslt, cop);
            else {
	      CMPIStatus st = { CMPI_RC_ERR_FAILED, NULL };
	      return st;
            }
	    if (bi->next < bi->dSize && getNext(bi,NULL,&kp,&ekl)) {
	      continue;
	    }
	    break;
	  }
	}
	freeBlobIndex(&bi,1);
      }
      if (i<ac) cns=(char*)CMGetArrayElementAt(ar,i,NULL).value.string->hdl;
      else cns=NULL;  
   }
   _SFCB_RETURN(st);
}

UtilStringBuffer *instanceToString(CMPIInstance * ci, char **props);

static CMPIStatus enumInstances(CMPIInstanceMI * mi, 
				const CMPIContext * ctx, void *rslt,
				const CMPIObjectPath * ref, 
				const char **properties,
				void(*retFnc)(void*,const CMPIInstance*), 
				int ignprov)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIStatus sti = { CMPI_RC_OK, NULL };
   BlobIndex *bi;
   CMPIString *cn = CMGetClassName(ref, NULL);
   CMPIString *ns = CMGetNameSpace(ref, NULL);
   char *nss=ns->ft->getCharPtr(ns,NULL);
   char *cns=cn->ft->getCharPtr(cn,NULL);
   char *bnss=repositoryNs(nss);
   int len,i,ac=0;
   void *blob;
   CMPIInstance *ci;
   CMPIArgs *in,*out;
   CMPIObjectPath *op;
   CMPIArray *ar;
   CMPIData rv;

   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "enumInstances");
   _SFCB_TRACE(1,("--- %s %s",nss,cns));
  
   in=CMNewArgs(Broker,NULL);
   out=CMNewArgs(Broker,NULL);
   if (ignprov) CMAddArg(in,"classignoreprov",cns,CMPI_chars);
   else CMAddArg(in,"class",cns,CMPI_chars);
   
   op=CMNewObjectPath(Broker,bnss,"$ClassProvider$",&sti);
   _SFCB_TRACE(1,("--- getallchildren"));
   rv=CBInvokeMethod(Broker,ctx,op,"getallchildren",in,out,&sti);     
   _SFCB_TRACE(1,("--- getallchildren rc: %d",sti.rc));
   
   ar=CMGetArg(out,"children",NULL).value.array;
   if (ar) ac=CMGetArrayCount(ar,NULL);
   _SFCB_TRACE(1,("--- getallchildren ar: %p count: %d",ar,ac));
 
   for (i=0; cns; i++) {
       _SFCB_TRACE(1,("--- looking for %s",cns));
      if ((bi=_getIndex(bnss,cns))!=NULL) {
	for (blob=getFirst(bi,&len,NULL,0); blob; blob=getNext(bi,&len,NULL,0)) {
            ci=relocateSerializedInstance(blob);
            _SFCB_TRACE(1,("--- returning instance %p",ci));
            retFnc(rslt,ci);
   //         CMReturnInstance(rslt, ci);
         }
      } 
      if (i<ac) cns=(char*)CMGetArrayElementAt(ar,i,NULL).value.string->hdl;
      else cns=NULL;  
   }
   
   _SFCB_RETURN(st);
}

static void return2result(void *ret, const CMPIInstance *ci)
{
   CMPIResult * rslt=(CMPIResult*)ret; 
   CMReturnInstance(rslt, ci); 
}

static void return2lst(void *ret, const CMPIInstance *ci)
{
   UtilList *ul=(UtilList*)ret; 
   ul->ft->append(ul,ci); 
}

CMPIStatus InternalProviderEnumInstances(CMPIInstanceMI * mi, 
					 const CMPIContext * ctx, 
					 const CMPIResult * rslt,
                                         const CMPIObjectPath * ref, 
					 const char **properties)
{
   CMPIStatus st;
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderEnumInstances");
   st=enumInstances(mi,ctx,(void*)rslt,ref,properties,return2result,0);
   _SFCB_RETURN(st);
}

UtilList *SafeInternalProviderAddEnumInstances(UtilList *ul, CMPIInstanceMI * mi, 
					       const CMPIContext * ctx, const CMPIObjectPath * ref,
					       const char **properties, CMPIStatus *rc, int ignprov)
{
   CMPIStatus st;
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "SafeInternalProviderAddEnumInstances");
   st=enumInstances(mi,ctx,(void*)ul,ref,properties,return2lst,ignprov);
   if (rc) *rc=st;
   _SFCB_RETURN(ul);
}

UtilList *SafeInternalProviderEnumInstances(CMPIInstanceMI * mi, const CMPIContext * ctx, const CMPIObjectPath * ref,
                                         const char **properties, CMPIStatus *rc, int ignprov)
{
   UtilList *ul= UtilFactory->newList();
   return SafeInternalProviderAddEnumInstances(ul, mi, ctx, ref,properties,rc,ignprov);
}

CMPIInstance *internalProviderGetInstance(const CMPIObjectPath * cop, CMPIStatus *rc)
{
   int len;
   CMPIString *cn = CMGetClassName(cop, NULL);
   CMPIString *ns = CMGetNameSpace(cop, NULL);
   char *key = normalizeObjectPath(cop);
   CMPIInstance *ci=NULL;
   void *blob;
   char *nss=ns->ft->getCharPtr(ns,NULL);
   char *cns=cn->ft->getCharPtr(cn,NULL);
   char *bnss=repositoryNs(nss);
   CMPIStatus st = { CMPI_RC_OK, NULL };

   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "internalProviderGetInstance");
   _SFCB_TRACE(1,("--- Get instance for %s %s %s",nss,cns,key));
   
   if (testNameSpace(bnss,rc)==0) {
      _SFCB_TRACE(1,("--- Invalid namespace %s",nss));
      _SFCB_RETURN(NULL);
   }

   blob=getBlob(bnss,cns,key,&len);
   
   if (blob==NULL) {
      _SFCB_TRACE(1,("--- Instance not found"));
      st.rc=CMPI_RC_ERR_NOT_FOUND;
   }   
   else ci=relocateSerializedInstance(blob);

   *rc=st;
   _SFCB_RETURN(ci);
}

CMPIStatus InternalProviderGetInstance(CMPIInstanceMI * mi,
                                       const CMPIContext * ctx,
                                       const CMPIResult * rslt,
                                       const CMPIObjectPath * cop,
                                       const char **properties)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIInstance *ci;

   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderGetInstance");
   
   ci=internalProviderGetInstance(cop,&st);
   
   if (st.rc==CMPI_RC_OK) {
      CMReturnInstance(rslt, ci);
   }
   
   _SFCB_RETURN(st);    
}

CMPIStatus InternalProviderCreateInstance(CMPIInstanceMI * mi,
                                          const CMPIContext * ctx,
                                          const CMPIResult * rslt,
                                          const CMPIObjectPath * cop,
                                          const CMPIInstance * ci)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   unsigned long len;
   void *blob;
   CMPIString *cn = CMGetClassName(cop, NULL);
   CMPIString *ns = CMGetNameSpace(cop, NULL);
   char *key = normalizeObjectPath(cop);
   char *nss=ns->ft->getCharPtr(ns,NULL);
   char *cns=cn->ft->getCharPtr(cn,NULL);
   char *bnss=repositoryNs(nss);

   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderCreateInstance");
   
   if (testNameSpace(bnss,&st)==0) {
      return st;
   }

   if (existingBlob(bnss,cns,key)) {
      CMPIStatus st = { CMPI_RC_ERR_ALREADY_EXISTS, NULL };
      return st;
   }

   len=getInstanceSerializedSize(ci);
   blob=malloc(len+64);
   getSerializedInstance(ci,blob);
   
   if (addBlob(bnss,cns,key,blob,(int)len)) {
      CMPIStatus st = { CMPI_RC_ERR_FAILED, NULL };
      st.msg=native_new_CMPIString("Unable to write to repository",NULL);
      return st;
   }

   if (rslt) CMReturnObjectPath(rslt, cop);

   _SFCB_RETURN(st);
}

CMPIStatus InternalProviderModifyInstance(CMPIInstanceMI * mi,
					  const CMPIContext * ctx,
					  const CMPIResult * rslt,
					  const CMPIObjectPath * cop,
					  const CMPIInstance * ci, 
					  const char **properties)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   unsigned long len;
   void *blob;
   CMPIString *cn = CMGetClassName(cop, NULL);
   CMPIString *ns = CMGetNameSpace(cop, NULL);
   char *key = normalizeObjectPath(cop);
   char *nss=ns->ft->getCharPtr(ns,NULL);
   char *cns=cn->ft->getCharPtr(cn,NULL);
   char *bnss=repositoryNs(nss);

   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderSetInstance");
   
   if (testNameSpace(bnss,&st)==0) {
      return st;
   }

   if (existingBlob(bnss,cns,key)==0) {
      CMPIStatus st = { CMPI_RC_ERR_NOT_FOUND, NULL };
      return st;
   }

   len=getInstanceSerializedSize(ci);
   blob=malloc(len+64);
   getSerializedInstance(ci,blob);
   addBlob(bnss,cns,key,blob,(int)len);

   _SFCB_RETURN(st);
}

CMPIStatus InternalProviderDeleteInstance(CMPIInstanceMI * mi,
                                          const CMPIContext * ctx,
                                          const CMPIResult * rslt,
                                          const CMPIObjectPath * cop)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIString *cn = CMGetClassName(cop, NULL);
   CMPIString *ns = CMGetNameSpace(cop, NULL);
   char *key = normalizeObjectPath(cop);
   char *nss=ns->ft->getCharPtr(ns,NULL);
   char *cns=cn->ft->getCharPtr(cn,NULL);
   char *bnss=repositoryNs(nss);

   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderDeleteInstance");
   
   if (testNameSpace(bnss,&st)==0) {
      return st;
   }

   if (existingBlob(bnss,cns,key)==0) {
      CMPIStatus st = { CMPI_RC_ERR_NOT_FOUND, NULL };
      return st;
   }

   deleteBlob(bnss,cns,key);

   _SFCB_RETURN(st);
}

CMPIStatus InternalProviderExecQuery(CMPIInstanceMI * mi,
                                     const CMPIContext * ctx,
                                     const CMPIResult * rslt,
                                     const CMPIObjectPath * cop,
                                     const char *lang, const char *query)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   return st;
}


/* ------------------------------------------------------------------ *
 * Association MI Functions
 * ------------------------------------------------------------------ */

static int ASSOC      = 0;
static int ASSOC_NAME = 1;
static int REF        = 2;
static int REF_NAME   = 3;

CMPIConstClass *getConstClass(const char *ns, const char *cn);

static CMPIConstClass *assocForName(const char *nameSpace, const char *assocClass, 
                         const char *role, const char *resultRole)
{
   CMPIConstClass *cc =getConstClass(nameSpace, assocClass);
   
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "assocForName");
   _SFCB_TRACE(1,("--- nameSpace: %s assocClass: %s cc: %p",nameSpace,assocClass,cc));
   
   if (cc!=NULL && cc->ft->isAssociation(cc) != 0 &&
       (role==NULL || (cc->ft->getProperty(cc,role,NULL).state & CMPI_notFound)!=0) &&
       (resultRole==NULL || (cc->ft->getProperty(cc,resultRole,NULL).state & CMPI_notFound)!=0)) {
      _SFCB_RETURN(cc); 
   }   
   else _SFCB_RETURN(NULL);
}
 
static int objectPathEquals(UtilStringBuffer *pn, CMPIObjectPath *op, UtilStringBuffer **retName, int eq)
{
   int rc=0;
   UtilStringBuffer *opn=normalize_ObjectPath(op);
   if (strcmp(pn->ft->getCharPtr(pn),opn->ft->getCharPtr(opn))==0) rc=1;
   if (retName && rc==eq) *retName=opn;
   else opn->ft->release(opn);
   return rc;
}

CMPIStatus getRefs(const CMPIContext * ctx,  const CMPIResult * rslt,
                                       const CMPIObjectPath * cop,
                                       const char *assocClass,
                                       const char *resultClass,
                                       const char *role,
                                       const char *resultRole,
                                       const char **propertyList,
                                       int associatorFunction)
{
   UtilList *refs= UtilFactory->newList();
   char *ns=(char*)CMGetNameSpace(cop,NULL)->hdl;
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIUint32 newFlgs=FL_assocsOnly || CMPI_FLAG_DeepInheritance;
   
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "getRefs");
   
   if (assocClass != NULL) {
      CMPIObjectPath *path;
      if (assocForName(ns,assocClass,role,resultRole) == NULL) {
         setStatus(&st,CMPI_RC_ERR_INVALID_PARAMETER,assocClass);
         return st;
      }
      path=CMNewObjectPath(_broker,ns,assocClass,NULL);
      SafeInternalProviderAddEnumInstances(refs, NULL, ctx, path, propertyList, &st, 1);  
   }
    
   else {
      CMPIObjectPath *op=CMNewObjectPath(Broker,ns,"$ClassProvider$",&st);
      CMAddContextEntry((CMPIContext*)ctx, CMPIInvocationFlags,&newFlgs,CMPI_uint32);  
      CMPIEnumeration *enm=CBEnumInstanceNames(Broker,ctx,op,&st); 
      
      if (enm) while (CMHasNext(enm,NULL)) {      
         CMPIObjectPath *cop=CMGetNext(enm,NULL).value.ref;
         if (assocForName((char*)CMGetNameSpace(cop,NULL)->hdl,(char*)CMGetClassName(cop,NULL)->hdl,
               role,resultRole) != NULL)
            SafeInternalProviderAddEnumInstances(refs, NULL, ctx, cop, propertyList, &st, 1);  
      }
      else {
         st.rc=CMPI_RC_OK;
         _SFCB_RETURN(st);
      }
   }

   
   if (role) {
            // filter out the associations not matching the role property
      CMPIInstance *ci;      
      UtilStringBuffer *pn=normalize_ObjectPath(cop);
      for (ci=refs->ft->getFirst(refs); ci; ci=refs->ft->getNext(refs)) {
         CMPIData data=CMGetProperty(ci,role,NULL);
         if ((data.state & CMPI_notFound)==0 && data.type==CMPI_ref) {
            if (objectPathEquals(pn,data.value.ref,NULL,1)) continue;
            refs->ft->removeCurrent(refs);
         }
      }
      pn->ft->release(pn);
   } 
   
   else {
            // filter out associations not referencing pathName
      CMPIInstance *ci;
      int matched,i,m;      
      UtilStringBuffer *pn=normalize_ObjectPath(cop);
      for (ci=refs->ft->getFirst(refs); ci; ci=refs->ft->getNext(refs)) {
         for (matched=0,i=0,m=CMGetPropertyCount(ci,NULL); i<m; i++) {
            CMPIData data=CMGetPropertyAt(ci,i,NULL,NULL);
            if (data.type==CMPI_ref && objectPathEquals(pn,data.value.ref,NULL,1)) {
               matched=1;
               break;
            }
         }
         if (matched==0) refs->ft->removeCurrent(refs);
      }
   }
   
   if (associatorFunction==REF) {
      CMPIInstance *ci;
      for (ci=refs->ft->getFirst(refs); ci; ci=refs->ft->getNext(refs)) {
         CMReturnInstance(rslt,ci);
      }
      refs->ft->release(refs);
      _SFCB_RETURN(st);
   } 
   
   else if (associatorFunction==REF_NAME) {
      CMPIInstance *ci;
      for (ci=refs->ft->getFirst(refs); ci; ci=refs->ft->getNext(refs)) {
         CMPIObjectPath *ref=CMGetObjectPath(ci,NULL);
         CMReturnObjectPath(rslt,ref);
      }
      refs->ft->release(refs);
      _SFCB_RETURN(st);
   } 
   
   else {
            // Use hashtable to avoid dup'd associators
      CMPIInstance *ci;
      UtilHashTable *assocs = UtilFactory->newHashTable(61,UtilHashTable_charKey);
      UtilStringBuffer *pn=normalize_ObjectPath(cop);
      for (ci=refs->ft->getFirst(refs); ci; ci=refs->ft->getNext(refs)) {
                // Q: for ASSOC_NAME we should not require the
                // object exist if we go by the book, should we?
                // The current approach retrieves the instances
                // via the CIMOM handle
         if (resultRole) {
            CMPIData data=CMGetProperty(ci,resultRole,NULL);
            UtilStringBuffer *an=NULL;
            if (objectPathEquals(pn,data.value.ref,&an,0)==0) {
               if (CMClassPathIsA(Broker,cop,resultClass,NULL)) {
                  CMPIInstance *aci=CBGetInstance(Broker,ctx,data.value.ref,propertyList,&st);
                  assocs->ft->put(assocs,an->ft->getCharPtr(an),aci);
               }                  
            }
         } 
         
         else {
                   // must loop over the properties to find ref instances
            int i,m;
            for (i=0,m=CMGetPropertyCount(ci,NULL); i<m; i++) {
               CMPIData data=CMGetPropertyAt(ci,i,NULL,NULL);
               if (data.type==CMPI_ref) {
                  CMPIObjectPath *ref=data.value.ref;
                  CMPIString *tns=CMGetNameSpace(ref,NULL);
                  if (tns==NULL || tns->hdl==NULL) CMSetNameSpace(ref,ns);
                  UtilStringBuffer *an=NULL;
          //        CMPIString *pn=CMObjectPathToString(ref,NULL);
                  pn=normalize_ObjectPath(ref);
                  printf("ref::::: %s %s\n",(char*)tns->hdl,(char*)pn->hdl);
                  if (objectPathEquals(pn,ref,&an,0)==0) {
         
                     if (resultClass==NULL || CMClassPathIsA(Broker,ref,resultClass,NULL)) {
                        CMPIInstance *aci=CBGetInstance(Broker,ctx,ref,propertyList,&st);
                        if (aci) assocs->ft->put(assocs,an->ft->getCharPtr(an),aci);
                     }
                  }
               }
            }
         }
      }
      
      {
         HashTableIterator *it;
         char *an;
         CMPIInstance *aci;
         for (it=assocs->ft->getFirst(assocs,(void**)&an,(void**)&aci); it;
              it=assocs->ft->getNext(assocs,it,(void**)&an,(void**)&aci)) {
            if (associatorFunction == ASSOC) 
               CMReturnInstance(rslt,aci);
            else {
               CMPIObjectPath *op=CMGetObjectPath(aci,NULL);
               CMReturnObjectPath(rslt,op);
            }
         }
      }  
      
      _SFCB_RETURN(st);
        
   }
}

CMPIStatus InternalProviderAssociationCleanup(CMPIAssociationMI * mi, const CMPIContext * ctx, CMPIBoolean terminate)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderAssociationCleanup");
   
   _SFCB_RETURN(st);
}

                            
                            
CMPIStatus InternalProviderAssociators(CMPIAssociationMI * mi,
                                       const CMPIContext * ctx,
                                       const CMPIResult * rslt,
                                       const CMPIObjectPath * cop,
                                       const char *assocClass,
                                       const char *resultClass,
                                       const char *role,
                                       const char *resultRole,
                                       const char **propertyList)
{
   CMPIStatus st;
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderAssociators");
   st=getRefs(ctx, rslt, cop, assocClass, resultClass,
                        role, resultRole, propertyList, ASSOC);
   _SFCB_RETURN(st);
}

CMPIStatus InternalProviderAssociatorNames(CMPIAssociationMI * mi,
                                           const CMPIContext * ctx,
                                           const CMPIResult * rslt,
                                           const CMPIObjectPath * cop,
                                           const char *assocClass,
                                           const char *resultClass,
                                           const char *role,
                                           const char *resultRole)
{
   CMPIStatus st;
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderAssociatorNames");
   st=getRefs(ctx, rslt, cop, assocClass, resultClass,
                        role, resultRole, NULL, ASSOC_NAME);
   _SFCB_RETURN(st);
}

CMPIStatus InternalProviderReferences(CMPIAssociationMI * mi,
                                      const CMPIContext * ctx,
                                      const CMPIResult * rslt,
                                      const CMPIObjectPath * cop,
                                      const char *assocClass,
                                      const char *role, const char **propertyList)
{
   CMPIStatus st;
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderReferences");
   st=getRefs(ctx, rslt, cop, assocClass, NULL,
                        role, NULL, propertyList, REF);
   _SFCB_RETURN(st);
}


CMPIStatus InternalProviderReferenceNames(CMPIAssociationMI * mi,
                                          const CMPIContext * ctx,
                                          const CMPIResult * rslt,
                                          const CMPIObjectPath * cop,
                                          const char *assocClass,
                                          const char *role)
{
   CMPIStatus st;
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderReferenceNames");
   st=getRefs(ctx, rslt, cop, assocClass, NULL,
                        role, NULL, NULL, REF_NAME);
   _SFCB_RETURN(st);
}



/* ------------------------------------------------------------------ *
 * Method MI Functions
 * ------------------------------------------------------------------ */

CMPIStatus InternalProviderMethodCleanup(CMPIMethodMI * mi,
					 const CMPIContext * ctx,
					 CMPIBoolean terminate)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };   
   return st;
}

CMPIStatus InternalProviderInvokeMethod(CMPIMethodMI * mi,
					const CMPIContext * ctx,
					const CMPIResult * rslt,
					const CMPIObjectPath * ref,
					const char *methodName,
					const CMPIArgs * in, CMPIArgs * out)
{
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "InternalProviderInvokeMethod");
   CMReturnWithChars(_broker, CMPI_RC_ERR_FAILED, "DefaultProvider does not support invokeMethod operations");
}
/* ------------------------------------------------------------------ *
 * Instance MI Factory
 *
 * NOTE: This is an example using the convenience macros. This is OK
 *       as long as the MI has no special requirements, i.e. to store
 *       data between calls.
 * ------------------------------------------------------------------ */

CMInstanceMIStub(InternalProvider, InternalProvider, _broker, CMNoHook);

CMAssociationMIStub(InternalProvider, InternalProvider, _broker, CMNoHook);

CMMethodMIStub(InternalProvider, InternalProvider, _broker, CMNoHook);

