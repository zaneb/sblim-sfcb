
/*
 * array.c
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
 * CMPIArray implementation.

  * In contrast to a regular array, there exists an additional increase()
  * method, which is only used by the native CMPIResult implementation to
  * grow an array stepwise.
 *
*/


#include <stdlib.h>
#include <string.h>

#include "native.h"
#include "array.h"
#include "objectImpl.h"

extern const char *ClObjectGetClString(ClObjectHdr * hdr, ClString * id);

static struct native_array *__new_empty_array(int, CMPICount,
                                              CMPIType, CMPIStatus *);


/*****************************************************************************/

static void __make_NULL(struct native_array *a, int from, int to, int release)
{
   for (; from <= to; from++) {
      a->data[from].state = CMPI_nullValue;

      if (release) {
         native_release_CMPIValue(a->type, &a->data[from].value);
      }
   }
}


static CMPIStatus __aft_release(CMPIArray * array)
{
   struct native_array *a = (struct native_array *) array;

   if (a->mem_state == MEM_NOT_TRACKED) {

      int i = a->size;

      tool_mm_add(a);
      tool_mm_add(a->data);

      while (i--) {
         if (!(a->data[i].state & CMPI_nullValue)) {
            native_release_CMPIValue(a->type, &a->data[i].value);
         }
      }

      CMReturn(CMPI_RC_OK);
   }

   CMReturn(CMPI_RC_ERR_FAILED);
}


static CMPIArray *__aft_clone(CMPIArray * array, CMPIStatus * rc)
{
   CMPIStatus tmp;
   struct native_array *a = (struct native_array *) array;
   struct native_array *new = __new_empty_array(MEM_NOT_TRACKED,
                                                a->size,
                                                a->type,
                                                &tmp);

   int i = a->size;

   while (i-- && tmp.rc == CMPI_RC_OK) {
      new->data[i].state = a->data[i].state;
      if (!(new->data[i].state & CMPI_nullValue)) {

         new->data[i].value =
             native_clone_CMPIValue(a->type, &a->data[i].value, &tmp);
      }
   }

   if (rc)
      CMSetStatus(rc, tmp.rc);

   return (CMPIArray *) new;
}


static CMPICount __aft_getSize(CMPIArray * array, CMPIStatus * rc)
{
   struct native_array *a = (struct native_array *) array;

   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return a->size;
}


static CMPIType __aft_getSimpleType(CMPIArray * array, CMPIStatus * rc)
{
   struct native_array *a = (struct native_array *) array;

   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return a->type;
}


static CMPIData __aft_getElementAt(CMPIArray * array,
                                   CMPICount index, CMPIStatus * rc)
{
   struct native_array *a = (struct native_array *) array;

   CMPIData result = { a->type, CMPI_badValue, {0} };

   if (index < a->size) {

      result.state = a->data[index].state;
      result.value = a->data[index].value;
   }

   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return result;
}

/*
static CMPIStatus __aft_setElementAt(CMPIArray * array,
                                     CMPICount index,
                                     CMPIValue * val, CMPIType type)
{
   struct native_array *a = (struct native_array *) array;

   if ( a->dynamic && index==a->size ) {
      native_array_increase_size(array, 1); 
   }
      
   if (index < a->size) {
      CMPIValue v;

      if (type == CMPI_chars && a->type == CMPI_string) {

         v.string = native_new_CMPIString((char *) val, NULL);
         type = CMPI_string;
         val = &v;
      }

      if (type == a->type) {

         CMPIStatus rc = { CMPI_RC_OK, NULL };

         a->data[index].state = 0;
         a->data[index].value =
             (a->mem_state == MEM_TRACKED) ?
             *val : native_clone_CMPIValue(type, val, &rc);

         return rc;
      }

      if (type == CMPI_null) {

         if (!(a->data[index].state & CMPI_nullValue)) {

            __make_NULL(a, index, index, a->mem_state == MEM_NOT_TRACKED);
         }

         CMReturn(CMPI_RC_OK);
      }
   }
   
   CMReturn(CMPI_RC_ERR_FAILED);
}
*/
static CMPIStatus setElementAt ( CMPIArray * array, CMPICount index, CMPIValue * val,
       CMPIType type, int opt )
{
   struct native_array * a = (struct native_array *) array;

   if ( a->dynamic && index==a->size ) {
      native_array_increase_size(array, 1); 
   }
      
   if ( index < a->size ) {
      CMPIValue v;

      if ( type == CMPI_chars && a->type == CMPI_string ) {
         v.string = native_new_CMPIString ( (char *) val, NULL );
         type = CMPI_string;
         val  = &v;
      }

      if ( opt || type == a->type ) {
         CMPIStatus rc = {CMPI_RC_OK, NULL};
         a->data[index].state = 0;
         if (opt) a->data[index].value = *val;
         else a->data[index].value =  ( a->mem_state == MEM_TRACKED )?  *val:
               native_clone_CMPIValue ( type, val, &rc );
         return rc;
      }

      if ( type == CMPI_null ) {
         if ( ! ( a->data[index].state & CMPI_nullValue ) ) {
            __make_NULL ( a, index, index, a->mem_state == MEM_NOT_TRACKED );
         }
         CMReturn ( CMPI_RC_OK );
      }
   }
   CMReturn ( CMPI_RC_ERR_FAILED );
}

