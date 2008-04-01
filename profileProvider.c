
/*
 * interopProvider.c
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

/* ------------------------------------------------------------------------- */

extern CMPIInstance *relocateSerializedInstance(void *area);
extern char *sfcb_value2Chars(CMPIType type, CMPIValue * value);
extern CMPIObjectPath *getObjectPath(char *path, char **msg);


extern void closeProviderContext(BinRequestContext* ctx);
extern void setStatus(CMPIStatus *st, CMPIrc rc, char *msg);
extern int testNameSpace(char *ns, CMPIStatus *st);

/* ------------------------------------------------------------------------- */

static const CMPIBroker *_broker;

typedef struct profile {
  CMPIInstance  *pci;
  char          *InstanceID;  /* <OrgID>:<LocalID> */
  unsigned int   RegisteredOrganization;  /* "other" = 1 */
  char          *RegisteredName;   /* maxlen 256 */
  char          *RegisteredVersion;   /* major.minor.update */
} Profile;

static UtilHashTable *profileHt = NULL;

/* ------------------------------------------------------------------------- */

/* checks if an object path is for the /root/interop or /root/pg_interop */
static int interOpNameSpace(
	const CMPIObjectPath * cop,
	CMPIStatus * st) 
 {   
   char *ns = (char*)CMGetNameSpace(cop,NULL)->hdl;   
   if (strcasecmp(ns,"root/interop") && strcasecmp(ns,"root/pg_interop")) {
      if (st) setStatus(st, CMPI_RC_ERR_FAILED, "Object must reside in root/interop");
      return 0;
   }
   return 1;
}
   
/* ------------------------------------------------------------------------- */

static CMPIContext* prepareUpcall(CMPIContext *ctx)
{
    /* used to invoke the internal provider in upcalls, otherwise we will
     * be routed here (interOpProvider) again*/
    CMPIContext *ctxLocal;
    ctxLocal = native_clone_CMPIContext(ctx);
    CMPIValue val;
    val.string = sfcb_native_new_CMPIString("$DefaultProvider$", NULL,0);
    ctxLocal->ft->addEntry(ctxLocal, "rerouteToProvider", &val, CMPI_string);
    return ctxLocal;
}

/* ------------------------------------------------------------------------- */

/* 
 * add a profile to profileHt using the objectpath string as its key.
 * create hashtable if it doesn't exist (should never be the case)
 */
static Profile *addProfile(
	const CMPIInstance * ci,
	const char * key,
	const char * instid,
	const char * regname,
	const int   regorg,
	const char * regvers)
{
   Profile * pi;
      
   _SFCB_ENTER(TRACE_INDPROVIDER, "addProfile");
   
   _SFCB_TRACE(1,("--- Profile: >%s<",instid));
   _SFCB_TRACE(1,("--- Name: >%s<",regname));
   
   if (profileHt==NULL) {
      profileHt=UtilFactory->newHashTable(11,UtilHashTable_charKey);
      profileHt->ft->setReleaseFunctions(profileHt, free, NULL);
   }

   /* check to see if it already exists */
   pi=profileHt->ft->get(profileHt,key);
   if (pi) _SFCB_RETURN(NULL);
   
   pi=(Profile*)malloc(sizeof(Profile));
   pi->pci=CMClone(ci,NULL);
   pi->InstanceID=strdup(instid);
   pi->RegisteredOrganization=regorg;
   pi->RegisteredName=strdup(regname);
   pi->RegisteredVersion=strdup(regvers);
   profileHt->ft->put(profileHt,key,pi);
   _SFCB_RETURN(pi);
}

/* ------------------------------------------------------------------------- */

static Profile *getProfile(
	const char * key)
{
   Profile *prof;

   _SFCB_ENTER(TRACE_INDPROVIDER, "getProfile");

   if (profileHt==NULL) { return NULL; }
   prof=profileHt->ft->get(profileHt,key);

   _SFCB_RETURN(prof);
}

