
/*
 * objectimpl.h
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

#define ClCurrentVersion 1
#define ClCurrentLevel 0
#define ClTypeClassRep 1

typedef struct {
   unsigned long size;    // used to determine endianes
   unsigned short type;
   char id[10];           // "sfcb-rep\0" used to determine asci/ebcdic char encoding  
   unsigned short version;
   unsigned short level;
   unsigned short options;
   unsigned short reserved[5];
} ClVersionRecord;   

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
#define HDR_Version 128
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
   unsigned char quals;
#define ClClass_Q_Abstract 1
#define ClClass_Q_Association 2
#define ClClass_Q_Indication 4
#define ClClass_Q_Deprecated 8
   unsigned char parents;
   unsigned short reserved;
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
   unsigned char quals;
#define ClInst_Q_Association 2
#define ClInst_Q_Indication 4
   unsigned char parents;
   unsigned short reserved;
   ClString className;
   ClString nameSpace;
   ClSection qualifiers;
   ClSection properties;
   ClObjectPath *path;
} ClInstance;

typedef struct {
   short iUsed, iMax;
   long iOffset;
   long *aIndex;
   long bUsed, bMax;
   CMPIData buf[1];
} ClArrayBuf;

typedef struct {
   short iUsed, iMax;
   long iOffset;
   long *sIndex;
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
#define ClProperty_Deleted 2
   unsigned char quals;
#define ClProperty_Q_Key 1
#define ClProperty_Q_Propagated 2
#define ClProperty_Q_Deprecated 4
#define ClProperty_Q_EmbeddedObject 8
   unsigned char originId;
   ClSection qualifiers;
} ClProperty;

typedef struct {
   ClString id;
   CMPIType type;
   unsigned short flags;
   unsigned char quals;
   unsigned char originId;
   ClSection qualifiers;
   ClSection parameters;
} ClMethod;

typedef struct {
   CMPIType type;
   unsigned int arraySize;
   char *refName;  
} CMPIParameter;

typedef struct {
   ClString id;
   CMPIParameter parameter;
   unsigned short quals;
   ClSection qualifiers;
} ClParameter;

/* objectImpl.c */

