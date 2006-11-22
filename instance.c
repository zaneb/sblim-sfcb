/*
 * instance.c
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
 * Author:        Frank Scheffler
 * Contributions: Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * CMPIInstance implementation.
 *
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utilft.h"
#include "native.h"

#include "objectImpl.h"
#include "providerMgr.h"
#include "config.h"

#ifdef SFCB_IX86
#define SFCB_ASM(x) asm(x)
#else
#define SFCB_ASM(x)
#endif

extern int ClInstanceGetPropertyAt(ClInstance * inst, int id, CMPIData * data,
                                   char **name, unsigned long *quals);
extern int ClObjectLocateProperty(ClObjectHdr * hdr, ClSection * prps,
                                  const char *id);
extern int ClInstanceGetPropertyCount(ClInstance * inst);
extern int ClInstanceAddProperty(ClInstance * inst, const char *id, CMPIData d);
extern ClInstance *ClInstanceNew(const char *ns, const char *cn);
extern void ClInstanceFree(ClInstance * inst);
extern const char *ClInstanceGetClassName(ClInstance * inst);
extern const char *ClInstanceGetNameSpace(ClInstance * inst);
extern unsigned long ClSizeInstance(ClInstance * inst);
extern ClInstance *ClInstanceRebuild(ClInstance * inst, void *area);
extern void ClInstanceRelocateInstance(ClInstance * inst);
extern CMPIArray *native_make_CMPIArray(CMPIData * av, CMPIStatus * rc,
                                        ClObjectHdr * hdr);
extern CMPIObjectPath *getObjectPath(char *path, char **msg);
CMPIObjectPath *internal_new_CMPIObjectPath(int mode, const char *nameSpace,
                                            const char *className,
                                            CMPIStatus * rc);
extern const char *ClObjectGetClString(ClObjectHdr * hdr, ClString * id);
//extern CMPIString *__oft_toString(CMPIObjectPath * cop, CMPIStatus * rc);

extern CMPIBroker *Broker;

struct native_instance {
   CMPIInstance instance;
   int refCount;
   int mem_state;
   int filtered;
   char **property_list;
   char **key_list;
};

#ifdef HAVE_DEFAULT_PROPERTIES 
static void instFillDefaultProperties(struct native_instance *inst, 
				      const char * ns, const char * cn);
#endif

/****************************************************************************/

static void __release_list(char **list)
{
   if (list) {
      char **tmp = list;
      while (*tmp) free(*tmp++);
      free(list);
   }
}


static char **__duplicate_list(const char **list)
{
   char **result = NULL;

   if (list) {
      size_t size = 1;
      char **tmp = (char**)list;

      while (*tmp++) ++size;
      result = calloc(1,size*sizeof(char *));
      for (tmp = result; *list; tmp++) {
         *tmp = strdup(*list++);
      }
   }
   return result;
}


void memLinkInstance(CMPIInstance *ci)
{
   struct native_instance *i = (struct native_instance *) ci;
   memLinkEncObj(i,&i->mem_state);
}

static int __contained_list(char **list, const char *name)
{
   if (list) {
      while (*list) {
         if (strcasecmp(*list++, name) == 0) return 1;
      }   
   }
   return 0;
}


/****************************************************************************/


static CMPIStatus __ift_release(CMPIInstance * instance)
{
   struct native_instance *i = (struct native_instance *) instance;

   if (i->mem_state && i->mem_state != MEM_RELEASED) {
      __release_list(i->property_list);
      __release_list(i->key_list);
      ClInstanceFree((ClInstance *) instance->hdl);
      memUnlinkEncObj(i->mem_state);
      i->mem_state = MEM_RELEASED;
      free(i);
      CMReturn(CMPI_RC_OK);
   }
   CMReturn(CMPI_RC_ERR_FAILED);
}


static CMPIInstance *__ift_clone(const CMPIInstance * instance, CMPIStatus * rc)
{
   struct native_instance *i = (struct native_instance*) instance;
   struct native_instance *new = (struct native_instance*)
       malloc(sizeof(struct native_instance));

   new->mem_state=MEM_NOT_TRACKED;
   new->property_list = __duplicate_list((const char**)i->property_list);
   new->key_list = __duplicate_list((const char**)i->key_list);

   ((CMPIInstance*)new)->hdl =
       ClInstanceRebuild((ClInstance *) instance->hdl, NULL);
   ((CMPIInstance*)new)->ft = instance->ft;

   return (CMPIInstance *) new;
}


