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

#include <cmci.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "cimslpCMPI.h"
#include "cimslpUtil.h"


char * interOpNS;

void initializeService(cimSLPService  *rs) {
	rs->AuthenticationMechanismsSupported = NULL;
	rs->AuthenticationMechansimDescriptions = NULL;
	rs->Classinfo = NULL;
	rs->CommunicationMechanism = NULL;
	rs->FunctionalProfileDescriptions = NULL;
	rs->FunctionalProfilesSupported = NULL;
	rs->InteropSchemaNamespace = NULL;
	rs->MultipleOperationsSupported = NULL;
	rs->Namespace = NULL;
	rs->OtherCommunicationMechanismDescription = NULL;
	rs->ProtocolVersion = NULL;
	rs->RegisteredProfilesSupported = NULL;
	rs->service_hi_description = NULL;
	rs->service_hi_name = NULL;
	rs->service_id = NULL;
	rs->url_syntax = NULL;
}


//helper function ... until better solution is found to get to the
//interop Namespace
char ** getInterOpNS() 
{
	char ** retArr;

	interOpNS = "root/pg_interop";
	
	retArr = malloc(2 * sizeof(char *));
	retArr[0] = (char *)malloc((strlen(interOpNS) + 1) * sizeof(char));
	strcpy(retArr[0], interOpNS);
	retArr[1] = NULL;
	
	return retArr;
}




CMPIInstance ** myGetInstances(CMCIClient *cc, char * path, char * objectname)
{
	CMPIStatus status;
	CMPIObjectPath * objectpath;
	CMPIEnumeration * enumeration;
	CMPIInstance ** retArr = NULL;

	objectpath = newCMPIObjectPath(path, objectname, NULL);

	enumeration = cc->ft->enumInstances(cc, objectpath, 0, NULL, &status);

	//severe error, cimom not running etc.
	if(status.rc == CMPI_RC_ERR_FAILED) {
		printf("Fatal error. CIMOM not running? Connection params ok?\n");
		exit(0);
	}
	
	//object not found ?
	if(status.rc == CMPI_RC_ERR_INVALID_CLASS || status.rc == CMPI_RC_ERR_NOT_FOUND) {
		retArr = NULL;
	}
	
	if (!status.rc) {
		if (enumeration->ft->hasNext(enumeration, NULL)) {
			CMPIArray * arr;
			int n,i;

			arr = enumeration->ft->toArray(enumeration, NULL);
			n = CMGetArrayCount(arr, NULL);
			retArr = malloc(sizeof(CMPIInstance *) * (n + 1));
			for(i = 0; i < n; i++) {
				CMPIData ele = CMGetArrayElementAt(arr, i, NULL);
				retArr[i] = ele.value.inst->ft->clone(ele.value.inst, NULL);
			}
			retArr[n] = NULL;
		}
	}
	if (status.msg) CMRelease(status.msg);	
	if (objectpath) CMRelease(objectpath);
	if (enumeration) CMRelease(enumeration);				
	return retArr;
}

CMPIConstClass * myGetClass(CMCIClient *cc, char * path, char * objectname)
{
	CMPIStatus status;
	CMPIObjectPath * objectpath;
	CMPIConstClass * ccls;

	objectpath = newCMPIObjectPath(path, objectname, &status);

	ccls = cc->ft->getClass(cc, objectpath, CMPI_FLAG_IncludeQualifiers, NULL, &status);
	
	if (objectpath) CMRelease(objectpath);	
	
	if (!status.rc) {
		return ccls;
	} else {
		//printf("Could not get class... ?\n");
		//printf("Status: %d\n", status.rc);
		return NULL;
	}



}

