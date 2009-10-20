
/*
 * classProvider.c
 *
 * (C) Copyright IBM Corp. 2005
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
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
#include "native.h"
#include "control.h"
#include "config.h"

#define NEW(x) ((x *) malloc(sizeof(x)))

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpiftx.h"
#include "cmpimacs.h"
#include "cmpimacsx.h"


static const CMPIBroker *_broker;
static CMPIStatus invClassSt = { CMPI_RC_ERR_INVALID_CLASS, NULL };
static CMPIStatus notSuppSt = { CMPI_RC_ERR_NOT_SUPPORTED, NULL };
static CMPIStatus okSt = { CMPI_RC_OK, NULL };

//------------------------------------------------------------------


static char *getSfcbUuid()
{
   static char *uuid=NULL;
   static char *u=NULL;
   
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
      else if (u==NULL) {
         char hostName[512];
         gethostname(hostName,511);
         u=(char*)malloc(strlen(hostName)+32);
         strcpy(u,"sfcb:NO-UUID-FILE-");
         strcat(u,hostName);
      }  
      return u; 
   }   
   return uuid;
}

//------------------------------------------------------------------


static int genNameSpaceData(const char *ns, const char *dn, int dbl, 
			    const CMPIResult * rslt, CMPIObjectPath *op, 
			    CMPIInstance *ci,int nsOpt)
{
   if (ci) {
      if (nsOpt) CMSetProperty(ci,"Name",dn,CMPI_chars);
      else CMSetProperty(ci,"Name",ns+dbl+1,CMPI_chars);
      CMReturnInstance(rslt,ci);
   } else if (op) {
      if (nsOpt) CMAddKey(op,"Name",dn,CMPI_chars);
      else CMAddKey(op,"Name",ns+dbl+1,CMPI_chars);
      CMReturnObjectPath(rslt,op);
   }  
   return 0;
}   


static void gatherNameSpacesData(const char *dn, int dbl, 
				 const CMPIResult * rslt, 
				 CMPIObjectPath *op, CMPIInstance *ci, int nsOpt)
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
     
     genNameSpaceData(n,de->d_name,dbl,rslt,op,ci,nsOpt);
     if (nsOpt!=1) { 
        if (nsOpt==0) gatherNameSpacesData(n,dbl,rslt,op,ci,nsOpt);
     }
     free(n);
   }
   closedir(dir);     
} 


static void gatherOldNameSpacesData(const char *dn, int dbl, 
				    const CMPIResult * rslt, 
				    CMPIObjectPath *op, 
				    CMPIInstance *ci)
{
   
   char *ns = (char*)CMGetNameSpace(op,NULL)->hdl; 
   char *nns=alloca(strlen(dn)+strlen(ns)+8);
   
   strcpy(nns,dn);
   strcat(nns,"/");
   strcat(nns,ns);
   gatherNameSpacesData(nns,dbl,rslt,op,ci,1);     
}     

static CMPIStatus NameSpaceProviderGetInstance(CMPIInstanceMI * mi,
					       const CMPIContext * ctx,
					       const CMPIResult * rslt,
					       const CMPIObjectPath * cop,
					       const char **properties,
					       int nsOpt)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char *dirn,*dn,hostName[512];
   DIR *dir;
   CMPIObjectPath *op;
   CMPIInstance *ci;
   CMPIString *name;
   unsigned short info=0,dbl;
   char *ns;
   
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
      if (nsOpt) {
	ns = CMGetCharPtr(CMGetNameSpace(cop,NULL));
	if (ns) {
	  strcat(dn,ns);
	  strcat(dn,"/");
	}
      }
      dbl=strlen(dn);
      strcat(dn,(char*)name->hdl);
      
      if ((dir=opendir(dn))!=NULL) {
	if (nsOpt) {
	  op=CMNewObjectPath(_broker,"root/interop","__Namespace",NULL);
	  ci=CMNewInstance(_broker,op,NULL);
	} else {
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
	}
	CMSetProperty(ci,"Name",dn+dbl,CMPI_chars);
	CMReturnInstance(rslt,ci);
	closedir(dir);
      }
      else st.rc=CMPI_RC_ERR_NOT_FOUND;   
   }
   else st.rc=CMPI_RC_ERR_INVALID_PARAMETER;  
   
   _SFCB_RETURN(st);
}

static CMPIStatus NameSpaceProviderEnumInstances(CMPIInstanceMI * mi, 
						 const CMPIContext * ctx, 
						 const CMPIResult * rslt,
						 const CMPIObjectPath * ref, 
						 const char **properties,
						 int nsOpt)
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
   
   if (nsOpt) {
      op=CMNewObjectPath(_broker,"root/interop","__Namespace",&st);
      if (op) {
	ci=CMNewInstance(_broker,op,&st);
	if (ci) {
	  op=CMGetObjectPath(ci,NULL);
	  CMSetNameSpaceFromObjectPath(op,ref);
	  gatherOldNameSpacesData(dn,strlen(dn),rslt,op,ci);  
	}
      }
      _SFCB_RETURN(st);
   }
   
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
   
   gatherNameSpacesData(dn,strlen(dn),rslt,NULL,ci,0);
   
   _SFCB_RETURN(st);
}

static CMPIStatus NameSpaceProviderEnumInstanceNames(CMPIInstanceMI * mi,
						     const CMPIContext * ctx,
						     const CMPIResult * rslt,
						     const CMPIObjectPath * ref,
						     int nsOpt)
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
   
   if (nsOpt) {
      char *ns=(char*)CMGetNameSpace(ref,NULL)->hdl; 
      op=CMNewObjectPath(_broker,ns,"__Namespace",NULL);
      gatherOldNameSpacesData(dn,strlen(dn),rslt,op,NULL);  
      _SFCB_RETURN(st);
   }
   
   op=CMNewObjectPath(_broker,"root/interop","CIM_Namespace",NULL);
   
   CMAddKey(op,"CreationClassName","CIM_Namespace",CMPI_chars);
   CMAddKey(op,"ObjectManagerCreationClassName","CIM_ObjectManager",CMPI_chars);
   CMAddKey(op,"ObjectManagerName",getSfcbUuid(),CMPI_chars);
   CMAddKey(op,"SystemCreationClassName","CIM_ComputerSystem",CMPI_chars);
   hostName[0]=0;
   gethostname(hostName,511);
   CMAddKey(op,"SystemName",hostName,CMPI_chars);
   
   gatherNameSpacesData(dn,strlen(dn),rslt,op,NULL,nsOpt);
   
   _SFCB_RETURN(st);
}

/* 
   EnumInstanceNames for CIM_Service className 
   Currently used for CIM_ObjectManager, CIM_IndicationService
*/

