
/*
 * brokerEnc.c
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
 * Author:       Frank Scheffler 
 * Contributors: Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 * 
 * This module implements CMPI broker encapsulated functions.
 *
*/


#include "native.h"
#include "trace.h"
#include "constClass.h"
#include "utilft.h"

extern const char *opGetClassNameChars(CMPIObjectPath * cop);
extern const char *opGetNameSpaceChars(CMPIObjectPath * cop);
extern CMPIConstClass *getConstClass(const char *ns, const char *cn);
extern CMPIObjectPathFT *CMPI_ObjectPath_FT;
extern CMPIInstanceFT *CMPI_Instance_FT;
extern CMPIString *__oft_toString(CMPIObjectPath * cop, CMPIStatus * rc);
extern CMPIString *instance2String(CMPIInstance * inst, CMPIStatus * rc);
extern int verifyPropertyList(CMPIObjectPath * cop, char **list);
extern CMPISelectExp *TrackedCMPISelectExp(const char *queryString,
          const char *language, CMPIArray ** projection, CMPIStatus * rc);



static CMPIInstance *__beft_newInstance(CMPIBroker * broker,
                                        CMPIObjectPath * cop, CMPIStatus * rc)
{
   _SFCB_ENTER(TRACE_ENCCALLS,"newInstance");
   CMPIInstance *inst=TrackedCMPIInstance(cop, rc);
   _SFCB_RETURN(inst);
}


static CMPIObjectPath *__beft_newObjectPath(CMPIBroker * broker,
                                            const char *namespace,
                                            const char *classname,
                                            CMPIStatus * rc)
{
   _SFCB_ENTER(TRACE_ENCCALLS,"newObjectPath");
   CMPIObjectPath *path=TrackedCMPIObjectPath(namespace, classname, rc);
   _SFCB_RETURN(path);
}


static CMPIArgs *__beft_newArgs(CMPIBroker * broker, CMPIStatus * rc)
{
   _SFCB_ENTER(TRACE_ENCCALLS,"newArgs");
   CMPIArgs *args=TrackedCMPIArgs(rc);
   _SFCB_RETURN(args);
}


static CMPIString *__beft_newString(CMPIBroker * broker,
                                    const char *str, CMPIStatus * rc)
{
   _SFCB_ENTER(TRACE_ENCCALLS,"newString");
   CMPIString *s=native_new_CMPIString(str, rc);
   _SFCB_RETURN(s);
}


static CMPIArray *__beft_newArray(CMPIBroker * broker,
                                  CMPICount size,
                                  CMPIType type, CMPIStatus * rc)
{
   _SFCB_ENTER(TRACE_ENCCALLS,"newArray");
   CMPIArray *ar=TrackedCMPIArray(size, type, rc);
   _SFCB_RETURN(ar);
}


static CMPIDateTime *__beft_newDateTime(CMPIBroker * broker, CMPIStatus * rc)
{
   _SFCB_ENTER(TRACE_ENCCALLS,"newDateTime");
   CMPIDateTime *dt=native_new_CMPIDateTime(rc);
   _SFCB_RETURN(dt);
}


static CMPIDateTime *__beft_newDateTimeFromBinary(CMPIBroker * broker,
                                                  CMPIUint64 time,
                                                  CMPIBoolean interval,
                                                  CMPIStatus * rc)
{
   _SFCB_ENTER(TRACE_ENCCALLS,"newDateTimeFromBinary");
   CMPIDateTime *dt=native_new_CMPIDateTime_fromBinary(time, interval, rc);
   _SFCB_RETURN(dt);
}


static CMPIDateTime *__beft_newDateTimeFromChars(CMPIBroker * broker,
                                                 char *string, CMPIStatus * rc)
{
   _SFCB_ENTER(TRACE_ENCCALLS,"newDateTimeFromChars");
    CMPIDateTime *dt=native_new_CMPIDateTime_fromChars(string, rc);
   _SFCB_RETURN(dt);
}


static CMPISelectExp *__beft_newSelectExp(CMPIBroker * broker,
                                          const char *queryString,
                                          const char *language,
                                          CMPIArray ** projection,
                                          CMPIStatus * rc)
{
   _SFCB_ENTER(TRACE_ENCCALLS,"newSelectExp");
   CMPISelectExp *sx=TrackedCMPISelectExp(queryString, language, projection, rc);
   _SFCB_RETURN(sx);
}


