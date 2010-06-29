/*
 * cimslp.c
 *
 * (C) Copyright IBM Corp. 2006
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:        Sven Schuetz <sven@de.ibm.com>
 * Contributions:
 *
 * Description:
 *
 * Control functions, main if running standlone, or start thread
 * function if running in sfcb
 *
 */

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include "config.h"
#include "cimslp.h"
#include "trace.h"

#ifdef HAVE_SLP
#include "control.h"
#endif

extern void     addStartedAdapter(int pid);

void
freeCFG(cimomConfig * cfg)
{

  free(cfg->cimhost);
  free(cfg->cimpassword);
  free(cfg->cimuser);
  free(cfg->commScheme);
  free(cfg->port);
}

void
setUpDefaults(cimomConfig * cfg)
{
  cfg->commScheme = strdup("http");
  cfg->cimhost = strdup("localhost");
  cfg->port = strdup("5988");
  cfg->cimuser = strdup("");
  cfg->cimpassword = strdup("");
  cfg->keyFile = NULL;
  cfg->trustStore = NULL;
  cfg->certFile = NULL;
}

void
setUpTimes(int *slpLifeTime, int *sleepTime)
{
  if (*slpLifeTime < 16) {
    *slpLifeTime = 16;
  }
  if (*slpLifeTime > SLP_LIFETIME_MAXIMUM) {
    *slpLifeTime = SLP_LIFETIME_DEFAULT;
  }

  *sleepTime = *slpLifeTime - 15;
}

static void
handleSigUsr1(int sig)
{
  //deregisterCIMService();
  mlogf(M_INFO, M_SHOW, "--- %s terminating %d\n", processName, getpid());
  exit(0);
}

static void
handleSigHup(int sig)
{
  mlogf(M_INFO, M_SHOW, "--- %s restarting %d\n", processName, getpid());
}

#ifdef HAVE_SLP

void
forkSLPAgent(cimomConfig cfg, int slpLifeTime, int sleepTime)
{
  //cimSLPService   service;
  int             pid;
  pid = fork();
  if (pid < 0) {
    char           *emsg = strerror(errno);
    mlogf(M_ERROR, M_SHOW, "-#- slp agent fork: %s", emsg);
    exit(2);
  }
  if (pid == 0) {
    setSignal(SIGUSR1, handleSigUsr1, 0);
    setSignal(SIGINT, SIG_IGN, 0);
    setSignal(SIGTERM, SIG_IGN, 0);
    setSignal(SIGHUP, handleSigHup, 0);
    if (strcasecmp(cfg.commScheme, "http") == 0) {
      processName = "SLP Agent for HTTP Adapter";
    } else {
      processName = "SLP Agent for HTTPS Adapter";
    }
    while (1) {
      //service = getSLPData(cfg);
      //registerCIMService(service, slpLifeTime);
      sleep(sleepTime);
    }
    /*
     * if awaked-exit
     */
    exit(0);
  } else {
    slppid = pid;
    //addStartedAdapter(pid);
  }
}

int
slpAgent()
{
  int             slpLifeTime = SLP_LIFETIME_DEFAULT;
  int             sleepTime;
  cimomConfig     cfgHttp,
                  cfgHttps;
  int             enableHttp,
                  enableHttps = 0;

  extern char    *configfile;

  _SFCB_ENTER(TRACE_SLP, "slpAgent");

  setupControl(configfile);

  setUpDefaults(&cfgHttp);
  setUpDefaults(&cfgHttps);

  sleep(1);

  long            i;

  if (!getControlBool("enableHttp", &enableHttp)) {
    getControlNum("httpPort", &i);
    free(cfgHttp.port);
    cfgHttp.port = malloc(6 * sizeof(char));    // portnumber has max. 5
    // digits
    sprintf(cfgHttp.port, "%d", (int) i);
  }
  if (!getControlBool("enableHttps", &enableHttps)) {
    free(cfgHttps.commScheme);
    cfgHttps.commScheme = strdup("https");
    getControlNum("httpsPort", &i);
    free(cfgHttps.port);
    cfgHttps.port = malloc(6 * sizeof(char));   // portnumber has max. 5
    // digits 
    sprintf(cfgHttps.port, "%d", (int) i);
    getControlChars("sslClientTrustStore", &cfgHttps.trustStore);
    getControlChars("sslCertificateFilePath:", &cfgHttps.certFile);
    getControlChars("sslKeyFilePath", &cfgHttps.keyFile);
  }

  getControlNum("slpRefreshInterval", &i);
  slpLifeTime = (int) i;
  setUpTimes(&slpLifeTime, &sleepTime);

/* Disable slp registration agent for now,
 * since it will be going away soon anyhow.
  if (enableHttp)
    forkSLPAgent(cfgHttp, slpLifeTime, sleepTime);
  if (enableHttps)
    forkSLPAgent(cfgHttps, slpLifeTime, sleepTime);
*/
  
  freeCFG(&cfgHttp);
  freeCFG(&cfgHttps);
  _SFCB_RETURN(0);
}
#endif

/* MODELINES */
/* DO NOT EDIT BELOW THIS COMMENT */
/* Modelines are added by 'make pretty' */
/* -*- Mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* vi:set ts=2 sts=2 sw=2 expandtab: */
