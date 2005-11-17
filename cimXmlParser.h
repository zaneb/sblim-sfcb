
/*
 * cimXmlParser.h
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
 * Author:       Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * CIM XML lexer for sfcb to be used in connection with cimXmlOps.y.
 *
*/

#ifndef XMLSCAN_H
#define XMLSCAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include <msgqueue.h>
#include <providerMgr.h>

#define FL_localOnly CMPI_FLAG_LocalOnly
#define FL_deepInheritance CMPI_FLAG_DeepInheritance
#define FL_includeQualifiers CMPI_FLAG_IncludeQualifiers
#define FL_includeClassOrigin CMPI_FLAG_IncludeClassOrigin
#define FL_chunked 32

typedef enum typeValRef {
   typeValRef_InstanceName,
   typeValRef_InstancePath,
   typevalRef_LocalInstancePath
} TypeValRef;

typedef enum typeProperty {
   typeProperty_Missing,
   typeProperty_Value,
   typeProperty_Reference,
   typeProperty_Array
} TypeProperty;

typedef struct xmlBuffer {
   char *base;
   char *last;
   char *cur;
   char eTagFound;
   int etag;
   char nulledChar;
} XmlBuffer;

typedef struct xmlElement {
   char *attr;
} XmlElement;

typedef struct xmlAttr {
   char *attr;
} XmlAttr;


typedef struct requestHdr {
   XmlBuffer *xmlBuffer;
   int rc;
   int opType;
   int simple;
   char *id;
   char *iMethod;
   int methodCall;
   int chunkedMode;
   void *cimRequest;
   unsigned long cimRequestLength;
   char *errMsg;
} RequestHdr;


extern RequestHdr scanCimXmlRequest(char *xmlData);
extern void freeCimXmlRequest(RequestHdr * hdr);



typedef struct xtokNameSpace {
   char *ns;
   char *cns;                   // must be free'd
} XtokNameSpace;

typedef struct xtokMessage {
   char *id;
} XtokMessage;

typedef struct xtokValue {
   char *value;
} XtokValue;

typedef struct xtokValueArray {
   int max,next;
   char **values;
} XtokValueArray;

typedef struct xtokHost {
   char *host;
} XtokHost;

typedef struct xtokNameSpacePath {
    XtokHost host;
    char *nameSpacePath;
} XtokNameSpacePath;



struct xtokKeyBinding;
struct xtokValueReference;

typedef struct xtokKeyValue {
   char *valueType, *value;
} XtokKeyValue;

typedef struct xtokKeyBindings {
   int max, next;
   struct xtokKeyBinding *keyBindings; // must be free'd
} XtokKeyBindings;

typedef struct xtokInstanceName {
   char *className;
   XtokKeyBindings bindings;
} XtokInstanceName;

typedef struct xtokInstancePath {
   XtokNameSpacePath path;
   XtokInstanceName instanceName;
   int type;
} XtokInstancePath;

typedef struct xtokLocalInstancePath {
   char *path;
   XtokInstanceName instanceName;
   int type;
} XtokLocalInstancePath;

typedef struct xtokLocalClassPath {
   char *path;
   char *className;
   int type;
} XtokLocalClassPath;

typedef struct xtokValueReference {
   union {
      XtokInstancePath instancePath;
      XtokLocalInstancePath localInstancePath;
      XtokInstanceName instanceName;
   };   
   TypeValRef type;
} XtokValueReference;

typedef struct xtokKeyBinding {
   char *name, *value, *type;
   XtokValueReference ref;
} XtokKeyBinding;

typedef struct xtokValueRefArray {
   int max,next;
   XtokValueReference *values;
} XtokValueRefArray;




typedef struct xtokQualifier {
   struct xtokQualifier *next;
   char *name;
   CMPIType type;
   char *value;
   char propagated, overridable, tosubclass, toinstance, translatable;
} XtokQualifier;

typedef struct xtokQualifiers {
   XtokQualifier *last, *first; // must be free'd
} XtokQualifiers;


struct xtokPropertyList;

typedef struct xtokPropertyData {
   union {
      char *value;
      XtokValueReference ref;
      struct xtokPropertyList *list;
   };   
   XtokQualifiers qualifiers;
} XtokPropertyData;

typedef struct xtokProperty {
   struct xtokProperty *next;
 //  XtokQualifiers qualifiers;
   char *name;
   char *classOrigin;
   char propagated;
   char *referenceClass;
   CMPIType valueType;
   XtokPropertyData val;
/*   union {
      struct {
         char *value;
      };
      struct {
         XtokValueReference ref;
      };
   }; */      
   TypeProperty propType;
} XtokProperty;

typedef struct xtokProperties {
   XtokProperty *last, *first;  // must be free'd
} XtokProperties;

typedef struct xtokInstance {
   char *className;
   XtokProperties properties;
   XtokQualifiers qualifiers;
} XtokInstance;

typedef struct xtokInstanceData { 
   XtokProperties properties;
   XtokQualifiers qualifiers;
} XtokInstanceData;

