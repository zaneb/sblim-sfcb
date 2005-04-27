
/*
 * providerMgr.h
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
 * Based on concepts developed by Viktor Mihajlovski <mihajlov@de.ibm.com>
 *
 * Description:
 *
 * Provider manager support.
 *
*/


#ifndef providerMgr_h
#define providerMgr_h

#include "msgqueue.h"
#include "httpComm.h"
#include "providerRegister.h"

#define OPS_GetClass 1
#define OPS_GetInstance 2
#define OPS_DeleteClass 3
#define OPS_DeleteInstance 4
#define OPS_CreateClass 5
#define OPS_CreateInstance 6
#define OPS_ModifyClass 7
#define OPS_ModifyInstance 8
#define OPS_EnumerateClasses 9
#define OPS_EnumerateClassNames 10
#define OPS_EnumerateInstances 11
#define OPS_EnumerateInstanceNames 12
#define OPS_ExecQuery 13
#define OPS_Associators 14
#define OPS_AssociatorNames 15
#define OPS_References 16
#define OPS_ReferenceNames 17
#define OPS_GetProperty 18
#define OPS_SetProperty 19
#define OPS_GetQualifier 20
#define OPS_SetQualifier 21
#define OPS_DeleteQualifier 22
#define OPS_EnumerateQualifiers 23
#define OPS_InvokeMethod 24

#define OPS_LoadProvider 25
#define OPS_PingProvider 26

#define OPS_IndicationLookup   27
#define OPS_ActivateFilter     28
#define OPS_DeactivateFilter   29
#define OPS_DisableIndications 30
#define OPS_EnableIndications  31

#define BINREQ(oper,count) {{oper,0,NULL,0,count}}

typedef struct operationHdr {
   unsigned long type;
   unsigned long count;         // maps to MsgList
   MsgSegment nameSpace;
   MsgSegment className;
   union {
   MsgSegment resultClass;
      MsgSegment query;
   };
   union {
   MsgSegment role;
      MsgSegment queryLang;
   };
   MsgSegment assocClass;
   MsgSegment resultRole;
} OperationHdr;

typedef struct binRequestHdr {
   unsigned short operation;
   unsigned short options;
   void *provId;
   unsigned int flags;
   unsigned long count;         // maps to MsgList
   MsgSegment object[1];
} BinRequestHdr;

typedef struct binResponseHdr {
   long rc;
   CMPIData rv;   //need to check for string returns
   MsgSegment rvEnc;
   unsigned char rvValue,chunkedMode,moreChunks;
   unsigned long count;         // maps to MsgList
   MsgSegment object[1];
} BinResponseHdr;

struct chunkFunctions;
struct commHndl;
struct requestHdr;

typedef struct provAddr {
   int socket;
   ProvIds ids;
} ProvAddr;

typedef struct binRequestContext {
   OperationHdr *oHdr;
   BinRequestHdr *bHdr;
   struct requestHdr *rHdr;
   unsigned long bHdrSize;
   int requestor;
   int chunkedMode,xmlAs,noResp;
   struct chunkFunctions *chunkFncs;
   struct commHndl *commHndl;
   CMPIType type;
   ProvAddr provA; 
   ProvAddr* pAs;
   unsigned long pCount,pDone;
   unsigned long rCount;
   int rc;
   MsgXctl *ctlXdata;
} BinRequestContext;

#define XML_asObj 1
#define XML_asClassName 2
#define XML_asClass 4

typedef struct chunkFunctions {
   void (*writeChunk)(BinRequestContext*,BinResponseHdr*);
} ChunkFunctions;


typedef union getClassReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
      MsgSegment properties[1];
   };
} GetClassReq;

typedef union enumClassNamesReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
   };
} EnumClassNamesReq;

typedef union enumClassesReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
   };
} EnumClassesReq;

typedef union enumInstanceNamesReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
   };
} EnumInstanceNamesReq;

typedef union enumInstancesReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
      MsgSegment properties[1];
  };
} EnumInstancesReq;

typedef union execQueryReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
      MsgSegment query;
      MsgSegment queryLang;
  };
} ExecQueryReq;

typedef union associatorsReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
      MsgSegment resultClass;
      MsgSegment role;
      MsgSegment assocClass;
      MsgSegment resultRole;
      MsgSegment properties[1];
   };
} AssociatorsReq;

typedef union referencesReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
      MsgSegment resultClass;
      MsgSegment role;
      MsgSegment properties[1];
   };
} ReferencesReq;

typedef union associatorNamesReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
      MsgSegment resultClass;
      MsgSegment role;
      MsgSegment assocClass;
      MsgSegment resultRole;
   };
} AssociatorNamesReq;

typedef union referenceNamesReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
      MsgSegment resultClass;
      MsgSegment role;
   };
} ReferenceNamesReq;

typedef union getInstanceReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
      MsgSegment properties[1];
   };
} GetInstanceReq;

typedef union createClassReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment path;
      MsgSegment cls;
   };
} CreateClassReq;

typedef union createInstanceReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment path;
      MsgSegment instance;
   };
} CreateInstanceReq;

typedef union modifyInstanceReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment path;
      MsgSegment instance;
      MsgSegment properties[1];
   };
} ModifyInstanceReq;

typedef union deleteInstanceReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
   };
} DeleteInstanceReq;

typedef union deleteClassReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
   };
} DeleteClassReq;

typedef union invokeMethodReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
      MsgSegment method;
      MsgSegment in;
      MsgSegment out;
   };
} InvokeMethodReq;

typedef union loadProviderReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment className;
      MsgSegment libName;
      MsgSegment provName;
      unsigned int unload;
   };
} LoadProviderReq;

typedef union pingProviderReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
   };
} PingProviderReq;

typedef union indicationReq {
   BinRequestHdr hdr;
   struct {
      unsigned short operation;
      unsigned short options;
      void *provId;
      unsigned int flags;
      unsigned long count;      // maps to MsgList
      MsgSegment principal;
      MsgSegment objectPath;
      MsgSegment query;
      MsgSegment language;
      MsgSegment type;
      MsgSegment sns;
      void* filterId;
    };
} IndicationReq;

int getProviderContext(BinRequestContext * ctx, OperationHdr * ohdr);
BinResponseHdr **invokeProviders(BinRequestContext * binCtx, int *err,
                                 int *count);
BinResponseHdr *invokeProvider(BinRequestContext * ctx);

typedef struct providerProcess {
   char *group;
   int pid;
   int id;
   int unload;
   ProviderInfo *firstProv;
   ComSockets providerSockets;
   time_t lastActivity;
} ProviderProcess;

#endif
