
/*
 * providerRegister.h
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
 * Provider registration support.
 *
*/


#ifndef _ProviderRegister_h_
#define _ProviderRegister_h_

#include <stdio.h>
#include <sys/types.h>
#include "native.h"
#include "msgqueue.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct providerProcess;

typedef union provIds {
   void *ids;
   struct { 
      short procId;
      short provId;
   } ;  
} ProvIds; 

typedef struct _ProviderInfo {
   char *className;
   unsigned long type;
   char *providerName;
   char *location;
   char *group;
   char **ns;
   int id;
   pid_t pid;
   void *library;
   ComSockets providerSockets;
   ProvIds provIds;
   int unload,initialized;
   pthread_t idleThread;
   pthread_mutex_t initMtx;
   time_t lastActivity;
   struct _ProviderInfo *next;
   struct providerProcess *proc;
   CMPIInstanceMI *instanceMI;
   CMPIAssociationMI *associationMI;
   CMPIMethodMI *methodMI;
   CMPIIndicationMI *indicationMI;
   CMPIPropertyMI *propertyMI;
   CMPIClassMI *classMI;
} ProviderInfo;

#define INSTANCE_PROVIDER       1
#define ASSOCIATION_PROVIDER    2
#define INDICATION_PROVIDER     4
#define METHOD_PROVIDER         8
#define PROPERTY_PROVIDER       16
#define CLASS_PROVIDER          32
#define FORCE_PROVIDER_NOTFOUND 128


#ifdef __cplusplus
}
#endif
#endif                          // _ProviderRegister_h_
