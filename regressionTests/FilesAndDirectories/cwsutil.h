
/*
 * cwsutil.h
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


#ifndef CWSUTIL_H
#define CWSUTIL_H

#include <time.h>

#ifdef __cplusplus 
/*extern "C" {*/
#endif
  
/* ------------------------------------------------------------------
 * Utilities for file info retrieval
 * ----------------------------------------------------------------- */

#define CWS_MAXPATH    1025

#define CWS_TYPE_DIR   'd'
#define CWS_TYPE_PLAIN 'f'

#define SINT64 long long

struct _CWS_FILE {
  char      cws_name[CWS_MAXPATH];
  SINT64 cws_size;
  time_t    cws_ctime;
  time_t    cws_mtime;
  time_t    cws_atime;
  unsigned  cws_mode;
};
typedef struct _CWS_FILE CWS_FILE;

/* ------------------------------------------------------------------
 * File Enumeration Support, use like this:
 *
 *  CWS_FILE filebuf;
 *  void * hdl = CWS_Begin_Enum("/test",CWS_TYPE_FILE);
 *  if (hdl) {
 *    while(CWS_Next_Enum(hdl,&filebuf) {...}
 *    CWS_End_Enum(hdl);
 *  }
 * ----------------------------------------------------------------- */



void* CWS_Begin_Enum(const char *topdir, int filetype);
int CWS_Next_Enum(void *handle, CWS_FILE* cwsf);
void CWS_End_Enum(void *handle);

int CWS_Get_File(const char *file, CWS_FILE* cwsf);
int CWS_Update_File(CWS_FILE* cwsf);
int CWS_Create_Directory(CWS_FILE* cwsf);
int CWS_Get_FileType(const char *file, char* typestring, size_t tslen);

#ifdef __cplusplus 
/*}*/
#endif

#endif
