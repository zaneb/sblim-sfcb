/*
 * cimslpCMPI.h
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
 * Functions getting slp relevant information from the CIMOM utilizing sfcc
 *
 */

#ifndef _cimslpCMPI_h
#define _cimslpCMPI_h

#include <unistd.h>
#include <stdio.h>
#include "cmpidt.h"
#include "cmpimacs.h"

typedef struct {
  char           *url_syntax;
  char           *service_hi_name;
  char           *service_hi_description;
  char           *service_id;
  char           *CommunicationMechanism;
  char           *OtherCommunicationMechanismDescription;
  char          **InteropSchemaNamespace;
  char           *ProtocolVersion;
  char          **FunctionalProfilesSupported;
  char          **FunctionalProfileDescriptions;
  char           *MultipleOperationsSupported;
  char          **AuthenticationMechanismsSupported;
  char          **AuthenticationMechansimDescriptions;
  char          **Namespace;
  char          **Classinfo;
  char          **RegisteredProfilesSupported;
} cimSLPService;

typedef struct {
  char           *commScheme;   // http or https
  char           *cimhost;
  char           *port;
  char           *cimuser;
  char           *cimpassword;
  char           *trustStore;
  char           *certFile;
  char           *keyFile;
} cimomConfig;

extern char    *value2Chars(CMPIType type, CMPIValue * value);

void            initializeService(cimSLPService * rs);
cimSLPService   getSLPData(cimomConfig cfg, const CMPIBroker *_broker,
                           const CMPIContext *ctx,
                           const char *urlsyntax);
char           *myGetProperty(CMPIInstance *instance, char *propertyName);
char          **myGetPropertyArray(CMPIInstance *instance,
                                   char *propertyName);
char          **myGetPropertyArrayFromArray(CMPIInstance **instances,
                                            char *propertyName);
CMPIInstance  **myGetInstances(const CMPIBroker *_broker,
                               const CMPIContext * ctx,
                               const char *path,
                               const char *objectname,
                               const char *urlsyntax);
char           *transformValue(char *cssf, CMPIObjectPath * op,
                               char *propertyName);
char          **transformValueArray(char **cssf, CMPIObjectPath * op,
                                    char *propertyName);
char          **myGetRegProfiles(const CMPIBroker *_broker,
                                 CMPIInstance **instances,
                                 const CMPIContext * ctx);
char          **getInterOpNS();
char           *getUrlSyntax(char *sn, char *cs, char *port);
void            updateSLPReg(const CMPIContext *ctx, int slpLifeTime);

#endif
/* MODELINES */
/* DO NOT EDIT BELOW THIS COMMENT */
/* Modelines are added by 'make pretty' */
/* -*- Mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* vi:set ts=2 sts=2 sw=2 expandtab: */
