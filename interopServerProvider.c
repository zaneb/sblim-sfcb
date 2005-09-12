
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

#define NEW(x) ((x *) malloc(sizeof(x)))

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpiftx.h"
#include "cmpimacs.h"
#include "cmpimacsx.h"


static CMPIBroker *_broker;

static int genNameSpaceData(char *ns, int dbl, CMPIResult * rslt, CMPIObjectPath *op, 
   CMPIInstance *ci)
{
   if (op) {
      CMAddKey(op,"name",ns+dbl+1,CMPI_chars);
      CMReturnObjectPath(rslt,op);
   }  
   else if (ci) {
      CMSetProperty(ci,"name",ns+dbl+1,CMPI_chars);
      CMReturnInstance(rslt,ci);
   }  
   return 0;
}   


static void gatherNameSpacesData(char *dn, int dbl, CMPIResult * rslt, CMPIObjectPath *op, 
     CMPIInstance *ci)
{
   DIR *dir;
   struct dirent *de, *de_test;
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
     free(de_test);
     genNameSpaceData(n,dbl,rslt,op,ci);
     
     gatherNameSpacesData(n,dbl,rslt,op,ci);
   }
   closedir(dir);     
} 


CMPIStatus NameSpaceProviderCleanup(CMPIInstanceMI * mi, CMPIContext * ctx)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, "NameSpaceProviderCleanup");
   
   _SFCB_RETURN(st);
}

CMPIStatus NameSpaceProviderGetInstance(CMPIInstanceMI * mi,
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
         CMSetProperty(ci,"ObjectManagerName","CIM_ObjectManagerNameValue",CMPI_chars);
         CMSetProperty(ci,"SystemCreationClassName","CIM_ComputerSystem",CMPI_chars);
         hostName[0]=0;
         gethostname(hostName,511);
         CMSetProperty(ci,"SystemName",hostName,CMPI_chars);
         CMSetProperty(ci,"ClassInfo",&info,CMPI_uint16);
         CMSetProperty(ci,"DescriptionOfClassInfo","namespace",CMPI_chars);
         CMSetProperty(ci,"name",dn+dbl,CMPI_chars);
         CMReturnInstance(rslt,ci);
         closedir(dir);
      }
      else st.rc=CMPI_RC_ERR_NOT_FOUND;   
   }
   else st.rc=CMPI_RC_ERR_NO_SUCH_PROPERTY;  
   
   _SFCB_RETURN(st);
}

CMPIStatus NameSpaceProviderCreateInstance(CMPIInstanceMI * mi,
                                          CMPIContext * ctx,
                                          CMPIResult * rslt,
                                          CMPIObjectPath * cop,
                                          CMPIInstance * ci)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, "NameSpaceProviderCreateInstance");
   
   _SFCB_RETURN(st);
}

CMPIStatus NameSpaceProviderSetInstance(CMPIInstanceMI * mi,
                                       CMPIContext * ctx,
                                       CMPIResult * rslt,
                                       CMPIObjectPath * cop,
                                       CMPIInstance * ci, char **properties)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, "NameSpaceProviderSetInstance");
   
   _SFCB_RETURN(st);
}

CMPIStatus NameSpaceProviderDeleteInstance(CMPIInstanceMI * mi,
                                             CMPIContext * ctx,
                                             CMPIResult * rslt,
                                             CMPIObjectPath * ref)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, "NameSpaceProviderDeleteInstance");
   
   _SFCB_RETURN(st);
}

CMPIStatus NameSpaceProviderEnumInstances(CMPIInstanceMI * mi, 
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
   CMSetProperty(ci,"ObjectManagerName","CIM_ObjectManagerNameValue",CMPI_chars);
   CMSetProperty(ci,"SystemCreationClassName","CIM_ComputerSystem",CMPI_chars);
   hostName[0]=0;
   gethostname(hostName,511);
   CMSetProperty(ci,"SystemName",hostName,CMPI_chars);
   CMSetProperty(ci,"ClassInfo",&info,CMPI_uint16);
   CMSetProperty(ci,"DescriptionOfClassInfo","namespace",CMPI_chars);
   
   gatherNameSpacesData(dn,strlen(dn),rslt,NULL,ci);
   
   _SFCB_RETURN(st);
}

CMPIStatus NameSpaceProviderEnumInstanceNames(CMPIInstanceMI * mi,
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
   CMAddKey(op,"ObjectManagerName","CIM_ObjectManagerNameValue",CMPI_chars);
   CMAddKey(op,"SystemCreationClassName","CIM_ComputerSystem",CMPI_chars);
   hostName[0]=0;
   gethostname(hostName,511);
   CMAddKey(op,"SystemName",hostName,CMPI_chars);
   
   gatherNameSpacesData(dn,strlen(dn),rslt,op,NULL);
   
   _SFCB_RETURN(st);
}

static CMPIStatus NameSpaceProviderExecQuery(CMPIInstanceMI * mi,
                                     CMPIContext * ctx,
                                     CMPIResult * rslt,
                                     CMPIObjectPath * cop,
                                     char *lang, char *query)
{
   CMPIStatus st = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, "NameSpaceProviderExecQuery");
   
   _SFCB_RETURN(st);
}


 
CMInstanceMIStub(NameSpaceProvider, NameSpaceProvider, _broker, CMNoHook);
