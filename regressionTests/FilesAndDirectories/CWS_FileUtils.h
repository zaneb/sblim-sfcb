
/*
 * CWS_FileUtils.c
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

#ifndef CWS_FILEUTILS_H
#define CWS_FILEUTILS_H

#include "cwsutil.h"
#include "cmpidt.h"

#if defined SIMULATED
 #define CWS_FILEROOT  "/Simulated/CMPI/tests/"
 #define SILENT 1
#else
 #define CWS_FILEROOT  "/home/mihajlov/pkg"
 #define SILENT 0
#endif

char * CSCreationClassName();
char * CSName();
char * FSCreationClassName();
char * FSName();


CMPIObjectPath *makePath(CMPIBroker *broker, const char *classname,
			 const char *Namespace, CWS_FILE *cwsf);
CMPIInstance   *makeInstance(CMPIBroker *broker, const char *classname,
			     const char *Namespace, CWS_FILE *cwsf);
int             makeFileBuf(CMPIInstance *instance, CWS_FILE *cwsf);

int silentMode();

#endif