static CMPIBoolean __beft_classPathIsA(CMPIBroker * broker,
                                       CMPIObjectPath * cop,
                                       const char *type, CMPIStatus * rc)
{
   CMPIConstClass *cc;
   char *scn, *ns;
   if (rc) CMSetStatus(rc, CMPI_RC_OK);

   CMPIString *clsn = CMGetClassName(cop, NULL);

   _SFCB_ENTER(TRACE_ENCCALLS,"classPathIsA");

   if (clsn && clsn->hdl) {
      if (strcasecmp(type, (char *) clsn->hdl) == 0)
         _SFCB_RETURN(1);
   }
   else _SFCB_RETURN(0);

   ns = (char *) opGetNameSpaceChars(cop);
   cc = (CMPIConstClass *) getConstClass(ns, opGetClassNameChars(cop));

   if (cc) for (; (scn = (char *) cc->ft->getCharSuperClassName(cc)) != NULL;) {
      if (strcasecmp(scn, type) == 0) return 1;
      cc = (CMPIConstClass *) getConstClass(ns, scn);
      if (cc == NULL) break;
   }
   _SFCB_RETURN(0);
}

static CMPIString *__beft_toString(CMPIBroker * broker,
                                   void *object, CMPIStatus * rc)
{
   CMPIString *str;
   _SFCB_ENTER(TRACE_ENCCALLS,"toString");
   if (object) {
      if (((CMPIInstance *) object)->ft) {
         if (((CMPIObjectPath *) object)->ft == CMPI_ObjectPath_FT) {
            str=__oft_toString((CMPIObjectPath *) object, rc);
            _SFCB_RETURN(str);
         }   
         if (((CMPIInstance *) object)->ft == CMPI_Instance_FT) {
            str=instance2String((CMPIInstance *) object, rc);
            _SFCB_RETURN(str);
         }   
      }
   }
   _SFCB_TRACE(1,("This operation is not yet supported."));
   if (rc) CMSetStatus(rc, CMPI_RC_ERR_NOT_SUPPORTED);
   _SFCB_RETURN(NULL);
}


static CMPIBoolean __beft_isOfType(CMPIBroker * broker,
                                   void *object,
                                   const char *type, CMPIStatus * rc)
{
   char *t = *((char **) object);

   _SFCB_ENTER(TRACE_ENCCALLS,"isOfType");

   if (rc) CMSetStatus(rc, CMPI_RC_OK);
   _SFCB_RETURN(strcmp(t, type) == 0);
}


static CMPIString *__beft_getType(CMPIBroker * broker,
                                  void *object, CMPIStatus * rc)
{
   _SFCB_ENTER(TRACE_ENCCALLS,"getType");
   _SFCB_RETURN(__beft_newString(broker, *((char **) object), rc));
}


static CMPIString *__beft_getMessage(CMPIBroker * broker,
                                     const char *msgId, const char *defMsg,
                                     CMPIStatus * rc, unsigned int count, ...)
{
   CMPIStatus nrc;
   CMPIString *msg;
   va_list argptr;
   va_start(argptr, count);

   msg =
       ((NativeCMPIBrokerFT *) (broker->bft))->getMessage(broker, msgId,
                                                          defMsg, &nrc,
                                                          count, argptr);
   if (rc)
      *rc = nrc;
   return msg;
}


static CMPIArray *__beft_getKeyNames(CMPIBroker * broker,
                                     CMPIContext * context,
                                     CMPIObjectPath * cop, CMPIStatus * rc)
{
   CMPIArray *ar;    
   CMPIConstClass *cc;
   
   cc = (CMPIConstClass *)
       getConstClass(opGetNameSpaceChars(cop), opGetClassNameChars(cop));
   if (cc) {
      ar=cc->ft->getKeyList(cc);
   //   cc->ft->release(cc);
      return ar;
   }   
   else return NewCMPIArray(0, CMPI_string, NULL);
}

CMPIArray *getKeyListAndVerifyPropertyList(CMPIObjectPath * cop, 
                                     char **props,
                                     int *ok,
                                     CMPIStatus * rc)
{
   CMPIArray *ar;    
   CMPIConstClass *cc;
   
   cc = (CMPIConstClass *)
       getConstClass(opGetNameSpaceChars(cop), opGetClassNameChars(cop));
   if (cc) {
      ar=cc->ft->getKeyList(cc);
      *ok=verifyPropertyList(cc,props);
      return ar;
   }   
   else return NewCMPIArray(0, CMPI_string, NULL);
}

/****************************************************************************/


CMPIBrokerEncFT native_brokerEncFT = {
   NATIVE_FT_VERSION,
   __beft_newInstance,
   __beft_newObjectPath,
   __beft_newArgs,
   __beft_newString,
   __beft_newArray,
   __beft_newDateTime,
   __beft_newDateTimeFromBinary,
   __beft_newDateTimeFromChars,
   __beft_newSelectExp,
   __beft_classPathIsA,
   __beft_toString,
   __beft_isOfType,
   __beft_getType,
   __beft_getMessage,
   __beft_getKeyNames,
};

CMPIBrokerEncFT *BrokerEncFT = &native_brokerEncFT;

