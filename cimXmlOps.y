%{

/*
 * cimXmlOps.y
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
 * CMPI broker encapsulated functionality.
 *
 * CIM XML grammar for sfcb.
 *
*/


/*
**==============================================================================
**
** Includes
**
**==============================================================================
*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "cimXmlParser.h"


//
// Define the global parser state object:
//

#define YYPARSE_PARAM parm
#define YYLEX_PARAM parm
#define YYERROR_VERBOSE 1

extern int yyerror(char*);
extern int yylex (void *lvalp, ParserControl *parm);


int isBoolean(CMPIData data)
{
   if (data.type==CMPI_chars) {
      if (strcasecmp(data.value.chars,"true")==0) return 0xffff;
      if (strcasecmp(data.value.chars,"false")==0) return 0;
   }
   return -1;
}

static void setRequest(void *parm, void *req, unsigned long size, int type)
{
   ((ParserControl*)parm)->reqHdr.cimRequestLength=size;
   ((ParserControl*)parm)->reqHdr.cimRequest=malloc(size);
   memcpy(((ParserControl*)parm)->reqHdr.cimRequest,req,size);
   ((ParserControl*)parm)->reqHdr.opType = type;
}

static void addProperty(XtokProperties *ps, XtokProperty *p)
{
   XtokProperty *np;
   np=(XtokProperty*)malloc(sizeof(XtokProperty));
   memcpy(np,p,sizeof(XtokProperty));
   np->next=NULL;
   if (ps->last) {
      ps->last->next=np;
   }
   else ps->first=np;
   ps->last=np;
}

static void addParamValue(XtokParamValues *vs, XtokParamValue *v)
{
   XtokParamValue *nv;
   nv=(XtokParamValue*)malloc(sizeof(XtokParamValue));
   memcpy(nv,v,sizeof(XtokParamValue));
   nv->next=NULL;
   if (vs->last) {
      vs->last->next=nv;
   }
   else vs->first=nv;
   vs->last=nv;
}

static void addQualifier(XtokQualifiers *qs, XtokQualifier *q)
{
   XtokQualifier *nq;
   nq=(XtokQualifier*)malloc(sizeof(XtokQualifier));
   memcpy(nq,q,sizeof(XtokQualifier));
   nq->next=NULL;
   if (qs->last) {
      qs->last->next=nq;
   }
   else qs->first=nq;
   qs->last=nq;
}
%}

%pure_parser

/*
**==============================================================================
**
** Union used to pass tokens from Lexer to this Parser.
**
**==============================================================================
*/

%union
{
   int                           intValue;
   char                          boolValue;
   char*                         className;
   void*                         tokCim;

   XtokMessage                   xtokMessage;
   XtokNameSpace                 xtokNameSpace;
   char*                         xtokLocalNameSpacePath;
   XtokNameSpacePath             xtokNameSpacePath;
   XtokHost                      xtokHost;
   XtokInstancePath              xtokInstancePath;
   XtokLocalInstancePath         xtokLocalInstancePath;
   XtokLocalClassPath            xtokLocalClassPath;

   XtokValue                     xtokValue;
   XtokValueArray                xtokValueArray;
   XtokValueReference            xtokValueReference;
   XtokPropertyList              xtokPropertyList;

   XtokInstanceName              xtokInstanceName;
   XtokKeyBinding                xtokKeyBinding;
   XtokKeyBindings               xtokKeyBindings;
   XtokKeyValue                  xtokKeyValue;

   XtokInstance                  xtokInstance;
   XtokNamedInstance             xtokNamedInstance;

   XtokProperty                  xtokProperty;
   XtokPropertyPart              xtokPropertyPart;
   XtokPropertyPartList          xtokPropertyPartList;

   XtokQualifier                 xtokQualifier;
   
   XtokParamValue                xtokParamValue;
  
   XtokMethodCall                xtokMethodCall;

   XtokGetClassParmsList         xtokGetClassParmsList;
   XtokGetClassParms             xtokGetClassParms;
   XtokGetClass                  xtokGetClass;

   XtokEnumClassNames            xtokEnumClassNames;
   XtokEnumClassNamesParmsList   xtokEnumClassNamesParmsList;
   XtokEnumClassNamesParms       xtokEnumClassNamesParms;

   XtokEnumClasses               xtokEnumClasses;
   XtokEnumClassesParmsList      xtokEnumClassesParmsList;
   XtokEnumClassesParms          xtokEnumClassesParms;

   XtokGetInstance               xtokGetInstance;
   XtokGetInstanceParmsList      xtokGetInstanceParmsList;
   XtokGetInstanceParms          xtokGetInstanceParms;

   XtokDeleteClass               xtokDeleteClass;
   XtokDeleteClassParm           xtokDeleteClassParm;

   XtokDeleteInstance            xtokDeleteInstance;
   XtokDeleteInstanceParm        xtokDeleteInstanceParm;

   XtokCreateInstance            xtokCreateInstance;
   XtokCreateInstanceParm        xtokCreateInstanceParm;

   XtokModifyInstance            xtokModifyInstance;
   XtokModifyInstanceParmsList   xtokModifyInstanceParmsList;
   XtokModifyInstanceParms       xtokModifyInstanceParms;

   XtokEnumInstanceNames         xtokEnumInstanceNames;

   XtokEnumInstances             xtokEnumInstances;
   XtokEnumInstancesParmsList	 xtokEnumInstancesParmsList;
   XtokEnumInstancesParms        xtokEnumInstancesParms;

   XtokExecQuery                 xtokExecQuery;
   
   XtokAssociators               xtokAssociators;
   XtokAssociatorsParmsList      xtokAssociatorsParmsList;
   XtokAssociatorsParms          xtokAssociatorsParms;

   XtokReferences                xtokReferences;
   XtokReferencesParmsList       xtokReferencesParmsList;
   XtokReferencesParms           xtokReferencesParms;

   XtokAssociatorNames           xtokAssociatorNames;
   XtokAssociatorNamesParmsList  xtokAssociatorNamesParmsList;
   XtokAssociatorNamesParms      xtokAssociatorNamesParms;

   XtokReferenceNames            xtokReferenceNames;
   XtokReferenceNamesParmsList   xtokReferenceNamesParmsList;
   XtokReferenceNamesParms       xtokReferenceNamesParms;
};

%token <tokCim>                  XTOK_XML
%token <intValue>                ZTOK_XML

%token <tokCim>                  XTOK_CIM
%token <intValue>                ZTOK_CIM

%token <xtokMessage>             XTOK_MESSAGE
%token <intValue>                ZTOK_MESSAGE
%type  <xtokMessage>             message

%token <intValue>                XTOK_SIMPLEREQ
%token <intValue>                ZTOK_SIMPLEREQ

%token <xtokGetClass>            XTOK_GETCLASS
%type  <xtokGetClass>            getClass
%type  <xtokGetClassParmsList>   getClassParmsList
%type  <xtokGetClassParms>       getClassParms

%token <xtokEnumClassNames>      XTOK_ENUMCLASSNAMES
%type  <xtokEnumClassNames>      enumClassNames
%type  <xtokEnumClassNamesParmsList> enumClassNamesParmsList
%type  <xtokEnumClassNamesParms> enumClassNamesParms

%token <xtokEnumClasses>         XTOK_ENUMCLASSES
%type  <xtokEnumClasses>         enumClasses
%type  <xtokEnumClassesParmsList> enumClassesParmsList
%type  <xtokEnumClassesParms>    enumClassesParms

%token <xtokCreateInstance>      XTOK_CREATEINSTANCE
%type  <xtokCreateInstance>      createInstance
%type  <xtokCreateInstanceParm>  createInstanceParm

%token <xtokDeleteClass>         XTOK_DELETECLASS
%type  <xtokDeleteClass>         deleteClass
%type  <xtokDeleteClassParm>     deleteClassParm

%token <xtokDeleteInstance>      XTOK_DELETEINSTANCE
%type  <xtokDeleteInstance>      deleteInstance
%type  <xtokDeleteInstanceParm>  deleteInstanceParm

%token <xtokModifyInstance>      XTOK_MODIFYINSTANCE
%type  <xtokModifyInstance>      modifyInstance
%type  <xtokModifyInstanceParmsList> modifyInstanceParmsList
%type  <xtokModifyInstanceParms>  modifyInstanceParms

%token <xtokGetInstance>         XTOK_GETINSTANCE
%type  <xtokGetInstance>         getInstance
%type  <xtokGetInstanceParmsList> getInstanceParmsList
%type  <xtokGetInstanceParms>    getInstanceParms

%token <xtokEnumInstanceNames>   XTOK_ENUMINSTANCENAMES
%type  <xtokEnumInstanceNames>   enumInstanceNames

%token <xtokEnumInstances>       XTOK_ENUMINSTANCES
%type  <xtokEnumInstances>       enumInstances
%type  <xtokEnumInstancesParmsList> enumInstancesParmsList
%type  <xtokEnumInstancesParms>  enumInstancesParms

%token <xtokExecQuery>           XTOK_EXECQUERY
%type  <xtokExecQuery>           execQuery

%token <xtokAssociators>         XTOK_ASSOCIATORS
%type  <xtokAssociators>         associators
%type  <xtokAssociatorsParmsList> associatorsParmsList
%type  <xtokAssociatorsParms>    associatorsParms

%token <xtokReferences>          XTOK_REFERENCES
%type  <xtokReferences>          references
%type  <xtokReferencesParmsList> referencesParmsList
%type  <xtokReferencesParms>     referencesParms

%token <xtokAssociatorNames>     XTOK_ASSOCIATORNAMES
%type  <xtokAssociatorNames>     associatorNames
%type  <xtokAssociatorNamesParmsList> associatorNamesParmsList
%type  <xtokAssociatorNamesParms> associatorNamesParms

%token <xtokReferenceNames>      XTOK_REFERENCENAMES
%type  <xtokReferenceNames>      referenceNames
%type  <xtokReferenceNamesParmsList> referenceNamesParmsList
%type  <xtokReferenceNamesParms> referenceNamesParms

%token <intValue>                ZTOK_IMETHODCALL

%token <intValue>                XTOK_METHODCALL
%token <intValue>                ZTOK_METHODCALL
%type  <xtokMethodCall>          methodCall

%type <tokCim>                   cimOperation

%token <xtokNameSpacePath>       XTOK_NAMESPACEPATH
%token <intValue>                ZTOK_NAMESPACEPATH
%type  <xtokNameSpacePath>       nameSpacePath

%token <xtokLocalNameSpacePath>  XTOK_LOCALNAMESPACEPATH
%token <intValue>                ZTOK_LOCALNAMESPACEPATH
%type  <xtokLocalNameSpacePath>  localNameSpacePath

%token <xtokNameSpace>           XTOK_NAMESPACE
%token <intValue>                ZTOK_NAMESPACE
%type  <xtokNameSpace>           namespaces

%token <intValue>                ZTOK_IPARAMVALUE

%token <xtokHost>                XTOK_HOST
%type  <xtokHost>                host
%token <intValue>                ZTOK_HOST

%token <xtokValue>               XTOK_VALUE
%type  <xtokValue>               value
%token <intValue>                ZTOK_VALUE

%token <xtokValueArray>          XTOK_VALUEARRAY
%type  <xtokValueArray>          valueArray
%token <intValue>                ZTOK_VALUEARRAY

%token <intValueReference>       XTOK_VALUEREFERENCE
%type  <xtokValueReference>      valueReference
%token <intValue>                ZTOK_VALUEREFERENCE

%token <className>               XTOK_CLASSNAME
%token <intValue>                ZTOK_CLASSNAME
%type  <className>               className

%token <xtokInstanceName>        XTOK_INSTANCENAME
%token <intValue>                ZTOK_INSTANCENAME
%type  <xtokInstanceName>        instanceName

%token <xtokKeyBinding>          XTOK_KEYBINDING
%token <intValue>                ZTOK_KEYBINDING
%type  <xtokKeyBinding>          keyBinding
%type  <xtokKeyBindings>         keyBindings

%token <xtokKeyValue>            XTOK_KEYVALUE
%token <intValue>                ZTOK_KEYVALUE

%token <boolValue>               XTOK_IP_LOCALONLY
%token <boolValue>               XTOK_IP_INCLUDEQUALIFIERS
%token <boolValue>               XTOK_IP_INCLUDECLASSORIGIN
%token <boolValue>               XTOK_IP_DEEPINHERITANCE
%token <className>               XTOK_IP_CLASSNAME
%token <instance>                XTOK_IP_INSTANCE
%token <xmodifiedInstance>       XTOK_IP_MODIFIEDINSTANCE
%token <xtokInstanceName>        XTOK_IP_INSTANCENAME
%token <xtokInstanceName>        XTOK_IP_OBJECTNAME
%token <className>               XTOK_IP_ASSOCCLASS
%token <className>               XTOK_IP_RESULTCLASS
%token <className>               XTOK_IP_ROLE
%token <className>               XTOK_IP_RESULTROLE
%token <className>               XTOK_IP_QUERY
%token <className>               XTOK_IP_QUERYLANG

%token <xtokPropertyList>        XTOK_IP_PROPERTYLIST
%type  <boolValue>               boolValue
%type  <xtokPropertyList>        propertyList

%token <xtokNamedInstance>       XTOK_VALUENAMEDINSTANCE
%token <intValue>                ZTOK_VALUENAMEDINSTANCE
%type  <xtokNamedInstance>       namedInstance

%token <xtokQualifier>           XTOK_QUALIFIER
%type  <xtokQualifier>           qualifier
%token <intValue>                ZTOK_QUALIFIER

%token <xtokProperty>            XTOK_PROPERTY
%token <intValue>                ZTOK_PROPERTY
%token <xtokPropertyArray>       XTOK_PROPERTYARRAY
%token <intValue>                ZTOK_PROPERTYARRAY
%token <xtokProperty>            XTOK_PROPERTYREFERENCE
%token <intValue>                ZTOK_PROPERTYREFERENCE

%type  <xtokPropertyPart>        propertyPart
%type  <xtokPropertyPart>        propertyReferencePart
%type  <xtokPropertyPartList>    propertyPartList
%type  <xtokPropertyPartList>    propertyReferencePartList

%token <xtokInstance>            XTOK_INSTANCE
%token <intValue>                ZTOK_INSTANCE
%type  <xtokInstance>            instance
%type  <xtokInstanceParts>       instanceParts

%type  <xtokParamValue>          paramValue
%token <xtokParamValue>          XTOK_PARAMVALUE
%token <intValue>                ZTOK_PARAMVALUE

%type  <xtokInstancePath>        instancePath
%token <xtokInstancePath>        XTOK_INSTANCEPATH
%token <intValue>                ZTOK_INSTANCEPATH

%type  <xtokLocalInstancePath>   localInstancePath
%token <xtokLocalInstancePath>   XTOK_LOCALINSTANCEPATH
%token <intValue>                ZTOK_LOCALINSTANCEPATH

%type  <xtokLocalClassPath>      localClassPath
%token <xtokLocalClassPath>      XTOK_LOCALCLASSPATH
%token <intValue>                ZTOK_LOCALCLASSPATH

%%

/*
**==============================================================================
**
** The grammar itself.
**
**==============================================================================
*/

start
    : XTOK_XML ZTOK_XML cimOperation
    {
    }
;

cimOperation
    : XTOK_CIM message ZTOK_CIM
    {
    }
;

message
    : XTOK_MESSAGE simpleReq ZTOK_MESSAGE
    {
    }
;

simpleReq
    : XTOK_SIMPLEREQ iMethodCall ZTOK_SIMPLEREQ
    {
    }
;

iMethodCall
    : XTOK_GETCLASS getClass ZTOK_IMETHODCALL
    {
    }
    | XTOK_DELETECLASS deleteClass ZTOK_IMETHODCALL
    {
    }
    | XTOK_ENUMCLASSNAMES enumClassNames ZTOK_IMETHODCALL
    {
    }
    | XTOK_ENUMCLASSES enumClasses ZTOK_IMETHODCALL
    {
    }
    | XTOK_DELETEINSTANCE deleteInstance ZTOK_IMETHODCALL
    {
    }
    | XTOK_GETINSTANCE getInstance ZTOK_IMETHODCALL
    {
    }
    | XTOK_MODIFYINSTANCE modifyInstance ZTOK_IMETHODCALL
    {
    }
    | XTOK_CREATEINSTANCE createInstance ZTOK_IMETHODCALL
    {
    }
    | XTOK_ENUMINSTANCENAMES enumInstanceNames ZTOK_IMETHODCALL
    {
    }
    | XTOK_ENUMINSTANCES enumInstances ZTOK_IMETHODCALL
    {
    }
    | XTOK_ASSOCIATORS associators ZTOK_IMETHODCALL
    {
    }
    | XTOK_ASSOCIATORNAMES associatorNames ZTOK_IMETHODCALL
    {
    }
    | XTOK_REFERENCES references ZTOK_IMETHODCALL
    {
    }
    | XTOK_REFERENCENAMES referenceNames ZTOK_IMETHODCALL
    {
    }
    | XTOK_EXECQUERY execQuery ZTOK_IMETHODCALL
    {
    }
    | XTOK_METHODCALL methodCall ZTOK_METHODCALL
    {
    }
;

/*
 *    methodCall
*/

methodCall
    : localClassPath 
    {
       $$.op.count = 2;
       $$.op.type = OPS_InvokeMethod;
       $$.op.nameSpace=setCharsMsgSegment($1.path);
       $$.op.className=setCharsMsgSegment($1.className);
       $$.instName=0;
       $$.paramValues=((ParserControl*)parm)->paramValues;
       
       setRequest(parm,&$$,sizeof(XtokMethodCall),OPS_InvokeMethod);
    }   
    | localClassPath paramValues
    {
       $$.op.count = 2;
       $$.op.type = OPS_InvokeMethod;
       $$.op.nameSpace=setCharsMsgSegment($1.path);
       $$.op.className=setCharsMsgSegment($1.className);
       $$.instName=0;
       $$.paramValues=((ParserControl*)parm)->paramValues;
       
       setRequest(parm,&$$,sizeof(XtokMethodCall),OPS_InvokeMethod);
    }   
    | localInstancePath 
    {
       $$.op.count = 2;
       $$.op.type = OPS_InvokeMethod;
       $$.op.nameSpace=setCharsMsgSegment($1.path);
       $$.op.className=setCharsMsgSegment($1.instanceName.className);
       $$.instanceName=$1.instanceName;
       $$.instName=1;
       $$.paramValues=((ParserControl*)parm)->paramValues;
       
       setRequest(parm,&$$,sizeof(XtokMethodCall),OPS_InvokeMethod);
    }   
    | localInstancePath paramValues
    {
       $$.op.count = 2;
       $$.op.type = OPS_InvokeMethod;
       $$.op.nameSpace=setCharsMsgSegment($1.path);
       $$.op.className=setCharsMsgSegment($1.instanceName.className);
       $$.instanceName=$1.instanceName;
       $$.instName=1;
       $$.paramValues=((ParserControl*)parm)->paramValues;
       
       setRequest(parm,&$$,sizeof(XtokMethodCall),OPS_InvokeMethod);
    }   
;    

paramValues
    : paramValue
    | paramValues paramValue
;

paramValue
    : XTOK_PARAMVALUE value ZTOK_PARAMVALUE
    {
       $1.value=$2;
       addParamValue(&(((ParserControl*)parm)->paramValues),&$1);
    }   
    | XTOK_PARAMVALUE valueArray ZTOK_PARAMVALUE
    {
       $1.valueArray=$2;
       addParamValue(&(((ParserControl*)parm)->paramValues),&$1);
    }   
    | XTOK_PARAMVALUE valueReference ZTOK_PARAMVALUE
    {
       $1.valueRef=$2;
       addParamValue(&(((ParserControl*)parm)->paramValues),&$1);
    }   


/*
 *    getClass
*/

getClass
    : localNameSpacePath
    {
       $$.op.count = 2;
       $$.op.type = OPS_GetClass;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);
       $$.flags = FL_localOnly;
       $$.propertyList = NULL;
       $$.properties=0;

       setRequest(parm,&$$,sizeof(XtokGetClass),OPS_GetClass);
    }
    | localNameSpacePath getClassParmsList
    {
       $$.op.count = 2;
       $$.op.type = OPS_GetClass;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.className);
       $$.flags = ($2.flags &  $2.flagsSet) | ((~$2.flagsSet) & (FL_localOnly));
       $$.propertyList = $2.propertyList;
       $$.properties=$2.properties;

       setRequest(parm,&$$,sizeof(XtokGetClass),OPS_GetClass);
    }
