
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

#ifdef HAVE_SLP
#include "cimslp.h"
#include "cimslpCMPI.h"
#include "control.h"
pthread_t       slpUpdateThread;
pthread_once_t  slpUpdateInitMtx = PTHREAD_ONCE_INIT;
pthread_mutex_t slpUpdateMtx = PTHREAD_MUTEX_INITIALIZER;
int             slpLifeTime = SLP_LIFETIME_DEFAULT;
// This is an awfully brutish way
// to track two adapters.
char           *http_url = NULL;
char           *http_attr = "NULL";
char           *https_url = NULL;
char           *https_attr = "NULL";
#endif

#define LOCALCLASSNAME "ProfileProvider"

/*
 * ------------------------------------------------------------------------- 
 */

static const CMPIBroker *_broker;

/*
 * ------------------------------------------------------------------------- 
 */

CMPIContext *
prepareUpcall(const CMPIContext *ctx)
{
  /*
   * used to invoke the internal provider in upcalls, otherwise we will be 
   * routed here again
   */
  CMPIContext    *ctxLocal;
  ctxLocal = native_clone_CMPIContext(ctx);
  CMPIValue       val;
  val.string = sfcb_native_new_CMPIString("$DefaultProvider$", NULL, 0);
  ctxLocal->ft->addEntry(ctxLocal, "rerouteToProvider", &val, CMPI_string);
  return ctxLocal;
}


/*
 * --------------------------------------------------------------------------
 */
/*
 * Instance Provider Interface 
 */
/*
 * --------------------------------------------------------------------------
 */

CMPIStatus
ProfileProviderCleanup(CMPIInstanceMI * mi,
                       const CMPIContext *ctx, CMPIBoolean terminate)
{
  CMPIStatus      st = { CMPI_RC_OK, NULL };
  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderCleanup");
#ifdef HAVE_SLP
  // Tell SLP update thread that we're shutting down
  _SFCB_TRACE(1, ("--- Stopping SLP thread"));
  pthread_kill(slpUpdateThread, SIGUSR2);
  // Wait for thread to complete
  pthread_join(slpUpdateThread, NULL);
  _SFCB_TRACE(1, ("--- SLP Thread stopped"));
#endif // HAVE_SLP
  _SFCB_RETURN(st);
}

/*
 * ------------------------------------------------------------------------- 
 */

CMPIStatus
ProfileProviderEnumInstanceNames(CMPIInstanceMI * mi,
                                 const CMPIContext *ctx,
                                 const CMPIResult *rslt,
                                 const CMPIObjectPath * ref)
{
  CMPIStatus      st = { CMPI_RC_OK, NULL };
  CMPIEnumeration *enm;
  CMPIContext    *ctxLocal;
  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderEnumInstanceNames");

  ctxLocal = prepareUpcall((CMPIContext *) ctx);
  enm = _broker->bft->enumerateInstanceNames(_broker, ctxLocal, ref, &st);
  CMRelease(ctxLocal);

  while (enm && enm->ft->hasNext(enm, &st)) {
    CMReturnObjectPath(rslt, (enm->ft->getNext(enm, &st)).value.ref);
  }
  if (enm)
    CMRelease(enm);
  _SFCB_RETURN(st);
}

/*
 * ------------------------------------------------------------------------- 
 */

CMPIStatus
ProfileProviderEnumInstances(CMPIInstanceMI * mi,
                             const CMPIContext *ctx,
                             const CMPIResult *rslt,
                             const CMPIObjectPath * ref,
                             const char **properties)
{
  CMPIStatus      st = { CMPI_RC_OK, NULL };
  CMPIEnumeration *enm;
  CMPIContext    *ctxLocal;
  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderEnumInstances");

  ctxLocal = prepareUpcall((CMPIContext *) ctx);
  enm =
      _broker->bft->enumerateInstances(_broker, ctxLocal, ref, properties,
                                       &st);
  CMRelease(ctxLocal);

  while (enm && enm->ft->hasNext(enm, &st)) {
    CMReturnInstance(rslt, (enm->ft->getNext(enm, &st)).value.inst);
  }
  if (enm)
    CMRelease(enm);
  _SFCB_RETURN(st);
}

/*
 * ------------------------------------------------------------------------- 
 */