/* ------------------------------------------------------------------------- */

static void removeProfile(
	Profile * prof,
	char * key)
{
   _SFCB_ENTER(TRACE_INDPROVIDER, "removeProfile");

   if (profileHt) {
      profileHt->ft->remove(profileHt,key);
   }
   if (prof) {
      CMRelease(prof->pci);
      free (prof);
   }

   _SFCB_EXIT();
}

/* ------------------------------------------------------------------------- */

static int setProfileProperties(CMPIInstance *in, Profile *prof)
{
  if (in && prof) {

    CMSetProperty(in,"InstanceID",prof->InstanceID,CMPI_chars);
    CMSetProperty(in,"RegisteredName",prof->RegisteredName,CMPI_chars);
    CMSetProperty(in,"RegisteredVersion",prof->RegisteredVersion,CMPI_chars);
    CMSetProperty(in,"RegisteredOrganization",(CMPIValue*)&(prof->RegisteredOrganization),CMPI_uint16);

    return 0;
  } else {
    return -1;
  }
}


/* ------------------------------------------------------------------------- */

extern int isChild(const char *ns, const char *parent, const char* child);

static int isa(const char *sns, const char *child, const char *parent)
{
   int rv;
   _SFCB_ENTER(TRACE_INDPROVIDER, "isa");
   
   if (strcasecmp(child,parent)==0) return 1;
   rv=isChild(sns,parent,child);
   _SFCB_RETURN(rv);
}


/* ------------------------------------------------------------------ *
 * InterOp initialization
 * ------------------------------------------------------------------ */

