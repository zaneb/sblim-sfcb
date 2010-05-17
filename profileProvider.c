
/*
 * profileProvider.c
 *
 * (C) Copyright IBM Corp. 2008
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:     Chris Buccella <buccella@linux.vnet.ibm.com>
 *
 * Description:
 *
 * A provider for sfcb implementing CIM_RegisteredProfile
 *
 * Based on the InteropProvider by Adrian Schuur
 *
*/


#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "fileRepository.h"
#include "utilft.h"
#include "trace.h"
#include "providerMgr.h"
#include "internalProvider.h"
#include "native.h"
#include "objectpath.h"
#include <time.h>

#define LOCALCLASSNAME "ProfileProvider"

static const CMPIBroker *_broker;

static CMPIContext* prepareUpcall(CMPIContext *ctx)
{
  /* used to invoke the internal provider in upcalls, otherwise we will
   * be routed here again*/
  CMPIContext *ctxLocal;
  ctxLocal = native_clone_CMPIContext(ctx);
  CMPIValue val;
  val.string = sfcb_native_new_CMPIString("$DefaultProvider$", NULL,0);
  ctxLocal->ft->addEntry(ctxLocal, "rerouteToProvider", &val, CMPI_string);
  return ctxLocal;
}

/* --------------------------------------------------------------------------*/
/*                       Instance Provider Interface                         */
/* --------------------------------------------------------------------------*/
 
CMPIStatus ProfileProviderCleanup(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx, CMPIBoolean terminate)
{
  CMPIStatus st = { CMPI_RC_OK, NULL };
  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderCleanup");
  _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus ProfileProviderEnumInstanceNames(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * ref)
{
  CMPIStatus st = { CMPI_RC_OK, NULL };
  CMPIEnumeration *enm;
  CMPIContext *ctxLocal;
  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderEnumInstanceNames");

  ctxLocal = prepareUpcall((CMPIContext *)ctx);
  enm = _broker->bft->enumerateInstanceNames(_broker, ctxLocal, ref, &st);
  CMRelease(ctxLocal);
                                      
  while(enm && enm->ft->hasNext(enm, &st)) {
    CMReturnObjectPath(rslt, (enm->ft->getNext(enm, &st)).value.ref);   
  }
  if(enm) CMRelease(enm);
  _SFCB_RETURN(st);   
}

/* ------------------------------------------------------------------------- */

CMPIStatus ProfileProviderEnumInstances(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * ref,
	const char ** properties)
{
  CMPIStatus st = { CMPI_RC_OK, NULL };
  CMPIEnumeration *enm;
  CMPIContext *ctxLocal;
  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderEnumInstances");

  ctxLocal = prepareUpcall((CMPIContext *)ctx);
  enm = _broker->bft->enumerateInstances(_broker, ctxLocal, ref, properties, &st);
  CMRelease(ctxLocal);
                                      
  while(enm && enm->ft->hasNext(enm, &st)) {
    CMReturnInstance(rslt, (enm->ft->getNext(enm, &st)).value.inst);   
  }   
  if(enm) CMRelease(enm);
  _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus ProfileProviderGetInstance(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const char ** properties)
{

  CMPIStatus st = { CMPI_RC_OK, NULL };
  CMPIContext *ctxLocal;
  CMPIInstance *ci;
   
  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderGetInstance");

  ctxLocal = prepareUpcall((CMPIContext *)ctx);

  ci = _broker->bft->getInstance(_broker, ctxLocal, cop, properties, &st);
  if (st.rc==CMPI_RC_OK) {
    CMReturnInstance(rslt, ci);
  }
   
  CMRelease(ctxLocal);

  _SFCB_RETURN(st);

}

/* ------------------------------------------------------------------------- */

CMPIStatus ProfileProviderCreateInstance(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const CMPIInstance * ci)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIContext *ctxLocal;

   _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderCreateInstance");
  
   ctxLocal = prepareUpcall((CMPIContext *)ctx);
   CMReturnObjectPath(rslt, _broker->bft->createInstance(_broker, ctxLocal, cop, ci, &st));
   CMRelease(ctxLocal);
    
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus ProfileProviderModifyInstance(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const CMPIInstance * ci,
	const char ** properties)
{
  CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };	
  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderModifyInstance");
  _SFCB_RETURN(st);   
}

/* ------------------------------------------------------------------------- */

CMPIStatus ProfileProviderDeleteInstance(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop)
{
  CMPIStatus st = { CMPI_RC_OK, NULL };
  CMPIContext *ctxLocal;

  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderDeleteInstance");

  ctxLocal = prepareUpcall((CMPIContext *)ctx);
  st = _broker->bft->deleteInstance(_broker, ctxLocal, cop);
  CMRelease(ctxLocal);

  _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus ProfileProviderExecQuery(
	CMPIInstanceMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const char * lang,
	const char * query)
{
  CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderExecQuery");
  _SFCB_RETURN(st);
}

CMPIStatus
ProfileProviderInvokeMethod(CMPIMethodMI * mi,
			    const CMPIContext *ctx,
			    const CMPIResult *rslt,
			    const CMPIObjectPath * ref,
			    const char *methodName,
			    const CMPIArgs * in, CMPIArgs * out)
{
  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderInvokeMethod");
  CMPIStatus      st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
  if (strcmp(methodName, "_startup"))
    st.rc = CMPI_RC_OK;
  _SFCB_RETURN(st);
}

CMPIStatus ProfileProviderMethodCleanup(CMPIMethodMI * mi,
                                        const CMPIContext * ctx, CMPIBoolean terminate)
{
  CMPIStatus st = { CMPI_RC_OK, NULL };
  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderMethodCleanup");
  _SFCB_RETURN(st);
}


/* ------------------------------------------------------------------ *
 * Instance MI Factory
 *
 * NOTE: This is an example using the convenience macros. This is OK
 *       as long as the MI has no special requirements, i.e. to store
 *       data between calls.
 * ------------------------------------------------------------------ */

CMInstanceMIStub(ProfileProvider, ProfileProvider, _broker, CMNoHook); 
CMIMethodMIStub(ProfileProvider, ProfileProvider, _broker, CMNoHook); 
