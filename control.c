
/*
 * control.c
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
 * sfcb.cfg config parser.
 *
*/


#include "utilft.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


#ifndef SFCB_CONFDIR
#define SFCB_CONFDIR "/etc/sfcb"
#endif

#ifndef SFCB_STATEDIR
#define SFCB_STATEDIR "/var/lib/sfcb"
#endif

typedef struct control {
   char *id;
   int type;
   char *strValue;
} Control;

static UtilHashTable *ct = NULL;

Control init[] = {
   {"httpPort",         1, "5988"},
   {"enableHttp",       2, "true"},
   {"httpProcs",        1, "8"},
   {"httpsPort",        1, "5989"},
   {"enableHttps",      2, "false"},
   {"httpsProcs",       1, "8"},
   {"provProcs",        1, "32"},
   {"basicAuthLib",     0, "sfcBasicAuthentication"},
   {"doBasicAuth",      2, "false"},
   
   {"useChunking",      2, "false"},
   {"chunkSize",        1, "50000"},

   {"providerSampleInterval",  1, "30"},
   {"providerTimeoutInterval", 1, "60"},

   {"sslKeyFilePath",   0, SFCB_CONFDIR "/file.pem"},
   {"sslCertificateFilePath", 0, SFCB_CONFDIR "/server.pem"},

   {"registrationDir", 0, SFCB_STATEDIR "/registration"},
};

int setupControl(char *fn)
{
   FILE *in;
   char fin[1024], *stmt = NULL;
   int i, m, n=0, err=0;
   CntlVals rv;

   if (ct)
      return 0;

   ct = UtilFactory->newHashTable(61, UtilHashTable_charKey |
                UtilHashTable_ignoreKeyCase);

   for (i = 0, m = sizeof(init) / sizeof(Control); i < m; i++) {
      ct->ft->put(ct, init[i].id, &init[i]);
   }

   strcpy(fin, SFCB_CONFDIR);

   strcat(fin, "/sfcb.cfg");
   in = fopen(fin, "r");
   if (in == NULL) {
      fprintf(stderr, "--- %s not found\n", fin);
      return -2;
   }

   while (fgets(fin, 1024, in)) {
      n++;
      if (stmt) free(stmt);
      stmt = strdup(fin);
      switch (cntlParseStmt(fin, &rv)) {
      case 0:
      case 1:
         printf("--- control statement not recognized: \n\t%d: %s\n", n, stmt);
         err = 1;
         break;
      case 2:
         for (i=0; i<sizeof(init)/sizeof(Control); i++) {
            if (strcmp(rv.id, init[i].id) == 0) {
               init[i].strValue=strdup(cntlGetVal(&rv));
               goto ok;
            }
         }
         printf("--- invalid control statement: \n\t%d: %s\n", n, stmt);
         err = 1;
      ok:
         break;
      case 3:
         break;
      }
   }

   if (stmt) free(stmt);

   if (err) {
      printf("--- Broker termintaed because of previous error(s)\n");
      abort();
   }

   return 0;
}

int getControlChars(char *id, char **val)
{
   Control *ctl;
   int rc = -1;
   if ((ctl = ct->ft->get(ct, id))) {
      if (ctl->type == 0) {
         *val = ctl->strValue;
         return 0;
      }
      rc = -2;
   }
   *val = NULL;
   return rc;
}

int getControlNum(char *id, long *val)
{
   Control *ctl;
   int rc = -1;
   if ((ctl = ct->ft->get(ct, id))) {
      if (ctl->type == 1) {
         *val = atol(ctl->strValue);
         return 0;
      }
      rc = -2;
   }
   *val = 0;
   return rc;
}

int getControlBool(char *id, int *val)
{
   Control *ctl;
   int rc = -1;
   if ((ctl = ct->ft->get(ct, id))) {
      if (ctl->type == 2) {
         *val = strcasecmp(ctl->strValue,"true")==0;
         return 0;
      }
      rc = -2;
   }
   *val = 0;
   return rc;
}
