
/*
 * CWS_PlainFile.c
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

#define LOCALCLASSNAME "CWS_PlainFile"

static CMPIBroker * _broker;

/* ------------------------------------------------------------------ *
 * Instance MI Cleanup 
 * ------------------------------------------------------------------ */

CMPIStatus CWS_PlainFileCleanup( CMPIInstanceMI * mi, 
				 CMPIContext * ctx) 
{
  CMReturn(CMPI_RC_OK);
}

/* ------------------------------------------------------------------ *
 * Instance MI Functions
 * ------------------------------------------------------------------ */


CMPIStatus CWS_PlainFileEnumInstanceNames( CMPIInstanceMI * mi, 
					   CMPIContext * ctx, 
					   CMPIResult * rslt, 
					   CMPIObjectPath * ref) 
{
  CMPIObjectPath *op;
  CMPIStatus      st = {CMPI_RC_OK,NULL};
  void           *enumhdl;
  CWS_FILE        filebuf;

  if (!silentMode()) fprintf(stderr,"--- CWS_PlainFileEnumInstanceNames() \n");
  
  enumhdl = CWS_Begin_Enum(CWS_FILEROOT,CWS_TYPE_PLAIN);
  
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

CMPIStatus CWS_PlainFileEnumInstances( CMPIInstanceMI * mi, 
				       CMPIContext * ctx, 
				       CMPIResult * rslt, 
				       CMPIObjectPath * ref, 
				       char ** properties) 
{
  CMPIInstance   *in; 
  CMPIStatus      st = {CMPI_RC_OK,NULL};
  void           *enumhdl;
  CWS_FILE        filebuf;
  
  if (!silentMode()) fprintf(stderr,"--- CWS_PlainFileEnumInstances() \n");
  
  enumhdl = CWS_Begin_Enum(CWS_FILEROOT,CWS_TYPE_PLAIN);
  
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


CMPIStatus CWS_PlainFileGetInstance( CMPIInstanceMI * mi, 
				     CMPIContext * ctx, 
				     CMPIResult * rslt, 
				     CMPIObjectPath * cop, 
				     char ** properties) 
{
  CMPIInstance *in = NULL;
  CMPIStatus    st = {CMPI_RC_OK,NULL};
  CMPIData      nd = CMGetKey(cop,"Name",&st);
  CWS_FILE      filebuf;

  if (!silentMode()) fprintf(stderr,"--- CWS_PlainFileGetInstance() \n");
  
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

CMPIStatus CWS_PlainFileCreateInstance( CMPIInstanceMI * mi, 
					CMPIContext * ctx, 
					CMPIResult * rslt, 
					CMPIObjectPath * cop, 
					CMPIInstance * ci) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus CWS_PlainFileSetInstance( CMPIInstanceMI * mi, 
				     CMPIContext * ctx, 
				     CMPIResult * rslt, 
				     CMPIObjectPath * cop,
				     CMPIInstance * ci, 
				     char **properties) 
{
  CMPIStatus st = {CMPI_RC_OK,NULL};
  CWS_FILE   filebuf;
  
  if (!silentMode()) fprintf(stderr,"--- CWS_PlainFileSetInstance() \n");
  
  if (!makeFileBuf(ci,&filebuf) || !CWS_Update_File(&filebuf))
    CMSetStatusWithChars(_broker,&st,CMPI_RC_ERR_FAILED,
			 "Could not update instance");
  
  return st;
}

CMPIStatus CWS_PlainFileDeleteInstance( CMPIInstanceMI * mi, 
					CMPIContext * ctx, 
					CMPIResult * rslt, 
					CMPIObjectPath * cop) 
{ 
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus CWS_PlainFileExecQuery( CMPIInstanceMI * mi, 
				   CMPIContext * ctx, 
				   CMPIResult * rslt, 
				   CMPIObjectPath * cop, 
				   char * lang, 
				   char * query) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

/* ------------------------------------------------------------------ *
 * Method MI Cleanup 
 * ------------------------------------------------------------------ */

CMPIStatus CWS_PlainFileMethodCleanup( CMPIMethodMI * mi, 
				       CMPIContext * ctx) 
{
  CMReturn(CMPI_RC_OK);
}

/* ------------------------------------------------------------------ *
 * Method MI Functions
 * ------------------------------------------------------------------ */

CMPIStatus CWS_PlainFileInvokeMethod( CMPIMethodMI * mi, 
				      CMPIContext * ctx, 
				      CMPIResult * rslt,
				      CMPIObjectPath * cop,
				      char * method,
				      CMPIArgs * in,
				      CMPIArgs * out)
{
  CMPIStatus st = {CMPI_RC_OK,NULL};
  CMPIData   dt;
  char typebuf[1000];

  if (!silentMode()) fprintf(stderr,"--- CWS_PlainFileInvokeMethod()\n");

  dt=CMGetKey(cop,"Name",&st);
  if (st.rc != CMPI_RC_OK) {
    CMSetStatusWithChars(_broker,&st,CMPI_RC_ERR_FAILED,
			 "Could not get instance name");
  } else if (CWS_Get_FileType(CMGetCharPtr(dt.value.string),typebuf,
			      sizeof(typebuf))) {
    CMSetStatusWithChars(_broker,&st,CMPI_RC_ERR_FAILED,
			 "Could not get type");
  } else {
    CMReturnData(rslt,typebuf,CMPI_chars);
    CMReturnDone(rslt);
  }
  
  return st;
}

/* ------------------------------------------------------------------ *
 * Instance MI Factory
 * 
 * NOTE: This is an example using the convenience macros. This is OK 
 *       as long as the MI has no special requirements, i.e. to store
 *       data between calls.
 * ------------------------------------------------------------------ */

CMInstanceMIStub( CWS_PlainFile,
		  CWS_PlainFileProvider,
		  _broker,
		  CMNoHook);

/* ------------------------------------------------------------------ *
 * Method MI Factory
 * 
 * ------------------------------------------------------------------ */

CMMethodMIStub( CWS_PlainFile,
		CWS_PlainFileProvider,
		_broker,
		CMNoHook);