cimSLPService getSLPData(cimomConfig cfg)
{
	CMCIClient *cc;
	CMPIInstance **ci;
	CMPIStatus status;
	CMPIConstClass *ccls;

	cimSLPService rs; //service which is going to be returned to the calling function
	initializeService(&rs);

	cc = cmciConnect(cfg.cimhost,
					cfg.commScheme,
					cfg.port,
					cfg.cimuser,
					cfg.cimpassword,
					&status);
	
	if(status.rc) {
		printf("Could not connect to CIMOM. Check if it is running as well as your parameters.\n");
		exit(0);
	}
	

	//first of all, get the interop namespace, needed for all further connections
	//this call fills the array as well as sets the global interOpNS variable
	rs.InteropSchemaNamespace = getInterOpNS();

	//extract all relavant stuff for SLP out of CIM_ObjectManager

	//construct the server string
	ci = myGetInstances(cc, interOpNS, "CIM_ObjectManager");
	if(ci) {
		rs.url_syntax = getUrlSyntax(myGetProperty(ci[0], "SystemName"),
									cfg.commScheme, cfg.port);	
		rs.service_hi_name = myGetProperty(ci[0], "ElementName");
		rs.service_hi_description = myGetProperty(ci[0], "Description");
		rs.service_id = myGetProperty(ci[0], "Name");
		freeInstArr(ci);
	}


	//extract all relavant stuff for SLP out of CIM_ObjectManagerCommunicationMechanism
	ci = myGetInstances(cc, interOpNS, "CIM_ObjectManagerCommunicationMechanism");
	if(ci) {
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
		freeInstArr(ci);
	}
	
	//extract all relavant stuff for SLP out of CIM_Namespace
	ci = myGetInstances(cc, interOpNS, "CIM_Namespace");
	if(ci) {
		rs.Namespace = myGetPropertyArrayFromArray(ci, "Name");
		rs.Classinfo = myGetPropertyArrayFromArray(ci, "ClassInfo");
		freeInstArr(ci);
	}
	
	//extract all relavant stuff for SLP out of CIM_RegisteredProfile
	ci = myGetInstances(cc, interOpNS, "CIM_RegisteredProfile");
	if(ci) {
		rs.RegisteredProfilesSupported = myGetRegProfiles(ci, cc);
		freeInstArr(ci);
	}
	
	//do the transformations from numbers to text via the qualifiers
	ccls = myGetClass(cc, interOpNS, "CIM_ObjectManagerCommunicationMechanism");
	if(ccls) {
		rs.CommunicationMechanism = transformValue(rs.CommunicationMechanism,
					ccls, "CommunicationMechanism");
		rs.FunctionalProfilesSupported = transformValueArray(rs.FunctionalProfilesSupported,
					ccls, "FunctionalProfilesSupported");
		rs.AuthenticationMechanismsSupported = transformValueArray(rs.AuthenticationMechanismsSupported,
					ccls, "AuthenticationMechanismsSupported");
		CMRelease(ccls);
	}

	if (cc) CMRelease(cc);

	return rs;
				
}

char ** myGetRegProfiles(CMPIInstance **instances, CMCIClient *cc)
{
    CMPIObjectPath * objectpath;
	CMPIEnumeration * enumeration = NULL;
    CMPIStatus status;
    CMPIConstClass * ccls;
	char ** retArr;
	int i,j=0;
	
	//count instances
	for(i = 0; instances != NULL && instances[i] != NULL; i++){}

    if(i == 0) {
		return NULL;
	}
				
	//allocating memory for the return array
	//a little too much memory will be allocated, since not each instance is a RegisteredProfile, for which a
	//string needs to be constructed ... but allocating dynamically would involve too much burden and overhead (?)

	retArr = (char **) malloc((i+1) * sizeof(char *));
		
	//getting the class including qualifiers, needed to resolve RegisteredOrganization (Values/ValueMap)
	ccls = myGetClass(cc, interOpNS, "CIM_RegisteredProfile");		
		
		
    for(i = 0; instances[i] != NULL; i++) {
		objectpath = instances[i]->ft->getObjectPath(instances[i], &status);
		if(status.rc) {
			//no object path ??
			return NULL;
		}
		objectpath->ft->setNameSpace(objectpath, interOpNS);
		
		if (enumeration) CMRelease(enumeration);
		enumeration = cc->ft->associatorNames(cc,
										objectpath,
										"CIM_SubProfileRequiresProfile",
										NULL,
										"Dependent",
										NULL,
										NULL);

		//if the result is not null, we are operating on a CIM_RegisteredSubProfile, which we don't want
		if(!enumeration || !enumeration->ft->hasNext(enumeration, &status)) {
			CMPIData propertyData;
			
			propertyData = instances[i]->ft->getProperty(instances[i],
													"RegisteredOrganization",
													&status);
	        retArr[j] = value2Chars(propertyData.type, &propertyData.value);

			retArr[j] = transformValue(retArr[j], ccls, "RegisteredOrganization");
	        
			propertyData = instances[i]->ft->getProperty(instances[i],
													"RegisteredName",
													&status);
			
			char * tempString = value2Chars(propertyData.type, &propertyData.value);
						
			retArr[j] = realloc(retArr[j], strlen(retArr[j]) + strlen(tempString) + 2);
			retArr[j] = strcat(retArr[j], ":");
			retArr[j] = strcat(retArr[j], tempString);
			free(tempString);

			//now search for a CIM_RegisteredSubProfile for this instance
			if (enumeration) CMRelease(enumeration);
			enumeration = cc->ft->associators(cc,
										objectpath,
										"CIM_SubProfileRequiresProfile",
										NULL,
										"Antecedent",
										NULL,
										0,
										NULL,
										NULL);
			if(!enumeration || !enumeration->ft->hasNext(enumeration, NULL)) {
				j++;
			} else 
			while(enumeration->ft->hasNext(enumeration, &status)) {
				CMPIData data = enumeration->ft->getNext(enumeration, NULL);
				propertyData = data.value.inst->ft->getProperty(data.value.inst,
															"RegisteredName",
															&status);
				char * subprofilestring = value2Chars(propertyData.type,
													&propertyData.value);
				retArr[j] = realloc(retArr[j],
								strlen(retArr[j]) + strlen(subprofilestring) + 2);
				retArr[j] = strcat(retArr[j], ":");
				retArr[j] = strcat(retArr[j], subprofilestring);
				j++;
				free(subprofilestring);
			}
		}
		if (objectpath) CMRelease(objectpath);
	}
	retArr[j] = NULL;
	
	if (enumeration) CMRelease(enumeration);	
	if (ccls) CMRelease(ccls);
	if (status.msg) CMRelease(status.msg);

	
	return retArr;
}