static CMPIStatus ServiceProviderEnumInstanceNames(CMPIInstanceMI * mi,
						 const CMPIContext * ctx,
						 const CMPIResult * rslt,
						 const CMPIObjectPath * ref, 
						 const char* className, 
                                                 const char* sccn)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char hostName[512];
   CMPIObjectPath *op;

   _SFCB_ENTER(TRACE_PROVIDERS, "ServiceProviderEnumInstanceNames");

   op=CMNewObjectPath(_broker,"root/interop",className,NULL);
   
   CMAddKey(op,"CreationClassName",className,CMPI_chars);
   CMAddKey(op,"SystemCreationClassName",sccn,CMPI_chars);
   hostName[0]=0;
   gethostname(hostName,511);
   CMAddKey(op,"SystemName",hostName,CMPI_chars);
   CMAddKey(op,"Name",getSfcbUuid(),CMPI_chars);
   
   CMReturnObjectPath(rslt,op);
   
   _SFCB_RETURN(st);
}

static CMPIStatus ObjectManagerProviderEnumInstances(CMPIInstanceMI * mi,
						     const CMPIContext * ctx,
						     const CMPIResult * rslt,
						     const CMPIObjectPath * ref,
						     const char **properties)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char str[512];
   CMPIObjectPath *op;
   CMPIInstance *ci;
   CMPIUint16 state;
   CMPIBoolean bul=0;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "ObjectManagerProviderEnumInstances");

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

static CMPIStatus IndServiceProviderEnumInstances(CMPIInstanceMI * mi,
                                             const CMPIContext * ctx,
                                             const CMPIResult * rslt,
                                             const CMPIObjectPath * ref,
                                             const char **properties)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char str[512];
   CMPIObjectPath *op;
   CMPIInstance *ci = NULL;
   CMPIBoolean filterCreation=1;
   CMPIUint16 retryAttempts=0; /* only try to deliver indications once */
   CMPIUint32 retryInterval=30;
   CMPIUint16 subRemoval=3; /* 3 == "Disable" */
   CMPIUint32 subRemovalInterval=2592000; /* 30 days */

   
   _SFCB_ENTER(TRACE_PROVIDERS, "IndServiceProviderEnumInstances");

    CMPIContext *ctxLocal;
    ctxLocal = native_clone_CMPIContext(ctx);
    CMPIValue val;
    val.string = sfcb_native_new_CMPIString("$DefaultProvider$", NULL,0);
    ctxLocal->ft->addEntry(ctxLocal, "rerouteToProvider", &val, CMPI_string);

   op=CMNewObjectPath(_broker,"root/interop","CIM_IndicationService",NULL);
   val.chars = "CIM_ObjectManager";
   CMAddKey(op,"CreationClassName",(CMPIValue *)&val,CMPI_chars);
   val.chars = "CIM_ComputerSystem";
   CMAddKey(op,"SystemCreationClassName",(CMPIValue *)&val,CMPI_chars);
   ci = CBGetInstance(_broker, ctxLocal, op, NULL, &st);
   if(st.rc == CMPI_RC_ERR_NOT_FOUND) {
     //     fprintf(stderr, "SMS -- It's here.\n");
     ci=CMNewInstance(_broker,op,NULL);
   
     CMSetProperty(ci,"CreationClassName","CIM_ObjectManager",CMPI_chars);
     CMSetProperty(ci,"SystemCreationClassName","CIM_ComputerSystem",CMPI_chars);
     str[0]=0;
     gethostname(str,511);
     CMSetProperty(ci,"SystemName",str,CMPI_chars);
     CMSetProperty(ci,"Name",getSfcbUuid(),CMPI_chars);

     CMSetProperty(ci,"FilterCreationEnabled",&filterCreation,CMPI_boolean);
     CMSetProperty(ci,"ElementName","sfcb",CMPI_chars);
     CMSetProperty(ci,"Description",PACKAGE_STRING,CMPI_chars);
     CMSetProperty(ci,"DeliveryRetryAttempts",&retryAttempts,CMPI_uint16);
     CMSetProperty(ci,"DeliveryRetryInterval",&retryInterval,CMPI_uint32);
     CMSetProperty(ci,"SubscriptionRemovalAction",&subRemoval,CMPI_uint16);
     CMSetProperty(ci,"SubscriptionRemovalTimeInterval",&subRemovalInterval,CMPI_uint32);
     CBCreateInstance(_broker, ctxLocal, op, ci, &st);
//     if(st.rc != CMPI_RC_OK) fprintf(stderr, "SMS -- %s\n", CMGetCharPtr(st.msg));
       
   } else if(st.rc != CMPI_RC_OK) {
     //     fprintf(stderr, "SMS -- GetInstance effed\n");
     goto done;
   }

   CMReturnInstance(rslt,ci);
   CMReturnDone(rslt);
   
done:
   if(ctxLocal) CMRelease(ctxLocal);
   _SFCB_RETURN(st);
}


// ---------------------------------------------------------------

