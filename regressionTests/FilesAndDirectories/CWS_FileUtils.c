
/*
 * CWS_Utils.c
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
#include <unistd.h>

#include <string.h>


static char cscn[] = "CIM_UnitaryComputerSystem";
static char fscn[] = "CIM_FileSystem";
static char csn[500] = "";
static char fsn[] = CWS_FILEROOT;

int silentMode()
{
   return SILENT;
}

char * CSCreationClassName()
{
  return cscn;
}

char * CSName()
{
  if (*csn == 0)
#if defined SIMULATED
    strcpy(csn,"localhost");
#else
    gethostname(csn,sizeof(csn));
#endif
  return csn;
}

char * FSCreationClassName()
{
  return fscn;
}

char * FSName()
{
  return fsn;
}


CMPIObjectPath *makePath(CMPIBroker *broker, const char * classname,
			 const char * Namespace, CWS_FILE *cwsf)
{
  CMPIObjectPath *op;
  op = CMNewObjectPath(broker,
		       (char*)Namespace,
		       (char*)classname,
		       NULL);  CMSetHostname(op,CSName());
  if (!CMIsNullObject(op)) {
    CMAddKey(op,"CSCreationClassName",CSCreationClassName(),CMPI_chars);
    CMAddKey(op,"CSName",CSName(),CMPI_chars);
    CMAddKey(op,"FSCreationClassName",FSCreationClassName(),CMPI_chars);
    CMAddKey(op,"FSName",FSName(),CMPI_chars);
    CMAddKey(op,"CreationClassName",classname,CMPI_chars);
    CMAddKey(op,"Name",cwsf->cws_name,CMPI_chars);
  }
  return op;
}

CMPIInstance   *makeInstance(CMPIBroker *broker, const char * classname,
			     const char * Namespace, CWS_FILE *cwsf)
{
  CMPIInstance   *in = NULL;
  CMPIValue       val;
  CMPIObjectPath *op = CMNewObjectPath(broker,
				       (char*)Namespace,
				       (char*)classname,
				       NULL);  CMSetHostname(op,CSName());
  
  if (!CMIsNullObject(op)) {
    in = CMNewInstance(broker,op,NULL);
    if (!CMIsNullObject(in)) {
      CMSetProperty(in,"CSCreationClassName",CSCreationClassName(),CMPI_chars);
      CMSetProperty(in,"CSName",CSName(),CMPI_chars);
      CMSetProperty(in,"FSCreationClassName",FSCreationClassName(),CMPI_chars);
      CMSetProperty(in,"FSName",FSName(),CMPI_chars);
      CMSetProperty(in,"CreationClassName",classname,CMPI_chars);
      CMSetProperty(in,"Name",cwsf->cws_name,CMPI_chars); 
      CMSetProperty(in,"FileSize",(CMPIValue*)&cwsf->cws_size,CMPI_uint64);
      /* val.uint64 = cwsf->cws_ctime;
      val.dateTime = CMNewDateTimeFromBinary(broker,val.uint64*1000000,0,NULL);
      CMSetProperty(in,"CreationDate",&val,CMPI_dateTime);
      val.uint64 = cwsf->cws_mtime;
      val.dateTime = CMNewDateTimeFromBinary(broker,val.uint64*1000000,0,NULL);
      CMSetProperty(in,"LastModified",&val,CMPI_dateTime);
      val.uint64 = cwsf->cws_atime;
      val.dateTime = CMNewDateTimeFromBinary(broker,val.uint64*1000000,0,NULL);
      CMSetProperty(in,"LastAccessed",&val,CMPI_dateTime); */
      val.uint64=0L;
      val.boolean=(cwsf->cws_mode & 0400) != 0;
      CMSetProperty(in,"Readable",&val,CMPI_boolean);
      val.boolean=(cwsf->cws_mode & 0200) != 0;
      CMSetProperty(in,"Writeable",&val,CMPI_boolean);
      val.boolean=(cwsf->cws_mode & 0100) != 0;
      CMSetProperty(in,"Executable",&val,CMPI_boolean);
    }
  }
  return in;
}

int makeFileBuf(CMPIInstance *instance, CWS_FILE *cwsf)
{
  CMPIData dt; 
  if (instance && cwsf) {
    dt=CMGetProperty(instance,"Name",NULL);
    strcpy(cwsf->cws_name,CMGetCharPtr(dt.value.string));
    dt=CMGetProperty(instance,"FileSize",NULL);
    cwsf->cws_size=dt.value.uint64;
    /* dt=CMGetProperty(instance,"CreationDate",NULL); */
    /* cwsf->cws_ctime=CMGetBinaryFormat(dt.value.dateTime,NULL); */
    /* dt=CMGetProperty(instance,"LastModified",NULL); */
    /* cwsf->cws_mtime=CMGetBinaryFormat(dt.value.dateTime,NULL); */
    /* dt=CMGetProperty(instance,"LastAccessed",NULL); */
    /* cwsf->cws_atime=CMGetBinaryFormat(dt.value.dateTime,NULL); */
    dt=CMGetProperty(instance,"Readable",NULL);
    cwsf->cws_mode=dt.value.boolean ? 0400 : 0;
    dt=CMGetProperty(instance,"Writeable",NULL);
    cwsf->cws_mode+=(dt.value.boolean ? 0200 : 0);
    dt=CMGetProperty(instance,"Executable",NULL);
    cwsf->cws_mode+=(dt.value.boolean ? 0100 : 0);
    return 1;
  }
  return 0;
}