CMPIStatus
ProfileProviderGetInstance(CMPIInstanceMI * mi,
                           const CMPIContext *ctx,
                           const CMPIResult *rslt,
                           const CMPIObjectPath * cop,
                           const char **properties)
{

  CMPIStatus      st = { CMPI_RC_OK, NULL };
  CMPIContext    *ctxLocal;
  CMPIInstance   *ci;

  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderGetInstance");

  ctxLocal = prepareUpcall((CMPIContext *) ctx);

  ci = _broker->bft->getInstance(_broker, ctxLocal, cop, properties, &st);
  if (st.rc == CMPI_RC_OK) {
    CMReturnInstance(rslt, ci);
  }

  CMRelease(ctxLocal);

  _SFCB_RETURN(st);

}

/*
 * ------------------------------------------------------------------------- 
 */

CMPIStatus
ProfileProviderCreateInstance(CMPIInstanceMI * mi,
                              const CMPIContext *ctx,
                              const CMPIResult *rslt,
                              const CMPIObjectPath * cop,
                              const CMPIInstance *ci)
{
  CMPIStatus      st = { CMPI_RC_OK, NULL };
  CMPIContext    *ctxLocal;
  //cimomConfig     cfg;

  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderCreateInstance");

  ctxLocal = prepareUpcall((CMPIContext *) ctx);
  CMReturnObjectPath(rslt,
                     _broker->bft->createInstance(_broker, ctxLocal, cop,
                                                  ci, &st));
  CMRelease(ctxLocal);
#ifdef HAVE_SLP
  //updateSLPRegistration
  updateSLPReg(ctx, slpLifeTime);
#endif // HAVE_SLP

  _SFCB_RETURN(st);
}

/*
 * ------------------------------------------------------------------------- 
 */

CMPIStatus
ProfileProviderModifyInstance(CMPIInstanceMI * mi,
                              const CMPIContext *ctx,
                              const CMPIResult *rslt,
                              const CMPIObjectPath * cop,
                              const CMPIInstance *ci,
                              const char **properties)
{
  CMPIStatus      st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderModifyInstance");
  _SFCB_RETURN(st);
}

/*
 * ------------------------------------------------------------------------- 
 */

CMPIStatus
ProfileProviderDeleteInstance(CMPIInstanceMI * mi,
                              const CMPIContext *ctx,
                              const CMPIResult *rslt,
                              const CMPIObjectPath * cop)
{
  CMPIStatus      st = { CMPI_RC_OK, NULL };
  CMPIContext    *ctxLocal;

  _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderDeleteInstance");

  ctxLocal = prepareUpcall((CMPIContext *) ctx);
  st = _broker->bft->deleteInstance(_broker, ctxLocal, cop);
  CMRelease(ctxLocal);
#ifdef HAVE_SLP
  //updateSLPRegistration
  updateSLPReg(ctx, slpLifeTime);
#endif // HAVE_SLP

  _SFCB_RETURN(st);
}

/*
 * ------------------------------------------------------------------------- 
 */