static CMPIStatus ComMechProviderEnumInstanceNames(CMPIInstanceMI * mi,
						   const CMPIContext * ctx,
						   const CMPIResult * rslt,
						   const CMPIObjectPath * ref)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char hostName[512];
   CMPIObjectPath *op;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "ComMechProviderEnumInstanceNames");

   op=CMNewObjectPath(_broker,"root/interop","CIM_ObjectManagerCommunicationMechanism",NULL);
   
   CMAddKey(op,"SystemCreationClassName","CIM_ObjectManager",CMPI_chars);
   CMAddKey(op,"CreationClassName","CIM_ObjectManagerCommunicationMechanism",CMPI_chars);
   hostName[0]=0;
   gethostname(hostName,511);
   CMAddKey(op,"SystemName",hostName,CMPI_chars);
   CMAddKey(op,"Name",getSfcbUuid(),CMPI_chars);
   
   CMReturnObjectPath(rslt,op);
   
   _SFCB_RETURN(st);
}

static CMPIStatus ComMechProviderEnumInstances(CMPIInstanceMI * mi,
					       const CMPIContext * ctx,
					       const CMPIResult * rslt,
					       const CMPIObjectPath * ref, 
					       const char **properties)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   char hostName[512];
   CMPIObjectPath *op;
   CMPIInstance *ci;
   CMPIUint16 mech;
   CMPIBoolean bul=0;
   int i;
   
   CMPIArray *fps;
   CMPIUint16 fpa[6]={2,3,5,6,7,9};
   CMPIArray *as;
   CMPIUint16 aa[1]={3};
   
   _SFCB_ENTER(TRACE_PROVIDERS, "ComMechProviderEnumInstanceNames");

   op=CMNewObjectPath(_broker,"root/interop","CIM_ObjectManagerCommunicationMechanism",NULL);
   ci=CMNewInstance(_broker,op,NULL);
      
   CMSetProperty(ci,"SystemCreationClassName","CIM_ObjectManager",CMPI_chars);
   CMSetProperty(ci,"CreationClassName","CIM_ObjectManagerCommunicationMechanism",CMPI_chars);
   hostName[0]=0;
   gethostname(hostName,511);
   CMSetProperty(ci,"SystemName",hostName,CMPI_chars);
   CMSetProperty(ci,"Name",getSfcbUuid(),CMPI_chars);
   /* Version of CIM-XML that is supported */
   CMSetProperty(ci,"Version","1.0",CMPI_chars);
   
   mech=2;
   CMSetProperty(ci,"CommunicationMechanism",&mech,CMPI_uint16);
   
   fps=CMNewArray(_broker,sizeof(fpa)/sizeof(CMPIUint16),CMPI_uint16,NULL);
   for (i=0; i<sizeof(fpa)/sizeof(CMPIUint16); i++)
      CMSetArrayElementAt(fps,i,&fpa[i],CMPI_uint16);
   CMSetProperty(ci,"FunctionalProfilesSupported",&fps,CMPI_uint16A);
   
   as=CMNewArray(_broker,sizeof(aa)/sizeof(CMPIUint16),CMPI_uint16,NULL);
   for (i=0; i<sizeof(aa)/sizeof(CMPIUint16); i++)
      CMSetArrayElementAt(as,i,&aa[i],CMPI_uint16);
   CMSetProperty(ci,"AuthenticationMechanismsSupported",&as,CMPI_uint16A);
   
   CMSetProperty(ci,"MultipleOperationsSupported",&bul,CMPI_boolean);
      
   CMReturnInstance(rslt,ci);
   
   _SFCB_RETURN(st);
}

static CMPIStatus ServiceProviderGetInstance(CMPIInstanceMI * mi,
                                             const CMPIContext * ctx,
                                             const CMPIResult * rslt,
                                             const CMPIObjectPath * ref, 
                                             const char **properties,
                                             const char* className)
{
  CMPIStatus st = { CMPI_RC_OK, NULL };
  CMPIString *name=CMGetKey(ref,"name",NULL).value.string;
   
   _SFCB_ENTER(TRACE_PROVIDERS, "ServiceProviderGetInstance");

   if (name && name->hdl) {
      if (strcasecmp((char*)name->hdl,getSfcbUuid())==0)  
	if (strcasecmp(className, "cim_objectmanager"))
         return ObjectManagerProviderEnumInstances(mi,ctx,rslt,ref,properties);
	if (strcasecmp(className, "cim_objectmanagercommunicationMechanism"))
         return ComMechProviderEnumInstances(mi,ctx,rslt,ref,properties);
	if (strcasecmp(className, "cim_indicationservice"))
         return IndServiceProviderEnumInstances(mi,ctx,rslt,ref,properties);

      else st.rc=CMPI_RC_ERR_NOT_FOUND;   
   }
   else st.rc=CMPI_RC_ERR_INVALID_PARAMETER;  
   
   _SFCB_RETURN(st);
}

// ---------------------------------------------------------------

static CMPIStatus ServerProviderCleanup(CMPIInstanceMI * mi, const CMPIContext * ctx, CMPIBoolean terminate)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   
   return (st);
}

static CMPIStatus ServerProviderGetInstance(CMPIInstanceMI * mi,
					    const CMPIContext * ctx,
					    const CMPIResult * rslt,
					    const CMPIObjectPath * ref,
					    const char **properties)
{
   CMPIString *cls=CMGetClassName(ref,NULL);
   
   if (strcasecmp((char*)cls->hdl,"cim_namespace")==0) 
     return NameSpaceProviderGetInstance(mi, ctx, rslt, ref, properties, 0);
   else if (strcasecmp((char*)cls->hdl,"__namespace")==0) 
     return NameSpaceProviderGetInstance(mi, ctx, rslt, ref, properties, 1);
   if (strcasecmp((char*)cls->hdl,"cim_objectmanager")==0) 
      return ServiceProviderGetInstance(mi, ctx, rslt, ref, properties, 
                                                          "cim_objectmanager");
   if (strcasecmp((char*)cls->hdl,"cim_objectmanagercommunicationMechanism")==0) 
      return ServiceProviderGetInstance(mi, ctx, rslt, ref, properties, 
                                    "cim_objectmanagercommunicationMechanism");
   if (strcasecmp((char*)cls->hdl,"cim_indicationservice")==0) 
      return ServiceProviderGetInstance(mi, ctx, rslt, ref, properties, 
                                                      "cim_indicationservice");
   
   return invClassSt;
}

