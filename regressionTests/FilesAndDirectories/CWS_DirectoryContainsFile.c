
/*
 * CWS_DirectoryContainsFile.c
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
#include <libgen.h>

#include "trace.h"

#define LOCALCLASSNAME "CWS_DirectoryContainsFile"
#define DIRECTORYCLASS "CWS_Directory"
#define FILECLASS      "CWS_PlainFile"

static CMPIBroker * _broker;

/* ------------------------------------------------------------------ *
 * Instance MI Cleanup
 * ------------------------------------------------------------------ */

CMPIStatus CWS_DirectoryContainsFileCleanup( CMPIInstanceMI * mi,
				 CMPIContext * ctx)
{
  CMReturn(CMPI_RC_OK);
}

/* ------------------------------------------------------------------ *
 * Instance MI Functions
 * ------------------------------------------------------------------ */


CMPIStatus CWS_DirectoryContainsFileEnumInstanceNames( CMPIInstanceMI * mi,
					   CMPIContext * ctx,
					   CMPIResult * rslt,
					   CMPIObjectPath * ref)
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus CWS_DirectoryContainsFileEnumInstances( CMPIInstanceMI * mi,
				       CMPIContext * ctx, 
				       CMPIResult * rslt, 
				       CMPIObjectPath * ref, 
				       char ** properties) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}


CMPIStatus CWS_DirectoryContainsFileGetInstance( CMPIInstanceMI * mi, 
				     CMPIContext * ctx,
				     CMPIResult * rslt, 
				     CMPIObjectPath * cop, 
				     char ** properties) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus CWS_DirectoryContainsFileCreateInstance( CMPIInstanceMI * mi, 
					CMPIContext * ctx, 
					CMPIResult * rslt, 
					CMPIObjectPath * cop, 
					CMPIInstance * ci) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus CWS_DirectoryContainsFileSetInstance( CMPIInstanceMI * mi, 
				     CMPIContext * ctx, 
				     CMPIResult * rslt, 
				     CMPIObjectPath * cop,
				     CMPIInstance * ci, 
				     char **properties) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus CWS_DirectoryContainsFileDeleteInstance( CMPIInstanceMI * mi, 
					CMPIContext * ctx, 
					CMPIResult * rslt, 
					CMPIObjectPath * cop) 
{ 
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus CWS_DirectoryContainsFileExecQuery( CMPIInstanceMI * mi, 
				   CMPIContext * ctx, 
				   CMPIResult * rslt, 
				   CMPIObjectPath * cop, 
				   char * lang, 
				   char * query) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

/* ------------------------------------------------------------------ *
 * Association MI Cleanup 
 * ------------------------------------------------------------------ */

CMPIStatus CWS_DirectoryContainsFileAssociationCleanup( CMPIAssociationMI * mi,
					      CMPIContext * ctx) 
{
  CMReturn(CMPI_RC_OK);
}

/* ------------------------------------------------------------------ *
 * Association MI Functions
 * ------------------------------------------------------------------ */

CMPIStatus CWS_DirectoryContainsFileAssociators( CMPIAssociationMI * mi,
				       CMPIContext * ctx,
				       CMPIResult * rslt,
				       CMPIObjectPath * cop,
				       const char * assocClass,
				       const char * resultClass,
				       const char * role,
				       const char * resultRole,
				       char ** propertyList ) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}

CMPIStatus CWS_DirectoryContainsFileAssociatorNames( CMPIAssociationMI * mi,
					   CMPIContext * ctx,
					   CMPIResult * rslt,
					   CMPIObjectPath * cop,
					   const char * assocClass,
					   const char * resultClass,
					   const char * role,
					   const char * resultRole ) 
{
  CMPIStatus      st = {CMPI_RC_OK,NULL};
  CMPIString     *clsname;
  CMPIData        data;
  CMPIObjectPath *op;
  void           *enumhdl;
  CWS_FILE        filebuf;

  if (!silentMode()) fprintf(stderr,"--- CWS_DirectoryContainsFileAssociatorNames()\n");

  /*
   * Check if the object path belongs to a supported class
   */
  clsname = CMGetClassName(cop,NULL);
  if (clsname) {
    if (strcasecmp(DIRECTORYCLASS,CMGetCharPtr(clsname))==0) {
      /* we have a directory and can return the children */
      data = CMGetKey(cop,"Name",NULL);
      
      enumhdl = CWS_Begin_Enum(CMGetCharPtr(data.value.string),CWS_TYPE_PLAIN);
      
      if (enumhdl == NULL) {
	CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
			     "Could not begin file enumeration");
	return st;
      } else {
	while (CWS_Next_Enum(enumhdl,&filebuf)) {
	  /* build object path from file buffer */
	  op = makePath(_broker,
			FILECLASS,
			CMGetCharPtr(CMGetNameSpace(cop,NULL)), 
			&filebuf);  CMSetHostname(op,CSName());
	  if (CMIsNullObject(op)) {
	    CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
				 "Could not construct object path");
	    break;
	  }
	  CMReturnObjectPath(rslt,op);
	}
	CWS_End_Enum(enumhdl);
      }

    }
    
    if (strcasecmp(FILECLASS,CMGetCharPtr(clsname))==0 ||
	strcasecmp(DIRECTORYCLASS,CMGetCharPtr(clsname))==0) {
      /* we can always return the parent */
      data = CMGetKey(cop,"Name",NULL);
      if (CWS_Get_File(dirname(CMGetCharPtr(data.value.string)),&filebuf)) {
	op = makePath(_broker,
		      DIRECTORYCLASS,
		      CMGetCharPtr(CMGetNameSpace(cop,NULL)), 
		      &filebuf);  CMSetHostname(op,CSName());
	if (CMIsNullObject(op)) {
	  CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
			       "Could not construct object path");
	  return st;  
	}
	CMReturnObjectPath(rslt,op);
      }
      
    }
    
    else {
      if (!silentMode()) fprintf(stderr,"--- CWS_DirectoryContainsFileAssociatorNames() unsupported class \n");
    }
    CMReturnDone(rslt); 
  } /* if (clsname) */    
  
  return st;
}

