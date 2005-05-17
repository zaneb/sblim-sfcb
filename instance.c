
/*
 * instance.c
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
extern CMPIString *__oft_toString(CMPIObjectPath * cop, CMPIStatus * rc);

extern CMPIBroker *Broker;

struct native_instance {
   CMPIInstance instance;
   int mem_state;
   int filtered;
   char **property_list;
   char **key_list;
};

/****************************************************************************/

static void __release_list(char **list)
{
   if (list) {
      char **tmp = list;
      while (*tmp) free(*tmp++);
      free(list);
   }
}


static char **__duplicate_list(char **list)
{
   char **result = NULL;

   if (list) {
      size_t size = 1;
      char **tmp = list;

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


static CMPIInstance *__ift_clone(CMPIInstance * instance, CMPIStatus * rc)
{
   struct native_instance *i = (struct native_instance*) instance;
   struct native_instance *new = (struct native_instance*)
       malloc(sizeof(struct native_instance));

   new->property_list = __duplicate_list(i->property_list);
   new->key_list = __duplicate_list(i->key_list);

   ((CMPIInstance*)new)->hdl =
       ClInstanceRebuild((ClInstance *) instance->hdl, NULL);
   ((CMPIInstance*)new)->ft = instance->ft;

   return (CMPIInstance *) new;
}


CMPIData __ift_getPropertyAt(CMPIInstance * ci, CMPICount i, CMPIString ** name,
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
      rv.value.string = native_new_CMPIString(rv.value.chars, NULL);
      rv.type = CMPI_string;
   }
   else if (rv.type == CMPI_ref) {
      char *msg;
      rv.value.ref = getObjectPath(
         (char*)ClObjectGetClString(&inst->hdr, (ClString*)&rv.value.chars), &msg);
   }
   else if (rv.type & CMPI_ARRAY) {
      rv.value.array =
          native_make_CMPIArray((CMPIData *) rv.value.array, NULL, &inst->hdr);
   }

   if (name) {
      *name = native_new_CMPIString(n, NULL);
      free(n);
   }
   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return rv;
}

CMPIData __ift_getProperty(CMPIInstance * ci, const char *id, CMPIStatus * rc)
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

static CMPICount __ift_getPropertyCount(CMPIInstance * ci, CMPIStatus * rc)
{
   ClInstance *inst = (ClInstance *) ci->hdl;
   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return (CMPICount) ClInstanceGetPropertyCount(inst);
}


static CMPIStatus __ift_setProperty(CMPIInstance * instance,
                                    const char *name,
                                    CMPIValue * value, CMPIType type)
{
   struct native_instance *i = (struct native_instance *) instance;
   ClInstance *inst = (ClInstance *) instance->hdl;
   CMPIData data = { type, CMPI_goodValue, {0} };
   int rc;

   if (type == CMPI_chars)
      data.value.chars = (char *) value;
   else if (type == CMPI_string) {
      if (value && value->string && value->string->hdl)
         data.value.chars = (char *) value->string->hdl;
      else data.value.chars=NULL;
      data.type=CMPI_chars;
   }
   else if (type == CMPI_sint64 || type == CMPI_uint64 || type == CMPI_real64)
      data.value = *value;
   else data.value.Int = value->Int;

   if (i->filtered == 0 ||
       i->property_list == NULL ||
       __contained_list(i->property_list, name) ||
       __contained_list(i->key_list, name)) {

      rc=ClInstanceAddProperty(inst, name, data);
      if (rc<0) CMReturn(-rc);      
   }
   CMReturn(CMPI_RC_OK);
}


static CMPIObjectPath *__ift_getObjectPath(CMPIInstance * instance,
                                           CMPIStatus * rc)
{
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
      if (d.type & CMPI_ARRAY) {
         d.value.array->ft->release(d.value.array);
      }   
   }

   if (f == 0) {
      CMPIArray *kl;
      CMPIData d;
      CMPIContext *ctx;
      unsigned int e, m;

      if (klt == NULL) klt = UtilFactory->newHashTable(61,
                 UtilHashTable_charKey | UtilHashTable_ignoreKeyCase);

      if ((kl = klt->ft->get(klt, cn)) == NULL) {
         kl = Broker->eft->getKeyList(Broker, ctx, cop, NULL);
         klt->ft->put(klt, strdup(cn), kl);
      }
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
                                          char **propertyList, char **keys)
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

extern char *value2Chars(CMPIType type, CMPIValue * value);
extern CMPIString *__oft_toString(CMPIObjectPath * cop, CMPIStatus * rc);
extern CMPIString *__oft_getClassName(CMPIObjectPath * cop, CMPIStatus * rc);

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
   name = __oft_getClassName(path, rc);
   add(&buf, &bp, &bm, (char *) name->hdl);
   add(&buf, &bp, &bm, " {\n");
   ps = __oft_toString(path, rc);
   add(&buf, &bp, &bm, " PATH: ");
   add(&buf, &bp, &bm, (char *) ps->hdl);
   add(&buf, &bp, &bm, "\n");

   for (i = 0, m = __ift_getPropertyCount(inst, rc); i < m; i++) {
      data = __ift_getPropertyAt(inst, i, &name, rc);
      add(&buf, &bp, &bm, " ");
      add(&buf, &bp, &bm, (char *) name->hdl);
      add(&buf, &bp, &bm, " = ");
      v = value2Chars(data.type, &data.value);
      add(&buf, &bp, &bm, v);
      free(v);
      add(&buf, &bp, &bm, " ;\n");
   }
   add(&buf, &bp, &bm, "}\n");
   rv = native_new_CMPIString(buf, rc);
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

unsigned long getInstanceSerializedSize(CMPIInstance * ci)
{
   ClInstance *cli = (ClInstance *) ci->hdl;
   return ClSizeInstance(cli) + sizeof(struct native_instance);
}

void getSerializedInstance(CMPIInstance * ci, void *area)
{
   memcpy(area, ci, sizeof(struct native_instance));
   ClInstanceRebuild((ClInstance *) ci->hdl,
                     (void *) ((char *) area + sizeof(struct native_instance)));
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

CMPIInstance *internal_new_CMPIInstance(int mode, CMPIObjectPath * cop,
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
      asm("int $3");
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
   
   return (CMPIInstance*)tInst;
}

CMPIInstance *TrackedCMPIInstance(CMPIObjectPath * cop, CMPIStatus * rc)
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
      sb->ft->appendString(sb, name);
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

/****************************************************************************/

/*** Local Variables:  ***/
/*** mode: C           ***/
/*** c-basic-offset: 8 ***/
/*** End:              ***/