static CMPIStatus ServerProviderEnumInstanceNames(CMPIInstanceMI * mi,
						  const CMPIContext * ctx,
						  const CMPIResult * rslt,
						  const CMPIObjectPath * ref)
{
   CMPIString *cls=CMGetClassName(ref,NULL);
   
   if (strcasecmp((char*)cls->hdl,"cim_namespace")==0) 
      return NameSpaceProviderEnumInstanceNames(mi, ctx, rslt, ref,0);
   if (strcasecmp((char*)cls->hdl,"__namespace")==0) 
      return NameSpaceProviderEnumInstanceNames(mi, ctx, rslt, ref,1);
   if (strcasecmp((char*)cls->hdl,"cim_objectmanager")==0) 
      return ServiceProviderEnumInstanceNames(mi, ctx, rslt, ref, 
                                    "CIM_ObjectManager", "CIM_ComputerSystem");
   if (strcasecmp((char*)cls->hdl,"cim_objectmanagercommunicationMechanism")==0) 
      return ComMechProviderEnumInstanceNames(mi, ctx, rslt, ref);
   if (strcasecmp((char*)cls->hdl,"cim_indicationservice")==0) 
     return ServiceProviderEnumInstanceNames(mi, ctx, rslt, ref, 
                                 "CIM_IndicationService", "CIM_ObjectManager");
  
   return okSt;
}                                                

static CMPIStatus ServerProviderEnumInstances(CMPIInstanceMI * mi, 
					      const CMPIContext * ctx, 
					      const CMPIResult * rslt,
					      const CMPIObjectPath * ref, 
					      const char **properties)
{
   CMPIString *cls=CMGetClassName(ref,NULL);
   
   if (strcasecmp((char*)cls->hdl,"cim_namespace")==0) 
      return NameSpaceProviderEnumInstances(mi, ctx, rslt, ref, properties, 0);
   if (strcasecmp((char*)cls->hdl,"__namespace")==0) 
      return NameSpaceProviderEnumInstances(mi, ctx, rslt, ref, properties, 1);
   if (strcasecmp((char*)cls->hdl,"cim_objectmanager")==0) 
      return ObjectManagerProviderEnumInstances(mi, ctx, rslt, ref, properties);
   if (strcasecmp((char*)cls->hdl,"cim_objectmanagercommunicationMechanism")==0) 
      return ComMechProviderEnumInstances(mi, ctx, rslt, ref, properties);
   if (strcasecmp((char*)cls->hdl,"cim_interopservice")==0) /* do we still need this? */
      return ComMechProviderEnumInstances(mi, ctx, rslt, ref, properties);
   if (strcasecmp((char*)cls->hdl,"cim_indicationservice")==0)
      return IndServiceProviderEnumInstances(mi, ctx, rslt, ref, properties);
   
   return okSt;
}                                                

static CMPIStatus ServerProviderCreateInstance(CMPIInstanceMI * mi,
					       const CMPIContext * ctx,
					       const CMPIResult * rslt,
					       const CMPIObjectPath * cop,
					       const CMPIInstance * ci)
{
   return notSuppSt;
}

static CMPIStatus ServerProviderModifyInstance(CMPIInstanceMI * mi,
					       const CMPIContext * ctx,
					       const CMPIResult * rslt,
					       const CMPIObjectPath * cop,
					       const CMPIInstance * ci, 
					       const char **properties)
{
   return notSuppSt;
}

static CMPIStatus ServerProviderDeleteInstance(CMPIInstanceMI * mi,
					       const CMPIContext * ctx,
					       const CMPIResult * rslt,
					       const CMPIObjectPath * ref)
{
   return notSuppSt;
}

static CMPIStatus ServerProviderExecQuery(CMPIInstanceMI * mi,
					  const CMPIContext * ctx,
					  const CMPIResult * rslt,
					  const CMPIObjectPath * cop,
					  const char *lang, 
					  const char *query)
{
   return notSuppSt;
}


 
CMInstanceMIStub(ServerProvider, ServerProvider, _broker, CMNoHook);

/*---------------------- Association interface --------------------------*/ 


CMPIStatus ServerProviderAssociationCleanup(CMPIAssociationMI * mi, const CMPIContext * ctx, CMPIBoolean terminate)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, "ServerProviderAssociationCleanup");
   
   _SFCB_RETURN(st);
}

                            
/** \brief buildAssoc - Builds the Association instances
 *
 *  buildAssoc returns a set of instances represented 
 *  by op that is passed in. 
 *  The propertyList is used as a filter on the results
 *  and the target determines if names or instances should
 *  be returned.
 */

CMPIStatus buildAssoc(const CMPIContext * ctx,
                     const CMPIResult * rslt,
                     const CMPIObjectPath * op,
                     const char **propertyList,
                     const char *target)
{
    CMPIEnumeration *enm = NULL;
    CMPIStatus rc = {CMPI_RC_OK, NULL};

    if (strcasecmp(target,"AssocNames") == 0 ) {
        enm = _broker->bft->enumerateInstanceNames(_broker, ctx, op, &rc);
        while(enm && enm->ft->hasNext(enm, &rc)) {
            CMReturnObjectPath(rslt, (enm->ft->getNext(enm, &rc).value.ref));
        }
    } else if (strcasecmp(target,"Assocs") == 0 ){
        enm = _broker->bft->enumerateInstances(_broker, ctx, op, NULL, &rc);
        while(enm && enm->ft->hasNext(enm, &rc)) {
            CMPIData inst=CMGetNext(enm,&rc);
            if (propertyList) {
                CMSetPropertyFilter(inst.value.inst,propertyList,NULL);
            }
            CMReturnInstance(rslt, (inst.value.inst));
        }
    }
    if(enm) CMRelease(enm);
    CMReturnDone( rslt );
    CMReturn(CMPI_RC_OK);
}