;

getClassParmsList
    : getClassParms
    {
       $$.flags=$1.flags;
       $$.flagsSet=$1.flagsSet;
       if ($1.clsNameSet) $$.className=$1.className;
       $$.clsNameSet = $1.clsNameSet;
       if ($1.propertyList) {
          $$.propertyList=$1.propertyList;
          $$.properties=$1.properties;
       }
    }
    | getClassParmsList getClassParms
    {
       $$.flags=$1.flags|$2.flags;
       $$.flagsSet=$1.flagsSet|$2.flagsSet;
       if ($2.clsNameSet) $$.className=$2.className;
       $$.clsNameSet |= $2.clsNameSet;
       if ($2.propertyList) {
          $$.propertyList=$2.propertyList;
          $$.properties=$2.properties;
       }
    }
;

getClassParms
    : XTOK_IP_CLASSNAME className ZTOK_IPARAMVALUE
    {
       $$.className = $2;
       $$.flags = $$.flagsSet = 0 ;
       $$.clsNameSet = 1;
       $$.propertyList=0;
       $$.properties=0;
    }
    | XTOK_IP_LOCALONLY boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_localOnly : 0 ;
       $$.flagsSet = FL_localOnly;
       $$.properties=$$.clsNameSet=0;
       $$.propertyList=0;
    }
    | XTOK_IP_INCLUDEQUALIFIERS boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeQualifiers : 0 ;
       $$.flagsSet = FL_includeQualifiers;
       $$.properties=$$.clsNameSet=0;
       $$.propertyList=0;
    }
    | XTOK_IP_INCLUDECLASSORIGIN boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeClassOrigin : 0 ;
       $$.flagsSet = FL_includeClassOrigin;
       $$.properties=$$.clsNameSet=0;
       $$.propertyList=0;
    }
    | XTOK_IP_PROPERTYLIST propertyList ZTOK_IPARAMVALUE
    {
       $$.propertyList=$2.list.values;
       $$.properties=$2.list.next;
       $$.clsNameSet=0;
       $$.flags = $$.flagsSet = 0 ;
    }