char ** transformValueArray(char ** cssf, CMPIConstClass * ccls, char * propertyName)
{
	int i;

	for(i = 0; cssf[i] != NULL; i++) {
		cssf[i] = transformValue(cssf[i], ccls, propertyName);
	}
	return cssf;
}

//transforms numerical values into their string counterpart
//utilizing the Values and ValueMap qualifiers
char * transformValue(char* cssf, CMPIConstClass * ccls, char * propertyName)
//cssf = cimSLPService Field in the struct
{
	CMPIData qd;
	CMPIStatus status;
	char * valuestr;

	qd=ccls->ft->getPropertyQualifier(ccls, propertyName, "ValueMap", &status);
	if (status.rc) {
		printf("getPropertyQualifier failed ... Status: %d\n", status.rc);
		return NULL;
	}
   
	if (CMIsArray(qd)) {
		CMPIArray *arr   = qd.value.array;
		CMPIType  eletyp = qd.type & ~CMPI_ARRAY;
		int j = 0;
		int n;
		n = CMGetArrayCount(arr, NULL);
		CMPIData ele;
			ele = CMGetArrayElementAt(arr, j, NULL);
			valuestr = value2Chars(eletyp, &ele.value);
			j++;
		while(strcmp(valuestr, cssf)) {
			free(valuestr);
			ele = CMGetArrayElementAt(arr, j, NULL);
			valuestr = value2Chars(eletyp, &ele.value);
			j++;
		}
		free(valuestr);
		free(cssf);
		if(j-1 <= n) {
			qd=ccls->ft->getPropertyQualifier(ccls, propertyName, "Values", &status);
			arr   = qd.value.array;
			eletyp = qd.type & ~CMPI_ARRAY;
			ele = CMGetArrayElementAt(arr, j-1, NULL);
			cssf = value2Chars(eletyp, &ele.value);
			return cssf;
		} else {
			//printf("No Valuemap Entry for %s in %s. Exiting ...\n", cssf, propertyName);
			return NULL;
		}
	}
	
	else {
		//printf("No qualifier found for %s. Exiting ...\n", propertyName);
		return NULL;
	}
}

char * myGetProperty(CMPIInstance *instance, char * propertyName) 
{

	CMPIData propertyData;
	CMPIStatus status;

	if(! instance) return NULL;
	
	propertyData = instance->ft->getProperty(instance, propertyName, &status);

	if (!status.rc) {
		return value2Chars(propertyData.type, &propertyData.value);
	} else {
		return NULL;
	}
}

char ** myGetPropertyArrayFromArray(CMPIInstance **instances, char *propertyName)
{
	int i;
	char **propertyArray;


	//count elements
	for(i = 0; instances != NULL && instances[i] != NULL; i++){}

	if(i == 0) {
		return NULL;
	}

	propertyArray = malloc((i + 1) * sizeof(char*));
		
	for(i = 0; instances[i] != NULL; i++) {
		propertyArray[i] =  myGetProperty(instances[i], propertyName);
	}
	propertyArray[i] = NULL;
	return propertyArray;
	
}

char ** myGetPropertyArray(CMPIInstance *instance, char *propertyName)
{

	CMPIData propertyData;
	CMPIStatus status;
	char **propertyArray = NULL;

	propertyData = instance->ft->getProperty(instance, propertyName, &status);
	if (!status.rc) {
		if(CMIsArray(propertyData)) {
			CMPIArray *arr = propertyData.value.array;
			CMPIType  eletyp = propertyData.type & ~CMPI_ARRAY;
			int n, i;
			n = CMGetArrayCount(arr, NULL);
			propertyArray = malloc(sizeof(char*)*(n + 1));
			for(i = 0; i < n; i++) {
				CMPIData ele = CMGetArrayElementAt(arr, i, NULL);
				propertyArray[i] = value2Chars(eletyp, &ele.value);
			}
			propertyArray[n] = NULL;

		}
	}
	return propertyArray;
}

char * getUrlSyntax(char* sn, char * cs, char * port)
{
	char * url_syntax;
	
	//colon, double slash, colon, \0, service:wbem = 18
	url_syntax = (char *) malloc((strlen(sn) + strlen(cs) + strlen(port) + 18) * sizeof(char));
	sprintf(url_syntax, "service:wbem:%s://%s:%s", cs, sn, port);
	
	free(sn);
	
	return url_syntax;
}