/** \brief buildRefs - Builds the Reference instances
 *
 *  buildAssoc returns a set of instances of the
 *  SFCB_ServiceAffectsElement class that associate
 *  the objects represented by op to the IndicationService.
 *  isop is an objectPath pointer to the IndicationService,
 *  saeop is an objectPath pointer to SFCB_ServiceAffectsElement.
 *  The propertyList is used as a filter on the results
 *  and the target determines if names or instances should
 *  be returned.
 */
CMPIStatus buildRefs(const CMPIContext * ctx,
                     const CMPIResult * rslt,
                     const CMPIObjectPath * op,
                     const CMPIObjectPath * isop,
                     const CMPIObjectPath * saeop,
                     const char **propertyList,
                     const char *target)
{
    CMPIEnumeration *enm = NULL;
    CMPIEnumeration *isenm = NULL;
    CMPIStatus rc = {CMPI_RC_OK, NULL};
    CMPIStatus rc2 = {CMPI_RC_OK, NULL};
    CMPIInstance *ci;

    // Get the single instance of IndicationService
    isenm = _broker->bft->enumerateInstanceNames(_broker, ctx, isop, &rc);
    CMPIData isinst=CMGetNext(isenm,&rc);
    // Create an instance of SAE
    ci=CMNewInstance(_broker,saeop,&rc2);
    CMSetProperty(ci,"AffectingElement",&(isinst.value.ref),CMPI_ref);

    if (CMGetKeyCount(op,NULL) == 0) {
        enm = _broker->bft->enumerateInstanceNames(_broker, ctx, op, &rc);
        while(enm && CMHasNext(enm, &rc)) {
            CMPIData inst=CMGetNext(enm,&rc);
            CMSetProperty(ci,"AffectedElement",&(inst.value.ref),CMPI_ref);
            if (strcasecmp(target,"Refs") == 0 ) {
                if (propertyList) {
                    CMSetPropertyFilter(ci,propertyList,NULL);
                }
                CMReturnInstance(rslt, ci);
            } else {
                CMReturnObjectPath(rslt,CMGetObjectPath(ci,NULL));
            }
        }
    } else {
        CMSetProperty(ci,"AffectedElement",&(op),CMPI_ref);
        if (strcasecmp(target,"Refs") == 0 ) {
            if (propertyList) {
                CMSetPropertyFilter(ci,propertyList,NULL);
            }
            CMReturnInstance(rslt, ci);
        } else {
            CMReturnObjectPath(rslt,CMGetObjectPath(ci,NULL));
        }
    }

    if(ci) CMRelease(ci);
    if(enm) CMRelease(enm);
    if(isenm) CMRelease(isenm);
    CMReturnDone( rslt );
    CMReturn(CMPI_RC_OK);
}
                            
/** \brief buildObj - Builds the Association or Reference instances
 *
 *  buildObj calls buildAssoc or buildRefs as required.
 *  op is the target objectPath.
 *  isop is an objectPath pointer to the IndicationService,
 *  saeop is an objectPath pointer to SFCB_ServiceAffectsElement.
 *  The propertyList is used as a filter on the results
 *  and the target determines if names or instances should
 *  be returned and whether association or references.
 */
CMPIStatus buildObj(const CMPIContext * ctx,
                     const CMPIResult * rslt,
                     const char *resultClass,
                     const CMPIObjectPath * op,
                     const CMPIObjectPath * isop,
                     const CMPIObjectPath * saeop,
                     const char **propertyList,
                     const char *target)
{
    CMPIStatus rc = {CMPI_RC_OK, NULL};

    if ( ((strcasecmp(target,"Assocs") == 0 ) || (strcasecmp(target,"AssocNames") == 0 ))
    && (( resultClass==NULL ) || ( CMClassPathIsA(_broker,op,resultClass,&rc) == 1 ) )) {
        // Association was requested
        buildAssoc(ctx,rslt,op,propertyList,target);
    } else if ( ((strcasecmp(target,"Refs") == 0 ) || (strcasecmp(target,"RefNames") == 0 )) 
    && (( resultClass==NULL ) || ( CMClassPathIsA(_broker,saeop,resultClass,&rc) == 1 ) )) {
        // Reference was requested
        buildRefs(ctx,rslt,op,isop,saeop,propertyList,target);
    }
    CMReturnDone( rslt );
    CMReturn(CMPI_RC_OK);
}

/** \brief makeCIM_System - Builds a CIM_System instance
 *  Creates an instance for a (dummy) CIM_System
*/
CMPIStatus  makeCIM_System(CMPIInstance * csi)
{
    CMSetProperty(csi,"CreationClassName","CIM_System",CMPI_chars);
    CMSetProperty(csi,"Name",getSfcbUuid(),CMPI_chars);
    CMReturn(CMPI_RC_OK);
}

/** \brief makeHostedService - Builds a CIM_HostedService instance
 *  
 *  Creates and returns the instance (or name) of a CIM_HostedService
 *  association between CIM_System and CIM_IndicationService
*/
CMPIStatus makeHostedService(CMPIAssociationMI * mi,
                     const CMPIContext * ctx,
                     const CMPIResult * rslt,
                     const CMPIObjectPath * isop,
                     const CMPIObjectPath * hsop,
                     const CMPIObjectPath * csop,
                     const char **propertyList,
                     const char *target)
{
    CMPIEnumeration *isenm = NULL;
    CMPIStatus rc = {CMPI_RC_OK, NULL};
    CMPIInstance  *hsi, *cci ;

    cci=CMNewInstance(_broker,csop,&rc);
    makeCIM_System(cci);

    // Get the single instance of IndicationService
    isenm = _broker->bft->enumerateInstanceNames(_broker, ctx, isop, &rc);
    CMPIData isinst=CMGetNext(isenm,&rc);
    // Create an instance 
    hsi=CMNewInstance(_broker,hsop,&rc);
    CMPIValue cciop;
    cciop.ref=CMGetObjectPath(cci,NULL);
    
    CMSetProperty(hsi,"Dependent",&(isinst.value),CMPI_ref);
    CMSetProperty(hsi,"Antecedent",&(cciop),CMPI_ref);
    if (strcasecmp(target,"Refs") == 0 ) {
        if (propertyList) {
            CMSetPropertyFilter(hsi,propertyList,NULL);
        }
        CMReturnInstance(rslt, hsi);
    } else {
        CMReturnObjectPath(rslt,CMGetObjectPath(hsi,NULL));
    }
    if(cci) CMRelease(cci);
    if(hsi) CMRelease(hsi);
    if(isenm) CMRelease(isenm);
    CMReturnDone( rslt );
    CMReturn(CMPI_RC_OK);
}

