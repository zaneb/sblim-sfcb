
/*
 * array.h
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
 * CMPIArray implementation.
 *
*/


#ifndef array_h
#define array_h

struct native_array_item {
   CMPIValueState state;
   CMPIValue value;
};


struct native_array {
   CMPIArray array;
   int mem_state;

   CMPICount size,max;
   CMPIType type;
   struct native_array_item *data;
};

#endif