CMPIData __ift_getPropertyAt(const CMPIInstance * ci, CMPICount i, CMPIString ** name,
                             CMPIStatus * rc)
{
   ClInstance *inst = (ClInstance *) ci->hdl;
   char *n;
   CMPIData rv = { 0, CMPI_notFound, {0} };
   if (ClInstanceGetPropertyAt(inst, i, &rv, name ? &n : NULL, NULL)) {
      if (rc)
         CMSetStatus(rc, CMPI_RC_ERR_NOT_FOUND);
      return rv;
   }
   if (rv.type == CMPI_chars) {
      rv.value.string = sfcb_native_new_CMPIString(rv.value.chars, NULL);
      rv.type = CMPI_string;
   }
   else if (rv.type == CMPI_ref) {
      char *msg;
      rv.value.ref = getObjectPath(
         (char*)ClObjectGetClString(&inst->hdr, (ClString*)&rv.value.chars), &msg);
   }
   else if (rv.type & CMPI_ARRAY && rv.value.array) {
      rv.value.array =
          native_make_CMPIArray((CMPIData *) rv.value.array, NULL, &inst->hdr);
   }

   if (name) {
      *name = sfcb_native_new_CMPIString(n, NULL);
      free(n);
   }
   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return rv;
}

CMPIData __ift_getProperty(const CMPIInstance * ci, const char *id, CMPIStatus * rc)
{
   ClInstance *inst = (ClInstance *) ci->hdl;
   ClSection *prps = &inst->properties;
   CMPIData rv = { 0, CMPI_notFound, {0} };
   int i;

   if ((i = ClObjectLocateProperty(&inst->hdr, prps, id)) != 0) {
      return __ift_getPropertyAt(ci, i - 1, NULL, rc);
   }

   if (rc)
      CMSetStatus(rc, CMPI_RC_ERR_NOT_FOUND);
   return rv;
}

static CMPICount __ift_getPropertyCount(const CMPIInstance * ci, CMPIStatus * rc)
{
   ClInstance *inst = (ClInstance *) ci->hdl;
   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return (CMPICount) ClInstanceGetPropertyCount(inst);
}


static CMPIStatus __ift_setProperty(CMPIInstance * instance,
                                    const char *name,
                                    const CMPIValue * value, CMPIType type)
{
   struct native_instance *i = (struct native_instance *) instance;
   ClInstance *inst = (ClInstance *) instance->hdl;
   CMPIData data = { type, CMPI_goodValue, {0LL} };
   int rc;

   if (type == CMPI_chars) {
      /* VM: is this OK or do we need a __new copy */
      data.value.chars = (char *) value;
   } else if (type == CMPI_string) {
      data.type=CMPI_chars;
      if (value && value->string && value->string->hdl) {
	 /* VM: is this OK or do we need a __new copy */
         data.value.chars = (char *) value->string->hdl;
      } else {
	 data.value.chars=NULL;
      }
   } else if (value) {
      data.value = *value;
   }

   if (((type & CMPI_ENCA) && data.value.chars == NULL) || value == NULL) {
     data.state=CMPI_nullValue;
   }

   if (i->filtered == 0 ||
       i->property_list == NULL ||
       __contained_list(i->property_list, name) ||
       __contained_list(i->key_list, name)) {

      rc=ClInstanceAddProperty(inst, name, data);
      if (rc<0) CMReturn(-rc);      
   }
   CMReturn(CMPI_RC_OK);
}


