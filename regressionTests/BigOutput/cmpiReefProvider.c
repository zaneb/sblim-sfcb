
/*
 * cmpiReefProvider.c
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
 * Author:       
 *
 * Description:
 *
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"
#include "cmpiReefProvider.h"

typedef unsigned char UCHAR;
typedef int  INT32;


    typedef struct SmRasMrLogicalVolumeStruct
    {
        UCHAR storFacilImageMTMS[18];
        INT32 volumeType;
        UCHAR logicalVolumeNum[3];
        UCHAR logicalSubsystem[2];
        INT32 lssGroup;
        UCHAR addressGroup[2];
        INT32 aliasVolumeGroup;
        UCHAR origBaseLogVolNum[3];
        UCHAR userName[17];
        INT32 segPool;
        INT32 dataType;
        INT32 dynamicSegAlloc;
        INT32 dataSharingAllowed;
        INT32 overridDefPriority;
        INT32 defPriorityOverVal;
        INT32 ckdPriorityOffset;
        INT32 requestedCapacity;
        INT32 capacity;
        INT32 deviceMTM;
        UCHAR ckdVolumeSerialNum[7];
        INT32 accessState;
        INT32 dataState;
        INT32 configurationState;
        INT32 creationDate;
        UCHAR codeLevel[17];
    } SmRasMrLogicalVolume;

static CMPIBroker * _broker;
static char * _ClassName     = "Reef_LogicalVolume";

#define	REEF_MAX_STRING_SIZE		256
//#define	REEF_NUM_INSTANCES		16000
#define	REEF_NUM_INSTANCES		4000 

// ==========================================================================

SmRasMrLogicalVolume next_lv; 

// ==========================================================================

// Property names.  These values are returned by the provider as
// the property names.
// ==========================================================================
#define PROPERTY_LOGICAL_VOLUME_STORAGE_FACILITY_IMAGE_MTMS		"storageFacilityImageMTMS"
#define PROPERTY_LOGICAL_VOLUME_VOLUME_TYPE				"volumeType"
#define PROPERTY_LOGICAL_VOLUME_LOGICAL_VOLUME_NUM			"logicalVolumeNum"
#define PROPERTY_LOGICAL_VOLUME_LOGICAL_SUBSYSTEM			"logicalSubsystem"
#define PROPERTY_LOGICAL_VOLUME_LSS_GROUP				"lssGroup"
#define PROPERTY_LOGICAL_VOLUME_ADDRESS_GROUP				"addressGroup"
#define PROPERTY_LOGICAL_VOLUME_ALIAS_VOLUME_GROUPE			"aliasVolumeGroup"
#define PROPERTY_LOGICAL_VOLUME_ORIG_BASE_LOG_VOL_NUM			"origBaseLogVolNum"
#define PROPERTY_LOGICAL_VOLUME_USER_NAME				"userName"
#define PROPERTY_LOGICAL_VOLUME_SEG_POOL				"segPool"
#define PROPERTY_LOGICAL_VOLUME_DATA_TYPE				"dataType"
#define PROPERTY_LOGICAL_VOLUME_DYNAMIC_SEG_ALLOC			"dynamicSegAlloc"
#define PROPERTY_LOGICAL_VOLUME_DATA_SHARING_ALLOWED			"dataSharingAllowed"
#define PROPERTY_LOGICAL_VOLUME_OVERRIDE_DEF_PRIORITY			"overridDefPriority"
#define PROPERTY_LOGICAL_VOLUME_DEF_PRIORITY_OVER_VAL			"defPriorityOverVal"
#define PROPERTY_LOGICAL_VOLUME_CKD_PRIORITY_OFFSET			"ckdPriorityOffset"
#define PROPERTY_LOGICAL_VOLUME_REQUESTED_CAPACITY			"requestedCapacity"
#define PROPERTY_LOGICAL_VOLUME_CAPACITY				"capacity"
#define PROPERTY_LOGICAL_VOLUME_DEVICE_MTM				"deviceMTM"
#define PROPERTY_LOGICAL_VOLUME_CKD_VOLUME_SERIAL_NUM			"ckdVolumeSerialNum"
#define PROPERTY_LOGICAL_VOLUME_ACCESS_STATE				"accessState"
#define PROPERTY_LOGICAL_VOLUME_DATA_STATE				"dataState"
#define PROPERTY_LOGICAL_VOLUME_CONFIGURATION_STATE			"configurationState"
#define PROPERTY_LOGICAL_VOLUME_CREATION_DATE				"creationDate"
#define PROPERTY_LOGICAL_VOLUME_CODE_LEVEL				"codeLevel"


void
make_dummy_LogicalVolume(void)  
{
      /* Fill the object */
      
      memset( (char *)&next_lv, 0, sizeof(SmRasMrLogicalVolume));
      
      strcpy((char *)next_lv.storFacilImageMTMS, "storFacilImageM");
      next_lv.volumeType = 5;
      
      next_lv.capacity = 89+7;
      next_lv.deviceMTM = 98 * 7;
      next_lv.dataState = 5665%1000;
      next_lv.configurationState = 4 + 16000;
      next_lv.overridDefPriority = 999;
      strcpy((char *)next_lv.codeLevel, "aaabbbccc");
      next_lv.segPool = 999%1000;
      next_lv.lssGroup = 16000 - 7;
      next_lv.aliasVolumeGroup = 45 * 3;
      next_lv.dataType = 56%10;
      strcpy((char *)next_lv.ckdVolumeSerialNum, "abcdef");
      strcpy((char *)next_lv.userName, "Avi Weit");

}