;


/*
 *    enumClassNames
*/

enumClassNames
    : localNameSpacePath
    {
       $$.op.count = 2;
       $$.op.type = OPS_EnumerateClassNames;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);
       $$.flags = 0;

       setRequest(parm,&$$,sizeof(XtokEnumClassNames),OPS_EnumerateClassNames);
    }
    | localNameSpacePath enumClassNamesParmsList
    {
       $$.op.count = 2;
       $$.op.type = OPS_EnumerateClassNames;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.className);
       $$.flags = $2.flags;

       setRequest(parm,&$$,sizeof(XtokEnumClassNames),OPS_EnumerateClassNames);
    }
;

enumClassNamesParmsList
    : enumClassNamesParms
    {
       if ($1.className) $$.className=$1.className;
       $$.flags=$1.flags;
    }
    | enumClassNamesParmsList enumClassNamesParms
    {
       if ($2.className) $$.className=$2.className;
       $$.flags = ($2.flags & $2.flagsSet) | ((~$2.flagsSet) & FL_deepInheritance);
    }
;

enumClassNamesParms
    : XTOK_IP_CLASSNAME className ZTOK_IPARAMVALUE
    {
       $$.className = $2;
       $$.flags = $$.flagsSet = 0 ;
    }
    | XTOK_IP_DEEPINHERITANCE boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_deepInheritance : 0 ;
       $$.flagsSet = FL_deepInheritance;
       $$.className=0;
    }
