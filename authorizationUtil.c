
/*
 * authorizationUtil.c
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
 * Author:       Gareth Bestor 
 * Contributors: Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Utility functions for Authorization provider for sfcb
 *
*/


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* Include required CMPI library headers */
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"
#include "trace.h"


/* Compare two CIM data values to see if they are identical */
int _CMSameValue( CMPIData value1, CMPIData value2 )
{
   _SFCB_ENTER(TRACE_PROVIDERS, "_CMSameValue");
   
   CMPIValue v1 = value1.value;
   CMPIValue v2 = value2.value;

   /* Check that the type of the two CIM values is the same */
   if (value1.type != value2.type) return 0;

   /* Check that the value of the two CIM values is the same */
   switch (value1.type) {
      case CMPI_string:   return !strcmp(CMGetCharPtr(v1.string), CMGetCharPtr(v2.string));
      case CMPI_dateTime: return CMGetBinaryFormat(v1.dateTime, NULL) == CMGetBinaryFormat(v2.dateTime, NULL);
      case CMPI_boolean:  return v1.boolean == v2.boolean;
      case CMPI_char16:   return v1.char16 == v2.char16;
      case CMPI_uint8:    return v1.uint8 == v2.uint8;
      case CMPI_sint8:    return v1.sint8 == v2.sint8;
      case CMPI_uint16:   return v1.uint16 == v2.uint16;
      case CMPI_sint16:   return v1.sint16 == v2.sint16;
      case CMPI_uint32:   return v1.uint32 == v2.uint32;
      case CMPI_sint32:   return v1.sint32 == v2.sint32;
      case CMPI_uint64:   return v1.uint64 == v2.uint64;
      case CMPI_sint64:   return v1.sint64 == v2.sint64;
      case CMPI_real32:   return v1.real32 == v2.real32;
      case CMPI_real64:   return v1.real64 == v2.real64;
      default: _SFCB_TRACE(1,("_CMSameValue() : Unrecognized type CIM type=%d",value1.type)); 
         return 0;
   }
}


/* Compare two CIM object paths to see if they refer to the same object */
int _CMSameObject( CMPIObjectPath * object1, CMPIObjectPath * object2 )
{
   CMPIData key1, key2;
   CMPIString * keyname = NULL;
   int numkeys1, numkeys2, i;

   /* Check that the two objects have the same number of keys */
   numkeys1 = CMGetKeyCount(object1, NULL);
   numkeys2 = CMGetKeyCount(object2, NULL);
   if (numkeys1 != numkeys2) return 0;

   /* Go through the list of keys for the first object */
   for (i=0; i<numkeys1; i++) {
      /* Retrieve the key from both objects */
      key1 = CMGetKeyAt(object1, i, &keyname, NULL);
      key2 = CMGetKey(object2, CMGetCharPtr(keyname), NULL);

      /* Check that both keys exist and have been set */
      if (CMIsNullValue(key1) || CMIsNullValue(key2)) return 0;

      /* Check that the type of the two keys is the same */
      if (key1.type != key2.type) return 0;

      /* Check that the value of the two keys is the same */
      if (!_CMSameValue(key1,key2)) return 0;
   }

   /* If we get here then all the keys must have matched */
   return 1;
}