CMPIStatus Reef_ProviderCleanup( CMPIInstanceMI * mi, 
           CMPIContext * ctx) { 

  CMReturn(CMPI_RC_OK);
}


CMPIStatus Reef_ProviderEnumInstanceNames( CMPIInstanceMI * mi, 
					   CMPIContext * ctx, 
					   CMPIResult * rslt, 
					   CMPIObjectPath * ref) { 
  
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 

  return rc;
}

CMPIStatus Reef_ProviderEnumInstances( CMPIInstanceMI * mi, 
						    CMPIContext * ctx, 
						    CMPIResult * rslt, 
						    CMPIObjectPath * ref, 
						    char ** properties) { 
  CMPIInstance*	ci     = NULL;
  CMPIStatus	rc    = {CMPI_RC_OK, NULL};
  int		i= 0;
  CMPISint32 key=0;
  
  /* temporary */
  make_dummy_LogicalVolume();

  do
    {
      ci = _makeInst_Reef_LogicalVolume( _broker, ctx, ref, &rc );
      
      
      if( ci == NULL || rc.rc != CMPI_RC_OK ) { 
	
	//printf("Reef_ProviderEnumInstances: Error occured");
	CMSetStatusWithChars( _broker, &rc,
			      CMPI_RC_ERR_FAILED, "Transformation from internal structure to CIM Instance failed." ); 
	goto exit;
      }
      
        key++;
        CMSetProperty(ci,"key",&key,CMPI_sint32);
     
      rc = CMReturnInstance( rslt, ci );
      if( rc.rc != CMPI_RC_OK )
	printf("Error in CMReturnInstance!!\n");
      
      //printf("Returned instance num: %d\n");
      i++;	
      if ( i == REEF_NUM_INSTANCES )
	break;

    }while(1);
  CMReturnDone( rslt );
  
 exit:
  return rc;
}

CMPIStatus Reef_ProviderGetInstance( CMPIInstanceMI * mi, 
				     CMPIContext * ctx, 
				     CMPIResult * rslt, 
				     CMPIObjectPath * cop, 
				     char ** properties) {  
  
  CMPIStatus           rc    = {CMPI_RC_OK, NULL};
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  return rc;
}



CMPIStatus Reef_ProviderCreateInstance( CMPIInstanceMI * mi, 
					CMPIContext * ctx, 
					CMPIResult * rslt, 
					CMPIObjectPath * cop, 
					CMPIInstance * ci) {
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  
  
  return rc;
}

