
/**
 * <br><br>
 * IBM Confidential
 * OCO Source Materials
 * Samll Footprint Cim Broker (sfcb)
 * (C) Copyright IBM Corp. 2004
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U. S. Copyright Office.
 *
 * @author:
 * @version
 *
 * NAME:
 * CLASS DESCRIPTION:
 * USAGE:
 *
 * Change History
 * Change Flag  Date     Prog    Description
 *------------------------------------------------------------------------------
 *
 */
 
#define CMPI_VERSION 90 

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpios.h"
#include "cmpimacs.h"
#include "utilft.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "trace.h"


static CMPIBroker *_broker;
UtilStringBuffer *instanceToString(CMPIInstance * ci, char **props);

static int enabled=0;
static int activeFilters=0;
static char* nameSpace=NULL;

static void showStatus(CMPIStatus *st, char *msg)
{
   char *m=NULL;
   if (st->msg) m=(char*)st->msg->hdl;
   printf("--- showStatus (%s): %d %s\n",msg,st->rc,m);
}   


CMPIInstance *buildIndication(char *ns, CMPIInstance *ci)
{
   UtilStringBuffer *sb;      
   CMPIStatus st;
   CMPIObjectPath *cop=CMNewObjectPath(_broker,ns,"CIM_InstCreation",&st);

   CMPIInstance *ind=CMNewInstance(_broker,cop,&st);
 
   st=CMSetProperty(ind,"SourceInstance",&ci,CMPI_instance);
   // showStatus(&st,"Setting sourceInstance");
   
   sb=instanceToString(ind,NULL);
     
   return ind;
}


CMPI_THREAD_RETURN indThread(void *parm)
{
   CMPIInstance *ind;
   CMPIContext *ctx=(CMPIContext*)parm;
   CMPIUint32 count=0;
   CMPIInstance *ei=NULL;
   CMPIStatus st;
   CMPIObjectPath *cop;   
   
   CBAttachThread(_broker,ctx);
   
   cop=CMNewObjectPath(_broker,nameSpace,"TST_LifeCycleIndication",&st);
   for (; activeFilters; ) {
      sleep(5);
      if (activeFilters && enabled) {
         count++;
         ei=CMNewInstance(_broker,cop,&st);
         CMSetProperty(ei,"id",&count,CMPI_sint32);
         CMSetProperty(ei,"msg","test message",CMPI_chars);
         ind=buildIndication("root/interop",ei);
         CBDeliverIndication(_broker,ctx,nameSpace,ind);
      }   
   }
   return NULL;
}

/* ------------------------------------------------------------------ *
 * Indication MI Cleanup
 * ------------------------------------------------------------------ */

CMPIStatus  CreateInstIndTestProviderIndicationCleanup(CMPIIndicationMI * mi, 
                                              CMPIContext * ctx)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, " CreateInstIndTestProviderIndicationCleanup");   
       printf("cleanup ??\n");
  _SFCB_RETURN(st);
}


/* ------------------------------------------------------------------ *
 * Indication MI Functions
 * ------------------------------------------------------------------ */


CMPIStatus  CreateInstIndTestProviderAuthorizeFilter(CMPIIndicationMI * mi,
                                             CMPIContext * ctx,
                                             CMPIResult * rslt,
                                             CMPISelectExp* se, 
                                             const char *type, 
                                             CMPIObjectPath* op, 
                                             const char *user)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, " CreateInstIndTestProviderAuthorizeFilter");
   printf(" CreateInstIndTestProviderAuthorizeFilter\n");
   _SFCB_RETURN(st);
}


CMPIStatus  CreateInstIndTestProviderMustPoll(CMPIIndicationMI * mi,
                                         CMPIContext * ctx,
                                         CMPIResult * rslt,
                                         CMPISelectExp* se, 
                                         const char *type, 
                                         CMPIObjectPath* op)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, " CreateInstIndTestProviderMustPoll");
   CMPIBoolean false=0;
   
   CMReturnData(rslt,&false,CMPI_boolean);
   printf(" CreateInstIndTestProviderMustPoll\n");
   _SFCB_RETURN(st);
}