;

/*
 *    enumClasses
*/

enumClasses
    : localNameSpacePath
    {
       $$.op.count = 2;
       $$.op.type = OPS_EnumerateClasses;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);
       $$.flags = FL_localOnly;

       setRequest(parm,&$$,sizeof(XtokEnumClasses),OPS_EnumerateClasses);
    }
    | localNameSpacePath enumClassesParmsList
    {
       $$.op.count = 2;
       $$.op.type = OPS_EnumerateClasses;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.className);
       $$.flags = ($2.flags & $2.flagsSet) | ((~$2.flagsSet) & FL_localOnly);

       setRequest(parm,&$$,sizeof(XtokEnumClasses),OPS_EnumerateClasses);
    }
;

enumClassesParmsList
    : enumClassesParms
    {
       $$.flags=$1.flags;
       $$.flagsSet=$1.flagsSet;
       if ($1.className) $$.className=$1.className;
    }
    | enumClassesParmsList enumClassesParms
    {
       $$.flags=$1.flags|$2.flags;
       $$.flagsSet=$1.flagsSet|$2.flagsSet;
       if ($2.className) $$.className=$2.className;
    }
;

enumClassesParms
    : XTOK_IP_CLASSNAME className ZTOK_IPARAMVALUE
    {
       $$.className = $2;
       $$.flags = $$.flagsSet = 0 ;
    }
    | XTOK_IP_DEEPINHERITANCE boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_deepInheritance : 0 ;
       $$.flagsSet = FL_deepInheritance;
       $$.className=0;
    }
    | XTOK_IP_LOCALONLY boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_localOnly : 0 ;
       $$.flagsSet = FL_localOnly;
       $$.className=0;
    }
    | XTOK_IP_INCLUDEQUALIFIERS boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeQualifiers : 0 ;
       $$.flagsSet = FL_includeQualifiers;
       $$.className=0;
    }
    | XTOK_IP_INCLUDECLASSORIGIN boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeClassOrigin : 0 ;
       $$.flagsSet = FL_includeClassOrigin;
       $$.className=0;
    }
;

/*
 *    getInstance
*/

getInstance
    : localNameSpacePath
    {
       $$.op.count = 2;
       $$.op.type = OPS_GetInstance;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);
       $$.flags = FL_localOnly;
       $$.propertyList = NULL;
       $$.properties=0;
       $$.instNameSet = 0;

       setRequest(parm,&$$,sizeof(XtokGetInstance),OPS_GetInstance);
    }
    | localNameSpacePath getInstanceParmsList
    {
       $$.op.count = 2;
       $$.op.type = OPS_GetInstance;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.instanceName.className);
       $$.flags = ($2.flags & $2.flagsSet) | ((~$2.flagsSet) & (FL_localOnly));
       $$.instanceName = $2.instanceName;
       $$.instNameSet = $2.instNameSet;
       $$.propertyList = $2.propertyList;
       $$.properties=$2.properties;

       setRequest(parm,&$$,sizeof(XtokGetInstance),OPS_GetInstance);
    }
;

getInstanceParmsList
    : getInstanceParms
    {
       $$.flags=$1.flags;
       $$.flagsSet=$1.flagsSet;
       if ($1.instNameSet) $$.instanceName=$1.instanceName;
       $$.instNameSet = $1.instNameSet;
       if ($1.propertyList) {
          $$.propertyList=$1.propertyList;
          $$.properties=$1.properties;
       }
    }
    | getInstanceParmsList getInstanceParms
    {
       $$.flags=$1.flags|$2.flags;
       $$.flagsSet=$1.flagsSet|$2.flagsSet;
       if ($2.instNameSet) $$.instanceName=$2.instanceName;
       $$.instNameSet = $2.instNameSet;
       if ($2.propertyList) {
          $$.propertyList=$2.propertyList;
          $$.properties=$2.properties;
       }
    }
;

getInstanceParms
    : XTOK_IP_INSTANCENAME instanceName ZTOK_IPARAMVALUE
    {
       $$.instanceName = $2;
       $$.flags = $$.flagsSet = 0 ;
       $$.propertyList=0;
       $$.instNameSet = 1;
       $$.properties=0;
    }
    | XTOK_IP_LOCALONLY boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_localOnly : 0 ;
       $$.flagsSet = FL_localOnly;
       $$.propertyList=0;
       $$.properties=$$.instNameSet=0;
    }
    | XTOK_IP_INCLUDEQUALIFIERS boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeQualifiers : 0 ;
       $$.flagsSet = FL_includeQualifiers;
       $$.propertyList=0;
       $$.properties=$$.instNameSet=0;
    }
    | XTOK_IP_INCLUDECLASSORIGIN boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeClassOrigin : 0 ;
       $$.flagsSet = FL_includeClassOrigin;
       $$.propertyList=0;
       $$.properties=$$.instNameSet=0;
    }
    | XTOK_IP_PROPERTYLIST propertyList ZTOK_IPARAMVALUE
    {
       $$.propertyList=$2.list.values;
       $$.properties=$2.list.next;
       $$.instNameSet=0;
       $$.flags = $$.flagsSet = 0 ;
    }
;


/*
 *    createInstance
*/


createInstance
    : localNameSpacePath
    {
       $$.op.count = 2;
       $$.op.type = OPS_CreateInstance;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);

       setRequest(parm,&$$,sizeof(XtokCreateInstance),OPS_CreateInstance);
    }
    | localNameSpacePath createInstanceParm
    {
       $$.op.count = 2;
       $$.op.type = OPS_CreateInstance;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.instance.className);
       $$.instance = $2.instance;

       setRequest(parm,&$$,sizeof(XtokCreateInstance),OPS_CreateInstance);
    }