extern const char *ClObjectGetClString(ClObjectHdr *hdr, ClString *id);
extern const CMPIData *ClObjectGetClArray(ClObjectHdr *hdr, ClArray *id);
extern void *ClObjectGetClSection(ClObjectHdr *hdr, ClSection *s);
extern int ClClassAddQualifier(ClObjectHdr *hdr, ClSection *qlfs, const char *id, CMPIData d);
extern int ClClassAddPropertyQualifier(ClObjectHdr *hdr, ClProperty *p, const char *id, CMPIData d);
extern int ClClassAddMethodQualifier(ClObjectHdr *hdr, ClMethod *m, const char *id, CMPIData d);
extern int ClClassAddMethParamQualifier(ClObjectHdr *hdr, ClParameter *p,const char *id, CMPIData d);
extern int ClClassGetQualifierAt(ClClass *cls, int id, CMPIData *data, char **name);
extern int ClClassGetQualifierCount(ClClass *cls);
extern int ClClassGetMethParameterCount(ClClass * cls, int id);
extern int ClClassAddMethParameter(ClObjectHdr *hdr, ClMethod *m, const char *id, CMPIParameter cp);
extern int ClClassLocateMethod(ClObjectHdr *hdr, ClSection *mths, const char *id);
extern int ClClassGetMethQualifierCount(ClClass * cls, int id);
extern int ClClassGetMethParamQualifierCount(ClClass * cls, ClParameter *p);
extern int ClObjectLocateProperty(ClObjectHdr *hdr, ClSection *prps, const char *id);
extern void showClHdr(void *ihdr);
extern unsigned char ClClassAddGrandParent(ClClass *cls, char *gp);
extern ClClass *ClClassNew(const char *cn, const char *pa);
extern unsigned long ClSizeClass(ClClass *cls);
extern ClClass *ClClassRebuildClass(ClClass *cls, void *area);
extern void ClClassRelocateClass(ClClass *cls);
extern void ClClassFreeClass(ClClass *cls);
extern char *ClClassToString(ClClass *cls);
extern int ClClassAddProperty(ClClass *cls, const char *id, CMPIData d);
extern int ClClassGetPropertyCount(ClClass *cls);
extern int ClClassGetPropertyAt(ClClass *cls, int id, CMPIData *data, char **name, unsigned long *quals);
extern int ClClassGetPropQualifierCount(ClClass *cls, int id);
extern int ClClassGetPropQualifierAt(ClClass *cls, int id, int qid, CMPIData *data, char **name);
extern int ClClassAddMethod(ClClass *cls, const char *id, CMPIType t);
extern int ClClassGetMethodCount(ClClass *cls);
extern int ClClassGetMethodAt(ClClass *cls, int id, CMPIType *data, char **name, unsigned long *quals);
extern int ClClassGetMethQualifierAt(ClClass *cls, ClMethod *m, int qid, CMPIData *data, char **name);
extern int ClClassGetMethParameterAt(ClClass *cls, ClMethod *m, int pid, CMPIParameter *parm, char **name);
extern int ClClassGetMethParamQualifierAt(ClClass * cls, ClParameter *parm, int id, CMPIData *d, char **name);
extern int isInstance(const CMPIInstance *ci);
extern ClInstance *ClInstanceNew(const char *ns, const char *cn);
extern unsigned long ClSizeInstance(ClInstance *inst);
extern ClInstance *ClInstanceRebuild(ClInstance *inst, void *area);
extern void ClInstanceRelocateInstance(ClInstance *inst);
extern void ClInstanceFree(ClInstance *inst);
extern char *ClInstanceToString(ClInstance *inst);
extern int ClInstanceGetPropertyCount(ClInstance *inst);
extern int ClInstanceGetPropertyAt(ClInstance *inst, int id, CMPIData *data, char **name, unsigned long *quals);
extern int ClInstanceAddProperty(ClInstance *inst, const char *id, CMPIData d);
extern const char *ClInstanceGetClassName(ClInstance *inst);
extern const char *ClInstanceGetNameSpace(ClInstance *inst);
extern const char *ClGetStringData(CMPIInstance *ci, int id);
extern ClObjectPath *ClObjectPathNew(const char *ns, const char *cn);
extern unsigned long ClSizeObjectPath(ClObjectPath *op);
extern ClObjectPath *ClObjectPathRebuild(ClObjectPath *op, void *area);
extern void ClObjectPathRelocateObjectPath(ClObjectPath *op);
extern void ClObjectPathFree(ClObjectPath *op);
extern char *ClObjectPathToString(ClObjectPath *op);
extern int ClObjectPathGetKeyCount(ClObjectPath *op);
extern int ClObjectPathGetKeyAt(ClObjectPath *op, int id, CMPIData *data, char **name);
extern int ClObjectPathAddKey(ClObjectPath *op, const char *id, CMPIData d);
extern void ClObjectPathSetHostName(ClObjectPath *op, const char *hn);
extern const char *ClObjectPathGetHostName(ClObjectPath *op);
extern void ClObjectPathSetNameSpace(ClObjectPath *op, const char *ns);
extern const char *ClObjectPathGetNameSpace(ClObjectPath *op);
extern void ClObjectPathSetClassName(ClObjectPath *op, const char *cn);
extern const char *ClObjectPathGetClassName(ClObjectPath *op);
extern ClArgs *ClArgsNew(void);
extern unsigned long ClSizeArgs(ClArgs *arg);
extern ClArgs *ClArgsRebuild(ClArgs *arg, void *area);
extern void ClArgsRelocateArgs(ClArgs *arg);
extern void ClArgsFree(ClArgs *arg);
extern int ClArgsGetArgCount(ClArgs *arg);
extern int ClArgsGetArgAt(ClArgs *arg, int id, CMPIData *data, char **name);
extern int ClArgsAddArg(ClArgs *arg, const char *id, CMPIData d);

#endif
