
/*
 * constClass.h
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
 * Internal CMPIConstClass implementation.
 *
*/

#ifndef CMPI_constClass_h
#define CMPI_constClass_h

#include "cmpidt.h"

#define MALLOCED(a) (((a) & 0xff000000)!=0xff000000)

#define FL_assocsOnly 64

struct _CMPIConstClass_FT;
typedef struct _CMPIConstClass_FT CMPIConstClass_FT;

struct _CMPIConstClass {
   void *hdl;
   CMPIConstClass_FT *ft;
};
typedef struct _CMPIConstClass CMPIConstClass;

struct _CMPIConstClass_FT {
   int version;
    CMPIStatus(*release) (CMPIConstClass * cc);
    CMPIConstClass *(*clone) (CMPIConstClass * cc, CMPIStatus * rc);
    void (*relocate) (CMPIConstClass * cc);
    const char *(*getCharClassName) (CMPIConstClass * br);
    const char *(*getCharSuperClassName) (CMPIConstClass * br);
    CMPIBoolean(*isAssociation) (CMPIConstClass * cc);
    CMPIBoolean(*isAbstract) (CMPIConstClass * cc);
    CMPIBoolean(*isIndication) (CMPIConstClass * cc);
    CMPICount(*getPropertyCount) (CMPIConstClass * cc, CMPIStatus * rc);
    CMPIData(*getProperty) (CMPIConstClass * cc, const char *prop,
                            unsigned long *quals, CMPIStatus * rc);
    CMPIData(*getPropertyAt) (CMPIConstClass * cc, CMPICount i,
                              CMPIString ** name, unsigned long *quals,
                              CMPIStatus * rc);
    CMPIData(*getQualifierAt) (CMPIConstClass * cc, CMPICount i,
                               CMPIString ** name, CMPIStatus * rc);
    CMPIData(*getPropQualifierAt) (CMPIConstClass * cc, CMPICount p,
                                   CMPICount i, CMPIString ** name,
                                   CMPIStatus * rc);
/*      CMPIString (*getClassName)(CMPIConstClass* cc);
      CMPIString (*getSuperClassName)(CMPIConstClass* cc);
      CMPICount (*getPropertyCount)(CMPIConstClass* cc);
      CMPIData (*getProperty)(CMPIConstClass* cc, const char* prop, CMPIStatus* rc);
      CMPIData (*getPropertyAt)(CMPIConstClass* cc, CMPICount i, CMPIString **name, CMPIStatus* rc);
      CMPIData (*getQualifier)(CMPIConstClass* cc, const char* prop, CMPIStatus* rc);
      CMPIData (*getPropertyQualifier)(CMPIConstClass* cc, const char* prop, const char* qual, CMPIStatus* rc); */
   CMPIArray *(*getKeyList) (CMPIConstClass * cc);
   char *(*toString) (CMPIConstClass * cc);
};

extern CMPIConstClass_FT *CMPIConstClassFT;

//   extern CMPIConstClass* newCMPIConstClass(const char *cn, const char *pn);

#endif