;


createInstanceParm
    : XTOK_IP_INSTANCE instance ZTOK_IPARAMVALUE
    {
       $$.instance = $2;
    }
;

/*
 *    modifyInstance
*/


modifyInstance
    : localNameSpacePath
    {
       $$.op.count = 2;
       $$.op.type = OPS_ModifyInstance;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);
       $$.flags = FL_includeQualifiers;
       $$.propertyList = 0;
       $$.properties=0;

       setRequest(parm,&$$,sizeof(XtokModifyInstance),OPS_ModifyInstance);
    }
    | localNameSpacePath modifyInstanceParmsList
    {
       $$.op.count = 2;
       $$.op.type = OPS_ModifyInstance;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.namedInstance.path.className);
       $$.namedInstance = $2.namedInstance;
       $$.flags = $2.flags | ((~$2.flagsSet) & (FL_includeQualifiers));
       $$.propertyList = $2.propertyList;
       $$.properties=$2.properties;

       setRequest(parm,&$$,sizeof(XtokModifyInstance),OPS_ModifyInstance);
    }
;

modifyInstanceParmsList
    : modifyInstanceParms
    {
       $$.flags=$1.flags;
       $$.flagsSet=$1.flagsSet;
       if ($1.namedInstSet) $$.namedInstance=$1.namedInstance;
       $$.namedInstSet = $1.namedInstSet;
       if ($1.propertyList) {
          $$.propertyList=$1.propertyList;
          $$.properties=$1.properties;
       }
    }
    | modifyInstanceParmsList modifyInstanceParms
    {
       $$.flags=$1.flags|$2.flags;
       $$.flagsSet=$1.flagsSet|$2.flagsSet;
       if ($2.namedInstSet) $$.namedInstance=$2.namedInstance;
       $$.namedInstSet = $2.namedInstSet;
       if ($2.propertyList) {
          $$.propertyList=$2.propertyList;
          $$.properties=$2.properties;
       }
    }
;


modifyInstanceParms
    : XTOK_IP_MODIFIEDINSTANCE namedInstance ZTOK_IPARAMVALUE
    {
       $$.namedInstance=$2;
       $$.namedInstSet=1;
       $$.properties=0;
       $$.flags = $$.flagsSet = 0 ;
    }
    | XTOK_IP_INCLUDEQUALIFIERS boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeQualifiers : 0 ;
       $$.flagsSet = FL_includeQualifiers;
       $$.propertyList=0;
       $$.properties=$$.namedInstSet=0;
    }
    | XTOK_IP_PROPERTYLIST propertyList ZTOK_IPARAMVALUE
    {
       $$.propertyList=$2.list.values;
       $$.properties=$2.list.next;
       $$.namedInstSet=0;
       $$.flags = $$.flagsSet = 0 ;
    }
;


/*
 *    deleteClass
*/

deleteClass
    : localNameSpacePath
    {
       $$.op.count = 2;
       $$.op.type = OPS_DeleteClass;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);

       setRequest(parm,&$$,sizeof(XtokDeleteClass),OPS_DeleteClass);
    }
    | localNameSpacePath deleteClassParm
    {
       $$.op.count = 2;
       $$.op.type = OPS_DeleteClass;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.className);
       $$.className = $2.className;

       setRequest(parm,&$$,sizeof(XtokDeleteClass),OPS_DeleteClass);
    }
;


deleteClassParm
    : XTOK_IP_CLASSNAME className ZTOK_IPARAMVALUE
    {
       $$.className = $2;
    }
;


/*
 *    deleteInstance
*/

deleteInstance
    : localNameSpacePath
    {
       $$.op.count = 2;
       $$.op.type = OPS_DeleteInstance;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);

       setRequest(parm,&$$,sizeof(XtokDeleteInstance),OPS_DeleteInstance);
    }
    | localNameSpacePath deleteInstanceParm
    {
       $$.op.count = 2;
       $$.op.type = OPS_DeleteInstance;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.instanceName.className);
       $$.instanceName = $2.instanceName;

       setRequest(parm,&$$,sizeof(XtokDeleteInstance),OPS_DeleteInstance);
    }
;


deleteInstanceParm
    : XTOK_IP_INSTANCENAME instanceName ZTOK_IPARAMVALUE
    {
       $$.instanceName = $2;
    }
;



/*
 *    enumInstanceNames
*/

enumInstanceNames
    : localNameSpacePath XTOK_IP_CLASSNAME className ZTOK_IPARAMVALUE
    {
       $$.op.count = 2;
       $$.op.type = OPS_EnumerateInstanceNames;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($3);

       setRequest(parm,&$$,sizeof(XtokEnumInstanceNames),OPS_EnumerateInstanceNames);
    }
;


/*
 *    enumInstances
*/


enumInstances
    : localNameSpacePath
    {
       $$.op.count = 2;
       $$.op.type = OPS_EnumerateInstances;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);
       $$.flags = FL_localOnly | FL_deepInheritance;
       $$.propertyList = NULL;
       $$.properties=0;

       setRequest(parm,&$$,sizeof(XtokEnumInstances),OPS_EnumerateInstances);
    }
    | localNameSpacePath enumInstancesParmsList
    {
       $$.op.count = 2;
       $$.op.type = OPS_EnumerateInstances;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.className);
       $$.flags = ($2.flags & $2.flagsSet) | ((~$2.flagsSet) & (FL_localOnly));
       $$.propertyList = $2.propertyList;
       $$.properties=$2.properties;

       setRequest(parm,&$$,sizeof(XtokEnumInstances),OPS_EnumerateInstances);
    }
;

enumInstancesParmsList
    : enumInstancesParms
    {
       $$.flags=$1.flags;
       $$.flagsSet=$1.flagsSet;
       if ($1.className) $$.className=$1.className;
       if ($1.propertyList) {
          $$.propertyList=$1.propertyList;
          $$.properties=$1.properties;
       }
    }
    | enumInstancesParmsList enumInstancesParms
    {
       $$.flags=$1.flags|$2.flags;
       $$.flagsSet=$1.flagsSet|$2.flagsSet;
       if ($2.className) $$.className=$2.className;
       if ($2.propertyList) {
          $$.propertyList=$2.propertyList;
          $$.properties=$2.properties;
       }
    }
;

