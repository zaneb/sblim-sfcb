
/*
 * args.c
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
 * Author:       Frank Scheffler 
 * Contributors: Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 * Extension of the CMPIArgs data type.
 * This structure stores the information needed to represent arguments for
 * CMPI providers, i.e. within invokeMethod() calls.
 *
*/



#include <stdio.h>
#include <string.h>

#include "native.h"
#include "objectImpl.h"
#include "msgqueue.h"

struct native_args {
   CMPIArgs args;               /*!< the inheriting data structure  */
   int mem_state;               /*!< states, whether this object is
                                   registered within the memory mangagement or
                                   represents a cloned object */
};

extern int ClObjectLocateProperty(ClObjectHdr * hdr, ClSection * prps,
                                  const char *id);
extern void ClArgsFree(ClArgs * arg);
extern ClArgs *ClArgsRebuild(ClArgs * arg, void *area);
extern int ClArgsGetArgAt(ClArgs * arg, int id, CMPIData * data, char **name);
extern int ClArgsAddArg(ClArgs * arg, const char *id, CMPIData d);
extern int ClArgsGetArgCount(ClArgs * arg);
extern ClArgs *ClArgsNew();
extern unsigned long ClSizeArgs(ClArgs * arg);
extern void ClArgsRelocateArgs(ClArgs * arg);
extern CMPIArray *native_make_CMPIArray(CMPIData * av, CMPIStatus * rc,
                                        ClObjectHdr * hdr);
extern CMPIObjectPath *getObjectPath(char *path, char **msg);
extern const char *ClObjectGetClString(ClObjectHdr * hdr, ClString * id);

static struct native_args *__new_empty_args(int, CMPIStatus *);
MsgSegment setArgsMsgSegment(CMPIArgs * args);

/****************************************************************************/


static CMPIStatus __aft_release(CMPIArgs * args)
{
   struct native_args *a = (struct native_args *) args;

   if (a->mem_state == TOOL_MM_NO_ADD) {
      ClArgsFree((ClArgs *) a->args.hdl);
      free(args);
      CMReturn(CMPI_RC_OK);
   }

   CMReturn(CMPI_RC_ERR_FAILED);
}


static CMPIArgs *__aft_clone(CMPIArgs * args, CMPIStatus * rc)
{
   struct native_args *a = (struct native_args *) args;
   struct native_args *na = __new_empty_args(TOOL_MM_NO_ADD, rc);

   //  if (rc->rc == CMPI_RC_OK) {
   na->args.hdl = ClArgsRebuild((ClArgs *) a->args.hdl, NULL);
   //  }

   return (CMPIArgs *) na;
}


static CMPIStatus __aft_addArg(CMPIArgs * args, const char *name,
                               CMPIValue * value, CMPIType type)
{
   ClArgs *ca = (ClArgs *) args->hdl;
   CMPIData data = { type, CMPI_goodValue, {0} };

   if (type == CMPI_chars)
      data.value.chars = (char *) value;
   else if (type == CMPI_sint64 || type == CMPI_uint64 || type == CMPI_real64)
      data.value = *value;
   else
      data.value.Int = value->Int;
   ClArgsAddArg(ca, name, data);

   CMReturn(CMPI_RC_OK);
}


static CMPIData __aft_getArgAt(CMPIArgs * args,
                               unsigned int i,
                               CMPIString ** name, CMPIStatus * rc)
{
   ClArgs *ca = (ClArgs *) args->hdl;
   char *n;
   CMPIData rv = { 0, CMPI_notFound, {0} };

   if (ClArgsGetArgAt(ca, i, &rv, name ? &n : NULL)) {
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
      rv.value.ref = getObjectPath(ClObjectGetClString
         (&ca->hdr, (ClString *) & rv.value.chars), &msg);
   }
   else if (rv.type & CMPI_ARRAY) {
      rv.value.array =
          native_make_CMPIArray((CMPIData *) rv.value.array, NULL, &ca->hdr);
   }
   if (name) {
      *name = native_new_CMPIString(n, NULL);
      free(n);
   }
   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return rv;
}

static CMPIData __aft_getArg(CMPIArgs * args, const char *name, CMPIStatus * rc)
{
   ClArgs *ca = (ClArgs *) args->hdl;
   ClSection *prps = &ca->properties;
   CMPIData rv = { 0, CMPI_notFound, {0} };
   int i;

   if ((i = ClObjectLocateProperty(&ca->hdr, prps, name)) != 0) {
      return __aft_getArgAt(args, i - 1, NULL, rc);
   }

   if (rc)
      CMSetStatus(rc, CMPI_RC_ERR_NOT_FOUND);
   return rv;
}


static unsigned int __aft_getArgCount(CMPIArgs * args, CMPIStatus * rc)
{
   ClArgs *ca = (ClArgs *) args->hdl;
   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return (CMPICount) ClArgsGetArgCount(ca);
}


static CMPIArgsFT aft = {
   NATIVE_FT_VERSION,
   __aft_release,
   __aft_clone,
   __aft_addArg,
   __aft_getArg,
   __aft_getArgAt,
   __aft_getArgCount
};

static struct native_args *__new_empty_args(int mm_add, CMPIStatus * rc)
{
   static CMPIArgs a = {
      "CMPIArgs",
      &aft
   };

   struct native_args *args = (struct native_args *)
       tool_mm_alloc(mm_add, sizeof(struct native_args));

   args->args = a;
   args->mem_state = mm_add;
   args->args.hdl = ClArgsNew();

   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return args;
}


CMPIArgs *NewCMPIArgs(CMPIStatus * rc)
{
   return (CMPIArgs *) __new_empty_args(TOOL_MM_NO_ADD, rc);
}

CMPIArgs *TrackedCMPIArgs(CMPIStatus * rc)
{
   return (CMPIArgs *) __new_empty_args(TOOL_MM_ADD, rc);
}

unsigned long getArgsSerializedSize(CMPIArgs * args)
{
   ClArgs *ca = (ClArgs *) args->hdl;
   return ClSizeArgs(ca) + sizeof(struct native_args);
}

void getSerializedArgs(CMPIArgs * args, void *area)
{
   if (args) {
      memcpy(area, args, sizeof(struct native_args));
      ClArgsRebuild((ClArgs *) args->hdl,
                    (void *) ((char *) area + sizeof(struct native_args)));
   }
   else {

   }
}

CMPIArgs *relocateSerializedArgs(void *area)
{
   struct native_args *args = (struct native_args *) area;
   args->args.hdl = args + 1;
   args->args.ft = &aft;
   ClArgsRelocateArgs((ClArgs *) args->args.hdl);
   return (CMPIArgs *) args;
}

MsgSegment setArgsMsgSegment(CMPIArgs * args)
{
   MsgSegment s;
   s.data = args;
   s.type = MSG_SEG_ARGS;
   s.length = args ? getArgsSerializedSize(args) : 0;
   return s;
}

/*****************************************************************************/

/*** Local Variables:  ***/
/*** mode: C           ***/
/*** c-basic-offset: 8 ***/
/*** End:              ***/
