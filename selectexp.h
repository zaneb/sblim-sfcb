
/*
 * selectexp.h
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
 * Author:        Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * CMPISelectExp implementation.
 *
*/


#ifndef SELECTEXP_H
#define SELECTEXP_H 

#include "native.h"
#include "queryOperation.h"

typedef struct native_selectexp NativeSelectExp;

struct native_selectexp {
   CMPISelectExp exp;
   int mem_state;
   NativeSelectExp *next;
   char *queryString;
   char *language;
   char *sns;
   void *filterId;
   QLStatement *qs;
};

#endif