static CMPIObjectPath *__ift_getObjectPath(const CMPIInstance * instance,
                                           CMPIStatus * rc)
{
   static CMPI_MUTEX_TYPE * mtx = NULL;
   static UtilHashTable *klt = NULL;
   int j, f = 0;
   CMPIStatus tmp;
   const char *cn = ClInstanceGetClassName((ClInstance *) instance->hdl);
   const char *ns = ClInstanceGetNameSpace((ClInstance *) instance->hdl);
   char *id;

   CMPIObjectPath *cop;
   cop = TrackedCMPIObjectPath(ns, cn, rc);

   if (rc && rc->rc != CMPI_RC_OK)
      return NULL;

   j = __ift_getPropertyCount(instance, NULL);

   while (j--) {
      CMPIString *keyName;
      CMPIData d = __ift_getPropertyAt(instance, j, &keyName, &tmp);
      if (d.state & CMPI_keyValue) {
         CMAddKey(cop, CMGetCharsPtr(keyName, NULL), &d.value, d.type);
         f++;
      }
      if (d.type & CMPI_ARRAY && (d.state & CMPI_nullValue) == 0) {
         d.value.array->ft->release(d.value.array);
      }   
   }

   if (f == 0) {
      CMPIArray *kl;
      CMPIData d;
      unsigned int e, m;

      if (mtx == NULL) {
	 mtx = malloc(sizeof(CMPI_MUTEX_TYPE));
	 *mtx = Broker->xft->newMutex(0); 
      }
      Broker->xft->lockMutex(*mtx);
      if (klt == NULL) klt = UtilFactory->newHashTable(61,
                 UtilHashTable_charKey | UtilHashTable_ignoreKeyCase);

      if ((kl = klt->ft->get(klt, cn)) == NULL) {	 
	 CMPIConstClass * cc = getConstClass(ns,cn);
	 kl = cc->ft->getKeyList(cc);
         klt->ft->put(klt, strdup(cn), kl);
      }
      Broker->xft->unlockMutex(*mtx);
      m = kl->ft->getSize(kl, NULL);

      for (e = 0; e < m; e++) {
         CMPIString *n = kl->ft->getElementAt(kl, e, NULL).value.string;
         id=CMGetCharPtr(n);
         d = __ift_getProperty(instance, CMGetCharPtr(n), &tmp);
         if (tmp.rc == CMPI_RC_OK) {
            CMAddKey(cop, CMGetCharPtr(n), &d.value, d.type);
         }
      }
   }
   return cop;
}


static CMPIStatus __ift_setPropertyFilter(CMPIInstance * instance,
                                          const char **propertyList, 
					  const char **keys)
{
   struct native_instance *i = (struct native_instance *) instance;
//printf("__ift_setPropertyFilter %p %p\n",propertyList,keys);
   if (i->filtered) {
      __release_list(i->property_list);
      __release_list(i->key_list);
   }

   i->filtered = 1;
   i->property_list = __duplicate_list(propertyList);
   i->key_list = __duplicate_list(keys);

   CMReturn(CMPI_RC_OK);
}

void add(char **buf, unsigned int *p, unsigned int *m, char *data)
{
   unsigned int ds = strlen(data) + 1;

   if (*buf == NULL) {
      *buf = (char *) malloc(1024);
      *p = 0;
      *m = 1024;
   }
   if ((ds + (*p)) >= *m) {
      unsigned nm = *m;
      char *nb;
      while ((ds + (*p)) >= nm)
         nm *= 2;
      nb = (char *) malloc(nm);
      memcpy(nb, *buf, *p);
      free(*buf);
      *buf = nb;
      *m = nm;
   }
   memcpy(*buf + (*p), data, ds);
   *p += ds - 1;
}

extern char *sfcb_value2Chars(CMPIType type, CMPIValue * value);

CMPIString *instance2String(CMPIInstance * inst, CMPIStatus * rc)
{
   CMPIObjectPath *path;
   CMPIData data;
   CMPIString *name, *ps, *rv;
   unsigned int i, m;
   char *buf = NULL, *v;
   unsigned int bp, bm;

   add(&buf, &bp, &bm, "Instance of ");
   path = __ift_getObjectPath(inst, NULL);
   name =  path->ft->toString(path, rc);
   add(&buf, &bp, &bm, (char *) name->hdl);
   add(&buf, &bp, &bm, " {\n");
   ps = path->ft->toString(path, rc);
   add(&buf, &bp, &bm, " PATH: ");
   add(&buf, &bp, &bm, (char *) ps->hdl);
   add(&buf, &bp, &bm, "\n");

   for (i = 0, m = __ift_getPropertyCount(inst, rc); i < m; i++) {
      data = __ift_getPropertyAt(inst, i, &name, rc);
      add(&buf, &bp, &bm, " ");
      add(&buf, &bp, &bm, (char *) name->hdl);
      add(&buf, &bp, &bm, " = ");
      v = sfcb_value2Chars(data.type, &data.value);
      add(&buf, &bp, &bm, v);
      free(v);
      add(&buf, &bp, &bm, " ;\n");
   }
   add(&buf, &bp, &bm, "}\n");
   rv = sfcb_native_new_CMPIString(buf, rc);
   free(buf);
   return rv;
}

