/*
 * elementCapabilitiesProvider.c
 *
 * (C) Copyright IBM Corp. 2009
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Sean Swehla <smswehla@us.ibm.com>
 *
 * Description:
 *
 * ElementCapabilities provider for sfcb. This file has been created
 * explicitly to serve an instance of SFCB_IndiciationServiceCapabilities.
 * The initial implementation of this simply returns CMPI_RC_OK for each
 * of the function calls and relies on internal provider to do all of
 * of the real work. The important part of this file is the InitInstances
 * function, which is hooked by the CreateMI call. This will create the
 * static instance of the association the first time the provider is
 * loaded.
 *
 */

#include "utilft.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "constClass.h"
#include "providerRegister.h"
#include "trace.h"
#include "native.h"
#include "control.h"
#include "config.h"

#define NEW(x) ((x *) malloc(sizeof(x)))

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpiftx.h"
#include "cmpimacs.h"
#include "cmpimacsx.h"


static const CMPIBroker *_broker;
static CMPIStatus invClassSt = { CMPI_RC_ERR_INVALID_CLASS, NULL };
static CMPIStatus notSuppSt = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
static CMPIStatus okSt = { CMPI_RC_OK, NULL };

void ElementCapabilitiesInitInstances(const CMPIContext * ctx) {
  CMPIObjectPath * op = NULL,
                 * opLeft = NULL,
                 * opRight = NULL,
                 * left = NULL,
                 * right = NULL;
  CMPIValue val;
  CMPIEnumeration * enm = NULL;
  CMPIInstance * ci = NULL;
  CMPIContext * ctxLocal;

  ctxLocal = native_clone_CMPIContext(ctx);
  val.string = sfcb_native_new_CMPIString("$DefaultProvider$", NULL,0);
  ctxLocal->ft->addEntry(ctxLocal, "rerouteToProvider", &val, CMPI_string);

  op=CMNewObjectPath(_broker,"root/interop","SFCB_ElementCapabilities",NULL);
  ci=CMNewInstance(_broker,op,NULL);

  opLeft = CMNewObjectPath(_broker,"root/interop","CIM_IndicationService",NULL); 
  enm = CBEnumInstanceNames(_broker, ctx, opLeft, NULL);
  left = CMGetNext(enm, NULL).value.ref;
  opRight = CMNewObjectPath(_broker,"root/interop","SFCB_IndicationServiceCapabilities",NULL); 
  enm = CBEnumInstanceNames(_broker, ctx, opRight, NULL);
  right = CMGetNext(enm, NULL).value.ref;

  CMAddKey(op,"ManagedElement",&left,CMPI_ref);
  CMAddKey(op,"Capabilities",&right,CMPI_ref);
  CMSetProperty(ci,"ManagedElement",&left,CMPI_ref);
  CMSetProperty(ci,"Capabilities",&right,CMPI_ref);
  CBCreateInstance(_broker, ctxLocal, op, ci, NULL);
  return;
}

CMPIStatus ElementCapabilitiesAssociators(CMPIAssociationMI * mi,
                                       const CMPIContext * ctx,
                                       const CMPIResult * rslt,
                                       const CMPIObjectPath * cop,
                                       const char *assocClass,
                                       const char *resultClass,
                                       const char *role,
                                       const char *resultRole,
                                       const char **propertyList)
{
    return okSt;
}

CMPIStatus ElementCapabilitiesAssociatorNames(CMPIAssociationMI * mi,
                                           const CMPIContext * ctx,
                                           const CMPIResult * rslt,
                                           const CMPIObjectPath * cop,
                                           const char *assocClass,
                                           const char *resultClass,
                                           const char *role,
                                           const char *resultRole)
{
  return okSt;
}

CMPIStatus ElementCapabilitiesReferences(
	CMPIAssociationMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const char * resultClass,
	const char * role,
	const char ** propertyList)
{
  return okSt;
}

CMPIStatus ElementCapabilitiesReferenceNames(
	CMPIAssociationMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const char * resultClass,
	const char * role)
{
  return okSt;
}

CMPIStatus ElementCapabilitiesAssociationCleanup(CMPIAssociationMI * mi, const CMPIContext * ctx, CMPIBoolean terminate)
{
  return okSt;
}

  static CMPIAssociationMIFT assocMIFT__ElementCapabilities={
   CMPICurrentVersion,
   CMPICurrentVersion,
   "associationElementCapabilities",
   ElementCapabilitiesAssociationCleanup,
   ElementCapabilitiesAssociators,
   ElementCapabilitiesAssociatorNames,
   ElementCapabilitiesReferences,
   ElementCapabilitiesReferenceNames,
  };
  CMPI_EXTERN_C
  CMPIAssociationMI* ElementCapabilities_Create_AssociationMI(const CMPIBroker* brkr,const CMPIContext *ctx,  CMPIStatus *rc) {
   static CMPIAssociationMI mi={
      NULL,
      &assocMIFT__ElementCapabilities,
   };
   _broker=brkr;
   ElementCapabilitiesInitInstances(ctx);
   return &mi;
  }

 	  	 