CMPIStatus  CreateInstIndTestProviderActivateFilter(CMPIIndicationMI * mi,
                                       CMPIContext * ctx,
                                       CMPIResult * rslt,
                                       CMPISelectExp* se, 
                                       const char *type, 
                                       CMPIObjectPath* op, 
                                       CMPIBoolean first)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIString *ns=CMGetNameSpace(op,NULL);
   
   _SFCB_ENTER(TRACE_PROVIDERS, " CreateInstIndTestProviderActivateFilter");
   printf(" CreateInstIndTestProviderActivateFilter %d-%s-%s\n",currentProc,type,(char*)ns->hdl);
   
   if (activeFilters==0) {
      CMPIContext *indCtx=CBPrepareAttachThread(_broker,ctx);
      activeFilters++;
      nameSpace=(char*)ns->hdl;
      _broker->xft->newThread(indThread,indCtx,0);
   }   
   _SFCB_RETURN(st);    
}


CMPIStatus  CreateInstIndTestProviderDeActivateFilter(CMPIIndicationMI * mi,
                                          CMPIContext * ctx,
                                          CMPIResult * rslt,
                                          CMPISelectExp* se, 
                                          const char *type, 
                                          CMPIObjectPath* op, 
                                          CMPIBoolean last)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, " CreateInstIndTestProviderDeActivateFilter");
   printf(" CreateInstIndTestProviderDeActivateFilter\n");
   if (activeFilters) activeFilters--;
  _SFCB_RETURN(st);
}


void  CreateInstIndTestProviderEnableIndications(CMPIIndicationMI * mi)
{
   _SFCB_ENTER(TRACE_PROVIDERS, " CreateInstIndTestProviderEnableIndications");
   enabled=1;
   _SFCB_EXIT();
}


void  CreateInstIndTestProviderDisableIndications(CMPIIndicationMI * mi)
{
    _SFCB_ENTER(TRACE_PROVIDERS, " CreateInstIndTestProviderDisableIndications");
   _SFCB_EXIT();
}

static void initProvider(CMPIBroker *broker, CMPIContext *ctx)
{   
}

CMPIStatus CreateInstIndTestProviderCleanup( CMPIInstanceMI * mi, 
           CMPIContext * ctx) { 

  CMReturn(CMPI_RC_OK);
}


CMPIStatus CreateInstIndTestProviderEnumInstanceNames( CMPIInstanceMI * mi, 
					   CMPIContext * ctx, 
					   CMPIResult * rslt, 
					   CMPIObjectPath * ref) { 
  
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 

  return rc;
}

CMPIStatus CreateInstIndTestProviderEnumInstances( CMPIInstanceMI * mi, 
						    CMPIContext * ctx, 
						    CMPIResult * rslt, 
						    CMPIObjectPath * ref, 
						    char ** properties) { 
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 

}

CMPIStatus CreateInstIndTestProviderGetInstance( CMPIInstanceMI * mi, 
				     CMPIContext * ctx, 
				     CMPIResult * rslt, 
				     CMPIObjectPath * cop, 
				     char ** properties) {  
  
  CMPIStatus           rc    = {CMPI_RC_OK, NULL};
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  return rc;
}



CMPIStatus CreateInstIndTestProviderCreateInstance( CMPIInstanceMI * mi, 
					CMPIContext * ctx, 
					CMPIResult * rslt, 
					CMPIObjectPath * cop, 
					CMPIInstance * ci) {
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  
  
  return rc;
}

CMPIStatus CreateInstIndTestProviderSetInstance( CMPIInstanceMI * mi, 
				     CMPIContext * ctx, 
				     CMPIResult * rslt, 
				     CMPIObjectPath * cop,
				     CMPIInstance * ci, 
				     char **properties) {

  CMPIStatus           rc    = {CMPI_RC_OK, NULL};
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  
  return rc;
}


CMPIStatus CreateInstIndTestProviderDeleteInstance( CMPIInstanceMI * mi, 
					CMPIContext * ctx, 
					CMPIResult * rslt, 
					CMPIObjectPath * cop) {

  CMPIStatus rc = {CMPI_RC_OK, NULL}; 
  
  
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  
  return rc;
}


CMPIStatus CreateInstIndTestProviderExecQuery( CMPIInstanceMI * mi, 
				   CMPIContext * ctx, 
				   CMPIResult * rslt, 
				   CMPIObjectPath * ref, 
				   char * lang, 
				   char * query) {
  
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  
  return rc;
}




/* Our provider is currently Instance Provider only */

CMInstanceMIStub( CreateInstIndTestProvider, 
                  CreateInstIndTestProvider, 
                  _broker, 
                  CMNoHook);
CMIndicationMIStub( CreateInstIndTestProvider,  CreateInstIndTestProvider, _broker, initProvider(brkr,ctx));