static void initProfiles(
	const CMPIBroker *broker,
	const CMPIContext *ctx)
{

   CMPIObjectPath *op;
   CMPIEnumeration *enm;
   CMPIInstance *ci;
   CMPIStatus st;
   CMPIObjectPath *cop;
   CMPIContext *ctxLocal;
   char *key,*instid,*regname,*regvers;
   int regorg;
   int pr_exists = 0;
    
   _SFCB_ENTER(TRACE_INDPROVIDER, "initProfiles");

   /* only do this if profileHt hasn't been created and populated */
   if (profileHt == NULL) {
     profileHt=UtilFactory->newHashTable(11,UtilHashTable_charKey);
     _SFCB_TRACE(1,("--- checking for CIM_RegisteredProfile"));
     op=CMNewObjectPath(broker,"root/interop","cim_registeredprofile",&st);
     ctxLocal = prepareUpcall((CMPIContext *)ctx);
     enm = _broker->bft->enumerateInstances(_broker, ctxLocal, op, NULL, &st);
     CMRelease(ctxLocal);
   
     if(enm) {
       while(enm->ft->hasNext(enm, &st) && (ci=(enm->ft->getNext(enm, &st)).value.inst)) {
         cop=CMGetObjectPath(ci,&st);
         instid=(char*)CMGetProperty(ci,"InstanceID",&st).value.string->hdl;
         regname=(char*)CMGetProperty(ci,"RegisteredName",&st).value.string->hdl;
         regorg=CMGetProperty(ci,"RegisteredOrganization",&st).value.uint16;
         regvers=(char*)CMGetProperty(ci,"RegisteredVersion",&st).value.string->hdl;
	 key=normalizeObjectPathCharsDup(cop);
         addProfile(ci,key,instid,regname,regorg,regvers);
	 if (strncmp(instid, "CIM:SFCB_PR", 11) == 0) pr_exists = 1;
       }
       CMRelease(enm);
     }  
   }

   /* Add Profile Registration profile */
   if (!pr_exists) {
     op = CMNewObjectPath(broker,"root/interop","cim_registeredprofile",&st);
     ci = CMNewInstance(_broker, 
			CMNewObjectPath(_broker, "root/interop", "cim_registeredprofile", &st),
			&st);
     Profile* prof = (Profile*)malloc(sizeof(Profile));
     prof->InstanceID = "CIM:SFCB_PR";
     prof->RegisteredOrganization = 2;
     prof->RegisteredName = "Profile Registration";
     prof->RegisteredVersion = "1.0.0";
     CMAddKey(op,"InstanceID",prof->InstanceID,CMPI_chars);
     setProfileProperties(ci, prof);

     _broker->bft->createInstance(_broker, ctx, op, ci, &st);
     free(prof);   
   }

   _SFCB_EXIT(); 
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

   if (interOpNameSpace(ref,NULL)!=1) _SFCB_RETURN(st);
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

   if (interOpNameSpace(ref,NULL)!=1) _SFCB_RETURN(st);
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
   CMPIInstance *ci;
   Profile* prof;
   char* key; /* the requested InstanceID */

   _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderGetInstance");

   /* validate key given */
   //checkProfileKeyValuePair( _BROKER, cop, "InstanceID", &status );
   char * namespace = CMGetCharPtr(CMGetNameSpace(cop, NULL));
   ci = CMNewInstance(_broker, 
                      CMNewObjectPath(_broker, namespace, LOCALCLASSNAME, &st),
                      &st);
   if ((st.rc != CMPI_RC_OK) || CMIsNullObject(ci)) {
      CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
                           "Cannot create new instance");
     return st;
   }

   key = normalizeObjectPathCharsDup(cop);

   /* Get profile information using the requested object path */
   prof = getProfile(key);

   if (prof == NULL) {
     CMSetStatusWithChars(_broker, &st,
			  CMPI_RC_ERR_NOT_FOUND, "This instance does not exist (wrong InstanceID?)" );
     return st;
   }

   if (setProfileProperties(ci, prof) == 0) {
     CMReturnInstance(rslt,ci);
   } else {
     CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
			  "Cannot set instance properties");
   }

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
   CMPIString *cn = CMGetClassName(cop, NULL);
   char *cns = cn->ft->getCharPtr(cn,NULL);
   CMPIString *ns = CMGetNameSpace(cop, NULL);
   char *nss = ns->ft->getCharPtr(ns,NULL);
   CMPIContext *ctxLocal;
   CMPIInstance *ciLocal;

   _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderCreateInstance");
  
   if (interOpNameSpace(cop,&st)!=1) _SFCB_RETURN(st);
   
   ciLocal = ci->ft->clone(ci, NULL);
   memLinkInstance(ciLocal);
   
   if(isa(nss, cns, "cim_registeredprofile")) {
   
      _SFCB_TRACE(1,("--- create CIM_RegisteredProfile"));

      char *key=NULL;
      CMPIString *instid=ciLocal->ft->getProperty(ciLocal,"InstanceID",&st).value.string;
      CMPIString *regname=ciLocal->ft->getProperty(ciLocal,"RegisteredName",&st).value.string;
      int regorg=ciLocal->ft->getProperty(ciLocal,"RegisteredOrganization",&st).value.uint16;
      CMPIString *regvers=ciLocal->ft->getProperty(ciLocal,"RegisteredVersion",&st).value.string;

      /* do some basic checks (should break out to seperate function) */
      if (regname == NULL || regorg < 0) {
         setStatus(&st,CMPI_RC_ERR_FAILED, "Required property not found (RegisteredName or RegisteredOrganization)");
         _SFCB_RETURN(st);         
      }
      else if (strlen((char*)regname->hdl) > 256) {
         setStatus(&st,CMPI_RC_ERR_FAILED, "Value for RegisteredName is too long (256 chars max)");
         _SFCB_RETURN(st);         
      }
      
      _SFCB_TRACE(2,("--- New RegisteredProfile name is %s",regname));

      key=normalizeObjectPathCharsDup(cop);
      if (strchr(key, ':') == NULL) {
         _SFCB_TRACE(1,("Warning: InstanceID not in <OrgID>:<LocalID> format"));
      }

      if (getProfile(key)) {
         free(key);
         setStatus(&st,CMPI_RC_ERR_ALREADY_EXISTS,NULL);
         _SFCB_RETURN(st); 
      }
      addProfile(ciLocal,key,(char*)instid->hdl,(char*)regname->hdl,regorg,(char*)regvers->hdl);
   }
   
   else {
      setStatus(&st,CMPI_RC_ERR_NOT_SUPPORTED,"Class not supported");
      _SFCB_RETURN(st);         
   }
    
   if (st.rc==CMPI_RC_OK) {
      ctxLocal = prepareUpcall((CMPIContext *)ctx);
      CMReturnObjectPath(rslt, _broker->bft->createInstance(_broker, ctxLocal, cop, ciLocal, &st));
      CMRelease(ctxLocal);
   }
    
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
   CMPIString *cn = CMGetClassName(cop, NULL);
   char *cns = cn->ft->getCharPtr(cn,NULL);
   CMPIString *ns = CMGetNameSpace(cop, NULL);
   char *nss = ns->ft->getCharPtr(ns,NULL);
   //char *key = (char*)CMGetKey(cop, "instanceid", &st).value.string->hdl;
   char *key = normalizeObjectPathCharsDup(cop);
   Profile *pi;
   CMPIContext *ctxLocal;

   _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderDeleteInstance");
   
   if (isa(nss, cns, "cim_registeredprofile")) {
      _SFCB_TRACE(1,("--- delete CIM_RegisteredProfile >%s<",key));

      if ((pi=getProfile(key))) {
         removeProfile(pi,key);
      }
      else setStatus(&st,CMPI_RC_ERR_NOT_FOUND,NULL);
   }
   
   else setStatus(&st,CMPI_RC_ERR_NOT_SUPPORTED,"Class not supported");
      
   if (st.rc==CMPI_RC_OK) {
      ctxLocal = prepareUpcall((CMPIContext *)ctx);
      st = _broker->bft->deleteInstance(_broker, ctxLocal, cop);
      CMRelease(ctxLocal);
   }
   
   if (key) free(key);
   
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


