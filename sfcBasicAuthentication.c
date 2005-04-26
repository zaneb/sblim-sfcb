
/*
 * sfcBasicAuthentication.c
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
 * Author:        Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Basic Authentication exit.
 *
*/


#include <stdio.h>
#include <string.h>

extern int _sfcBasicAuthenticate(char *user, char *pw)
{
   printf("-#- Authentication request for %s\n",user);
   if (strcmp(user,"REJECT")==0) return 0;
   return 1;
}
