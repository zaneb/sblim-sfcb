
/*
 * cwsutil.c
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

#include "cwsutil.h"  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined SIMULATED

typedef struct {
  int next;
  int done;
  char type;
} CWS_Control;

#include "cwssimdata.c"

#ifdef PEGASUS_PLATFORM_WIN32_IX86_MSVC
char * dirname(char *path) {
	char drive[_MAX_DRIVE];
	char *dir = (char *)malloc(_MAX_DIR*sizeof(char));
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	_splitpath( path, drive, dir, fname, ext);
	return dir;
}
#endif

int locateFile(const char *fn) 
{
   int i;
   for (i=0; files[i].name; i++) 
      if (strcmp(files[i].name,fn)==0) return i;
   return -1;   
}

void* CWS_Begin_Enum(const char *topdir, int filetype)
{
  /* begin enumeration */
  CWS_Control *cc = (CWS_Control *)malloc(sizeof(CWS_Control));
  cc->type=filetype;
  cc->next=-1;
  cc->done=0;
  return cc;
}

int CWS_Next_Enum(void *handle, CWS_FILE* cwsf)
{
  CWS_Control *cc = (CWS_Control *)handle;
  int n;
  
  if (cc->done) return 0;
  n=++cc->next;
  while (files[n].name && files[n].ftype!=cc->type) n=++cc->next;
  if (files[n].name) {
    strcpy(cwsf->cws_name,files[n].name);
    cwsf->cws_size=files[n].size;
    cwsf->cws_ctime=files[n].cTime;
    cwsf->cws_mtime=files[n].mTime;
    cwsf->cws_atime=files[n].aTime;
    cwsf->cws_mode=files[n].mode;
    return 1;
  }
  else cc->done=0;
  return 0;
}

void CWS_End_Enum(void *handle)
{
  /* end enumeration */
  CWS_Control *cc = (CWS_Control *)handle;
  if (cc) {
    free(cc);
  }
}

int CWS_Get_File(const char *file, CWS_FILE* cwsf)
{
  int n;  
  if (file && cwsf && (n=locateFile(file))>=0) {
    strcpy(cwsf->cws_name,files[n].name);
    cwsf->cws_size=files[n].size;
    cwsf->cws_ctime=files[n].cTime;
    cwsf->cws_mtime=files[n].mTime;
    cwsf->cws_atime=files[n].aTime;
    cwsf->cws_mode=files[n].mode;
    return 1;
  }
  return 0;
}

int CWS_Update_File(CWS_FILE* cwsf)
{
  int mode;
  int n;  

  /* only change mode */
  if (cwsf && (n=locateFile(cwsf->cws_name))>=0) {
    mode=(files[n].mode & 07077) | (cwsf->cws_mode & 0700);
    files[n].mode=mode;
    return 1;
  }
  return 0;
}

int CWS_Create_Directory(CWS_FILE* cwsf)
{
  return 0;
/*  int         state=0;
  int         mode;

  if (cwsf) {
    mode=cwsf->cws_mode | 0077; // plus umask 
    state=mkdir(cwsf->cws_name,mode)==0;
  }
  return state; */
}

int CWS_Get_FileType(const char *file, char* typestring, size_t tslen)
{
  int n;
  if (file && (n=locateFile(file))>=0) {
     strncpy(typestring,files[n].type,tslen);
     return 0;
  }
  return 1;
}

#else

#include <sys/stat.h>

typedef struct {
  FILE *fp;
  char name[L_tmpnam];
} CWS_Control;

void* CWS_Begin_Enum(const char *topdir, int filetype)
{
  /* begin enumeration */
  char cmdbuffer[2000];
  CWS_Control *cc = malloc(sizeof(CWS_Control));
  if (cc && tmpnam(cc->name)) {
    sprintf(cmdbuffer,
	    "find %s -xdev -type %c -printf \"%%p %%s " 
	    "%%C@ %%A@ %%T@ %%m\n\" > %s",
	    topdir, filetype, cc->name);
    if (system(cmdbuffer)==0)
      cc->fp = fopen(cc->name,"r");
    else {
      free(cc);
      cc=NULL;
    }
  }
  return cc;
}

int CWS_Next_Enum(void *handle, CWS_FILE* cwsf)
{
  /* read next entry from result file */
  char result[2000];
  CWS_Control *cc = (CWS_Control *)handle;
  int state=0;
  if (cc && cwsf && fgets(result,sizeof(result),cc->fp))
    state=0<sscanf(result,"%s %lld %ld %ld %ld %o",
		   cwsf->cws_name,
		   &cwsf->cws_size,
		   &cwsf->cws_ctime,
		   &cwsf->cws_mtime,
		   &cwsf->cws_atime,
		   &cwsf->cws_mode);
  return state;
}

void CWS_End_Enum(void *handle)
{
  /* end enumeration */
  CWS_Control *cc = (CWS_Control *)handle;
  if (cc) {
    fclose(cc->fp);
    remove(cc->name);
    free(cc);
  }
}

int CWS_Get_File(const char *file, CWS_FILE* cwsf)
{
  int         state=0;
  struct stat statbuf;
  if (file && cwsf && stat(file,&statbuf)==0) {
    strcpy(cwsf->cws_name,file);
    cwsf->cws_size=statbuf.st_size;
    cwsf->cws_ctime=statbuf.st_ctime;
    cwsf->cws_mtime=statbuf.st_mtime;
    cwsf->cws_atime=statbuf.st_atime;
    cwsf->cws_mode=statbuf.st_mode;
    state=1;
  }
  return state;
}

int CWS_Update_File(CWS_FILE* cwsf)
{

  int         state=0;
  struct stat statbuf; 
  int         mode;

  /* only change mode */
  if (cwsf && stat(cwsf->cws_name,&statbuf)==0) {
    mode=(statbuf.st_mode & 07077) | (cwsf->cws_mode & 0700);
    state=chmod(cwsf->cws_name,mode)==0;
  }
  return state;
}

int CWS_Create_Directory(CWS_FILE* cwsf)
{

  int         state=0;
  int         mode;

  if (cwsf) {
    mode=cwsf->cws_mode | 0077; /* plus umask */
    state=mkdir(cwsf->cws_name,mode)==0;
  }
  return state;
}

int CWS_Get_FileType(const char *file, char* typestring, size_t tslen)
{
  char  cmdbuffer[300];
  char  cmdout[300];
  FILE *fcmdout;
  
  if (file && tmpnam(cmdout)) {
    sprintf(cmdbuffer,"file %s > %s",file,cmdout);
    if (system(cmdbuffer)==0 && 
	(fcmdout = fopen(cmdout,"r")) &&
	fgets(typestring,tslen,fcmdout))
      return 0;
  }
  return 1;
}

#endif
