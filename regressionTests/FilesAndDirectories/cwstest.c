
/*
 * cwstest.c
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

int main(int argc, char * argv[])
{
  void     *enumhdl;
  CWS_FILE  filebuf;

  if (argc != 2) {
    fprintf(stderr,"usage: %s directory\n",argv[0]);
    exit(-1);
  }

  printf("=== Searching for plain files in %s \n",argv[1]);
  enumhdl = CWS_Begin_Enum(argv[1],CWS_TYPE_PLAIN);
  if (enumhdl) {
    while (CWS_Next_Enum(enumhdl,&filebuf))
      printf("--- File: %s Size: %lld Mode: %u\n",
	     filebuf.cws_name, filebuf.cws_size, filebuf.cws_mode);
    CWS_End_Enum(enumhdl);
  }

  printf("=== Searching for directories in %s \n",argv[1]);
  enumhdl = CWS_Begin_Enum(argv[1],CWS_TYPE_DIR);
  if (enumhdl) {
    while (CWS_Next_Enum(enumhdl,&filebuf))
      printf("--- Dir: %s Size: %lld Mode: %u\n",
	     filebuf.cws_name, filebuf.cws_size, filebuf.cws_mode);
    CWS_End_Enum(enumhdl);
  }

  printf("=== Direct Access to Directory %s \n",argv[1]);
  if (CWS_Get_File(argv[1],&filebuf))
    printf("--- Dir: %s Size: %lld Mode: %u\n",
	   filebuf.cws_name, filebuf.cws_size, filebuf.cws_mode);
  
  return 0;
}