enumInstancesParms
    : XTOK_IP_CLASSNAME className ZTOK_IPARAMVALUE
    {
       $$.className = $2;
       $$.flags = $$.flagsSet = 0 ;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_LOCALONLY boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_localOnly : 0 ;
       $$.flagsSet = FL_localOnly;
       $$.className=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_INCLUDEQUALIFIERS boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeQualifiers : 0 ;
       $$.flagsSet = FL_includeQualifiers;
       $$.className=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_DEEPINHERITANCE boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_deepInheritance : 0 ;
       $$.flagsSet = FL_deepInheritance;
       $$.className=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_INCLUDECLASSORIGIN boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeClassOrigin : 0 ;
       $$.flagsSet = FL_includeClassOrigin;
       $$.className=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_PROPERTYLIST propertyList ZTOK_IPARAMVALUE
    {
       $$.propertyList=$2.list.values;
       $$.properties=$2.list.next;
       $$.className=0;
       $$.flags = $$.flagsSet = 0 ;
    }
;




/*
 *    execQuery
*/

execQuery
    : localNameSpacePath 
          XTOK_IP_QUERY value ZTOK_IPARAMVALUE
          XTOK_IP_QUERYLANG value ZTOK_IPARAMVALUE
    {
       $$.op.count = 3;
       $$.op.type = OPS_ExecQuery;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.query=setCharsMsgSegment($3.value);
       $$.op.queryLang=setCharsMsgSegment($6.value);

       setRequest(parm,&$$,sizeof(XtokExecQuery),OPS_ExecQuery);
    }        
    | localNameSpacePath 
          XTOK_IP_QUERYLANG value ZTOK_IPARAMVALUE
          XTOK_IP_QUERY value ZTOK_IPARAMVALUE
    {
       $$.op.count = 3;
       $$.op.type = OPS_ExecQuery;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.query=setCharsMsgSegment($6.value);
       $$.op.queryLang=setCharsMsgSegment($3.value);

       setRequest(parm,&$$,sizeof(XtokExecQuery),OPS_ExecQuery);
    }        
;    
    
    
/*
 *    associators
*/


associators
    : localNameSpacePath
    {
       $$.op.count = 6;
       $$.op.type = OPS_Associators;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);
       $$.op.assocClass=setCharsMsgSegment(NULL);
       $$.op.resultClass=setCharsMsgSegment(NULL);
       $$.op.role=setCharsMsgSegment(NULL);
       $$.op.resultRole=setCharsMsgSegment(NULL);
       $$.flags = 0;
       $$.objNameSet = 0;
       $$.propertyList = 0;
       $$.properties=0;

       setRequest(parm,&$$,sizeof(XtokAssociators),OPS_Associators);
    }
    | localNameSpacePath associatorsParmsList
    {
       $$.op.count = 6;
       $$.op.type = OPS_Associators;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.objectName.className);
       $$.op.assocClass=setCharsMsgSegment($2.assocClass);
       $$.op.resultClass=setCharsMsgSegment($2.resultClass);
       $$.op.role=setCharsMsgSegment($2.role);
       $$.op.resultRole=setCharsMsgSegment($2.resultRole);
       $$.flags = ($2.flags & $2.flagsSet) | (~$2.flagsSet & 0);
       $$.objectName = $2.objectName;
       $$.objNameSet = $2.objNameSet;
       $$.propertyList = $2.propertyList;
       $$.properties=$2.properties;

       setRequest(parm,&$$,sizeof(XtokAssociators),OPS_Associators);
    }
;

associatorsParmsList
    : associatorsParms
    {
       $$.flags=$1.flags;
       $$.flagsSet=$1.flagsSet;
       if ($1.objNameSet)  {
          $$.objectName=$1.objectName;
          $$.objNameSet = $1.objNameSet;
       }
       $$.assocClass=$$.resultClass=$$.role=$$.resultRole=0;
       if ($1.propertyList) {
          $$.propertyList=$1.propertyList;
          $$.properties=$1.properties;
       }
    }
    | associatorsParmsList associatorsParms
    {
       $$.flags=$1.flags|$2.flags;
       $$.flagsSet=$1.flagsSet|$2.flagsSet;
       if ($2.assocClass) $$.assocClass=$2.assocClass;
       else if ($2.resultClass) $$.resultClass=$2.resultClass;
       else if ($2.role) $$.role=$2.role;
       else if ($2.resultRole) $$.resultRole=$2.resultRole;
       else if ($2.objNameSet) {
          $$.objectName=$2.objectName;
          $$.objNameSet = $2.objNameSet;
       }
       else if ($2.propertyList) {
          $$.propertyList=$2.propertyList;
          $$.properties=$2.properties;
       }
    }
;

