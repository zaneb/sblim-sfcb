
/*
 * string.c
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
 *  CMPIString implementation.
 *
*/


#include <string.h>

#include "native.h"

struct native_string {
   CMPIString string;
   int refCount;
   int mem_state;
};


static struct native_string *__new_string(int, const char *, CMPIStatus *);


/*****************************************************************************/

static CMPIStatus __sft_release(CMPIString * string)
{
   struct native_string *s = (struct native_string *) string;

   if (s->mem_state && s->mem_state != MEM_RELEASED) {
      if (s->string.hdl) free(s->string.hdl);
      memUnlinkEncObj(s->mem_state);
      s->mem_state = MEM_RELEASED;
      free(string);
      CMReturn(CMPI_RC_OK);
   }

   CMReturn(CMPI_RC_ERR_FAILED); 
}


static CMPIString *__sft_clone(const CMPIString * string, CMPIStatus * rc)
{
   return (CMPIString *)
       __new_string(MEM_NOT_TRACKED, string->ft->getCharPtr(string, rc), rc);
}


static char *__sft_getCharPtr(const CMPIString * string, CMPIStatus * rc)
{
   return (char *) string->hdl;
}

static CMPIStringFT sft = {
   NATIVE_FT_VERSION,
   __sft_release,
   __sft_clone,
   __sft_getCharPtr
};

static struct native_string *__new_string(int mm_add,
                                          const char *ptr, CMPIStatus * rc)
{
   static CMPIString s = {
      NULL,
      &sft
   };
   struct native_string str,*tStr;
   int state;
   
   str.string = s;
   tStr=memAddEncObj(mm_add, &str, sizeof(str), &state);
   tStr->mem_state = state;   
   tStr->refCount=0;   
   tStr->string.hdl = (ptr) ? strdup(ptr) : NULL;
   
   if (rc) CMSetStatus(rc, CMPI_RC_OK);
   return (struct native_string*)tStr;
}


CMPIString *native_new_CMPIString(const char *ptr, CMPIStatus * rc)
{
   return (CMPIString *) __new_string(MEM_TRACKED, ptr, rc);
}

CMPIString *NewCMPIString(const char *ptr, CMPIStatus * rc)
{
   return (CMPIString *) __new_string(MEM_NOT_TRACKED, ptr, rc);
}


/****************************************************************************/

/*** Local Variables:  ***/
/*** mode: C           ***/
/*** c-basic-offset: 8 ***/
/*** End:              ***/
