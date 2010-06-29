/*
 * cimslpCMPI.c
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

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "cimslpCMPI.h"
#include "cimslpSLP.h"
#include "cimslpUtil.h"
#include "trace.h"
#include "config.h"
#include "control.h"

char           *interOpNS;
typedef int     (*getSlpHostname) (char **hostname);
extern void     libraryName(const char *dir, const char *location,
                            const char *fullName, int buf_size);
extern char    *configfile;
CMPIContext    *prepareUpcall(const CMPIContext *ctx);

// helper function ... until better solution is found to get to the
// interop Namespace
char          **
getInterOpNS()
{
  char          **retArr;

  interOpNS = "root/interop";

  retArr = malloc(2 * sizeof(char *));
  retArr[0] = (char *) malloc((strlen(interOpNS) + 1) * sizeof(char));
  strcpy(retArr[0], interOpNS);
  retArr[1] = NULL;

  return retArr;
}

/* this is pulled from SFCC; 
   TODO: combine this with SFCC's (and value.c's as well)
 */
char * value2Chars (CMPIType type, CMPIValue *value)
{
  char str[2048], *p;
  CMPIString *cStr;

  str[0]=0;
  if (type & CMPI_ARRAY) {
  }
  else if (type & CMPI_ENC) {

    switch (type) {
    case CMPI_instance:
      break;

    case CMPI_ref:
      if (value->ref) {
	cStr=value->ref->ft->toString(value->ref,NULL);
	p = strdup((char *) cStr->hdl);
	CMRelease(cStr);
      } else {
	p = strdup("NULL");
      }

      return p;

    case CMPI_args:
      break;

    case CMPI_filter:
      break;

    case CMPI_string:
    case CMPI_numericString:
    case CMPI_booleanString:
    case CMPI_dateTimeString:
    case CMPI_classNameString:
      return strdup((value->string && value->string->hdl) ?
		    (char*)value->string->hdl : "NULL");

    case CMPI_dateTime:
      if (value->dateTime) {
	cStr=CMGetStringFormat(value->dateTime,NULL);
	p = strdup((char *) cStr->hdl);
	CMRelease(cStr);
      } else
	p = strdup("NULL");
      return p;
    }

  }
  else if (type & CMPI_SIMPLE) {

    switch (type) {
    case CMPI_boolean:
      return strdup(value->boolean ? "true" : "false");

    case CMPI_char16:
      break;
    }

  }
  else if (type & CMPI_INTEGER) {

    switch (type) {
    case CMPI_uint8:
      sprintf(str, "%u", value->uint8);
      return strdup(str);
    case CMPI_sint8:
      sprintf(str, "%d", value->sint8);
      return strdup(str);
    case CMPI_uint16:
      sprintf(str, "%u", value->uint16);
      return strdup(str);
    case CMPI_sint16:
      sprintf(str, "%d", value->sint16);
      return strdup(str);
    case CMPI_uint32:
      sprintf(str, "%u", value->uint32);
      return strdup(str);
    case CMPI_sint32:
      sprintf(str, "%d", value->sint32);
      return strdup(str);
    case CMPI_uint64:
      sprintf(str, "%llu", value->uint64);
      return strdup(str);
    case CMPI_sint64:
      sprintf(str, "%lld", value->sint64);
      return strdup(str);
    }

  }
  else if (type & CMPI_REAL) {

    switch (type) {
    case CMPI_real32:
      sprintf(str, "%g", value->real32);
      return strdup(str);
    case CMPI_real64:
      sprintf(str, "%g", value->real64);
      return strdup(str);
    }

  }
  return strdup(str);
}


