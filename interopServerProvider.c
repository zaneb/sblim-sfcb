
/*
 * classProvider.c
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
 * InteropServer provider for sfcb .
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
#include "control.h"
#include "config.h"

#define NEW(x) ((x *) malloc(sizeof(x)))

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpiftx.h"
#include "cmpimacs.h"
#include "cmpimacsx.h"


static CMPIBroker *_broker;
static CMPIStatus invClassSt = { CMPI_RC_ERR_INVALID_CLASS, NULL };
static CMPIStatus notSuppSt = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };

//------------------------------------------------------------------


static char *getSfcbUuid()
{
   static char *uuid=NULL;
   
   if (uuid==NULL) {
      FILE *uuidFile;
      char *fn=alloca(strlen(SFCB_STATEDIR)+strlen("/uuid")+8);
      strcpy(fn,SFCB_STATEDIR);
      strcat(fn,"/uuid");
      uuidFile=fopen(fn,"r");
      if (uuidFile) {
         char u[512];
         if (fgets(u,512,uuidFile)!=NULL) {
            int l=strlen(u);
            if (l) u[l-1]=0;
            uuid=(char*)malloc(l+32);
            strcpy(uuid,"sfcb:");
            strcat(uuid,u);
            fclose(uuidFile);
            return uuid;
         }    
         fclose(uuidFile);
      }
      else {
         char hostName[512];
         gethostname(hostName,511);
         char *u=(char*)alloca(strlen(hostName)+32);
         strcpy(u,"sfcb:NO-UUID-FILE-");
         strcat(u,hostName);
         return u;
      }  
   }   
   return uuid;
}

//------------------------------------------------------------------


static int genNameSpaceData(char *ns, int dbl, CMPIResult * rslt, CMPIObjectPath *op, 
   CMPIInstance *ci)
{
   if (op) {
      CMAddKey(op,"Name",ns+dbl+1,CMPI_chars);
      CMReturnObjectPath(rslt,op);
   }  
   else if (ci) {
      CMSetProperty(ci,"Name",ns+dbl+1,CMPI_chars);
      CMReturnInstance(rslt,ci);
   }  
   return 0;
}   


static void gatherNameSpacesData(char *dn, int dbl, CMPIResult * rslt, CMPIObjectPath *op, 
     CMPIInstance *ci)
{
   DIR *dir, *de_test;
   struct dirent *de;
   char *n;
   int l;
   
   dir=opendir(dn);
   if (dir) while ((de=readdir(dir))!=NULL) {
     if (strcmp(de->d_name,".")==0) continue;
     if (strcmp(de->d_name,"..")==0) continue;
     l=strlen(dn)+strlen(de->d_name)+4;
     n=(char*)malloc(l+8);
     strcpy(n,dn);
     strcat(n,"/");
     strcat(n,de->d_name);
     de_test = opendir(n);
     if (de_test == NULL) {
       free(n);
       continue;
     }
     closedir(de_test);
     genNameSpaceData(n,dbl,rslt,op,ci);
     
     gatherNameSpacesData(n,dbl,rslt,op,ci);
   }
   closedir(dir);     
} 


static CMPIStatus NameSpaceProviderGetInstance(CMPIInstanceMI * mi,
                                       CMPIContext * ctx,
                                       CMPIResult * rslt,
                                       CMPIObjectPath * cop,
                                       char **properties)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char *dirn,*dn,hostName[512];
   DIR *dir;
   CMPIObjectPath *op;
   CMPIInstance *ci;
   CMPIString *name;
   unsigned short info=0,dbl;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "NameSpaceProviderGetInstance");
   
   if (getControlChars("registrationDir",&dirn)) {
     dirn = "/var/lib/sfcb/registration";
   }
   
   name=CMGetKey(cop,"name",NULL).value.string;
   
   if (name && name->hdl) {
      dn=(char*)alloca(strlen(dirn)+32+strlen((char*)name->hdl));
      strcpy(dn,dirn);
      if (dirn[strlen(dirn)-1]!='/') strcat(dn,"/");
      strcat(dn,"repository/");
      dbl=strlen(dn);
      strcat(dn,(char*)name->hdl);
      
      if ((dir=opendir(dn))!=NULL) {
         op=CMNewObjectPath(_broker,"root/interop","CIM_Namespace",NULL);
         ci=CMNewInstance(_broker,op,NULL);
   
         CMSetProperty(ci,"CreationClassName","CIM_Namespace",CMPI_chars);
         CMSetProperty(ci,"ObjectManagerCreationClassName","CIM_ObjectManager",CMPI_chars);
         CMSetProperty(ci,"ObjectManagerName",getSfcbUuid(),CMPI_chars);
         CMSetProperty(ci,"SystemCreationClassName","CIM_ComputerSystem",CMPI_chars);
         hostName[0]=0;
         gethostname(hostName,511);
         CMSetProperty(ci,"SystemName",hostName,CMPI_chars);
         CMSetProperty(ci,"ClassInfo",&info,CMPI_uint16);
         CMSetProperty(ci,"Name",dn+dbl,CMPI_chars);
         CMReturnInstance(rslt,ci);
         closedir(dir);
      }
      else st.rc=CMPI_RC_ERR_NOT_FOUND;   
   }
   else st.rc=CMPI_RC_ERR_NO_SUCH_PROPERTY;  
   
   _SFCB_RETURN(st);
}

static CMPIStatus NameSpaceProviderEnumInstances(CMPIInstanceMI * mi, 
                                          CMPIContext * ctx, 
                                          CMPIResult * rslt,
                                          CMPIObjectPath * ref, char **properties)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char *dir,*dn,hostName[512];
   CMPIObjectPath *op;
   CMPIInstance *ci;
   unsigned short info=0;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "NameSpaceProviderEnumInstances");

   if (getControlChars("registrationDir",&dir)) {
     dir = "/var/lib/sfcb/registration";
   }
   
   dn=(char*)alloca(strlen(dir)+32);
   strcpy(dn,dir);
   if (dir[strlen(dir)-1]!='/') strcat(dn,"/");
   strcat(dn,"repository");
   
   op=CMNewObjectPath(_broker,"root/interop","CIM_Namespace",NULL);
   ci=CMNewInstance(_broker,op,NULL);
   
   CMSetProperty(ci,"CreationClassName","CIM_Namespace",CMPI_chars);
   CMSetProperty(ci,"ObjectManagerCreationClassName","CIM_ObjectManager",CMPI_chars);
   CMSetProperty(ci,"ObjectManagerName",getSfcbUuid(),CMPI_chars);
   CMSetProperty(ci,"SystemCreationClassName","CIM_ComputerSystem",CMPI_chars);
   hostName[0]=0;
   gethostname(hostName,511);
   CMSetProperty(ci,"SystemName",hostName,CMPI_chars);
   CMSetProperty(ci,"ClassInfo",&info,CMPI_uint16);
   
   gatherNameSpacesData(dn,strlen(dn),rslt,NULL,ci);
   
   _SFCB_RETURN(st);
}

static CMPIStatus NameSpaceProviderEnumInstanceNames(CMPIInstanceMI * mi,
                                             CMPIContext * ctx,
                                             CMPIResult * rslt,
                                             CMPIObjectPath * ref)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char *dir,*dn,hostName[512];
   CMPIObjectPath *op;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "NameSpaceProviderEnumInstanceNames");

   if (getControlChars("registrationDir",&dir)) {
     dir = "/var/lib/sfcb/registration";
   }
   
   dn=(char*)alloca(strlen(dir)+32);
   strcpy(dn,dir);
   if (dir[strlen(dir)-1]!='/') strcat(dn,"/");
   strcat(dn,"repository");
   
   op=CMNewObjectPath(_broker,"root/interop","CIM_Namespace",NULL);
   
   CMAddKey(op,"CreationClassName","CIM_Namespace",CMPI_chars);
   CMAddKey(op,"ObjectManagerCreationClassName","CIM_ObjectManager",CMPI_chars);
   CMAddKey(op,"ObjectManagerName",getSfcbUuid(),CMPI_chars);
   CMAddKey(op,"SystemCreationClassName","CIM_ComputerSystem",CMPI_chars);
   hostName[0]=0;
   gethostname(hostName,511);
   CMAddKey(op,"SystemName",hostName,CMPI_chars);
   
   gatherNameSpacesData(dn,strlen(dn),rslt,op,NULL);
   
   _SFCB_RETURN(st);
}

//------------------------------------------------------------------

static CMPIStatus ObjectManagerProviderEnumInstanceNames(CMPIInstanceMI * mi,
                                             CMPIContext * ctx,
                                             CMPIResult * rslt,
                                             CMPIObjectPath * ref)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char hostName[512];
   CMPIObjectPath *op;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "ObjectManagerProviderEnumInstanceNames");

   op=CMNewObjectPath(_broker,"root/interop","CIM_ObjectManager",NULL);
   
   CMAddKey(op,"CreationClassName","CIM_ObjectManager",CMPI_chars);
   CMAddKey(op,"SystemCreationClassName","CIM_ComputerSystem",CMPI_chars);
   hostName[0]=0;
   gethostname(hostName,511);
   CMAddKey(op,"SystemName",hostName,CMPI_chars);
   CMAddKey(op,"Name",getSfcbUuid(),CMPI_chars);
   
   CMReturnObjectPath(rslt,op);
   
   _SFCB_RETURN(st);
}

static CMPIStatus ObjectManagerProviderEnumInstances(CMPIInstanceMI * mi,
                                             CMPIContext * ctx,
                                             CMPIResult * rslt,
                                             CMPIObjectPath * ref, char **properties)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char str[512];
   CMPIObjectPath *op;
   CMPIInstance *ci;
   CMPIUint16 state;
   CMPIBoolean bul=0;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "ObjectManagerProviderEnumInstanceNames");

   op=CMNewObjectPath(_broker,"root/interop","CIM_ObjectManager",NULL);
   ci=CMNewInstance(_broker,op,NULL);
   
   CMSetProperty(ci,"CreationClassName","CIM_ObjectManager",CMPI_chars);
   CMSetProperty(ci,"SystemCreationClassName","CIM_ComputerSystem",CMPI_chars);
   str[0]=0;
   gethostname(str,511);
   CMSetProperty(ci,"SystemName",str,CMPI_chars);
   CMSetProperty(ci,"Name",getSfcbUuid(),CMPI_chars);
   
   CMSetProperty(ci,"GatherStatisticalData",&bul,CMPI_boolean);
   CMSetProperty(ci,"ElementName","sfcb",CMPI_chars);
   CMSetProperty(ci,"Description",PACKAGE_STRING,CMPI_chars);
   state=5;
   CMSetProperty(ci,"EnabledState",&state,CMPI_uint16);
   CMSetProperty(ci,"RequestedState",&state,CMPI_uint16);
   state=2;
   CMSetProperty(ci,"EnabledDefault",&state,CMPI_uint16);
   
    CMReturnInstance(rslt,ci);
   
   _SFCB_RETURN(st);
}


static CMPIStatus ObjectManagerProviderGetInstance(CMPIInstanceMI * mi,
                                             CMPIContext * ctx,
                                             CMPIResult * rslt,
                                             CMPIObjectPath * ref, char **properties)
{
  CMPIStatus st = { CMPI_RC_OK, NULL };
  CMPIString *name=CMGetKey(ref,"name",NULL).value.string;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "ObjectManagerProviderGetInstance");

   if (name && name->hdl) {
      if (strcasecmp((char*)name->hdl,getSfcbUuid())==0)  
         return ObjectManagerProviderEnumInstances(mi,ctx,rslt,ref,properties);
      else st.rc=CMPI_RC_ERR_NOT_FOUND;   
   }
   else st.rc=CMPI_RC_ERR_NO_SUCH_PROPERTY;  
   
   _SFCB_RETURN(st);
}

// ---------------------------------------------------------------

static CMPIStatus ServerProviderCleanup(CMPIInstanceMI * mi, CMPIContext * ctx)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   
   return (st);
}

static CMPIStatus ServerProviderGetInstance(CMPIInstanceMI * mi,
                                       CMPIContext * ctx,
                                       CMPIResult * rslt,
                                       CMPIObjectPath * ref,
                                       char **properties)
{
   CMPIString *cls=CMGetClassName(ref,NULL);
   
   if (strcasecmp((char*)cls->hdl,"cim_namespace")==0) 
      return NameSpaceProviderGetInstance(mi, ctx, rslt, ref, properties);
   if (strcasecmp((char*)cls->hdl,"cim_objectmanager")==0) 
      return ObjectManagerProviderGetInstance(mi, ctx, rslt, ref, properties);
   
   return invClassSt;
}

static CMPIStatus ServerProviderEnumInstanceNames(CMPIInstanceMI * mi,
                                             CMPIContext * ctx,
                                             CMPIResult * rslt,
                                             CMPIObjectPath * ref)
{
   CMPIString *cls=CMGetClassName(ref,NULL);
   
   if (strcasecmp((char*)cls->hdl,"cim_namespace")==0) 
      return NameSpaceProviderEnumInstanceNames(mi, ctx, rslt, ref);
   if (strcasecmp((char*)cls->hdl,"cim_objectmanager")==0) 
      return ObjectManagerProviderEnumInstanceNames(mi, ctx, rslt, ref);
   
   return invClassSt;
}                                                

static CMPIStatus ServerProviderEnumInstances(CMPIInstanceMI * mi, 
                                          CMPIContext * ctx, 
                                          CMPIResult * rslt,
                                          CMPIObjectPath * ref, char **properties)
{
   CMPIString *cls=CMGetClassName(ref,NULL);
   
   if (strcasecmp((char*)cls->hdl,"cim_namespace")==0) 
      return NameSpaceProviderEnumInstances(mi, ctx, rslt, ref, properties);
   if (strcasecmp((char*)cls->hdl,"cim_objectmanager")==0) 
      return ObjectManagerProviderEnumInstances(mi, ctx, rslt, ref, properties);
   
   return invClassSt;
}                                                

static CMPIStatus ServerProviderCreateInstance(CMPIInstanceMI * mi,
                                          CMPIContext * ctx,
                                          CMPIResult * rslt,
                                          CMPIObjectPath * cop,
                                          CMPIInstance * ci)
{
   return notSuppSt;
}

static CMPIStatus ServerProviderSetInstance(CMPIInstanceMI * mi,
                                       CMPIContext * ctx,
                                       CMPIResult * rslt,
                                       CMPIObjectPath * cop,
                                       CMPIInstance * ci, char **properties)
{
   return notSuppSt;
}

static CMPIStatus ServerProviderDeleteInstance(CMPIInstanceMI * mi,
                                             CMPIContext * ctx,
                                             CMPIResult * rslt,
                                             CMPIObjectPath * ref)
{
   return notSuppSt;
}

static CMPIStatus ServerProviderExecQuery(CMPIInstanceMI * mi,
                                     CMPIContext * ctx,
                                     CMPIResult * rslt,
                                     CMPIObjectPath * cop,
                                     char *lang, char *query)
{
   return notSuppSt;
}


 
CMInstanceMIStub(ServerProvider, ServerProvider, _broker, CMNoHook);

