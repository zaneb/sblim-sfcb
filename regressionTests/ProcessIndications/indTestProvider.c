
/*
 * indTestProvider.c.c
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

CMPIInstance *buildIndication(char *ns, char *msg, CMPIUint32 code)
{
   UtilStringBuffer *sb;      
   CMPIStatus st;
   CMPIObjectPath *cop=CMNewObjectPath(_broker,ns,"TST_ProcessIndication",&st);

   CMPIInstance *ind=CMNewInstance(_broker,cop,&st);
 
   CMSetProperty(ind,"msg",msg,CMPI_chars);
   CMSetProperty(ind,"code",&code,CMPI_uint32);
   
   sb=instanceToString(ind,NULL);
     
   return ind;
}


CMPI_THREAD_RETURN indThread(void *parm)
{
   CMPIInstance *ind;
   CMPIContext *ctx=(CMPIContext*)parm;
   CMPIUint32 count=0;
   
   CBAttachThread(_broker,ctx);
   
   for (; activeFilters; ) {
      sleep(5);
      if (activeFilters && enabled) {
         ind=buildIndication("root/interop","indication",count++);
         CBDeliverIndication(_broker,ctx,nameSpace,ind);
      }   
   }
   return NULL;
}

/* ------------------------------------------------------------------ *
 * Indication MI Cleanup
 * ------------------------------------------------------------------ */

CMPIStatus IndTestProviderIndicationCleanup(CMPIIndicationMI * mi, 
                                              CMPIContext * ctx)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, "IndTestProviderIndicationCleanup");   
       printf("cleanup ??\n");
  _SFCB_RETURN(st);
}


/* ------------------------------------------------------------------ *
 * Indication MI Functions
 * ------------------------------------------------------------------ */


CMPIStatus IndTestProviderAuthorizeFilter(CMPIIndicationMI * mi,
                                             CMPIContext * ctx,
                                             CMPIResult * rslt,
                                             CMPISelectExp* se, 
                                             const char *ns, 
                                             CMPIObjectPath* op, 
                                             const char *user)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, "IndTestProviderAuthorizeFilter");
   printf("IndTestProviderAuthorizeFilter\n");
   _SFCB_RETURN(st);
} 


CMPIStatus IndTestProviderMustPoll(CMPIIndicationMI * mi,
                                         CMPIContext * ctx,
                                         CMPIResult * rslt,
                                         CMPISelectExp* se, 
                                         const char *ns, 
                                         CMPIObjectPath* op)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, "IndTestProviderMustPoll");
   CMPIBoolean false=0;
   
   CMReturnData(rslt,&false,CMPI_boolean);
   printf("IndTestProviderMustPoll\n");
   _SFCB_RETURN(st);
}


CMPIStatus IndTestProviderActivateFilter(CMPIIndicationMI * mi,
                                       CMPIContext * ctx,
                                       CMPIResult * rslt,
                                       CMPISelectExp* se, 
                                       const char *ns, 
                                       CMPIObjectPath* op, 
                                       CMPIBoolean first)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   
   _SFCB_ENTER(TRACE_PROVIDERS, "IndTestProviderActivateFilter");
   printf("IndTestProviderActivateFilter %d\n",currentProc);
   
   if (activeFilters==0) {
      CMPIContext *indCtx=CBPrepareAttachThread(_broker,ctx);
      activeFilters++;
      nameSpace=(char*)ns;
      _broker->xft->newThread(indThread,indCtx,0);
   }   
   _SFCB_RETURN(st);    
}


CMPIStatus IndTestProviderDeActivateFilter(CMPIIndicationMI * mi,
                                          CMPIContext * ctx,
                                          CMPIResult * rslt,
                                          CMPISelectExp* se, 
                                          const char *ns, 
                                          CMPIObjectPath* op, 
                                          CMPIBoolean last)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, "IndTestProviderDeActivateFilter");
   printf("IndTestProviderDeActivateFilter\n");
   if (activeFilters) activeFilters--;
  _SFCB_RETURN(st);
}


void IndTestProviderEnableIndications(CMPIIndicationMI * mi)
{
   _SFCB_ENTER(TRACE_PROVIDERS, "IndTestProviderEnableIndications");
   enabled=1;
   _SFCB_EXIT();
}


void IndTestProviderDisableIndications(CMPIIndicationMI * mi)
{
    _SFCB_ENTER(TRACE_PROVIDERS, "IndTestProviderDisableIndications");
   _SFCB_EXIT();
}

static void initProvider(CMPIBroker *broker, CMPIContext *ctx)
{   
}

CMIndicationMIStub(IndTestProvider, IndTestProvider, _broker, initProvider(brkr,ctx));