CMPIStatus Reef_ProviderSetInstance( CMPIInstanceMI * mi, 
				     CMPIContext * ctx, 
				     CMPIResult * rslt, 
				     CMPIObjectPath * cop,
				     CMPIInstance * ci, 
				     char **properties) {

  CMPIStatus           rc    = {CMPI_RC_OK, NULL};
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  
  return rc;
}


CMPIStatus Reef_ProviderDeleteInstance( CMPIInstanceMI * mi, 
					CMPIContext * ctx, 
					CMPIResult * rslt, 
					CMPIObjectPath * cop) {

  CMPIStatus rc = {CMPI_RC_OK, NULL}; 
  
  
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  
  return rc;
}


CMPIStatus Reef_ProviderExecQuery( CMPIInstanceMI * mi, 
				   CMPIContext * ctx, 
				   CMPIResult * rslt, 
				   CMPIObjectPath * ref, 
				   char * lang, 
				   char * query) {
  
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  
  CMSetStatusWithChars( _broker, &rc, 
			CMPI_RC_ERR_NOT_SUPPORTED, "CIM_ERR_NOT_SUPPORTED" ); 
  
  return rc;
}

CMPIInstance *_makeInst_Reef_LogicalVolume( CMPIBroker *_broker, 
					    CMPIContext *ctx,
					    CMPIObjectPath *ref,
					    CMPIStatus *rc)
{

  CMPIObjectPath *op	= NULL;
  CMPIInstance   *ci	= NULL;
  char s[REEF_MAX_STRING_SIZE];
  INT32  i32;

  op = CMNewObjectPath(_broker, CMGetCharPtr( CMGetNameSpace(ref, rc) ), _ClassName, rc);
  
  if( CMIsNullObject(op) ) { 
    CMSetStatusWithChars( _broker, rc, 
			  CMPI_RC_ERR_FAILED, "Create CMPIObjectPath failed." ); 
    goto exit;
  }

  ci = CMNewInstance( _broker, op, rc);
  if( CMIsNullObject(ci) ) { 
    CMSetStatusWithChars( _broker, rc, 
			  CMPI_RC_ERR_FAILED, "Create CMPIInstance failed." ); 
    goto exit; 
  }

  CMRelease(op);

  get_storFacilImageMTMS(s);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_STORAGE_FACILITY_IMAGE_MTMS, s, CMPI_chars);
  
  get_volumeType(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_VOLUME_TYPE, (CMPIValue *)&(i32), CMPI_uint32);

  get_logicalVolumeNum(s);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_LOGICAL_VOLUME_NUM, s, CMPI_chars);

  get_logicalSubsystem(s);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_LOGICAL_SUBSYSTEM, s, CMPI_chars);

  get_lssGroup(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_LSS_GROUP, (CMPIValue *)&(i32), CMPI_uint32);

  get_addressGroup(s);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_ADDRESS_GROUP, s, CMPI_chars);

  get_aliasVolumeGroup(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_ALIAS_VOLUME_GROUPE, (CMPIValue *)&(i32), CMPI_uint32);

  get_origBaseLogVolNum(s);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_ORIG_BASE_LOG_VOL_NUM, s, CMPI_chars);

  get_userName(s);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_USER_NAME, s, CMPI_chars);

  get_segPool(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_SEG_POOL, (CMPIValue *)&(i32), CMPI_uint32);
  
  get_dataType(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_DATA_TYPE, (CMPIValue *)&(i32), CMPI_uint32);

  get_dynamicSegAlloc(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_DYNAMIC_SEG_ALLOC, (CMPIValue *)&(i32), CMPI_uint32);

  get_dataSharingAllowed(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_DATA_SHARING_ALLOWED, (CMPIValue *)&(i32), CMPI_uint32);

  get_overridDefPriority(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_OVERRIDE_DEF_PRIORITY, (CMPIValue *)&(i32), CMPI_uint32);

  get_defPriorityOverVal(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_DEF_PRIORITY_OVER_VAL, (CMPIValue *)&(i32), CMPI_uint32);

  get_ckdPriorityOffset(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_CKD_PRIORITY_OFFSET, (CMPIValue *)&(i32), CMPI_uint32);

  get_requestedCapacity(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_REQUESTED_CAPACITY, (CMPIValue *)&(i32), CMPI_uint32);

  get_capacity(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_CAPACITY, (CMPIValue *)&(i32), CMPI_uint32);

  get_deviceMTM(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_DEVICE_MTM, (CMPIValue *)&(i32), CMPI_uint32);

  get_ckdVolumeSerialNum(s);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_CKD_VOLUME_SERIAL_NUM, s, CMPI_chars);

  get_accessState(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_ACCESS_STATE, (CMPIValue *)&(i32), CMPI_uint32);

  get_dataState(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_DATA_STATE, (CMPIValue *)&(i32), CMPI_uint32);
  
  get_configurationState(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_CONFIGURATION_STATE, (CMPIValue *)&(i32), CMPI_uint32);

  get_creationDate(&i32);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_CREATION_DATE, (CMPIValue *)&(i32), CMPI_uint32);

  get_codeLevel(s);
  CMSetProperty(ci, PROPERTY_LOGICAL_VOLUME_CODE_LEVEL, s, CMPI_chars);
  
 exit:
  
  return ci;
}