CMPIInstance  **
myGetInstances(const CMPIBroker *_broker, const CMPIContext * ctx,
               const char *path, const char *objectname,
               const char *urlsyntax)
{
  CMPIStatus      status;
  CMPIObjectPath *objectpath;
  CMPIEnumeration *enumeration;
  CMPIInstance  **retArr = NULL;

  _SFCB_ENTER(TRACE_SLP, "myGetInstances");

  objectpath = CMNewObjectPath(_broker, path, objectname, NULL);

  enumeration = CBEnumInstances(_broker, ctx, objectpath, NULL, &status);

  // severe error
  if (status.rc == CMPI_RC_ERR_FAILED) {
    printf
        ("--- Error enumerating %s. Deregistering service with slp\n",
         objectname);
    deregisterCIMService(urlsyntax);
    if (status.msg)
      CMRelease(status.msg);
    if (objectpath)
      CMRelease(objectpath);
    if (enumeration)
      CMRelease(enumeration);
    exit(0);
  }
  // object not found ?
  if (status.rc == CMPI_RC_ERR_INVALID_CLASS
      || status.rc == CMPI_RC_ERR_NOT_FOUND) {
    retArr = NULL;
  }

  if (!status.rc) {
    if (CMHasNext(enumeration, NULL)) {
      CMPIArray      *arr;
      int             n,
                      i;

      arr = CMToArray(enumeration, NULL);
      n = CMGetArrayCount(arr, NULL);
      retArr = malloc(sizeof(CMPIInstance *) * (n + 1));
      for (i = 0; i < n; i++) {
        CMPIData        ele = CMGetArrayElementAt(arr, i, NULL);
        retArr[i] = CMClone(ele.value.inst, NULL);
      }
      retArr[n] = NULL;
    }
  }
  if (status.msg)
    CMRelease(status.msg);
  if (objectpath)
    CMRelease(objectpath);
  if (enumeration)
    CMRelease(enumeration);
  _SFCB_RETURN(retArr);
}

/*
CMPIConstClass *
myGetClass(CMPIContext * ctx, char *path, char *objectname)
{
  CMPIStatus      status;
  CMPIObjectPath *objectpath;
  CMPIConstClass *ccls;

  _SFCB_ENTER(TRACE_SLP, "myGetClass");

  objectpath = newCMPIObjectPath(path, objectname, &status);

  ccls =
      cc->ft->getClass(cc, objectpath, CMPI_FLAG_IncludeQualifiers, NULL,
                       &status);

  if (objectpath)
    CMRelease(objectpath);

  if (!status.rc) {
    _SFCB_RETURN(ccls);
  } else {
    // printf("Could not get class... ?\n");
    // printf("Status: %d\n", status.rc);
    _SFCB_RETURN(NULL);
  }

}
*/