CMPIStatus
ProfileProviderExecQuery(CMPIInstanceMI * mi,
                         const CMPIContext *ctx,
                         const CMPIResult *rslt,
                         const CMPIObjectPath * cop,
                         const char *lang, const char *query)
{
  CMPIStatus      st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
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


#ifdef HAVE_SLP
#define UPDATE_SLP_REG  spawnUpdateThread(ctx)
void 
updateSLPReg(const CMPIContext *ctx, int slpLifeTime)
{
  cimSLPService   service;
  cimomConfig     cfgHttp,
                  cfgHttps;
  int             enableHttp,
                  enableHttps = 0;
  int             enableSlp = 0;
  long            i;
  int             errC = 0;

  extern char    *configfile;

  _SFCB_ENTER(TRACE_SLP, "updateSLPReg");

  pthread_mutex_lock(&slpUpdateMtx);

  getControlBool("enableSlp", &enableSlp);
  if(!enableSlp) {
    _SFCB_TRACE(1, ("--- SLP disabled"));
    pthread_mutex_unlock(&slpUpdateMtx);
    _SFCB_EXIT();
  }
  setUpDefaults(&cfgHttp);
  setUpDefaults(&cfgHttps);

//  sleep(1);
  getControlBool("enableHttp", &enableHttp);
  if (enableHttp) {
    getControlNum("httpPort", &i);
    free(cfgHttp.port);
    cfgHttp.port = malloc(6 * sizeof(char));    // portnumber has max. 5
                                                // digits
    sprintf(cfgHttp.port, "%d", (int) i);
    service = getSLPData(cfgHttp, _broker, ctx, http_url);
    if((errC = registerCIMService(service, slpLifeTime,
                                  &http_url, &http_attr)) != 0) {
      _SFCB_TRACE(1, ("--- Error registering http with SLP: %d", errC));
    }
  }
  getControlBool("enableHttps", &enableHttps);
  if (enableHttps) {
    free(cfgHttps.commScheme);
    cfgHttps.commScheme = strdup("https");
    getControlNum("httpsPort", &i);
    free(cfgHttps.port);
    cfgHttps.port = malloc(6 * sizeof(char));   // portnumber has max. 5
                                                // digits 
    sprintf(cfgHttps.port, "%d", (int) i);
    getControlChars("sslClientTrustStore", &cfgHttps.trustStore);
    getControlChars("sslCertificateFilePath", &cfgHttps.certFile);
    getControlChars("sslKeyFilePath", &cfgHttps.keyFile);

    service = getSLPData(cfgHttps, _broker, ctx, https_url);
    if((errC = registerCIMService(service, slpLifeTime,
                                  &https_url, &https_attr)) != 0) {
      _SFCB_TRACE(1, ("--- Error registering https with SLP: %d", errC));
    }
  }
  
  freeCFG(&cfgHttp);
  freeCFG(&cfgHttps);
  pthread_mutex_unlock(&slpUpdateMtx);
  return;
}

static int slp_shutting_down = 0;

void
handle_sig_slp(int signum)
{
  //Indicate that slp is shutting down
  slp_shutting_down = 1;
}

void
slpUpdateInit(void)
{
  slpUpdateThread = pthread_self();
}

void *
slpUpdate(void *args)
{
  int             sleepTime;
  long            i;
  extern char    *configfile;
  int             enableSlp = 0;

  // set slpUpdateThread to appropriate thread info
  pthread_once(&slpUpdateInitMtx, slpUpdateInit);
  // exit thread if another already exists
  if(!pthread_equal(slpUpdateThread, pthread_self())) return NULL;
  _SFCB_ENTER(TRACE_SLP, "slpUpdate");
  //Setup signal handlers
  struct sigaction sa;
  sa.sa_handler = handle_sig_slp;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGUSR2, &sa, NULL);
 
  //Get context from args
  CMPIContext *ctx = (CMPIContext *)args;
  // Get enableSlp config value
  setupControl(configfile);
  getControlBool("enableSlp", &enableSlp);
  // If enableSlp is false, we don't really need this thread
  if(!enableSlp) {
    _SFCB_TRACE(1, ("--- SLP disabled in config. Update thread not starting."));
    _SFCB_RETURN(NULL);
  }
  //Get configured value for refresh interval
  getControlNum("slpRefreshInterval", &i);
  slpLifeTime = (int) i;
  setUpTimes(&slpLifeTime, &sleepTime);
  
  //Start update loop
  while(1) {
    //Update reg
    updateSLPReg(ctx, slpLifeTime);
    //Sleep for refresh interval
    int timeLeft = sleep(sleepTime);
    if(slp_shutting_down) break;
    _SFCB_TRACE(4, ("--- timeLeft: %d, slp_shutting_down: %s\n",
            timeLeft, slp_shutting_down ? "true" : "false"));
  }
  //End loop
  CMRelease(ctx);
  if(http_url) {
    _SFCB_TRACE(2, ("--- Deregistering http advertisement"));
    deregisterCIMService(http_url);
  }
  if(https_url) {
    _SFCB_TRACE(2, ("--- Deregistering https advertisement"));
    deregisterCIMService(https_url);
  }
  _SFCB_RETURN(NULL);
}

void
spawnUpdateThread(const CMPIContext *ctx)
{
  pthread_attr_t  attr;
  pthread_t       newThread;
  int             rc = 0;
  //CMPIStatus      st = { 0 , NULL };
  void           *thread_args = NULL;
  thread_args = (void *)native_clone_CMPIContext(ctx);

  // create a thread
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  rc = pthread_create(&newThread, &attr, slpUpdate, thread_args);
  if(rc) {
    // deal with thread creation error
    exit(1);
  }
}

#else // no HAVE_SLP
#define UPDATE_SLP_REG CMNoHook
#endif // HAVE_SLP
/*
 * ------------------------------------------------------------------ *
 * Instance MI Factory NOTE: This is an example using the convenience
 * macros. This is OK as long as the MI has no special requirements, i.e.
 * to store data between calls.
 * ------------------------------------------------------------------ 
 */

//CMInstanceMIStub(ProfileProvider, ProfileProvider, _broker, CMNoHook);
CMInstanceMIStub(ProfileProvider, ProfileProvider, _broker, UPDATE_SLP_REG);
CMMethodMIStub(ProfileProvider, ProfileProvider, _broker, CMNoHook);

/* MODELINES */
/* DO NOT EDIT BELOW THIS COMMENT */
/* Modelines are added by 'make pretty' */
/* -*- Mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* vi:set ts=2 sts=2 sw=2 expandtab: */
