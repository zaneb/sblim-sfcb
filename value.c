
/*
 * value.c
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
 * CMPIValue implementation.
 *
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native.h"
#include "constClass.h"

void native_release_CMPIValue(CMPIType type, CMPIValue * val)
{
   switch (type) {

   case CMPI_instance:
      CMRelease(val->inst);
      break;

   case CMPI_class:
      CMRelease(val->inst);
      break;

   case CMPI_ref:
      CMRelease(val->ref);
      break;

   case CMPI_args:
      CMRelease(val->args);
      break;

   case CMPI_filter:
      CMRelease(val->filter);
      break;

   case CMPI_enumeration:
      CMRelease(val->Enum);
      break;

   case CMPI_string:
      CMRelease(val->string);
      break;

   case CMPI_chars:
      free(val->chars);
      break;

   case CMPI_dateTime:
      CMRelease(val->dateTime);
      break;

   default:
      if (type & CMPI_ARRAY) {
         CMRelease(val->array);
      }
   }
}


CMPIValue native_clone_CMPIValue(const CMPIType type,
                                 const CMPIValue * val, CMPIStatus * rc)
{
   CMPIValue v;
   CMPIConstClass *cl;

   if (type & CMPI_ENC) {

      switch (type) {

      case CMPI_instance:
         v.inst = CMClone(val->inst, rc);
         break;

      case CMPI_class:
         cl=(CMPIConstClass*)val->inst;
         v.inst = (CMPIInstance*)CMClone(cl, rc);
         break;

      case CMPI_ref:
         v.ref = CMClone(val->ref, rc);
         break;

      case CMPI_args:
         v.args = CMClone(val->args, rc);
         break;

      case CMPI_filter:
         v.filter = CMClone(val->filter, rc);
         break;

      case CMPI_enumeration:
         v.Enum = CMClone(val->Enum, rc);
         break;

      case CMPI_string:
         v.string = CMClone(val->string, rc);
         break;

      case CMPI_chars:
         v.chars = strdup(val->chars);
         CMSetStatus(rc, CMPI_RC_OK);
         break;

      case CMPI_dateTime:
         v.dateTime = CMClone(val->dateTime, rc);
         break;
      }

   }
   else if (type & CMPI_ARRAY) {

      v.array = CMClone(val->array, rc);
   }
   else {

      v = *val;
      CMSetStatus(rc, CMPI_RC_OK);
   }

   return v;
}

extern CMPIString *__oft_toString(CMPIObjectPath * cop, CMPIStatus * rc);

char *value2Chars(CMPIType type, CMPIValue * value)
{
   char str[256], *p;
   unsigned int size;
   CMPIString *cStr;

   if (type & CMPI_ARRAY) {

   }
   else if (type & CMPI_ENC) {

      switch (type) {
      case CMPI_instance:
         break;

      case CMPI_ref:
         cStr = __oft_toString(value->ref, NULL);
         return strdup((char *) cStr->hdl);
         break;

      case CMPI_args:
         break;

      case CMPI_filter:
         break;

      case CMPI_string:
      case CMPI_numericString:
      case CMPI_booleanString:
      case CMPI_dateTimeString:
      case CMPI_classNameString:
         size = strlen((char *) value->string->hdl);
         p = malloc(size + 8);
         sprintf(p, "\"%s\"", (char *) value->string->hdl);
         return p;
	 break;

      case CMPI_dateTime:
         cStr=CMGetStringFormat(value->dateTime,NULL);
         size = strlen((char *) cStr->hdl);
         p = malloc(size + 8);
         sprintf(p, "\"%s\"", (char *) cStr->hdl);
         return p;
         break;
      }

   }
   else if (type & CMPI_SIMPLE) {

      switch (type) {
      case CMPI_boolean:
         return strdup(value->boolean ? "true" : "false");

      case CMPI_char16:
         break;
      }

   }
   else if (type & CMPI_INTEGER) {

      switch (type) {
      case CMPI_uint8:
         sprintf(str, "%u", value->uint8);
         return strdup(str);
      case CMPI_sint8:
         sprintf(str, "%d", value->sint8);
         return strdup(str);
      case CMPI_uint16:
         sprintf(str, "%u", value->uint16);
         return strdup(str);
      case CMPI_sint16:
         sprintf(str, "%d", value->sint16);
         return strdup(str);
      case CMPI_uint32:
         sprintf(str, "%u", value->uint32);
         return strdup(str);
      case CMPI_sint32:
         sprintf(str, "%d", value->sint32);
         return strdup(str);
      case CMPI_uint64:
         sprintf(str, "%llu", value->uint64);
         return strdup(str);
      case CMPI_sint64:
         sprintf(str, "%lld", value->sint64);
         return strdup(str);
      }

   }
   else if (type & CMPI_REAL) {

      switch (type) {
      case CMPI_real32:
         sprintf(str, "%g", value->real32);
         return strdup(str);
      case CMPI_real64:
         sprintf(str, "%g", value->real64);
         return strdup(str);
      }

   }
   return strdup(str);
}