typedef struct xtokNamedInstance {
   XtokInstanceName path;
   XtokInstance instance;
} XtokNamedInstance;

typedef struct xtokPropertyList {
   XtokValueArray list;
} XtokPropertyList;


typedef struct xtokParamValue {
  struct xtokParamValue *next;
  char *name;
  CMPIType type;
  union {
     XtokValue value;
     XtokValueReference valueRef;
     XtokValueArray valueArray;
     XtokValueRefArray valueRefArray;
  };
} XtokParamValue;

typedef struct xtokParamValues {
   XtokParamValue *last, *first;  // must be free'd
} XtokParamValues;


typedef struct xtokParam {
   struct xtokParam *next;
   XtokQualifiers qualifiers;
   XtokQualifier qualifier;
   int qPart;
   int pType;
   char *name;
   char *refClass;
   char *arraySize;
   CMPIType type;
} XtokParam;

typedef struct xtokParams {
   XtokParam *last, *first;  // must be free'd
} XtokParams;


typedef struct xtokMethod {
   struct xtokMethod *next;
   XtokQualifiers qualifiers;
   XtokParams params;
   char *name;
   char *classOrigin;
   int propagated;
   CMPIType type;
} XtokMethod;

typedef struct xtokMethodData {
   XtokQualifiers qualifiers;
   XtokParams params;
} XtokMethodData;

typedef struct xtokMethods {
   XtokMethod *last, *first;  // must be free'd
} XtokMethods;


typedef struct xtokClass {
   char *className;
   char *superClass;
   XtokProperties properties;
   XtokQualifiers qualifiers;
   XtokMethods    methods;
} XtokClass;


/*
 *    methodCall
*/

typedef struct xtokMethodCall {
   OperationHdr op;
   int instName;
   XtokInstanceName instanceName;
   char *method;
   XtokParamValues paramValues;
} XtokMethodCall;



/*
 *    execQuery
*/

typedef struct xtokExecQuery {
   OperationHdr op;
   // Query           found in OperationHdr.className
   // QueryLanguage   found in OperationHdr.resultClass
} XtokExecQuery;

/*
 *    getClass
*/

typedef struct xtokGetClassParmsList {
   unsigned int flags;
   unsigned int flagsSet;
   char *className;
   int clsNameSet,properties;
   char **propertyList;
} XtokGetClassParmsList;

typedef struct xtokGetClassParms {
   unsigned int flags;
   unsigned int flagsSet;
   char *className;
   int clsNameSet,properties;
   char **propertyList;
} XtokGetClassParms;

typedef struct xtokGetClass {
   OperationHdr op;
   unsigned int flags;
   int clsNameSet,properties;
   char **propertyList;
} XtokGetClass;


/*
 *    enumClassNames
*/

typedef struct xtokEnumClassNamesParmsList {
   unsigned int flags;
   unsigned int flagsSet;
   char *className;
} XtokEnumClassNamesParmsList;

typedef struct xtokEnumClassNamesParms {
   unsigned int flags;
   unsigned int flagsSet;
   char *className;
} XtokEnumClassNamesParms;

typedef struct xtokEnumClassNames {
   OperationHdr op;
   unsigned int flags;
} XtokEnumClassNames;


/*
 *    enumClasses
*/

typedef struct xtokEnumClassesParmsList {
   unsigned int flags;
   unsigned int flagsSet;
   char *className;
} XtokEnumClassesParmsList;

typedef struct xtokEnumClassesParms {
   unsigned int flags;
   unsigned int flagsSet;
   char *className;
} XtokEnumClassesParms;

typedef struct xtokEnumClasses {
   OperationHdr op;
   unsigned int flags;
} XtokEnumClasses;


/*
 *    getInstance
*/

typedef struct xtokGetInstanceParmsList {
   unsigned int flags;
   unsigned int flagsSet;
   XtokInstanceName instanceName;
   int instNameSet,properties;
   char **propertyList;
} XtokGetInstanceParmsList;

typedef struct xtokGetInstanceParms {
   unsigned int flags;
   unsigned int flagsSet;
   XtokInstanceName instanceName;
   int instNameSet,properties;
   char **propertyList;
} XtokGetInstanceParms;

typedef struct xtokGetInstance {
   OperationHdr op;
   XtokInstanceName instanceName;
   unsigned int flags;
   int instNameSet,properties;
   char **propertyList;
} XtokGetInstance;


/*
 *    createClass
*/

typedef struct xtokCreateClassParm {
   XtokClass cls;
} XtokCreateClassParm;

typedef struct xtokCreateClass {
   OperationHdr op;
   XtokClass cls;
   char *className;
   char *superClass;
} XtokCreateClass;


/*
 *    createInstance
*/

typedef struct xtokCreateInstanceParm {
   XtokInstance instance;
} XtokCreateInstanceParm;

typedef struct xtokCreateInstance {
   OperationHdr op;
   XtokInstance instance;
   char *className;
} XtokCreateInstance;


/*
 *    modifyInstance
*/

typedef struct xtokModifyInstanceParmsList {
   unsigned int flags;
   unsigned int flagsSet;
   XtokNamedInstance namedInstance;
   int namedInstSet,properties;
   char **propertyList;
} XtokModifyInstanceParmsList;

