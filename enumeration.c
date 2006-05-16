
/*
 * enumeration.c
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
 * CMPIEnumeration implementation.
 *
*/


#include "native.h"


struct native_enum {
   CMPIEnumeration enumeration;
   int refCount;
   int mem_state;

   CMPICount current;
   CMPIArray *data;
};


extern void adjustArrayElementRefCount(CMPIArray * array, int n);
static struct native_enum *__new_enumeration(int, CMPIArray *, CMPIStatus *);


/*****************************************************************************/

static CMPIStatus __eft_release(CMPIEnumeration * enumeration)
{
   struct native_enum *e = (struct native_enum *) enumeration;

   if (e->mem_state && e->mem_state != MEM_RELEASED) {
      e->data->ft->release(e->data);
      memUnlinkEncObj(e->mem_state);
      e->mem_state = MEM_RELEASED;
      free(enumeration);
      CMReturn(CMPI_RC_OK);
   }
   
   CMReturn(CMPI_RC_ERR_FAILED);
}


static CMPIEnumeration *__eft_clone(const CMPIEnumeration * enumeration,
                                    CMPIStatus * rc)
{
   CMPIStatus tmp;
   struct native_enum *e = (struct native_enum *) enumeration;
   CMPIArray *data = CMClone(e->data, &tmp);

   if (tmp.rc != CMPI_RC_OK) {
      if (rc) CMSetStatus(rc, CMPI_RC_ERR_FAILED);
      return NULL;
   }

   return (CMPIEnumeration *) __new_enumeration(MEM_NOT_TRACKED, data, rc);
}


static CMPIData __eft_getNext(const CMPIEnumeration * enumeration, CMPIStatus * rc)
{
   struct native_enum *e = (struct native_enum *) enumeration;
   return CMGetArrayElementAt(e->data, e->current++, rc);
}


static CMPIBoolean __eft_hasNext(const CMPIEnumeration * enumeration, CMPIStatus * rc)
{
   struct native_enum *e = (struct native_enum *) enumeration;
   return (e->current < CMGetArrayCount(e->data, rc));
}


static CMPIArray *__eft_toArray(const CMPIEnumeration * enumeration, CMPIStatus * rc)
{
   struct native_enum *e = (struct native_enum *) enumeration;
	if ( rc ) CMSetStatus ( rc, CMPI_RC_OK );
   return e->data;
}

static CMPIEnumerationFT eft = {
   NATIVE_FT_VERSION,
   __eft_release,
   __eft_clone,
   __eft_getNext,
   __eft_hasNext,
   __eft_toArray
};

static struct native_enum *__new_enumeration(int mm_add,
                                             CMPIArray * array, CMPIStatus * rc)
{
   static CMPIEnumeration e = {
      "CMPIEnumeration",
      &eft
   };

   struct native_enum enm,*tEnm;
   int state;
   
   enm.enumeration = e;
   enm.current=0;
   enm.data=NULL;
   
   tEnm=memAddEncObj(mm_add, &enm, sizeof(enm), &state);
   tEnm->mem_state = state;
   tEnm->refCount=0;
   tEnm->data = //(mm_add == MEM_NOT_TRACKED) ? CMClone(array, rc) : 
      array;
   
   if (rc) CMSetStatus(rc, CMPI_RC_OK);
   return (struct native_enum*)tEnm;

}

void setEnumArray(CMPIEnumeration * enumeration,CMPIArray * array)
{
   struct native_enum *e = (struct native_enum *) enumeration;
   e->data=array; 
}     


CMPIEnumeration *native_new_CMPIEnumeration(CMPIArray * array, CMPIStatus * rc)
{
   return (CMPIEnumeration *) __new_enumeration(MEM_TRACKED, array, rc);
}

CMPIEnumeration *NewCMPIEnumeration(CMPIArray * array, CMPIStatus * rc)
{
   return (CMPIEnumeration *) __new_enumeration(MEM_NOT_TRACKED, array, rc);
}

/****************************************************************************/

/*** Local Variables:  ***/
/*** mode: C           ***/
/*** c-basic-offset: 8 ***/
/*** End:              ***/