/** \brief makeElementConforms - Builds a CIM_ElementConformsToProfile instance
 *  
 *  Creates and returns the instance (or name) of a CIM_ElementConformsToProfile
 *  association between CIM_RegisteredProfile and CIM_IndicationService
*/
CMPIStatus makeElementConforms(CMPIAssociationMI * mi,
                     const CMPIContext * ctx,
                     const CMPIResult * rslt,
                     const CMPIObjectPath * isop,
                     const CMPIObjectPath * ecop,
                     CMPIObjectPath * rpop,
                     const char **propertyList,
                     const char *target)
{
    CMPIEnumeration *isenm = NULL;
    CMPIStatus rc = {CMPI_RC_OK, NULL};
    CMPIInstance  *eci = NULL;

    // Get the single instance of IndicationService
    isenm = _broker->bft->enumerateInstanceNames(_broker, ctx, isop, &rc);
    CMPIData isinst=CMGetNext(isenm,&rc);
    // Get the IndicationProfile instance of RegisteredProfile
    CMAddKey(rpop, "InstanceID", "CIM:SFCB_IP", CMPI_chars);
    // Create an instance 
    eci=CMNewInstance(_broker,ecop,&rc);
    CMSetProperty(eci,"ManagedElement",&(isinst.value),CMPI_ref);
    CMSetProperty(eci,"ConformantStandard",&(rpop),CMPI_ref);
    if (strcasecmp(target,"Refs") == 0 ) {
        if (propertyList) {
            CMSetPropertyFilter(eci,propertyList,NULL);
        }
        CMReturnInstance(rslt, eci);
    } else {
        CMReturnObjectPath(rslt,CMGetObjectPath(eci,NULL));
    }
    if(eci) CMRelease(eci);
    if(isenm) CMRelease(isenm);
    CMReturnDone( rslt );
    CMReturn(CMPI_RC_OK);
}
/** \brief getAssociators - Builds the Association or Reference instances
 *
 *  Determines what needs to be returned for the various associator and
 *  reference calls.
 */
