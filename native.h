
/*
 * native.h
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
 * Header file for the encapsulated CMPI data type implementation.
 *
*/

#ifndef CMPI_NATIVE_DATA_H
#define CMPI_NATIVE_DATA_H

#define NATIVE_FT_VERSION 1

#define CMPI_VERSION 90

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"
#include "msgqueue.h"

#include "support.h"

//! Forward declaration for anonymous struct.
struct native_property;


//! Function table for native_property handling functions.
/*!
  This structure holds all the function pointers necessary to handle
  linked lists of native_property structs.

  \sa propertyFT in native.h
*/
struct native_propertyFT {

   //! Adds a new native_property to a list.
   int (*addProperty) (struct native_property **,
                       int,
                       const char *, CMPIType, CMPIValueState, CMPIValue *);

   //! Resets the values of an existing native_property, if existant.
   int (*setProperty) (struct native_property *,
                       int, const char *, CMPIType, CMPIValue *);

   //! Looks up a specifix native_property in CMPIData format.
    CMPIData(*getDataProperty) (struct native_property *,
                                const char *, CMPIStatus *);

   //! Extract an indexed native_property in CMPIData format.
    CMPIData(*getDataPropertyAt) (struct native_property *,
                                  unsigned int, CMPIString **, CMPIStatus *);

   //! Yields the number of native_property items in a list.
    CMPICount(*getPropertyCount) (struct native_property *, CMPIStatus *);

   //! Releases a complete list of native_property items.
   void (*release) (struct native_property *);

   //! Clones a complete list of native_property items.
   struct native_property *(*clone) (struct native_property *, CMPIStatus *);
};


struct _NativeCMPIBrokerFT {
   CMPIBrokerFT brokerFt;
   CMPIArray *(*getKeyNames) (CMPIBroker * broker,
                              CMPIContext * context, CMPIObjectPath * cop,
                              CMPIStatus * rc);
   CMPIString *(*getMessage) (CMPIBroker * mb, const char *msgId,
                              const char *defMsg, CMPIStatus * rc,
                              unsigned int count, va_list);
    CMPIBoolean(*classPathIsA) (CMPIBroker * broker, CMPIObjectPath * cop,
                                const char *type, CMPIStatus * rc);
};

typedef struct _NativeCMPIBrokerFT NativeCMPIBrokerFT;


/****************************************************************************/

void native_release_CMPIValue(CMPIType, CMPIValue * val);
CMPIValue native_clone_CMPIValue(CMPIType, CMPIValue * val, CMPIStatus *);

CMPIString *native_new_CMPIString(const char *, CMPIStatus *);

CMPIArray *internal_new_CMPIArray(int mode, CMPICount size, CMPIType type,
                                  CMPIStatus *);
CMPIArray *NewCMPIArray(CMPICount size, CMPIType type, CMPIStatus *);
CMPIArray *TrackedCMPIArray(CMPICount size, CMPIType type, CMPIStatus *);

void native_array_increase_size(CMPIArray *, CMPICount);
CMPIResult *native_new_CMPIResult(int, int, CMPIStatus *);
CMPIArray *native_result2array(CMPIResult *);

CMPIEnumeration *native_new_CMPIEnumeration(CMPIArray *, CMPIStatus *);

CMPIInstance *NewCMPIInstance(CMPIObjectPath *, CMPIStatus *);
CMPIInstance *TrackedCMPIInstance(CMPIObjectPath *, CMPIStatus *);
CMPIInstance *internal_new_CMPIInstance(int mode, CMPIObjectPath *,
                                        CMPIStatus *);

CMPIObjectPath *NewCMPIObjectPath(const char *, const char *, CMPIStatus *);
CMPIObjectPath *TrackedCMPIObjectPath(const char *, const char *, CMPIStatus *);
CMPIObjectPath *interal_new_CMPIObjectPath(int mode, const char *, const char *,
                                           CMPIStatus *);

CMPIArgs *NewCMPIArgs(CMPIStatus *);
CMPIArgs *TrackedCMPIArgs(CMPIStatus *);

CMPIDateTime *native_new_CMPIDateTime(CMPIStatus *);
CMPIDateTime *native_new_CMPIDateTime_fromBinary(CMPIUint64,
                                                 CMPIBoolean, CMPIStatus *);
CMPIDateTime *native_new_CMPIDateTime_fromChars(const char *, CMPIStatus *);
CMPISelectExp *native_new_CMPISelectExp(const char *,
                                        const char *,
                                        CMPIArray **, CMPIStatus *);
                                        
CMPIContext *native_new_CMPIContext(int mem_state, void*);
void native_release_CMPIContext(CMPIContext *);
CMPIContext *native_clone_CMPIContext(CMPIContext *ctx);

extern CMPIBrokerExtFT *CMPI_BrokerExt_Ftab;

MsgSegment setObjectPathMsgSegment(CMPIObjectPath * op);
CMPIInstance *relocateSerializedInstance(void *area);
CMPIObjectPath *relocateSerializedObjectPath(void *area);


/****************************************************************************/

extern CMPIBrokerEncFT native_brokerEncFT;
extern CMPIBrokerEncFT *BrokerEncFT;
extern CMPIBrokerFT *RequestFT;

//struct native_propertyFT propertyFT;

#endif

/*** Local Variables:  ***/
/*** mode: C           ***/
/*** c-basic-offset: 8 ***/
/*** End:              ***/