cimSLPService
getSLPData(cimomConfig cfg, const CMPIBroker *_broker,
           const CMPIContext *ctx, const char *urlsyntax)
{
  CMPIInstance  **ci;
  //CMPIStatus      status;
  //CMPIConstClass *ccls;
  cimSLPService   rs;           // service which is going to be returned
  // to the calling function
  char           *sn;

#ifdef SLP_HOSTNAME_LIB
  static void    *hostnameLib = NULL;
  static getSlpHostname gethostname;
  char           *ln;
  char            dlName[512];
  int             err;

  err = 1;
  if (getControlChars("slpHostnamelib", &ln) == 0) {
    libraryName(NULL, ln, dlName, 512);
    if ((hostnameLib = dlopen(dlName, RTLD_LAZY))) {
      gethostname = dlsym(hostnameLib, "_sfcGetSlpHostname");
      if (gethostname)
        err = 0;
    }
  }
  if (err)
    mlogf(M_ERROR, M_SHOW,
          "--- SLP Hostname exit %s not found. Defaulting to system hostname.\n",
          dlName);
#endif

  _SFCB_ENTER(TRACE_SLP, "getSLPData");

  memset(&rs, 0, sizeof(cimSLPService));

  // first of all, get the interop namespace, needed for all further
  // connections
  // this call fills the array as well as sets the global interOpNS
  // variable
  rs.InteropSchemaNamespace = getInterOpNS();

  // extract all relavant stuff for SLP out of CIM_ObjectManager

  // construct the server string
  ci = myGetInstances(_broker, ctx, interOpNS, "CIM_ObjectManager",
                      urlsyntax);
  if (ci) {
    sn = myGetProperty(ci[0], "SystemName");

#ifdef SLP_HOSTNAME_LIB
    if (!err) {
      char           *tmp;
      if ((err = gethostname(&tmp))) {
        free(sn);
        sn = tmp;
      } else {
        printf
            ("-#- SLP call to %s for hostname failed. Defaulting to system hostname.\n",
             dlName);
      }
    }
#endif
    rs.url_syntax = getUrlSyntax(sn, cfg.commScheme, cfg.port);
    rs.service_hi_name = myGetProperty(ci[0], "ElementName");
    rs.service_hi_description = myGetProperty(ci[0], "Description");
    rs.service_id = myGetProperty(ci[0], "Name");
    freeInstArr(ci);
  }
  // extract all relavant stuff for SLP out of
  // CIM_ObjectManagerCommunicationMechanism
  ci = myGetInstances(_broker, ctx, interOpNS,
                      "CIM_ObjectManagerCommunicationMechanism",
                      urlsyntax);
  if (ci) {
    rs.CommunicationMechanism =
        myGetProperty(ci[0], "CommunicationMechanism");
    rs.OtherCommunicationMechanismDescription =
        myGetProperty(ci[0], "OtherCommunicationMechanism");
    rs.ProtocolVersion = myGetProperty(ci[0], "Version");
    rs.FunctionalProfilesSupported =
        myGetPropertyArray(ci[0], "FunctionalProfilesSupported");
    rs.FunctionalProfileDescriptions =
        myGetPropertyArray(ci[0], "FunctionalProfileDescriptions");
    rs.MultipleOperationsSupported =
        myGetProperty(ci[0], "MultipleOperationsSupported");
    rs.AuthenticationMechanismsSupported =
        myGetPropertyArray(ci[0], "AuthenticationMechanismsSupported");
    rs.AuthenticationMechansimDescriptions =
        myGetPropertyArray(ci[0], "AuthenticationMechansimDescriptions");
    // do the transformations from numbers to text via the qualifiers
    //CMPIStatus myRc;
    //CMPIObjectPath *myOp = CMGetObjectPath(ci[0], &myRc);
    //CMPIData qd = CMGetPropertyQualifier(myOp, "CommunicationMechanism", "ValueMap", &myRc);
    rs.CommunicationMechanism = transformValue(rs.CommunicationMechanism,
                                               CMGetObjectPath(ci[0], NULL),
                                               "CommunicationMechanism");
    rs.FunctionalProfilesSupported =
        transformValueArray(rs.FunctionalProfilesSupported,
                            CMGetObjectPath(ci[0], NULL),
                            "FunctionalProfilesSupported");
    rs.AuthenticationMechanismsSupported =
        transformValueArray(rs.AuthenticationMechanismsSupported,
                            CMGetObjectPath(ci[0], NULL),
                            "AuthenticationMechanismsSupported");
    freeInstArr(ci);
  }
  // extract all relavant stuff for SLP out of CIM_Namespace
  ci = myGetInstances(_broker, ctx, interOpNS, "CIM_Namespace", urlsyntax);
  if (ci) {
    rs.Namespace = myGetPropertyArrayFromArray(ci, "Name");
    rs.Classinfo = myGetPropertyArrayFromArray(ci, "ClassInfo");
    freeInstArr(ci);
  }
  // extract all relavant stuff for SLP out of CIM_RegisteredProfile
  //CMPIContext *ctxLocal = prepareUpcall(ctx);
  //ci = myGetInstances(_broker, ctxLocal, interOpNS, "CIM_RegisteredProfile", urlsyntax);
  ci = myGetInstances(_broker, ctx, interOpNS, "CIM_RegisteredProfile", urlsyntax);
  if (ci) {
    rs.RegisteredProfilesSupported = myGetRegProfiles(_broker, ci, ctx);
    //rs.RegisteredProfilesSupported = myGetRegProfiles(_broker, ci, ctxLocal);
    freeInstArr(ci);
  }
  //CMRelease(ctxLocal);

  _SFCB_RETURN(rs);

}