typedef struct xtokModifyInstanceParms {
   unsigned int flags;
   unsigned int flagsSet;
   XtokNamedInstance namedInstance;
   int namedInstSet,properties;
   char **propertyList;
} XtokModifyInstanceParms;

typedef struct xtokModifyInstance {
   OperationHdr op;
   XtokNamedInstance namedInstance;
   unsigned int flags;
   char *className;
   int namedInstSet,properties;
   char **propertyList;
} XtokModifyInstance;


/*
 *    deleteInstance
*/

typedef struct xtokDeleteClassParm {
   char *className;
} XtokDeleteClassParm;

typedef struct xtokDeleteClass {
   OperationHdr op;
   char *className;
} XtokDeleteClass;


/*
 *    deleteInstance
*/

typedef struct xtokDeleteInstanceParm {
   XtokInstanceName instanceName;
} XtokDeleteInstanceParm;

typedef struct xtokDeleteInstance {
   OperationHdr op;
   XtokInstanceName instanceName;
   int instNameSet;
} XtokDeleteInstance;


/*
 *    enumInstanceNames
*/

typedef struct xtokEnumInstanceNames {
   OperationHdr op;
} XtokEnumInstanceNames;


/*
 *    enumInstances
*/

typedef struct xtokEnumInstancesParmsList {
   unsigned int flags;
   unsigned int flagsSet;
   char *className;
   int properties;
   char **propertyList;
} XtokEnumInstancesParmsList;

typedef struct xtokEnumInstancesParms {
   unsigned int flags;
   unsigned int flagsSet;
   char *className;
   int properties;
   char **propertyList;
} XtokEnumInstancesParms;

typedef struct xtokEnumInstances {
   OperationHdr op;
   unsigned int flags;
   int properties;
   char **propertyList;
} XtokEnumInstances;


/*
 *    associatorNames
*/

typedef struct xtokAssociatorNamesParmsList {
   int objNameSet;
   XtokInstanceName objectName;
   char *assocClass, *resultClass, *role, *resultRole;
} XtokAssociatorNamesParmsList;

typedef struct xtokAssociatorNamesParms {
   int objNameSet;
   XtokInstanceName objectName;
   char *assocClass, *resultClass, *role, *resultRole;
} XtokAssociatorNamesParms;

typedef struct xtokAssociatorNames {
   OperationHdr op;
   XtokInstanceName objectName;
   int objNameSet;
} XtokAssociatorNames;


/*
 *    referenceNames
*/

typedef struct xtokReferenceNamesParmsList {
   int objNameSet;
   XtokInstanceName objectName;
   char *resultClass, *role;
} XtokReferenceNamesParmsList;

typedef struct xtokReferenceNamesParms {
   int objNameSet;
   XtokInstanceName objectName;
   char *resultClass, *role;
} XtokReferenceNamesParms;

typedef struct xtokReferenceNames {
   OperationHdr op;
   XtokInstanceName objectName;
   int objNameSet;
} XtokReferenceNames;




/*
 *    associators
*/

typedef struct xtokAssociatorsParmsList {
   unsigned int flags;
   unsigned int flagsSet;
   int objNameSet;
   XtokInstanceName objectName;
   char *assocClass, *resultClass, *role, *resultRole;
   int properties;
   char **propertyList;
} XtokAssociatorsParmsList;

typedef struct xtokAssociatorsParms {
   unsigned int flags;
   unsigned int flagsSet;
   int objNameSet;
   XtokInstanceName objectName;
   char *assocClass, *resultClass, *role, *resultRole;
   int properties;
   char **propertyList;
} XtokAssociatorsParms;

typedef struct xtokAssociators {
   OperationHdr op;
   XtokInstanceName objectName;
   unsigned int flags;
   int objNameSet;
   int properties;
   char **propertyList;
} XtokAssociators;



/*
 *    references
*/

typedef struct xtokReferencesParmsList {
   unsigned int flags;
   unsigned int flagsSet;
   int objNameSet;
   XtokInstanceName objectName;
   char *resultClass, *role;
   int properties;
   char **propertyList;
} XtokReferencesParmsList;

typedef struct xtokReferencesParms {
   unsigned int flags;
   unsigned int flagsSet;
   int objNameSet;
   XtokInstanceName objectName;
   char *resultClass, *role;
   int properties;
   char **propertyList;
} XtokReferencesParms;

typedef struct xtokReferences {
   OperationHdr op;
   XtokInstanceName objectName;
   unsigned int flags;
   int objNameSet;
   int properties;
   char **propertyList;
} XtokReferences;


/*
 *    Parser control
*/

#include <setjmp.h>

typedef struct parser_control {
   XmlBuffer *xmb;
   RequestHdr reqHdr;
   XtokProperties properties;
   XtokQualifiers qualifiers;
   XtokMethods     methods;
   XtokParamValues paramValues;
   int Qs,Ps,Ms,MPs,MQs,MPQs;
   jmp_buf env;
} ParserControl;


#endif