static CMPIStatus __aft_setElementAt ( CMPIArray * array, CMPICount index, CMPIValue * val,
          CMPIType type )
{ 
   return setElementAt(array,index,val,type,0);
}

CMPIStatus arraySetElementNotTrackedAt(CMPIArray * array,
                                     CMPICount index,
                                     CMPIValue * val, CMPIType type)
{
   struct native_array *a = (struct native_array *) array;

   if (index < a->size) {
      CMPIValue v;

      if (type == CMPI_chars && a->type == CMPI_string) {
         v.string = native_new_CMPIString((char *) val, NULL);
         type = CMPI_string;
         val = &v;
      }

      if (type == a->type) {
         CMPIStatus rc = { CMPI_RC_OK, NULL };
         a->data[index].state = 0;
         a->data[index].value =*val;
         return rc;
      }

      if (type == CMPI_null) {
         if (!(a->data[index].state & CMPI_nullValue)) {
            __make_NULL(a, index, index, a->mem_state == MEM_NOT_TRACKED);
         }
         CMReturn(CMPI_RC_OK);
      }
   }

   CMReturn(CMPI_RC_ERR_FAILED);
}



void native_array_increase_size(CMPIArray * array, CMPICount increment)
{
   struct native_array *a = (struct native_array *) array;

   if ((a->size+increment)>a->max) {
      if (a->size==0) a->max=8;
      else while ((a->size+increment)>a->max) a->max*=2;
      a->data = (struct native_array_item *)
         tool_mm_realloc(a->data, a->max * sizeof(struct native_array_item));
      memset(&a->data[a->size], 0, sizeof(struct native_array_item) * increment);
   }
   a->size += increment;
}


void native_array_reset_size(CMPIArray * array, CMPICount increment)
{
   struct native_array *a = (struct native_array *) array;
   a->size = increment;
}


static struct native_array *__new_empty_array(int mm_add,
                                              CMPICount size,
                                              CMPIType type, CMPIStatus * rc)
{
   static CMPIArrayFT aft = {
      NATIVE_FT_VERSION,
      __aft_release,
      __aft_clone,
      __aft_getSize,
      __aft_getSimpleType,
      __aft_getElementAt,
      __aft_setElementAt
   };
   static CMPIArray a = {
      "CMPIArray",
      &aft
   };

   struct native_array *array = (struct native_array *)
       tool_mm_alloc(mm_add, sizeof(struct native_array));

   array->array = a;
   array->mem_state = mm_add;

   type        &= ~CMPI_ARRAY;
   array->type  = ( type == CMPI_chars )? CMPI_string: type;
   array->size  = size;
 
   if (array->size==0) {
      array->max=8;
      array->dynamic=1;
   }
   else {
      array->max=array->size;
      array->dynamic=0;
   }    
     
   array->data = (struct native_array_item *)
      tool_mm_alloc ( mm_add, array->max * sizeof ( struct native_array_item ) );

   __make_NULL(array, 0, size - 1, 0);

   if (rc)
      CMSetStatus(rc, CMPI_RC_OK);
   return array;
}


CMPIArray *TrackedCMPIArray(CMPICount size, CMPIType type, CMPIStatus * rc)
{
   void *array = internal_new_CMPIArray(MEM_TRACKED, size, type, rc);
   return (CMPIArray *) array;
}

CMPIArray *internal_new_CMPIArray(int mode, CMPICount size, CMPIType type,
                                  CMPIStatus * rc)
{
   void *array = __new_empty_array(mode, size, type, rc);
   return (CMPIArray *) array;
}

CMPIArray *NewCMPIArray(CMPICount size, CMPIType type, CMPIStatus * rc)
{
   void *array = internal_new_CMPIArray(MEM_NOT_TRACKED, size, type, rc);
   return (CMPIArray *) array;
}

CMPIArray *native_make_CMPIArray(CMPIData * av, CMPIStatus * rc,
                                 ClObjectHdr * hdr)
{
   void *array =
       __new_empty_array(MEM_NOT_TRACKED, av->value.sint32, av->type, rc);
   int i, m;

   for (i = 0, m = (int) av->value.sint32; i < m; i++)
      if (av[i + 1].type == CMPI_string) {
         char *chars = (char *) ClObjectGetClString(hdr, (ClString *) & av[i + 1].value.chars);
         __aft_setElementAt((CMPIArray *) array, i, (CMPIValue *) chars, CMPI_chars);
      }
      else __aft_setElementAt((CMPIArray *) array, i, &av[i + 1].value,
         av[i + 1].type);
                       
   return (CMPIArray *) array;
}

CMPIStatus simpleArrayAdd(CMPIArray * array, CMPIValue * val, CMPIType type)
{
   struct native_array * a = (struct native_array *) array;
   if (a->dynamic) {
      if (a->size==0) {
         a->type=type;
         if (a->type==CMPI_chars) a->type=CMPI_string;
      }   
      return setElementAt(array,a->size,val,type,1);
   }   
   CMReturn ( CMPI_RC_ERR_FAILED );
} 


/****************************************************************************/

/*** Local Variables:  ***/
/*** mode: C           ***/
/*** c-basic-offset: 8 ***/
/*** End:              ***/
