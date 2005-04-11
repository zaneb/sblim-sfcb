
/*
 * constClass.c
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
 * Internal constClass support for sfcb .
 *
*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constClass.h"
#include "objectImpl.h"
#include "msgqueue.h"

//#define DEB(x) x
#define DEB(x)

extern const char *ClObjectGetClString(ClObjectHdr * hdr, ClString * id);
extern char *ClClassToString(ClClass * cls);
extern int ClClassGetPropertyCount(ClClass * cls);
extern int ClClassGetPropertyAt(ClClass * cls, int id, CMPIData * data,
                                char **name, unsigned long *quals);
extern int ClClassGetQualifierAt(ClClass * cls, int id, CMPIData * data,
                                 char **name);
extern int ClClassGetPropQualifierAt(ClClass * cls, int id, int qid,
                                     CMPIData * data, char **name);
extern CMPIString *native_new_CMPIString(const char *ptr, CMPIStatus * rc);
extern int ClObjectLocateProperty(ClObjectHdr * hdr, ClSection * prps,
                                  const char *id);
extern unsigned long ClSizeClass(ClClass * cls);
extern ClClass *ClClassRebuildClass(ClClass * cls, void *area);
extern void ClClassRelocateClass(ClClass * cls);
extern CMPIArray *native_make_CMPIArray(CMPIData * av, CMPIStatus * rc);
extern CMPIObjectPath *getObjectPath(char *path, char **msg);

unsigned long getConstClassSerializedSize(CMPIConstClass * cl);
static CMPIConstClass *clone(CMPIConstClass * cc, CMPIStatus * rc);

static CMPIStatus release(CMPIConstClass * cc)
{
   CMPIStatus rc = { 0, NULL };
   free(cc);
   return rc;
}

static void relocate(CMPIConstClass * cc)
{
   ClClass *cls = (ClClass *) cc->hdl;
   ClStrBuf *buf;
   if MALLOCED
      (cls->hdr.strBufOffset)
          buf = (ClStrBuf *) cls->hdr.strBufOffset;
   else
      buf = (ClStrBuf *) ((char *) cls + abs(cls->hdr.strBufOffset));
   buf->index = (long *) ((char *) cls + abs(buf->iOffset));
}

static const char *getCharClassName(CMPIConstClass * cc)
{
   ClClass *cls = (ClClass *) cc->hdl;
   if (cls->name.id)
      return ClObjectGetClString(&cls->hdr, &cls->name);
   return NULL;
}

static const char *getCharSuperClassName(CMPIConstClass * cc)
{
   ClClass *cls = (ClClass *) cc->hdl;
   if (cls->parent.id)
      return ClObjectGetClString(&cls->hdr, &cls->parent);
   return NULL;
}

CMPIBoolean isAbstract(CMPIConstClass * cc)
{
   ClClass *cls = (ClClass *) cc->hdl;
   return (cls->quals & ClClass_Q_Abstract) != 0;
}

static CMPIBoolean isIndication(CMPIConstClass * cc)
{
   ClClass *cls = (ClClass *) cc->hdl;
   return (cls->quals & ClClass_Q_Indication) != 0;
}

static CMPIBoolean isAssociation(CMPIConstClass * cc)
{
   ClClass *cls = (ClClass *) cc->hdl;
   return (cls->quals & ClClass_Q_Association) != 0;
}

static char *toString(CMPIConstClass * cc)
{
   ClClass *cls = (ClClass *) cc->hdl;
   return ClClassToString(cls);
}

static CMPICount getPropertyCount(CMPIConstClass * cc, CMPIStatus * rc)
{
   ClClass *cls = (ClClass *) cc->hdl;
   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return (CMPICount) ClClassGetPropertyCount(cls);
}

CMPIData getPropertyAt(CMPIConstClass * cc, CMPICount i, CMPIString ** name,
                       unsigned long *quals, CMPIStatus * rc)
{
   ClClass *cls = (ClClass *) cc->hdl;
   char *n;
   CMPIData rv = { 0, CMPI_notFound, {0} };
   if (ClClassGetPropertyAt(cls, i, &rv, name ? &n : NULL, quals)) {
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
      rv.value.ref = getObjectPath(ClObjectGetClString(&cls->hdr, 
          (ClString *) & rv.value.chars), &msg);
   }
   if (rv.type & CMPI_ARRAY) {
      rv.value.array =
          native_make_CMPIArray((CMPIData *) & rv.value.array, NULL);
   }
   if (name) {
      *name = native_new_CMPIString(n, NULL);
      free(n);
   }
   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return rv;
}

CMPIData getQualifierAt(CMPIConstClass * cc, CMPICount i, CMPIString ** name,
                        CMPIStatus * rc)
{
   ClClass *cls = (ClClass *) cc->hdl;
   char *n;
   CMPIData rv = { 0, CMPI_notFound, {0} };
   if (ClClassGetQualifierAt(cls, i, &rv, name ? &n : NULL)) {
      if (rc)
         CMSetStatus(rc, CMPI_RC_ERR_NOT_FOUND);
      return rv;
   }
   if (rv.type == CMPI_chars) {
      rv.value.string =
          native_new_CMPIString(ClObjectGetClString
                                (&cls->hdr, (ClString *) & rv.value.chars),
                                NULL);
      rv.type = CMPI_string;
   }
   if (rv.type & CMPI_ARRAY) {
      rv.value.array =
          native_make_CMPIArray((CMPIData *) & rv.value.array, NULL);
   }
   if (name) {
      *name = native_new_CMPIString(n, NULL);
      free(n);
   }
   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return rv;
}

CMPIData getPropQualifierAt(CMPIConstClass * cc, CMPICount p, CMPICount i,
                            CMPIString ** name, CMPIStatus * rc)
{
   ClClass *cls = (ClClass *) cc->hdl;
   char *n;
   CMPIData rv = { 0, CMPI_notFound, {0} };
   if (ClClassGetPropQualifierAt(cls, p, i, &rv, name ? &n : NULL)) {
      if (rc)
         CMSetStatus(rc, CMPI_RC_ERR_NOT_FOUND);
      return rv;
   }
   if (rv.type == CMPI_chars) {
      rv.value.string =
          native_new_CMPIString(ClObjectGetClString
                                (&cls->hdr, (ClString *) & rv.value.chars),
                                NULL);
      rv.type = CMPI_string;
   }
   if (rv.type & CMPI_ARRAY) {
      rv.value.array =
          native_make_CMPIArray((CMPIData *) & rv.value.array, NULL);
   }
   if (name) {
      *name = native_new_CMPIString(n, NULL);
      free(n);
   }
   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return rv;
}

CMPIData getProperty(CMPIConstClass * cc, const char *id, unsigned long *quals,
                     CMPIStatus * rc)
{
   ClClass *cls = (ClClass *) cc->hdl;
   ClSection *prps = &cls->properties;
   CMPIData rv = { 0, CMPI_notFound, {0} };
   int i;

   if ((i = ClObjectLocateProperty(&cls->hdr, prps, id)) != 0) {
      return getPropertyAt(cc, i - 1, NULL, quals, rc);
   }

   if (rc)
      CMSetStatus(rc, CMPI_RC_ERR_NOT_FOUND);
   return rv;
}

CMPIArray *getKeyList(CMPIConstClass * cc)
{
   int i, m, c;
   unsigned long quals;
   CMPIArray *kar;
   CMPIString *name;
   int idx[32];

   for (i = c = 0, m = getPropertyCount(cc, NULL); i < m; i++) {
      getPropertyAt(cc, i, NULL, &quals, NULL);
      if (quals & ClProperty_Q_Key) {
         idx[c] = i;
         c++;
      }
   }
   kar = NewCMPIArray(c, CMPI_string, NULL);

   for (i = 0; i < c; i++) {
      getPropertyAt(cc, idx[i], &name, &quals, NULL);
      CMSetArrayElementAt(kar, i, &name, CMPI_string);
   }
   return kar;
}

struct _CMPIConstClass_FT ift = {
   1,
   release,
   clone,
   relocate,
   getCharClassName,
   getCharSuperClassName,
   isAssociation,
   isAbstract,
   isIndication,
   getPropertyCount,
   getProperty,
   getPropertyAt,
   getQualifierAt,
   getPropQualifierAt,
   getKeyList,
   toString
};

CMPIConstClass_FT *CMPIConstClassFT = &ift;

static CMPIConstClass *clone(CMPIConstClass * cc, CMPIStatus * rc)
{
   CMPIConstClass *cl =
       (CMPIConstClass *) malloc(getConstClassSerializedSize(cc));
   cl->hdl = cl + 1;
   cl->ft = &ift;
   ClClassRebuildClass((ClClass *) cc->hdl, cl->hdl);
   if (rc)
      rc->rc = 0;
   return cl;
}

unsigned long getConstClassSerializedSize(CMPIConstClass * cl)
{
   ClClass *cli = (ClClass *) cl->hdl;
   return ClSizeClass(cli) + sizeof(CMPIConstClass);
}

void getSerializedConstClass(CMPIConstClass * cl, void *area)
{
   memcpy(area, cl, sizeof(CMPIConstClass));
   ClClassRebuildClass((ClClass *) cl->hdl,
                       (void *) ((char *) area + sizeof(CMPIConstClass)));
}

CMPIConstClass *relocateSerializedConstClass(void *area)
{
   CMPIConstClass *cl = (CMPIConstClass *) area;
   cl->hdl = cl + 1;
   cl->ft = &ift;
   ClClassRelocateClass((ClClass *) cl->hdl);
   return (CMPIConstClass *) cl;
}

MsgSegment setConstClassMsgSegment(CMPIConstClass * cl)
{
   MsgSegment s;
   s.data = cl;
   s.type = MSG_SEG_CONSTCLASS;
   s.length = getConstClassSerializedSize(cl);
   return s;
}

int verifyPropertyList(CMPIConstClass *cl, char **list)
{
   CMPIStatus rc;
   int count=0;
   for (; *list; list++) {
      getProperty(cl, *list, NULL, &rc);
      if (rc.rc==0) count++;
   }
   return count;
}

CMPIConstClass initConstClass(ClClass *cl)
{
   CMPIConstClass c;
   c.hdl=cl;
   c.ft=&ift;
   return c;
}