associatorsParms
    : XTOK_IP_OBJECTNAME instanceName ZTOK_IPARAMVALUE
    {
       $$.objectName = $2;
       $$.objNameSet = 1;
       $$.flags = $$.flagsSet = 0 ;
       $$.assocClass=$$.resultClass=$$.role=$$.resultRole=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_ASSOCCLASS className ZTOK_IPARAMVALUE
    {
       $$.assocClass = $2;
       $$.objNameSet=$$.flags = $$.flagsSet = 0 ;
       $$.resultClass=$$.role=$$.resultRole=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_RESULTCLASS className ZTOK_IPARAMVALUE
    {
       $$.resultClass = $2;
       $$.objNameSet=$$.flags = $$.flagsSet = 0 ;
       $$.assocClass=$$.role=$$.resultRole=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_ROLE className ZTOK_IPARAMVALUE
    {
       $$.role = $2;
       $$.objNameSet=$$.flags = $$.flagsSet = 0 ;
       $$.assocClass=$$.resultClass=$$.resultRole=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_RESULTROLE className ZTOK_IPARAMVALUE
    {
       $$.resultRole = $2;
       $$.objNameSet=$$.flags = $$.flagsSet = 0 ;
       $$.assocClass=$$.resultClass=$$.role=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_INCLUDEQUALIFIERS boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeQualifiers : 0 ;
       $$.flagsSet = FL_includeQualifiers;
       $$.objNameSet=0;
       $$.assocClass=$$.resultClass=$$.role=$$.resultRole=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_INCLUDECLASSORIGIN boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeClassOrigin : 0 ;
       $$.flagsSet = FL_includeClassOrigin;
       $$.objNameSet=0;
       $$.assocClass=$$.resultClass=$$.role=$$.resultRole=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_PROPERTYLIST propertyList ZTOK_IPARAMVALUE
    {
       $$.propertyList=$2.list.values;
       $$.properties=$2.list.next;
       $$.objNameSet=$$.flags = $$.flagsSet = 0 ;
       $$.assocClass=$$.resultClass=$$.role=$$.resultRole=0;
    }
;




/*
 *    references
*/


references
    : localNameSpacePath
    {
       $$.op.count = 4;
       $$.op.type = OPS_References;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);
       $$.op.resultClass=setCharsMsgSegment(NULL);
       $$.op.role=setCharsMsgSegment(NULL);
       $$.flags = 0;
       $$.objNameSet = 0;
       $$.propertyList = 0;
       $$.properties=0;

       setRequest(parm,&$$,sizeof(XtokReferences),OPS_References);
    }
    | localNameSpacePath referencesParmsList
    {
       $$.op.count = 4;
       $$.op.type = OPS_References;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.objectName.className);
       $$.op.resultClass=setCharsMsgSegment($2.resultClass);
       $$.op.role=setCharsMsgSegment($2.role);
       $$.flags = ($2.flags & $2.flagsSet) | (~$2.flagsSet & 0);
       $$.objectName = $2.objectName;
       $$.objNameSet = $2.objNameSet;
       $$.propertyList = $2.propertyList;
       $$.properties=$2.properties;

       setRequest(parm,&$$,sizeof(XtokReferences),OPS_References);
    }
;

referencesParmsList
    : associatorsParms
    {
       $$.flags=$1.flags;
       $$.flagsSet=$1.flagsSet;
       if ($1.objNameSet)  {
          $$.objectName=$1.objectName;
          $$.objNameSet = $1.objNameSet;
       }
       $$.resultClass=$$.role=0;
       if ($1.propertyList) {
          $$.propertyList=$1.propertyList;
          $$.properties=$1.properties;
       }
    }
    | referencesParmsList referencesParms
    {
       $$.flags=$1.flags|$2.flags;
       $$.flagsSet=$1.flagsSet|$2.flagsSet;
       if ($2.resultClass) $$.resultClass=$2.resultClass;
       else if ($2.role) $$.role=$2.role;
       else if ($2.objNameSet) {
          $$.objectName=$2.objectName;
          $$.objNameSet = $2.objNameSet;
       }
       else if ($2.propertyList) {
          $$.propertyList=$2.propertyList;
          $$.properties=$2.properties;
       }
    }
;

referencesParms
    : XTOK_IP_OBJECTNAME instanceName ZTOK_IPARAMVALUE
    {
       $$.objectName = $2;
       $$.objNameSet = 1;
       $$.flags = $$.flagsSet = 0 ;
       $$.resultClass=$$.role=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_RESULTCLASS className ZTOK_IPARAMVALUE
    {
       $$.resultClass = $2;
       $$.objNameSet=$$.flags = $$.flagsSet = 0 ;
       $$.role=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_ROLE className ZTOK_IPARAMVALUE
    {
       $$.role = $2;
       $$.objNameSet=$$.flags = $$.flagsSet = 0 ;
       $$.resultClass=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_INCLUDEQUALIFIERS boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeQualifiers : 0 ;
       $$.flagsSet = FL_includeQualifiers;
       $$.objNameSet=0;
       $$.resultClass=$$.role=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_INCLUDECLASSORIGIN boolValue ZTOK_IPARAMVALUE
    {
       $$.flags = $2 ? FL_includeClassOrigin : 0 ;
       $$.flagsSet = FL_includeClassOrigin;
       $$.objNameSet=0;
       $$.resultClass=$$.role=0;
       $$.properties=0;
       $$.propertyList=0;
    }
    | XTOK_IP_PROPERTYLIST propertyList ZTOK_IPARAMVALUE
    {
       $$.propertyList=$2.list.values;
       $$.properties=$2.list.next;
       $$.objNameSet=$$.flags = $$.flagsSet = 0 ;
       $$.resultClass=$$.role=0;
    }
;


/*
 *    associatorNames
*/


associatorNames
    : localNameSpacePath
    {
       $$.op.count = 6;
       $$.op.type = OPS_AssociatorNames;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);
       $$.op.assocClass=setCharsMsgSegment(NULL);
       $$.op.resultClass=setCharsMsgSegment(NULL);
       $$.op.role=setCharsMsgSegment(NULL);
       $$.op.resultRole=setCharsMsgSegment(NULL);
       $$.objNameSet = 0;

       setRequest(parm,&$$,sizeof(XtokAssociatorNames),OPS_AssociatorNames);
    }
    | localNameSpacePath associatorNamesParmsList
    {
       $$.op.count = 6;
       $$.op.type = OPS_AssociatorNames;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.objectName.className);
       $$.op.assocClass=setCharsMsgSegment($2.assocClass);
       $$.op.resultClass=setCharsMsgSegment($2.resultClass);
       $$.op.role=setCharsMsgSegment($2.role);
       $$.op.resultRole=setCharsMsgSegment($2.resultRole);
       $$.objectName = $2.objectName;
       $$.objNameSet = $2.objNameSet;
       setRequest(parm,&$$,sizeof(XtokAssociatorNames),OPS_AssociatorNames);
    }
;

associatorNamesParmsList
    : associatorNamesParms
    {
       if ($1.objNameSet)  {
          $$.objectName=$1.objectName;
          $$.objNameSet = $1.objNameSet;
       }
      $$.assocClass=$$.resultClass=$$.role=$$.resultRole=0;
    }
    | associatorNamesParmsList associatorNamesParms
    {
       if ($2.assocClass) $$.assocClass=$2.assocClass;
       else if ($2.resultClass) $$.resultClass=$2.resultClass;
       else if ($2.role) $$.role=$2.role;
       else if ($2.resultRole) $$.resultRole=$2.resultRole;
       else if ($2.objNameSet) {
          $$.objectName=$2.objectName;
          $$.objNameSet = $2.objNameSet;
       }
    }
;

associatorNamesParms
    : XTOK_IP_OBJECTNAME instanceName ZTOK_IPARAMVALUE
    {
       $$.objectName = $2;
       $$.objNameSet = 1;
       $$.assocClass=$$.resultClass=$$.role=$$.resultRole=0;
    }
    | XTOK_IP_ASSOCCLASS className ZTOK_IPARAMVALUE
    {
       $$.assocClass = $2;
       $$.objNameSet = 0 ;
       $$.resultClass=$$.role=$$.resultRole=0;
    }
    | XTOK_IP_RESULTCLASS className ZTOK_IPARAMVALUE
    {
       $$.resultClass = $2;
       $$.objNameSet = 0 ;
       $$.assocClass=$$.role=$$.resultRole=0;
    }
    | XTOK_IP_ROLE className ZTOK_IPARAMVALUE
    {
       $$.role = $2;
       $$.objNameSet = 0 ;
       $$.assocClass=$$.resultClass=$$.resultRole=0;
    }
    | XTOK_IP_RESULTROLE className ZTOK_IPARAMVALUE
    {
       $$.resultRole = $2;
       $$.objNameSet= 0 ;
       $$.assocClass=$$.resultClass=$$.role=0;
    }
;



/*
 *    referenceNames
*/


referenceNames
    : localNameSpacePath
    {
       $$.op.count = 4;
       $$.op.type = OPS_ReferenceNames;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment(NULL);
       $$.op.resultClass=setCharsMsgSegment(NULL);
       $$.op.role=setCharsMsgSegment(NULL);
       $$.objNameSet = 0;

       setRequest(parm,&$$,sizeof(XtokReferenceNames),OPS_ReferenceNames);
    }
    | localNameSpacePath referenceNamesParmsList
    {
       $$.op.count = 4;
       $$.op.type = OPS_ReferenceNames;
       $$.op.nameSpace=setCharsMsgSegment($1);
       $$.op.className=setCharsMsgSegment($2.objectName.className);
       $$.op.resultClass=setCharsMsgSegment($2.resultClass);
       $$.op.role=setCharsMsgSegment($2.role);
       $$.objectName = $2.objectName;
       $$.objNameSet = $2.objNameSet;

       setRequest(parm,&$$,sizeof(XtokReferenceNames),OPS_ReferenceNames);
    }
;

referenceNamesParmsList
    : referenceNamesParms
    {
       if ($1.objNameSet)  {
          $$.objectName=$1.objectName;
          $$.objNameSet = $1.objNameSet;
       }
      $$.resultClass=$$.role=0;
    }
    | referenceNamesParmsList referenceNamesParms
    {
       if ($2.resultClass) $$.resultClass=$2.resultClass;
       else if ($2.role) $$.role=$2.role;
       else if ($2.objNameSet) {
          $$.objectName=$2.objectName;
          $$.objNameSet = $2.objNameSet;
       }
    }
;

referenceNamesParms
    : XTOK_IP_OBJECTNAME instanceName ZTOK_IPARAMVALUE
    {
       $$.objectName = $2;
       $$.objNameSet = 1;
       $$.resultClass=$$.role=0;
    }
    | XTOK_IP_RESULTCLASS className ZTOK_IPARAMVALUE
    {
       $$.resultClass = $2;
       $$.objNameSet = 0 ;
       $$.role=0;
    }
    | XTOK_IP_ROLE className ZTOK_IPARAMVALUE
    {
       $$.role = $2;
       $$.objNameSet = 0 ;
       $$.resultClass=0;
    }
;


/*
 *    valueNamedInstance
*/

namedInstance
    : XTOK_VALUENAMEDINSTANCE instanceName instance ZTOK_VALUENAMEDINSTANCE
    {
        $$.path=$2;
	$$.instance=$3;
    }
;


/*
 *    instance
*/


instance
    : XTOK_INSTANCE ZTOK_INSTANCE
    {
       memset(&$$.properties,0,sizeof($$.properties));
       memset(&$$.qualifiers,0,sizeof($$.qualifiers));
    }
    | XTOK_INSTANCE instancePartsList ZTOK_INSTANCE
    {
       $$.properties=((ParserControl*)parm)->properties;
       $$.qualifiers=((ParserControl*)parm)->qualifiers;
    }
;

instancePartsList
    : instanceParts
    {
    }
    | instancePartsList instanceParts
    {
    }
;


instanceParts
    : qualifier
    {
       addQualifier(&(((ParserControl*)parm)->qualifiers),&$1);
    }
    | XTOK_PROPERTY propertyPartList ZTOK_PROPERTY
    {
       $1.value=$2.value;
       $1.propType=typeProperty_Value;
       addProperty(&(((ParserControl*)parm)->properties),&$1);
    }
    | XTOK_PROPERTYREFERENCE propertyReferencePartList ZTOK_PROPERTYREFERENCE
    {
       $1.ref=$2.ref;
       $1.propType=typeProperty_Reference;
       addProperty(&(((ParserControl*)parm)->properties),&$1);
    } 
;


propertyPartList
    : propertyPart
    {
       if ($1.qPart) {
          addQualifier(&($$.qualifiers),&$1.qualifier);
       }
       else {
          $$.value=$1.value;
       }
    }
    | propertyPartList propertyPart 
    {
       if ($2.qPart) {
          addQualifier(&($$.qualifiers),&$2.qualifier);
       }
       else {
          $$.value=$1.value;
       }
    }
;

propertyPart
    : qualifier
    {
       $$.qPart=1;
       $$.qualifier=$1;
    }
    | value
    {
       $$.value=$1.value;
       $$.qPart=0;
    }
;


propertyReferencePartList
    : propertyReferencePart
    {
       if ($1.qPart) {
          addQualifier(&($$.qualifiers),&$1.qualifier);
       }
       else {
          $$.ref=$1.ref;
       }
    }
    | propertyReferencePartList propertyReferencePart
    {
       if ($2.qPart) {
          addQualifier(&($$.qualifiers),&$2.qualifier);
       }
       else {
          $$.ref=$1.ref;
       }
    }
;

propertyReferencePart
    : qualifier
    {
       $$.qPart=1;
       $$.qualifier=$1;
    }   
    | valueReference
    {
       $$.qPart=0;
       $$.ref=$1;
    }      
;


propertyArray
    : XTOK_PROPERTYARRAY  ZTOK_PROPERTYARRAY
    {
    printf("--- propertyArray\n");
    }
;

propertyList
    : XTOK_VALUEARRAY valueArray ZTOK_VALUEARRAY
    {
       $2.values[$2.next]=NULL;
       $$.list=$2;
    }
;

qualifier
    : XTOK_QUALIFIER value ZTOK_QUALIFIER
    {
    }
;

/*
 *    localNameSpacePath 
*/



localNameSpacePath
    : XTOK_LOCALNAMESPACEPATH namespaces ZTOK_LOCALNAMESPACEPATH
    {
       $$=$2.cns;
    }
;

namespaces
    : XTOK_NAMESPACE ZTOK_NAMESPACE
    {
       $$.cns=strdup($1.ns);
    }
    | namespaces XTOK_NAMESPACE ZTOK_NAMESPACE
    {
       int l=strlen($1.cns)+strlen($2.ns)+2;
       $$.cns=(char*)malloc(l);
       strcpy($$.cns,$1.cns);
       strcat($$.cns,"/");
       strcat($$.cns,$2.ns);
       free($1.cns);
    }
;


nameSpacePath
    : XTOK_NAMESPACEPATH host localNameSpacePath ZTOK_NAMESPACEPATH
    {
       $$.host=$2;
       $$.nameSpacePath=$3;
    }
;

host
    : XTOK_HOST ZTOK_HOST
    {
    }
;

instancePath
    : XTOK_INSTANCEPATH nameSpacePath instanceName ZTOK_INSTANCEPATH
    {
       $$.path=$2;
       $$.instanceName=$3;
       $$.type=1;
    }
 /*
    | nameSpacePath instanceName
    {
    }
    | XTOK_CLASSPATH nameSpacePath className  ZTOK_CLASSPATH
    {
    }
    | XTOK_LOCALCLASSPATH localNameSpacePath className ZTOK_LOCALCLASSPATH
    {
    } */
;

localInstancePath
    : XTOK_LOCALINSTANCEPATH localNameSpacePath instanceName ZTOK_LOCALINSTANCEPATH
    {
       $$.path=$2;
       $$.instanceName=$3;
       $$.type=1;
    }
;

localClassPath
    : XTOK_LOCALCLASSPATH localNameSpacePath className ZTOK_LOCALCLASSPATH
    {
       $$.path=$2;
       $$.className=$3;
       $$.type=1;
    }
;
/*
 *    value
*/


value
    : XTOK_VALUE ZTOK_VALUE
    {
       $$.value=$1.value;
    }
;

valueArray
    : value
    {
       $$.next=1;
       $$.max=64;
       $$.values=(char**)malloc(sizeof(char*)*64);
       $$.values[0]=$1.value;
    }
    | valueArray value
    {
       $$.values[$$.next]=$2.value;
       $$.next++;
    }
;

valueReference
    : XTOK_VALUEREFERENCE instancePath ZTOK_VALUEREFERENCE
    {
       $$.instancePath=$2;
       $$.type=typeValRef_InstancePath;
    }
    | XTOK_VALUEREFERENCE instanceName ZTOK_VALUEREFERENCE
    {
       $$.instanceName=$2;
       $$.type=typeValRef_InstanceName;
    }
;

boolValue
    : XTOK_VALUE ZTOK_VALUE
    {
//       int b=isBoolean($1.val);
//       if (b>=0) $$=(b!=0);
    }
;

className
    : XTOK_CLASSNAME ZTOK_CLASSNAME
    {
    }
;


/*
 *    instanceName
*/


instanceName
    : XTOK_INSTANCENAME keyBindings ZTOK_INSTANCENAME
    {
       $$.className=$1.className;
       $$.bindings=$2;
    }
;

keyBindings
    : keyBinding
    {
       $$.next=1;
       $$.max=16;
       $$.keyBindings=(XtokKeyBinding*)malloc(sizeof(XtokKeyBinding)*16);
       $$.keyBindings[0].name=$1.name;
       $$.keyBindings[0].value=$1.value;
       $$.keyBindings[0].type=$1.type;
       $$.keyBindings[0].ref=$1.ref;
    }
    | keyBindings keyBinding
    {
       $$.keyBindings[$$.next].name=$2.name;
       $$.keyBindings[$$.next].value=$2.value;
       $$.keyBindings[$$.next].type=$2.type;
       $$.keyBindings[$$.next].ref=$2.ref;
       $$.next++;
    }
;

keyBinding
    : XTOK_KEYBINDING XTOK_KEYVALUE ZTOK_KEYVALUE ZTOK_KEYBINDING
    {
       $$.name=$1.name;
       $$.value=$2.value;
       $$.type=$2.valueType;
    }
    | XTOK_KEYBINDING valueReference ZTOK_KEYBINDING
    {
       $$.name=$1.name;
       $$.value=NULL;
       $$.type="ref";
       $$.ref=$2;
    }
;

%%
