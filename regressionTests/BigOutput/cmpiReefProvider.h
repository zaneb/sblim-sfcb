
/*
 * cmpiReefProvider.h
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

#ifndef cmpi_ReefPROVIDER_H
#define cmpi_ReefPROVIDER_H

/* Helpers prototypes - start */
CMPIInstance *_makeInst_Reef_LogicalVolume( CMPIBroker *_broker, 
					    CMPIContext *ctx,
					    CMPIObjectPath *ref,
					    CMPIStatus *rc);

/* Helpers prototypes  - end */

/* Gettes - start */
void get_storFacilImageMTMS(char *s);
void get_volumeType(int *i32);
void get_logicalVolumeNum(char *s);
void get_logicalSubsystem(char *s);
void get_lssGroup(int *i32);
void get_addressGroup(char *s);
void get_aliasVolumeGroup(int *i32);
void get_origBaseLogVolNum(char *s);
void get_userName(char *s);
void get_segPool(int *i32);
void get_dataType(int *i32);
void get_dynamicSegAlloc(int *i32);
void get_dynamicSegAlloc(int *i32);
void get_dataSharingAllowed(int *i32);
void get_overridDefPriority(int *i32);
void get_defPriorityOverVal(int *i32);
void get_ckdPriorityOffset(int *i32);
void get_requestedCapacity(int *i32);
void get_capacity(int *i32);
void get_deviceMTM(int *i32);
void get_ckdVolumeSerialNum(char *s);
void get_accessState(int *i32);
void get_dataState(int *i32);
void get_configurationState(int *i32);
void get_creationDate(int *i32);
void get_codeLevel(char *s);

/* Getters - end */

#endif  /* #ifndef Reef_PROVIDER_H */