char          **
myGetRegProfiles(const CMPIBroker *_broker, CMPIInstance **instances,
                 const CMPIContext * ctx)
{
  CMPIObjectPath *objectpath;
  CMPIEnumeration *enumeration = NULL;
  CMPIStatus      status;
  char          **retArr;
  int             i,
                  j = 0;

  _SFCB_ENTER(TRACE_SLP, "myGetRegProfiles");

  // count instances
  for (i = 0; instances != NULL && instances[i] != NULL; i++) {
  }

  if (i == 0) {
    _SFCB_RETURN(NULL);;
  }
  // allocating memory for the return array
  // a little too much memory will be allocated, since not each instance
  // is a RegisteredProfile, for which a
  // string needs to be constructed ... but allocating dynamically would
  // involve too much burden and overhead (?)

  retArr = (char **) malloc((i + 1) * sizeof(char *));

  for (i = 0; instances[i] != NULL; i++) {

    // check to see if this profile wants to be advertised
    CMPIArray      *atArray;
    atArray =
        CMGetProperty(instances[i], "AdvertiseTypes", &status).value.array;
    if (status.rc == CMPI_RC_ERR_NO_SUCH_PROPERTY || atArray == NULL
        || CMGetArrayElementAt(atArray, 0, &status).value.uint16 != 3) {

      _SFCB_TRACE(1,
                  ("--- profile does not want to be advertised; skipping"));
      continue;
    }

    objectpath = instances[i]->ft->getObjectPath(instances[i], &status);
    if (status.rc) {
      // no object path ??
      free(retArr);
      _SFCB_RETURN(NULL);
    }
    objectpath->ft->setNameSpace(objectpath, interOpNS);

    if (enumeration)
      CMRelease(enumeration);
    enumeration = CBAssociatorNames(_broker, ctx,
                                    objectpath,
                                    "CIM_SubProfileRequiresProfile",
                                    NULL, "Dependent", NULL, NULL);

    // if the result is not null, we are operating on a
    // CIM_RegisteredSubProfile, which we don't want
    if (!enumeration || !CMHasNext(enumeration, &status)) {
      CMPIData        propertyData;

      propertyData = instances[i]->ft->getProperty(instances[i],
                                                   "RegisteredOrganization",
                                                   &status);
      char           *profilestring;
      profilestring = value2Chars(propertyData.type, &propertyData.value);

      profilestring =
          transformValue(profilestring, CMGetObjectPath(instances[i], NULL),
                         "RegisteredOrganization");

      propertyData = instances[i]->ft->getProperty(instances[i],
                                                   "RegisteredName",
                                                   &status);

      char           *tempString =
          value2Chars(propertyData.type, &propertyData.value);

      profilestring =
          realloc(profilestring,
                  strlen(profilestring) + strlen(tempString) + 2);
      profilestring = strcat(profilestring, ":");
      profilestring = strcat(profilestring, tempString);
      free(tempString);

      // now search for a CIM_RegisteredSubProfile for this instance
      if (enumeration)
        CMRelease(enumeration);
      enumeration = CBAssociators(_broker, ctx,
                                  objectpath,
                                  "CIM_SubProfileRequiresProfile",
                                  NULL,
                                  "Antecedent", NULL, NULL, NULL);
      if (!enumeration || !CMHasNext(enumeration, NULL)) {
        retArr[j] = strdup(profilestring);
        j++;
      } else
        while (CMHasNext(enumeration, &status)) {
          CMPIData        data =
              CMGetNext(enumeration, NULL);
          propertyData =
              CMGetProperty(data.value.inst,
                            "RegisteredName", &status);
          char           *subprofilestring = value2Chars(propertyData.type,
                                                         &propertyData.
                                                         value);
          retArr[j] =
              (char *) malloc(strlen(profilestring) +
                              strlen(subprofilestring) + 2);
          sprintf(retArr[j], "%s:%s", profilestring, subprofilestring);
          j++;
          free(subprofilestring);
        }
      free(profilestring);
    }
    if (objectpath)
      CMRelease(objectpath);
  }
  retArr[j] = NULL;

  if (enumeration)
    CMRelease(enumeration);
  if (status.msg)
    CMRelease(status.msg);

  _SFCB_RETURN(retArr);
}

char          **
transformValueArray(char **cssf, CMPIObjectPath * op, char *propertyName)
{
  int             i;

  for (i = 0; cssf[i] != NULL; i++) {
    cssf[i] = transformValue(cssf[i], op, propertyName);
  }
  return cssf;
}

