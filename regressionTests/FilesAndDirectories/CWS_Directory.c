
/*
 * CWS_Directory.c
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
 * Author:       Viktor Mihajlovski <mihajlov@de.ibm.com>
 *
 * Description:
 *
*/


#include "CWS_FileUtils.h"
#include "cwsutil.h"
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"
#include <string.h>

#define LOCALCLASSNAME "CWS_Directory"

static CMPIBroker * _broker;

/* ------------------------------------------------------------------ *
 * Instance MI Cleanup 
 * ------------------------------------------------------------------ */

CMPIStatus CWS_DirectoryCleanup( CMPIInstanceMI * mi, 
				 CMPIContext * ctx) 
{
  CMReturn(CMPI_RC_OK);
}

/* ------------------------------------------------------------------ *
 * Instance MI Functions
 * ------------------------------------------------------------------ */


CMPIStatus CWS_DirectoryEnumInstanceNames( CMPIInstanceMI * mi, 
					   CMPIContext * ctx, 
					   CMPIResult * rslt, 
					   CMPIObjectPath * ref) 
{
   CMPIObjectPath *op;
  CMPIStatus      st = {CMPI_RC_OK,NULL};
  void           *enumhdl;
  CWS_FILE        filebuf;

  if (!silentMode()) fprintf(stderr,"--- CWS_DirectoryEnumInstanceNames() \n");
  
  enumhdl = CWS_Begin_Enum(CWS_FILEROOT,CWS_TYPE_DIR);
  
  if (enumhdl == NULL) {
    CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
			 "Could not begin file enumeration");
    return st;
  } else {
    while (CWS_Next_Enum(enumhdl,&filebuf)) {
      /* build object path from file buffer */
      op = makePath(_broker, 
		    LOCALCLASSNAME,
		    CMGetCharPtr(CMGetNameSpace(ref,NULL)), 
		    &filebuf);
      if (CMIsNullObject(op)) {
	CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
			     "Could not construct object path");
	break;
      }
      CMReturnObjectPath(rslt,op);
    }
    CMReturnDone(rslt);
    CWS_End_Enum(enumhdl);
  }
 
  return st;
}

CMPIStatus CWS_DirectoryEnumInstances( CMPIInstanceMI * mi, 
				       CMPIContext * ctx, 
				       CMPIResult * rslt, 
				       CMPIObjectPath * ref, 
				       char ** properties) 
{
  CMPIInstance   *in; 
  CMPIStatus      st = {CMPI_RC_OK,NULL};
  void           *enumhdl;
  CWS_FILE        filebuf;
  
  if (!silentMode()) fprintf(stderr,"--- CWS_DirectoryEnumInstances() \n");
  
  enumhdl = CWS_Begin_Enum(CWS_FILEROOT,CWS_TYPE_DIR);
  
  if (enumhdl == NULL) {
    CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
			 "Could not begin file enumeration");
    return st;
  } else {
    while (CWS_Next_Enum(enumhdl,&filebuf)) {
      /* build instance from file buffer */
      in = makeInstance(_broker, 
			LOCALCLASSNAME,
			CMGetCharPtr(CMGetNameSpace(ref,NULL)), 
			&filebuf);
      if (CMIsNullObject(in)) {
	CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
			     "Could not construct instance");
	break;
      }
      CMReturnInstance(rslt,in);
    }
    CMReturnDone(rslt);
    CWS_End_Enum(enumhdl);
  }
 
  return st;
}


CMPIStatus CWS_DirectoryGetInstance( CMPIInstanceMI * mi, 
				     CMPIContext * ctx, 
				     CMPIResult * rslt, 
				     CMPIObjectPath * cop, 
				     char ** properties) 
{
  CMPIInstance *in = NULL;
  CMPIStatus    st = {CMPI_RC_OK,NULL};
  CMPIData      nd = CMGetKey(cop,"Name",&st);
  CWS_FILE      filebuf;

  if (!silentMode()) fprintf(stderr,"--- CWS_DirectoryGetInstance() \n");
  
  if (st.rc == CMPI_RC_OK &&
      nd.type == CMPI_string &&
      CWS_Get_File(CMGetCharPtr(nd.value.string),&filebuf))
    in = makeInstance(_broker, 
		      LOCALCLASSNAME,
		      CMGetCharPtr(CMGetNameSpace(cop,NULL)), 
		      &filebuf);
  
  if (CMIsNullObject(in)) {
    CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
			 "Could not find or construct instance");
  } else {
    CMReturnInstance(rslt,in);    
    CMReturnDone(rslt);
  }
  
  return st;
}

CMPIStatus CWS_DirectoryCreateInstance( CMPIInstanceMI * mi, 
					CMPIContext * ctx, 
					CMPIResult * rslt, 
					CMPIObjectPath * cop, 
					CMPIInstance * ci) 
{
  CMPIStatus st = {CMPI_RC_OK,NULL};
  CWS_FILE   filebuf;
  CMPIData   data;

  if (!silentMode()) fprintf(stderr,"--- CWS_DirectoryCreateInstance() \n");

  data = CMGetProperty(ci,"Name",NULL);
  if (!CMGetCharPtr(data.value.string) ||
      strncmp(CMGetCharPtr(data.value.string),CWS_FILEROOT, 
	      strlen(CWS_FILEROOT))) {
    CMSetStatusWithChars(_broker,&st,CMPI_RC_ERR_FAILED,
			 "Invalid path name");
  } else if (!makeFileBuf(ci,&filebuf) || !CWS_Create_Directory(&filebuf)) {
    CMSetStatusWithChars(_broker,&st,CMPI_RC_ERR_FAILED,
			 "Could not create instance");
  }
  
  return st;
}

CMPIStatus CWS_DirectorySetInstance( CMPIInstanceMI * mi, 
				     CMPIContext * ctx, 
				     CMPIResult * rslt, 
				     CMPIObjectPath * cop,
				     CMPIInstance * ci, 
				     char **properties) 
{
  CMPIStatus st = {CMPI_RC_OK,NULL};
  CWS_FILE   filebuf;
  
  if (!silentMode()) fprintf(stderr,"--- CWS_DirectorySetInstance() \n");
  
  if (!makeFileBuf(ci,&filebuf) || !CWS_Update_File(&filebuf))
    CMSetStatusWithChars(_broker,&st,CMPI_RC_ERR_FAILED,
			 "Could not update instance");
  
  return st;
}

CMPIStatus CWS_DirectoryDeleteInstance( CMPIInstanceMI * mi, 
					CMPIContext * ctx, 
					CMPIResult * rslt, 
					CMPIObjectPath * cop) 
{ 
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus CWS_DirectoryExecQuery( CMPIInstanceMI * mi, 
				   CMPIContext * ctx, 
				   CMPIResult * rslt, 
				   CMPIObjectPath * cop, 
				   char * lang, 
				   char * query) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

/* ------------------------------------------------------------------ *
 * Instance MI Factory
 * 
 * NOTE: This is an example using the convenience macros. This is OK 
 *       as long as the MI has no special requirements, i.e. to store
 *       data between calls.
 * ------------------------------------------------------------------ */

CMInstanceMIStub( CWS_Directory,
		  CWS_DirectoryProvider,
		  _broker,
		  CMNoHook);