CMPIStatus getAssociators(CMPIAssociationMI * mi,
                          const CMPIContext * ctx,
                          const CMPIResult * rslt,
                          const CMPIObjectPath * cop,
                          const char *assocClass,
                          const char *resultClass,
                          const char *role,
                          const char *resultRole,
                          const char **propertyList,
                          const char *target)
{

    CMPIStatus rc = {CMPI_RC_OK, NULL};
    CMPIObjectPath * saeop = NULL, * ldop = NULL, * ifop = NULL, * isop = NULL;
    CMPIObjectPath * csop = NULL, * hsop = NULL, * ecpop = NULL, * rpop = NULL;
    CMPIInstance *cci;
    CMPIEnumeration *isenm = NULL;

    // Make sure role & resultRole are valid
    if ( role && resultRole  && (strcasecmp(role,resultRole) == 0)) {
        CMSetStatusWithChars( _broker, &rc, CMPI_RC_ERR_FAILED, "role and resultRole cannot be equal." );
        return rc;
    }
    if ( role  && (strcasecmp(role,"AffectingElement") != 0) 
    && (strcasecmp(role,"AffectedElement") != 0) 
    && (strcasecmp(role,"ConformantStandard") != 0) 
    && (strcasecmp(role,"ManagedElement") != 0) 
    && (strcasecmp(role,"Antecedent") != 0) 
    && (strcasecmp(role,"Dependent") != 0)) {
        CMSetStatusWithChars( _broker, &rc, CMPI_RC_ERR_FAILED, "Invalid value for role ." );
        return rc;
    }
    if ( resultRole  && (strcasecmp(resultRole,"AffectingElement") != 0) 
    && (strcasecmp(resultRole,"AffectedElement") != 0) 
    && (strcasecmp(resultRole,"ConformantStandard") != 0) 
    && (strcasecmp(resultRole,"ManagedElement") != 0) 
    && (strcasecmp(resultRole,"Antecedent") != 0) 
    && (strcasecmp(resultRole,"Dependent") != 0)) {
        CMSetStatusWithChars( _broker, &rc, CMPI_RC_ERR_FAILED, "Invalid value for resultRole ." );
        return rc;
    }

    saeop = CMNewObjectPath( _broker, CMGetCharPtr(CMGetNameSpace(cop,&rc)), "SFCB_ServiceAffectsElement", &rc );
    hsop = CMNewObjectPath( _broker, "root/interop", "CIM_HostedService", &rc );
    ecpop = CMNewObjectPath( _broker, "root/interop", "CIM_ElementConformsToProfile", &rc );
    if (( saeop==NULL ) || ( hsop==NULL) || ( ecpop==NULL)) {
        CMSetStatusWithChars( _broker, &rc, CMPI_RC_ERR_FAILED, "Create CMPIObjectPath failed." );
        return rc;
    }

    // Make sure we are getting a request for the right assoc class
    if( ( assocClass==NULL ) || ( CMClassPathIsA(_broker,saeop,assocClass,&rc) == 1 ) ) {
        // Handle SFCB_ServiceAffectsElement
        
        // Get pointers to all the interesting classes.
        ldop = CMNewObjectPath( _broker, CMGetCharPtr(CMGetNameSpace(cop,&rc)), "CIM_listenerdestination", &rc );
        ifop = CMNewObjectPath( _broker, CMGetCharPtr(CMGetNameSpace(cop,&rc)), "CIM_indicationfilter", &rc );
        isop = CMNewObjectPath( _broker, CMGetCharPtr(CMGetNameSpace(cop,&rc)), "CIM_indicationservice", &rc );
        if (( ldop==NULL ) || ( ifop==NULL ) || ( isop==NULL )) {
            CMSetStatusWithChars( _broker, &rc, CMPI_RC_ERR_FAILED, "Create CMPIObjectPath failed." );
            return rc;
        }

        if ( (role == NULL  || ( strcasecmp(role,"affectingelement") == 0)) 
        && (resultRole == NULL || ( strcasecmp(resultRole,"affectedelement") == 0)) 
        && CMClassPathIsA(_broker,cop,"cim_indicationservice",&rc) == 1 ) { 
            // We were given an IndicationService, so we need to return 
            // IndicationFilters and ListenerDestinations
            // Get IndicationFilters
            buildObj(ctx,rslt,resultClass,ifop,isop,saeop,propertyList,target);
            // Get ListenerDestinations
            buildObj(ctx,rslt,resultClass,ldop,isop,saeop,propertyList,target);
        }
        if (( role == NULL || strcasecmp(role,"affectedelement") == 0) 
        && ( resultRole == NULL || strcasecmp(resultRole,"affectingelement") == 0) 
        && (( CMClassPathIsA(_broker,cop,"cim_indicationfilter",&rc) == 1) 
        || (CMClassPathIsA(_broker,cop,"cim_listenerdestination",&rc) == 1) )  ) { 
            // We were given either an IndicationFilter, or a ListenerDestination,
            if  ((strcasecmp(target,"Assocs") == 0 ) || (strcasecmp(target,"AssocNames") == 0 )) {
                // Here we need the IndicationService only
                buildObj(ctx,rslt,resultClass,isop,isop,saeop,propertyList,target);
            } else {
                // Here we need the refs for the given object
                buildObj(ctx,rslt,resultClass,cop,isop,saeop,propertyList,target);
            }
        }
    }

    // Handle CIM_HostedService

    if( ( assocClass==NULL ) || ( CMClassPathIsA(_broker,hsop,assocClass,&rc) == 1 ) ) {

        isop = CMNewObjectPath( _broker, CMGetCharPtr(CMGetNameSpace(cop,&rc)), "CIM_indicationservice", &rc );
        csop = CMNewObjectPath( _broker, "root/cimv2", "CIM_System", &rc );
        if (( csop==NULL ) || (isop==NULL) ) {
            CMSetStatusWithChars( _broker, &rc, CMPI_RC_ERR_FAILED, "Create CMPIObjectPath failed." );
            return rc;
        }

        if ( (role == NULL  || ( strcasecmp(role,"dependent") == 0)) 
        && (resultRole == NULL || ( strcasecmp(resultRole,"antecedent") == 0)) 
        && CMClassPathIsA(_broker,cop,"cim_indicationservice",&rc) == 1 ) { 
            // An IndicationService was passed in, so we need to return either the 
            // CIM_System instance or a CIM_HostedService association instance
            if  (((strcasecmp(target,"Assocs") == 0 ) || (strcasecmp(target,"AssocNames") == 0 )) 
            && (resultClass == NULL || ( strcasecmp(resultClass,"CIM_System") == 0)) ) {
                // Return the CIM_System instance
                cci=CMNewInstance(_broker,csop,&rc);
                makeCIM_System(cci);
                if (strcasecmp(target,"Assocs") == 0 ) {
                    if (propertyList) {
                        CMSetPropertyFilter(cci,propertyList,NULL);
                    }
                    CMReturnInstance(rslt, cci);
                } else {
                    CMReturnObjectPath(rslt,CMGetObjectPath(cci,NULL));
                }
                if(cci) CMRelease(cci);

            } else if (resultClass == NULL || ( strcasecmp(resultClass,"CIM_HostedService") == 0)) {
                // Return the CIM_HostedService instance
                makeHostedService(mi,ctx,rslt,isop,hsop,csop,propertyList,target);
            }

                
        }
        if (( role == NULL || strcasecmp(role,"antecedent") == 0) 
        && ( resultRole == NULL || strcasecmp(resultRole,"dependent") == 0) 
        && ( CMClassPathIsA(_broker,cop,"cim_system",&rc) == 1) ) { 
            // A CIM_System was passed in so wee need to return either the
            // CIM_IndicationService instance or a CIM_HostedService association instance
            if  ( ((strcasecmp(target,"Assocs") == 0 ) || (strcasecmp(target,"AssocNames") == 0 )) 
            && (resultClass == NULL || ( strcasecmp(resultClass,"CIM_IndicationService") == 0)) ) {
                // Return the CIM_IndicationService instance
                isenm = _broker->bft->enumerateInstances(_broker, ctx, isop, NULL, &rc);
                CMPIData inst=CMGetNext(isenm,&rc);
                if (strcasecmp(target,"Assocs") == 0 ) {
                    if (propertyList) {
                        CMSetPropertyFilter(inst.value.inst,propertyList,NULL);
                    }
                    CMReturnInstance(rslt, (inst.value.inst));
                } else {
                    CMReturnObjectPath(rslt,CMGetObjectPath(inst.value.inst,NULL));
                }
                if(isenm) CMRelease(isenm);
            } else if (resultClass == NULL || ( strcasecmp(resultClass,"CIM_HostedService") == 0)) {
                // Return the CIM_HostedService instance
                makeHostedService(mi,ctx,rslt,isop,hsop,csop,propertyList,target);
            }
        }
    }

    // Handle ElementConformstoProfile
    if( ( assocClass==NULL ) || ( CMClassPathIsA(_broker,ecpop,assocClass,&rc) == 1 ) ) {
        isop = CMNewObjectPath( _broker, CMGetCharPtr(CMGetNameSpace(cop,&rc)), "CIM_indicationservice", &rc );
        rpop = CMNewObjectPath( _broker, "root/interop", "SFCB_RegisteredProfile", &rc );
        if (( rpop==NULL ) || (isop==NULL) ) {
            CMSetStatusWithChars( _broker, &rc, CMPI_RC_ERR_FAILED, "Create CMPIObjectPath failed." );
            return rc;
        }
        if ( (role == NULL  || ( strcasecmp(role,"ManagedElement") == 0)) 
        && (resultRole == NULL || ( strcasecmp(resultRole,"ConformantStandard") == 0)) 
        && CMClassPathIsA(_broker,cop,"cim_indicationservice",&rc) == 1 ) { 
            // An IndicationService was passed in, so we need to return either the 
            // CIM_RegisteredProfile instance or a CIM_ElementConformstoProfile association instance
            if  (((strcasecmp(target,"Assocs") == 0 ) || (strcasecmp(target,"AssocNames") == 0 )) 
            && (resultClass == NULL || ( strcasecmp(resultClass,"SFCB_RegisteredProfile") == 0)) ) {
                // Return the CIM_RegisteredProfile instance
                //buildAssoc(ctx,rslt,rpop,propertyList,target);
                CMAddKey(rpop, "InstanceID", "CIM:SFCB_IP", CMPI_chars);
                if (strcasecmp(target,"AssocNames") == 0 ) {
                    CMReturnObjectPath(rslt, rpop);
                } else if (strcasecmp(target,"Assocs") == 0 ){
                    CMPIInstance * lci=CBGetInstance (_broker,ctx,rpop,NULL,&rc);
                    if (propertyList) {
                        CMSetPropertyFilter(lci,propertyList,NULL);
                    }
                    CMReturnInstance(rslt, lci);
                }
            } else if (resultClass == NULL || ( strcasecmp(resultClass,"CIM_ElementConformsToProfile") == 0)) {
                // Return the CIM_ElementConformsToProfile association
                makeElementConforms(mi,ctx,rslt,isop,ecpop,rpop,propertyList,target);
            }
        }
        if (( role == NULL || strcasecmp(role,"antecedent") == 0) 
        && ( resultRole == NULL || strcasecmp(resultRole,"dependent") == 0) 
        && ( CMClassPathIsA(_broker,cop,"cim_RegisteredProfile",&rc) == 1) ) { 
            // A CIM_RegisteredProfile was passed in so wee need to return either the
            // CIM_IndicationService instance or a CIM_ElementConformsToProfile association instance
            if  ( ((strcasecmp(target,"Assocs") == 0 ) || (strcasecmp(target,"AssocNames") == 0 )) 
            && (resultClass == NULL || ( strcasecmp(resultClass,"CIM_IndicationService") == 0)) ) {
                // Return the CIM_IndicationService instance
                buildAssoc(ctx,rslt,isop,propertyList,target);
            } else if (resultClass == NULL || ( strcasecmp(resultClass,"CIM_ElementConformsToProfile") == 0)) {
                // Return the CIM_ElementConformsToProfile association
                makeElementConforms(mi,ctx,rslt,isop,ecpop,rpop,propertyList,target);
            }
        }


    }

    CMReturnDone( rslt );
    CMReturn(CMPI_RC_OK);
}