/* 
 * The following functions are for retrieving information 
 * from LogicalVolume class 
 */

void get_storFacilImageMTMS(char *s)
{
  strcpy(s, (char *)next_lv.storFacilImageMTMS );
}

void get_volumeType(INT32 *i32)
{
  *i32 = next_lv.volumeType;
}

void get_logicalVolumeNum(char *s)
{
  strcpy(s,  (char *)next_lv.logicalVolumeNum );
}

void get_logicalSubsystem(char *s)
{
  strcpy(s, (char *)next_lv.logicalSubsystem );
}

void get_lssGroup(INT32 *i32)
{
  *i32 = next_lv.lssGroup;
}

void get_addressGroup(char *s)
{
  strcpy(s, (char *)next_lv.addressGroup);
}

void get_aliasVolumeGroup(INT32 *i32)
{
  *i32 = next_lv.aliasVolumeGroup;
}

void get_origBaseLogVolNum(char *s)
{
  strcpy(s, (char *)next_lv.origBaseLogVolNum);
}

void get_userName(char *s)
{
  strcpy(s, (char *)next_lv.userName);
}

void get_segPool(INT32 *i32)
{
  *i32 = next_lv.segPool;
}

void get_dataType(INT32 *i32)
{
  *i32 = next_lv.dataType;
}

void get_dynamicSegAlloc(INT32 *i32)
{
  *i32 = next_lv.dynamicSegAlloc;
}

void get_dataSharingAllowed(INT32 *i32)
{
  *i32 = next_lv.dataSharingAllowed;
}

void get_overridDefPriority(INT32 *i32)
{
  *i32 = next_lv.overridDefPriority;
}

void get_defPriorityOverVal(INT32 *i32)
{
  *i32 = next_lv.defPriorityOverVal;
}

void get_ckdPriorityOffset(INT32 *i32)
{
  *i32 = next_lv.ckdPriorityOffset;
}

void get_requestedCapacity(INT32 *i32)
{
  *i32 = next_lv.requestedCapacity;
}

void get_capacity(INT32 *i32)
{
  *i32 = next_lv.capacity;
}

void get_deviceMTM(INT32 *i32)
{
  *i32 = next_lv.deviceMTM;
}

void get_ckdVolumeSerialNum(char *s)
{
  strcpy(s, (char *)next_lv.ckdVolumeSerialNum);
}

void get_accessState(INT32 *i32)
{
  *i32 = next_lv.accessState;
}

void get_dataState(INT32 *i32)
{
  *i32 = next_lv.dataState;
}

void get_configurationState(INT32 *i32)
{
  *i32 = next_lv.configurationState;
}

void get_creationDate(INT32 *i32)
{
  *i32 = next_lv.creationDate;
}

void get_codeLevel(char *s)
{
  strcpy(s, (char *)next_lv.codeLevel);
}



/* Our provider is currently Instance Provider only */

CMInstanceMIStub( Reef_Provider, 
                  Reef_Provider, 
                  _broker, 
                  CMNoHook);
