
/*
 * objectimpl.h
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
 * Author:     Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Internal implementation support for cim objects.
 *
*/

#ifndef CLOBJECTS_H
#define CLOBJECTS_H

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include "native.h"

#define MALLOCED(a) (((a) & 0xff000000)!=0xff000000)

typedef struct {
   char *str;
   int used, max;
} stringControl;

typedef struct {
   //enum sectionType;
   long offset;
   short used, max;
} ClSection;

typedef struct {
   long size;
   //enum hdrType;
   short flags;
#define HDR_Rebuild 1
#define HDR_RebuildStrings 2
#define HDR_ContainsEmbeddedObject 4
   short type;
#define HDR_Class 1
#define HDR_Instance 2
#define HDR_ObjectPath 3
#define HDR_Args 4
   long strBufOffset;
   long arrayBufOffset;
} ClObjectHdr;

typedef struct {
   long id;
} ClString;

typedef struct {
   long id;
} ClArray;

typedef struct {
   ClObjectHdr hdr;
   unsigned long quals;
#define ClClass_Q_Abstract 1
#define ClClass_Q_Association 2
#define ClClass_Q_Indication 4
#define ClClass_Q_Deprecated 8
   ClString name;
   ClString parent;
   ClSection qualifiers;
   ClSection properties;
   ClSection methods;
} ClClass;

typedef struct {
   ClObjectHdr hdr;
   ClString hostName;
   ClString nameSpace;
   ClString className;
   ClSection properties;
} ClObjectPath;

typedef struct {
   ClObjectHdr hdr;
   ClSection properties;
} ClArgs;

typedef struct {
   ClObjectHdr hdr;
   unsigned long quals;
#define ClInst_Q_Association 2
#define ClInst_Q_Indication 4
   ClString className;
   ClString nameSpace;
   ClSection qualifiers;
   ClSection properties;
   ClObjectPath *path;
} ClInstance;

typedef struct {
   short iUsed, iMax;
   long iOffset;
   long *index;
   long bUsed, bMax;
   CMPIData buf[1];
} ClArrayBuf;

typedef struct {
   short iUsed, iMax;
   long iOffset;
   long *index;
   long bUsed, bMax;
   char buf[1];
} ClStrBuf;

typedef struct {
   int type;
   long data;
} ClData;

typedef struct {
   ClString id;
   CMPIData data;
} ClQualifier;

typedef struct {
   ClString id;
   CMPIData data;
   unsigned short flags;
#define ClProperty_EmbeddedObjectAsString 1
   unsigned short quals;
#define ClProperty_Q_Key 1
#define ClProperty_Q_Propagated 2
#define ClProperty_Q_Deprecated 4
#define ClProperty_Q_EmbeddedObject 8
   ClSection qualifiers;
} ClProperty;

const char *ClObjectPathGetNameSpace(ClObjectPath * op);
const char *ClObjectPathGetClassName(ClObjectPath * op);


#endif