CMPIStatus ServerProviderAssociators(CMPIAssociationMI * mi,
                                       const CMPIContext * ctx,
                                       const CMPIResult * rslt,
                                       const CMPIObjectPath * cop,
                                       const char *assocClass,
                                       const char *resultClass,
                                       const char *role,
                                       const char *resultRole,
                                       const char **propertyList)
{

    _SFCB_ENTER(TRACE_PROVIDERS, "ServerProviderAssociators");
    CMPIStatus rc = {CMPI_RC_OK, NULL};
    rc = getAssociators(mi,ctx,rslt,cop,assocClass,resultClass,role,resultRole,propertyList,"Assocs");
    _SFCB_RETURN(rc);
}

CMPIStatus ServerProviderAssociatorNames(CMPIAssociationMI * mi,
                                           const CMPIContext * ctx,
                                           const CMPIResult * rslt,
                                           const CMPIObjectPath * cop,
                                           const char *assocClass,
                                           const char *resultClass,
                                           const char *role,
                                           const char *resultRole)
{
    _SFCB_ENTER(TRACE_PROVIDERS, "ServerProviderAssociatorNames");
    CMPIStatus rc = {CMPI_RC_OK, NULL};
    rc = getAssociators(mi,ctx,rslt,cop,assocClass,resultClass,role,resultRole,NULL,"AssocNames");
    _SFCB_RETURN(rc);
}


/* ------------------------------------------------------------------------- */

CMPIStatus ServerProviderReferences(
	CMPIAssociationMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const char * resultClass,
	const char * role,
	const char ** propertyList)
{
   CMPIStatus rc = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, "ServerProviderReferences");
    rc = getAssociators(mi,ctx,rslt,cop,NULL,resultClass,role,NULL,propertyList,"Refs");
   _SFCB_RETURN(rc);
}

/* ------------------------------------------------------------------------- */

CMPIStatus ServerProviderReferenceNames(
	CMPIAssociationMI * mi,
	const CMPIContext * ctx,
	const CMPIResult * rslt,
	const CMPIObjectPath * cop,
	const char * resultClass,
	const char * role)
{
   CMPIStatus rc = { CMPI_RC_OK, NULL };
   _SFCB_ENTER(TRACE_PROVIDERS, "ServerProviderReferenceNames");
    rc = getAssociators(mi,ctx,rslt,cop,NULL,resultClass,role,NULL,NULL,"RefNames");
   _SFCB_RETURN(rc);
}


CMAssociationMIStub(ServerProvider, ServerProvider, _broker, CMNoHook);
