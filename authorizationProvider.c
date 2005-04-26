
/*
 * authorizationEnum.c
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
 * Author:       Gareth Bestor 
 * Contributors: Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Authorization provider for sfcb
 *
*/


#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"
#include "trace.h"

static CMPIBroker * _BROKER;

/* Include the support utility functions */
#include "authorizationUtil.h"

/* Include the customized instance enumeration functions */
#include "authorizationEnum.c"


/* ---------------------------------------------------------------------------
 * CMPI INSTANCE PROVIDER FUNCTIONS
 * --------------------------------------------------------------------------- */

/*
 * EnumInstanceNames
 */
CMPIStatus SFC_AuthorizationEnumInstanceNames(
		CMPIInstanceMI * mi, 
		CMPIContext * context, 
		CMPIResult * results, 
		CMPIObjectPath * reference) 
{
   CMPIStatus status = {CMPI_RC_OK, NULL};      /* CIM return status */
   CMPIInstance * instance;                     /* CIM instance for each new instance */
   CMPIObjectPath * objectpath;			/* CIM object path of each new instance */
   void * instances;				/* Handle for the list of instances */
   char * namespace = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Current CIM namespace */
   int count = 0;				/* Number of instances */
   
   _SFCB_ENTER(TRACE_PROVIDERS, "SFC_AuthorizationEnumInstanceNames");

   /* Get handle for the list of instances */
   if ((instances = _startReadingInstances()) == NULL) {
      _SFCB_TRACE(1,("EnumInstanceNames() : Failed to get list of instances"));
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Failed to get list of instances");
   }

   /* Enumerate all the instances and return a new CIM object path for each */
   while (_readNextInstance(instances, &instance, namespace) != EOF) {
      if (!CMIsNullObject(instance)) {
         count++;
         _SFCB_TRACE(1,("EnumInstanceNames() : Found instance #%d", count));
         /* Return the object path of this instance */
	 objectpath = CMGetObjectPath(instance, NULL);
	 CMSetNameSpace(objectpath, namespace);
	 CMReturnObjectPath(results, objectpath); 
      }
   }
   _endReadingInstances(instances);

   /* Check if found any instances */
   if (count == 0) {
      _SFCB_TRACE(1,("EnumInstanceNames() : No instances found"));
   }

   /* Finished */
   CMReturnDone(results);
   _SFCB_TRACE(1,("EnumInstanceNames() %s",(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
   return status;
}


/*
 * EnumInstances
 */
CMPIStatus SFC_AuthorizationEnumInstances(
		CMPIInstanceMI * mi, 
		CMPIContext * context, 
		CMPIResult * results, 
		CMPIObjectPath * reference, 
		char ** properties) 
{
   CMPIStatus status = {CMPI_RC_OK, NULL};      /* CIM return status */
   CMPIInstance * instance;			/* CIM instance for each new instance */
   void * instances;				/* Handle for the list of instances */
   char * namespace = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Current CIM namespace */
   int count = 0;                               /* Number of instances found */

   _SFCB_ENTER(TRACE_PROVIDERS, "SFC_AuthorizationEnumInstances");

   /* Get handle for the list of instances */
   if ((instances = _startReadingInstances()) == NULL) {
      _SFCB_TRACE(1,("EnumInstances() : Failed to get list of instances"));
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Failed to get list of instances");
   }

   /* Enumerate all the instances and return a new CIM instance for each */
   while (_readNextInstance(instances, &instance, namespace) != EOF) {
      if (!CMIsNullObject(instance)) {
	 count++;
         _SFCB_TRACE(1,("EnumInstances() : Found instance #%d", count));
         /* Return this instance */
         CMReturnInstance(results, instance);
      }
   }
   _endReadingInstances(instances);

   /* Check if found any instances */
   if (count == 0) {
      _SFCB_TRACE(1,("EnumInstances() : No instances found"));
   }

   /* Finished */
   CMReturnDone(results);
   _SFCB_TRACE(1,("EnumInstances() %s",(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
   return status;
}


/*
 * GetInstance
 */
CMPIStatus SFC_AuthorizationGetInstance(
		CMPIInstanceMI * mi, 
		CMPIContext * context, 
		CMPIResult * results, 
		CMPIObjectPath * reference, 
		char ** properties) 
{
   CMPIStatus status = {CMPI_RC_OK, NULL};      /* CIM return status */
   CMPIInstance * instance;      		/* CIM instance for each new instance */
   void * instances;                            /* Handle for the list of instances */
   char * namespace = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Current CIM namespace */
   int found = 0;                               /* Was the desired reference object found? */

   _SFCB_ENTER(TRACE_PROVIDERS, "SFC_AuthorizationGetInstance");

   /* Get handle for the list of instances */
   if ((instances = _startReadingInstances()) == NULL) {
      _SFCB_TRACE(1,("GetInstance() : Failed to get list of instances"));
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Failed to get list of instances");
   }

   /* Enumerate all the instances until we find the desired CIM instance */
   while (_readNextInstance(instances, &instance, namespace) != EOF) {
      if (!CMIsNullObject(instance)) {
	 /* Check if this instance matches the desired instance */
         if (_CMSameObject(CMGetObjectPath(instance, NULL), reference)) {
            found = 1;
            break;
         }
      } 
   }
   _endReadingInstances(instances);

   /* Check if found the desired instance */
   if (found && !CMIsNullObject(instance)) {
      /* Return the found instance */
      _SFCB_TRACE(1,("GetInstance() : Found requested instance"));
      CMReturnInstance(results, instance);
   }
   else {
      _SFCB_TRACE(1,("GetInstance() : Requested instance not found"));
      CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED, "Requested instance not found");
   } 

   /* Finished */
   CMReturnDone(results);
   _SFCB_TRACE(1,("GetInstance() %s",(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
   return status;
}


/*
 * SetInstance
 */
CMPIStatus SFC_AuthorizationSetInstance(
		CMPIInstanceMI * mi, 
		CMPIContext * context, 
		CMPIResult * results, 
		CMPIObjectPath * reference,
		CMPIInstance * newinstance, 
		char **properties) 
{
   CMPIStatus status = {CMPI_RC_OK, NULL};	/* CIM return status */
   CMPIInstance * instance;                     /* CIM instance for each new instance */
   void * instances;                            /* Handle for the old list of instances */
   void * newinstances;				/* Handle for the new list of instances */
   char * namespace = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Current CIM namespace */
   int found = 0;				/* Was the desired reference object found? */

   _SFCB_ENTER(TRACE_PROVIDERS, "SFC_AuthorizationSetInstance");
   
   /* Get handle for the old list of instances */
   if ((instances = _startReadingInstances()) == NULL) {
      _SFCB_TRACE(1,("SetInstance() : Failed to get old list of instances"));
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Failed to get old list of instances");
   }

   /* Start writing a new list of instances */
   if ((newinstances = _startWritingInstances()) == NULL) {
      _SFCB_TRACE(1,("SetInstance() : Failed to start new list of instances"));
      _endReadingInstances(instances);
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Failed to start new list of instances");
   }

   /* Enumerate all the old instances and copy them to the new instances */
   while (_readNextInstance(instances, &instance, namespace) != EOF) {
      if (!CMIsNullObject(instance)) {
	 /* Check if this instance matches the desired instance */
         if (_CMSameObject(CMGetObjectPath(instance, NULL), reference)) {
            /* Replace the old instance with the new instance */
	    instance = newinstance;
	    found = 1;
         }

         /* Copy this instance to the list of new instances */
         if (!_writeNextInstance(newinstances, instance)) {
            _SFCB_TRACE(1,("SetInstance() : Failed to write instance"));
            CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED, "Failed to write instance");
	    break;
	 }
      } 
   }
   _endReadingInstances(instances);

   /* Check if found the desired instance */
   if ((status.rc == CMPI_RC_OK) && !found) {
      _SFCB_TRACE(1,("SetInstance() : Requested instance not found"));
      CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED, "Requested instance not found");
   }

   /* Commit the changes only if everything worked OK */
   _endWritingInstances(newinstances, (status.rc == CMPI_RC_OK));

   /* Finished */
   CMReturnDone(results);
   _SFCB_TRACE(1,("SetInstance() %s",(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
   return status;
}


/*
 * CreateInstance
 */
CMPIStatus SFC_AuthorizationCreateInstance(
                CMPIInstanceMI * mi,
                CMPIContext * context,
                CMPIResult * results,
                CMPIObjectPath * reference,
                CMPIInstance * newinstance)
{
   CMPIStatus status = {CMPI_RC_OK, NULL};      /* CIM return status */
   CMPIInstance * instance;                     /* CIM instance for each new instance */
   CMPIObjectPath * newobjectpath;		/* CIM object path of the new instance */
   void * instances;                            /* Handle for the old list of instances */
   void * newinstances;                         /* Handle for the new list of instances */
   char * namespace = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Current CIM namespace */
   int found = 0;                               /* Does the desired reference object already exist */

   _SFCB_ENTER(TRACE_PROVIDERS, "SFC_AuthorizationCreateInstance");
   
   /* Get the old list of instances */
   if ((instances = _startReadingInstances()) == NULL) {
      _SFCB_TRACE(1,("CreateInstance() : Failed to get old list of instances"));
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Failed to get old list of instances");
   }

   /* Start writing a new list of instances */
   if ((newinstances = _startWritingInstances()) == NULL) {
      _SFCB_TRACE(1,("CreateInstance() : Failed to start new list of instances"));
      _endReadingInstances(instances);
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Failed to start new list of instances");
   }

   /* Enumerate all the old instances and copy them to the new instances */
   while (_readNextInstance(instances, &instance, namespace) != EOF) {
      if (!CMIsNullObject(instance)) {
         /* Check if this instance matches the desired instance */
//         if (_CMSameObject(CMGetObjectPath(instance, &status), reference)) {
         if (_CMSameObject(CMGetObjectPath(instance, &status), CMGetObjectPath(newinstance, &status))) {
            /* New instance already exists! */
	    found = 1;
	    break;
         }

         /* Copy this instance to the list of new instances */
         if (!_writeNextInstance(newinstances, instance)) { 
            _SFCB_TRACE(1,("CreateInstance() : Failed to write instance"));
            CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED, "Failed to write instance");
	    break;
	 }
      }
   }
   _endReadingInstances(instances);

   /* Check if found the desired instance */
   if ((status.rc == CMPI_RC_OK) && found) {
      _SFCB_TRACE(1,("CreateInstance() : Instance already exists"));
      CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED, "Instance already exists");
   }

   /* Write the new instance to the end of the list of instances */
   if ((status.rc == CMPI_RC_OK) && !_writeNextInstance(newinstances, newinstance)) {
      _SFCB_TRACE(1,("CreateInstance() : Failed to write new instance"));
      CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED, "Failed to write new instance");
   }

   /* Commit the changes only if everything worked OK */
   _endWritingInstances(newinstances, (status.rc == CMPI_RC_OK));

   /* Return the object path of the new instance */
   if (status.rc == CMPI_RC_OK) {
      newobjectpath = CMGetObjectPath(newinstance, NULL);
      CMSetNameSpace(newobjectpath, namespace);
      CMReturnObjectPath(results, newobjectpath);
   }
      
   /* Finished */
   CMReturnDone(results);
   _SFCB_TRACE(1,("CreateInstance() %s",(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
   return status;
}


/*
 * DeleteInstance
 */
CMPIStatus SFC_AuthorizationDeleteInstance(
		CMPIInstanceMI * mi, 
		CMPIContext * context, 
		CMPIResult * results, 
		CMPIObjectPath * reference) 
{
   CMPIStatus status = {CMPI_RC_OK, NULL};      /* CIM return status */
   CMPIInstance * instance;                     /* CIM instance for each new instance */
   void * instances;                            /* Handle for the old list of instances */
   void * newinstances;                         /* Handle for the new list of instances */
   char * namespace = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Current CIM namespace */
   int found = 0;                               /* Was the desired reference object found? */

   _SFCB_ENTER(TRACE_PROVIDERS, "SFC_AuthorizationDeleteInstance");
   
   /* Get the old list of instances */
   if ((instances = _startReadingInstances()) == NULL) {
      _SFCB_TRACE(1,("DeleteInstance() : Failed to get old list of instances"));
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Failed to get old list of instances");
   }

   /* Start a new list of instances */
   if ((newinstances = _startWritingInstances()) == NULL) {
      _SFCB_TRACE(1,("DeleteInstance() : Failed to start new list of instances"));
      _endReadingInstances(instances);
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Failed to start new list of instances");
   }

   /* Enumerate all the old instances and copy them to the new instances */
   while (_readNextInstance(instances, &instance, namespace) != EOF) {
      if (!CMIsNullObject(instance)) {
         /* Check if this instance matches the desired instance */
         if (_CMSameObject(CMGetObjectPath(instance, &status), reference)) {
	    /* Dont copy over this instance */
            found = 1;
	    continue;
         }
           
	 /* Copy the instance to the list of new instances */
         if (!_writeNextInstance(newinstances, instance)) {
            _SFCB_TRACE(1,("DeleteInstance() : Failed to write instance"));
            CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED, "Failed to write instance");
	    break;
         }
      }
   }
   _endReadingInstances(instances);

   /* Check if found the desired instance */
   if ((status.rc == CMPI_RC_OK) && !found) {
      _SFCB_TRACE(1,("DeleteInstance() : Requested instance not found"));
      CMSetStatusWithChars(_BROKER, &status, CMPI_RC_ERR_FAILED, "Requested instance not found");
   }

   /* Commit the changes only if everything worked OK */
   _endWritingInstances(newinstances, (status.rc == CMPI_RC_OK));
   
   /* Finished */
   CMReturnDone(results);
   _SFCB_TRACE(1,("DeleteInstance() %s",(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
   return status;
}


/*
 * ExecQuery
 */
CMPIStatus SFC_AuthorizationExecQuery(
		CMPIInstanceMI * mi, 
		CMPIContext * context, 
		CMPIResult * results, 
		CMPIObjectPath * reference, 
		char * language, 
		char * query) 
{
   /* Cannot execute queries against instances */
   CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}


/*
 * Cleanup
 */
CMPIStatus SFC_AuthorizationCleanup(
                CMPIInstanceMI * mi,
                CMPIContext * context)
{
//   CMPIStatus st={CMPI_RC_DO_NOT_UNLOAD,NULL};
   CMPIStatus st={CMPI_RC_OK,NULL};
   _SFCB_ENTER(TRACE_PROVIDERS, "SFC_AuthorizationCleanup");
   /* Nothing needs to be done for cleanup */
 //  CMReturn(CMPI_RC_OK);CMPI_RC_NEVER_UNLOAD
   _SFCB_RETURN(st);
   //CMReturn(CMPI_RC_NEVER_UNLOAD);
}


/*
 * Factory method 
 */
CMInstanceMIStub(SFC_Authorization , SFC_Authorization, _BROKER, CMNoHook);



/* ---------------------------------------------------------------------------
 * CMPI METHOD PROVIDER FUNCTIONS
 * --------------------------------------------------------------------------- */

/*
 * InvokeMethod
 */
CMPIStatus SFC_AuthorizationInvokeMethod(
		CMPIMethodMI * mi,
		CMPIContext * context,
		CMPIResult * results,
		CMPIObjectPath * reference,
		const char * methodname,
		CMPIArgs * argsin,
		CMPIArgs * argsout)
{
   CMPIStatus status = {CMPI_RC_OK, NULL};	/* CIM return status */
   CMPIData data;				/* General purpose CIM data storage for CIM property values */
   char * username = NULL;			/* Target Username, obtained from reference objectpath */
   char * classname = NULL;			/* Target Classname, obtained from reference objectpath */
   char * operation =  NULL;			/* Requested operation, obtained from input args */
//   char * operation = "Set";
   CMPIValue value;				/* Method's success/fail authorization result */
   CMPIInstance * instance;                     /* CIM instance for each new instance */
   void * instances;                            /* Handle for the list of instances */
   char * namespace = CMGetCharPtr(CMGetNameSpace(reference, NULL)); /* Current CIM namespace */
   int found = 0;                               /* Was the desired reference object found? */

   _SFCB_ENTER(TRACE_PROVIDERS, "SFC_AuthorizationInvokeMethod");

   if (strcmp(methodname,"call") == 0) {
      char *rv="just trying";
      CMPIStatus st={CMPI_RC_OK,NULL};
      CMReturnData(results, rv, CMPI_chars);
      CMAddArg(argsout,"test",rv, CMPI_chars);
      return st;
   }
   /* Get the target username */
   data = CMGetKey(reference, "Username", &status);
   if ((status.rc == CMPI_RC_OK) && !CMIsNullValue(data)) username = CMGetCharPtr(data.value.string);
   if (username == NULL || *username == '\0') {
      _SFCB_TRACE(1,("InvokeMethod() : Cannot determine target username"));
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Cannot determine target username");
   }

   /* Get the target classname */
   data = CMGetKey(reference, "Classname", &status);
   if ((status.rc == CMPI_RC_OK) && !CMIsNullValue(data)) classname = CMGetCharPtr(data.value.string);
   if (classname == NULL || *classname == '\0') {
      _SFCB_TRACE(1,("InvokeMethod() : Cannot determine target classname"));
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Cannot determine target classname");
   }

   /* Get the requested CMPI operation (the first input arg) */
   if (CMGetArgCount(argsin, NULL) >= 1) {
      data = CMGetArgAt(argsin, 0, NULL, &status);
      if ((status.rc == CMPI_RC_OK) && !CMIsNullValue(data)) operation = CMGetCharPtr(data.value.string);
   }
   if (operation == NULL || *operation == '\0') {
      _SFCB_TRACE(1,("InvokeMethod() : Cannot determine requested CMPI operation"));
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Cannot determine requested CMPI operation");
   }

//fprintf(stderr,"%s:%s - %s(%s)\n",username,classname,methodname,operation);

   /* Determine if the user is authorized for the desired CMPI operation on the class */
   if (strcmp(methodname,"IsAuthorized") == 0) {

      /* Get handle for the list of instances */
      if ((instances = _startReadingInstances()) == NULL) {
         _SFCB_TRACE(1,("InvokeMethod() : Failed to get list of instances"));
         CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Failed to get list of instances");
      }
      
      /* Enumerate all the instances until we find the desired CIM instance */
      while (_readNextInstance(instances, &instance, namespace) != EOF) {
         if (!CMIsNullObject(instance)) {
            /* Check if this instance matches the desired instance */
            if (_CMSameObject(CMGetObjectPath(instance, NULL), reference)) {
               found = 1;
               break;
            }
         }
      }
      _endReadingInstances(instances);
      
      /* By default, if no information then not authorized */
      /*** SPECIAL CASE: the root user is implicitly authorized for all classes/operations! ***/
      value.boolean = ((strcmp(username,"root") == 0)? 1:0);
      
      /* Check if found the desired instance */
      if (found && !CMIsNullObject(instance)) {
         data = CMGetProperty(instance,operation,&status);
         if ((status.rc == CMPI_RC_OK) && !CMIsNullValue(data)) value.boolean = data.value.boolean;
      }

      /* Return the result of the authorization request */ 
      CMReturnData(results, &value, CMPI_boolean);
   } else {
      _SFCB_TRACE(1,("InvokeMethod() : Unrecognized method '%s'",methodname));
      CMReturnWithChars(_BROKER, CMPI_RC_ERR_FAILED, "Unrecognized method");
   }

   /* Finished */
   CMReturnDone(results);
   _SFCB_TRACE(1,("InvokeMethod() %s",(status.rc == CMPI_RC_OK)? "succeeded":"failed"));
   return status;
}


/*
 * MethodCleanup
 */
CMPIStatus SFC_AuthorizationMethodCleanup(
		CMPIMethodMI * mi,
                CMPIContext * context)
{
//   CMPIStatus st={CMPI_RC_NEVER_UNLOAD,NULL};
   CMPIStatus st={CMPI_RC_OK,NULL};
   _SFCB_ENTER(TRACE_PROVIDERS, "SFC_AuthorizationMethodCleanup");
   _SFCB_RETURN(st);
}


/*
 * Factory method
 */
CMMethodMIStub(SFC_Authorization , SFC_Authorization, _BROKER, CMNoHook);