static CMPIInstanceFT ift = {
   NATIVE_FT_VERSION,
   __ift_release,
   __ift_clone,
   __ift_getProperty,
   __ift_getPropertyAt,
   __ift_getPropertyCount,
   __ift_setProperty,
   __ift_getObjectPath,
   __ift_setPropertyFilter
};

CMPIInstanceFT *CMPI_Instance_FT = &ift;

unsigned long getInstanceSerializedSize(const CMPIInstance * ci)
{
   ClInstance *cli = (ClInstance *) ci->hdl;
   return ClSizeInstance(cli) + sizeof(struct native_instance);
}

void getSerializedInstance(const CMPIInstance * ci, void *area)
{
   memcpy(area, ci, sizeof(struct native_instance));
   ClInstanceRebuild((ClInstance *) ci->hdl,
                     (void *) ((char *) area + sizeof(struct native_instance)));
   ((CMPIInstance *)(area))->hdl = (ClInstance *) ((char *) area + sizeof(struct native_instance));
}

CMPIInstance *relocateSerializedInstance(void *area)
{
   struct native_instance *ci = (struct native_instance *) area;
   ci->instance.hdl = ci + 1;
   ci->instance.ft = &ift;
   ci->mem_state=MEM_RELEASED;
   ci->property_list = NULL;
   ci->key_list = NULL;
   ClInstanceRelocateInstance((ClInstance *) ci->instance.hdl);
   return (CMPIInstance *) ci;
}

MsgSegment setInstanceMsgSegment(CMPIInstance * ci)
{
   MsgSegment s;
   s.data = ci;
   s.type = MSG_SEG_INSTANCE;
   s.length = getInstanceSerializedSize(ci);
   return s;
}

CMPIInstance *internal_new_CMPIInstance(int mode, const CMPIObjectPath * cop,
                                        CMPIStatus * rc)
{
   static CMPIInstance i = {
      "CMPIInstance",
      &ift
   };

   struct native_instance instance,*tInst;
   memset(&instance, 0, sizeof(instance));

   CMPIStatus tmp1, tmp2, tmp3;
   CMPIString *str;
   char *ns, *cn;
   int j,state;

   instance.instance = i;

   if (cop) {
      j = CMGetKeyCount(cop, &tmp1);
   str = CMGetClassName(cop, &tmp2);
   cn = CMGetCharsPtr(str, NULL);
   str = CMGetNameSpace(cop, &tmp3);
   ns = CMGetCharsPtr(str, NULL);
   }
   
   else {
      j=0;
      SFCB_ASM("int $3");
      ns = "*NoNameSpace*";
      cn = "*NoClassName*";
      tmp1.rc=tmp2.rc=tmp3.rc=CMPI_RC_OK;
   }   

   if (tmp1.rc != CMPI_RC_OK || tmp2.rc != CMPI_RC_OK || tmp3.rc != CMPI_RC_OK) {
      if (rc) CMSetStatus(rc, CMPI_RC_ERR_FAILED);
   }
   else {
      instance.instance.hdl = ClInstanceNew(ns, cn);

      while (j-- && (tmp1.rc == CMPI_RC_OK)) {
         CMPIString *keyName;
         CMPIData tmp = CMGetKeyAt(cop, j, &keyName, &tmp1);
         __ift_setProperty(&instance.instance, CMGetCharsPtr(keyName, NULL),
                           &tmp.value, tmp.type);
      }
      if (rc) CMSetStatus(rc, tmp1.rc);
   }

   tInst=memAddEncObj(mode, &instance, sizeof(instance),&state);
   tInst->mem_state=state;
   tInst->refCount=0;
   
#ifdef HAVE_DEFAULT_PROPERTIES 
   instFillDefaultProperties(tInst,ns,cn);
#endif

   return (CMPIInstance*)tInst;
}