// transforms numerical values into their string counterpart
// utilizing the Values and ValueMap qualifiers
char           *
transformValue(char *cssf, CMPIObjectPath * op, char *propertyName)
// cssf = cimSLPService Field in the struct
{
  CMPIData        qd;
  CMPIStatus      status;
  char           *valuestr;

  _SFCB_ENTER(TRACE_SLP, "transformValue");

  qd = CMGetPropertyQualifier(op, propertyName, "ValueMap", &status);
  if (status.rc) {
    printf("getPropertyQualifier failed ... Status: %d\n", status.rc);
    _SFCB_RETURN(NULL);
  }

  if (CMIsArray(qd)) {
    CMPIArray      *arr = qd.value.array;
    CMPIType        eletyp = qd.type & ~CMPI_ARRAY;
    int             j = 0;
    int             n;
    n = CMGetArrayCount(arr, NULL);
    CMPIData        ele;
    ele = CMGetArrayElementAt(arr, j, NULL);
    valuestr = value2Chars(eletyp, &ele.value);
    j++;
    while (strcmp(valuestr, cssf)) {
      free(valuestr);
      ele = CMGetArrayElementAt(arr, j, NULL);
      valuestr = value2Chars(eletyp, &ele.value);
      if (j == n) {
        free(valuestr);
        _SFCB_RETURN(cssf);     // nothing found, probably "NULL" ->
        // return it
      }
      j++;
    }
    free(valuestr);
    free(cssf);
    if (j - 1 <= n) {
      qd = CMGetPropertyQualifier(op, propertyName, "Values",
                                          &status);
      arr = qd.value.array;
      eletyp = qd.type & ~CMPI_ARRAY;
      ele = CMGetArrayElementAt(arr, j - 1, NULL);
      cssf = value2Chars(eletyp, &ele.value);
      _SFCB_RETURN(cssf);
    } else {
      // printf("No Valuemap Entry for %s in %s. Exiting ...\n", cssf,
      // propertyName);
      _SFCB_RETURN(NULL);
    }
  }

  else {
    // printf("No qualifier found for %s. Exiting ...\n", propertyName);
    _SFCB_RETURN(NULL);
  }
}

char           *
myGetProperty(CMPIInstance *instance, char *propertyName)
{

  CMPIData        propertyData;
  CMPIStatus      status;

  if (!instance)
    return NULL;

  propertyData =
      instance->ft->getProperty(instance, propertyName, &status);

  if (!status.rc) {
    return value2Chars(propertyData.type, &propertyData.value);
  } else {
    return NULL;
  }
}

char          **
myGetPropertyArrayFromArray(CMPIInstance **instances, char *propertyName)
{
  int             i;
  char          **propertyArray;

  // count elements
  for (i = 0; instances != NULL && instances[i] != NULL; i++) {
  }

  if (i == 0) {
    return NULL;
  }

  propertyArray = malloc((i + 1) * sizeof(char *));

  for (i = 0; instances[i] != NULL; i++) {
    propertyArray[i] = myGetProperty(instances[i], propertyName);
  }
  propertyArray[i] = NULL;
  return propertyArray;

}

char          **
myGetPropertyArray(CMPIInstance *instance, char *propertyName)
{

  CMPIData        propertyData;
  CMPIStatus      status;
  char          **propertyArray = NULL;

  propertyData =
      instance->ft->getProperty(instance, propertyName, &status);
  if (!status.rc && propertyData.state != CMPI_nullValue) {
    if (CMIsArray(propertyData)) {
      CMPIArray      *arr = propertyData.value.array;
      CMPIType        eletyp = propertyData.type & ~CMPI_ARRAY;
      int             n,
                      i;
      n = CMGetArrayCount(arr, NULL);
      propertyArray = malloc(sizeof(char *) * (n + 1));
      for (i = 0; i < n; i++) {
        CMPIData        ele = CMGetArrayElementAt(arr, i, NULL);
        propertyArray[i] = value2Chars(eletyp, &ele.value);
      }
      propertyArray[n] = NULL;

    }
  }
  return propertyArray;
}

char           *
getUrlSyntax(char *sn, char *cs, char *port)
{
  char           *url_syntax;

  // colon, double slash, colon, \0, service:wbem = 18
  url_syntax =
      (char *) malloc((strlen(sn) + strlen(cs) + strlen(port) + 18) *
                      sizeof(char));
  sprintf(url_syntax, "%s://%s:%s", cs, sn, port);

  free(sn);

  return url_syntax;
}
/* MODELINES */
/* DO NOT EDIT BELOW THIS COMMENT */
/* Modelines are added by 'make pretty' */
/* -*- Mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* vi:set ts=2 sts=2 sw=2 expandtab: */