CMPIStatus CWS_DirectoryContainsFileReferences( CMPIAssociationMI * mi,
				      CMPIContext * ctx,
				      CMPIResult * rslt,
				      CMPIObjectPath * cop,
				      const char * assocClass,
				      const char * role,
				      char ** propertyList ) 
{
  CMReturn( CMPI_RC_ERR_NOT_SUPPORTED );
}


CMPIStatus CWS_DirectoryContainsFileReferenceNames( CMPIAssociationMI * mi,
					  CMPIContext * ctx,
					  CMPIResult * rslt,
					  CMPIObjectPath * cop,
					  const char * assocClass,
					  const char * role) 
{
  CMPIStatus      st = {CMPI_RC_OK,NULL};
  CMPIString     *clsname;
  CMPIData        data;
  CMPIObjectPath *op;
  CMPIObjectPath *opRef;
  void           *enumhdl;
  CWS_FILE        filebuf;

  if (!silentMode()) fprintf(stderr,"--- CWS_DirectoryContainsFileReferenceNames()\n");
    
  /*
   * Check if the object path belongs to a supported class
   */
  clsname = CMGetClassName(cop,NULL);
  if (clsname) {
    if (strcasecmp(DIRECTORYCLASS,CMGetCharPtr(clsname))==0) {
      /* we have a directory and can return the children */
      data = CMGetKey(cop,"Name",NULL);

      enumhdl = CWS_Begin_Enum(CMGetCharPtr(data.value.string),CWS_TYPE_PLAIN);

      if (enumhdl == NULL) {
	CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
			     "Could not begin file enumeration");
	return st;
      } else {
	while (CWS_Next_Enum(enumhdl,&filebuf)) {
	  /* build object path from file buffer */
	  op = makePath(_broker,
			FILECLASS,
			CMGetCharPtr(CMGetNameSpace(cop,NULL)),
			&filebuf);
	  if (CMIsNullObject(op)) {
	    CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
				 "Could not construct object path");
	    break;
	  }
	  /* make reference object path */
	  opRef = CMNewObjectPath(_broker,
				  CMGetCharPtr(CMGetNameSpace(cop,NULL)),
				  LOCALCLASSNAME,
				  NULL);  CMSetHostname(opRef,CSName());
	  if (CMIsNullObject(op)) {
	    CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
				 "Could not construct object path");
	    break;
	  }
	  CMAddKey(opRef,"GroupComponent",&cop,CMPI_ref);
	  CMAddKey(opRef,"PathComponent",&op,CMPI_ref);
	  CMReturnObjectPath(rslt,opRef);
	}
	CWS_End_Enum(enumhdl);
      }

    }

    if (strcasecmp(FILECLASS,CMGetCharPtr(clsname))==0 ||
	strcasecmp(DIRECTORYCLASS,CMGetCharPtr(clsname))==0) {
      /* we can always return the parent */
      data = CMGetKey(cop,"Name",NULL);
      if (CWS_Get_File(dirname(CMGetCharPtr(data.value.string)),&filebuf)) {
	op = makePath(_broker,
		      DIRECTORYCLASS,
		      CMGetCharPtr(CMGetNameSpace(cop,NULL)),
		      &filebuf);
	if (CMIsNullObject(op)) {
	  CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
			       "Could not construct object path");
	  return st;
	}
	/* make reference object path */
	opRef = CMNewObjectPath(_broker,
				CMGetCharPtr(CMGetNameSpace(cop,NULL)),
				LOCALCLASSNAME,
				NULL);  CMSetHostname(opRef,CSName());
	if (CMIsNullObject(op)) {
	  CMSetStatusWithChars(_broker, &st, CMPI_RC_ERR_FAILED,
			       "Could not construct object path");
	  return st;
	}
	CMAddKey(opRef,"GroupComponent",&op,CMPI_ref);
	CMAddKey(opRef,"PartComponent",&cop,CMPI_ref);
	CMReturnObjectPath(rslt,opRef);
      }

    }

    else {
      if (!silentMode()) fprintf(stderr,"--- CWS_DirectoryContainsFileReferenceNames() unsupported class \n");
    }
    CMReturnDone(rslt);
  } /* if (clsname) */

  return st;
}

/* ------------------------------------------------------------------ *
 * Instance MI Factory
 * 
 * NOTE: This is an example using the convenience macros. This is OK 
 *       as long as the MI has no special requirements, i.e. to store
 *       data between calls.
 * ------------------------------------------------------------------ */

CMInstanceMIStub( CWS_DirectoryContainsFile,
		  CWS_DirectoryContainsFileProvider,
		  _broker,
		  CMNoHook);

CMAssociationMIStub( CWS_DirectoryContainsFile,
		  CWS_DirectoryContainsFileProvider,
		  _broker,
		  CMNoHook);