CMPIInstance *TrackedCMPIInstance(const CMPIObjectPath * cop, CMPIStatus * rc)
{
   return internal_new_CMPIInstance(MEM_TRACKED, cop, rc);
}

CMPIInstance *NewCMPIInstance(CMPIObjectPath * cop, CMPIStatus * rc)
{
   return internal_new_CMPIInstance(MEM_NOT_TRACKED, cop, rc);
}


static void dataToString(CMPIData d, UtilStringBuffer * sb)
{
   char str[256];
   char *sp = str;

   if (d.type & CMPI_ARRAY) {
      sb->ft->appendChars(sb, "[]");
      return;
   }
   if (d.type & CMPI_UINT) {
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
   else if (d.type == CMPI_string) {
      sp = (char *) d.value.string->hdl;
   }
   sb->ft->appendChars(sb, sp);
}

UtilStringBuffer *instanceToString(CMPIInstance * ci, char **props)
{
   unsigned int i, m;
   CMPIData data;
   CMPIString *name;
   UtilStringBuffer *sb = UtilFactory->newStrinBuffer(64);

   for (i = 0, m = CMGetPropertyCount(ci, NULL); i < m; i++) {
      data = CMGetPropertyAt(ci, i, &name, NULL);
      sb->ft->appendChars(sb, (char*)name->hdl);
      sb->ft->appendChars(sb, "=");
      dataToString(data, sb);
      sb->ft->appendChars(sb, "\n");
   }
   return sb;
}

const char *instGetClassName(CMPIInstance * ci)
{
   ClInstance *inst = (ClInstance *) ci->hdl;
   return ClInstanceGetClassName(inst);
}

#ifdef HAVE_DEFAULT_PROPERTIES 
static void instFillDefaultProperties(struct native_instance *inst, 
				      const char * ns, const char * cn)
{
   static CMPI_MUTEX_TYPE * mtx = NULL;
   static UtilHashTable *clt = NULL;
   CMPIConstClass *cc;
   CMPICount       pc;
   CMPIData        pd;
   CMPIStatus      ps;
   CMPIString     *pn = NULL;
   CMPIValue      *vp;
   
   if (mtx == NULL) {
      mtx = malloc(sizeof(CMPI_MUTEX_TYPE));
      *mtx = Broker->xft->newMutex(0); 
   }
   Broker->xft->lockMutex(*mtx);
   if (clt == NULL) clt = 
      UtilFactory->newHashTable(61,
				UtilHashTable_charKey | UtilHashTable_ignoreKeyCase);
   
   if ((cc = clt->ft->get(clt, cn)) == NULL) {	 
      cc = getConstClass(ns,cn);
      clt->ft->put(clt, strdup(cn), cc->ft->clone(cc,NULL));
   }
   Broker->xft->unlockMutex(*mtx);
   if (cc) {
      pc = cc->ft->getPropertyCount(cc,NULL);
      while (pc > 0) {
	 pc -= 1;
	 pd = cc->ft->getPropertyAt(cc,pc,&pn,&ps);
	 if (ps.rc == CMPI_RC_OK && pn ) {
	    vp = &pd.value;
	    if (pd.state & CMPI_nullValue) {
	       /* must set  null value indication: 
		  CMPI doesn't allow to do that properly */
	       pd.value.chars = NULL;
	       if ((pd.type & (CMPI_SIMPLE|CMPI_REAL|CMPI_INTEGER)) && 
		   (pd.type & CMPI_ARRAY) == 0 ) {
		  vp = NULL;
	       }
	    }
	    __ift_setProperty(&inst->instance,CMGetCharsPtr(pn,NULL),
			      vp,pd.type);
	 }
      }
   }
}
#endif

/****************************************************************************/

/*** Local Variables:  ***/
/*** mode: C           ***/
/*** c-basic-offset: 3 ***/
/*** End:              ***/