/* --------------------------------------------------------------------------*/
/*                        Method Provider Interface                          */
/* --------------------------------------------------------------------------*/

CMPIStatus ProfileProviderMethodCleanup(
	CMPIMethodMI * mi,
	const CMPIContext * ctx, CMPIBoolean terminate)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderMethodCleanup");
   _SFCB_RETURN(st);
}

/* ------------------------------------------------------------------------- */

CMPIStatus ProfileProviderInvokeMethod(
	CMPIMethodMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * ref,
	const char * methodName,
	const CMPIArgs * in,
	CMPIArgs * out)
{ 
   CMPIStatus st = { CMPI_RC_OK, NULL };
   
   _SFCB_ENTER(TRACE_INDPROVIDER, "ProfileProviderInvokeMethod");
   
   if (interOpNameSpace(ref,&st)!=1) _SFCB_RETURN(st);
   
   _SFCB_TRACE(1,("--- Method: %s",methodName)); 

   if (strcasecmp(methodName, "_startup") == 0) {
      initProfiles(_broker,ctx);
   }

   else {
      _SFCB_TRACE(1,("--- Invalid request method: %s",methodName));
      setStatus(&st, CMPI_RC_ERR_METHOD_NOT_FOUND, "Invalid request method");
   }
   
   _SFCB_RETURN(st);
}


/* associator interface to go here */

/* ------------------------------------------------------------------ *
 * Instance MI Factory
 *
 * NOTE: This is an example using the convenience macros. This is OK
 *       as long as the MI has no special requirements, i.e. to store
 *       data between calls.
 * ------------------------------------------------------------------ */

//CMInstanceMIStub(ProfileProvider, ProfileProvider, _broker, CMNoHook); 
CMInstanceMIStub(ProfileProvider, ProfileProvider, _broker, CMNoHook); 

CMMethodMIStub(ProfileProvider, ProfileProvider, _broker, CMNoHook);
//CMAssociationMIStub(ProfileProvider, ProfileProvider, _broker, CMNoHook);

