
/*
 * string.c
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
 *  CMPIString implementation.
 *
*/


#include <string.h>

#include "native.h"

struct native_string {
   CMPIString string;
   int mem_state;
};


static struct native_string *__new_string(int, const char *, CMPIStatus *);


/*****************************************************************************/

static CMPIStatus __sft_release(CMPIString * string)
{
   struct native_string *s = (struct native_string *) string;

   if (s->mem_state == TOOL_MM_NO_ADD) {

      tool_mm_add(s);
      tool_mm_add(s->string.hdl);

      CMReturn(CMPI_RC_OK);
   }

   CMReturn(CMPI_RC_ERR_FAILED);
}


static CMPIString *__sft_clone(CMPIString * string, CMPIStatus * rc)
{
   return (CMPIString *)
       __new_string(TOOL_MM_NO_ADD, string->ft->getCharPtr(string, rc), rc);
}


static char *__sft_getCharPtr(CMPIString * string, CMPIStatus * rc)
{
   return (char *) string->hdl;
}


static struct native_string *__new_string(int mm_add,
                                          const char *ptr, CMPIStatus * rc)
{
   static CMPIStringFT sft = {
      NATIVE_FT_VERSION,
      __sft_release,
      __sft_clone,
      __sft_getCharPtr
   };

   struct native_string *string = (struct native_string *)
       tool_mm_alloc(mm_add, sizeof(struct native_string));

   string->string.hdl = (ptr) ? strdup(ptr) : NULL;
   string->string.ft = &sft;
   string->mem_state = mm_add;

   if (mm_add == TOOL_MM_ADD) {
      tool_mm_add(string->string.hdl);
   }

   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return string;
}


CMPIString *native_new_CMPIString(const char *ptr, CMPIStatus * rc)
{
   return (CMPIString *) __new_string(TOOL_MM_ADD, ptr, rc);
}


/****************************************************************************/

/*** Local Variables:  ***/
/*** mode: C           ***/
/*** c-basic-offset: 8 ***/
/*** End:              ***/
