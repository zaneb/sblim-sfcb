
/*
 * providerRegister.c
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
 * Based on concepts developed by Viktor Mihajlovski <mihajlov@de.ibm.com>
 *
 * Description:
 *
 * Provider registration support.
 *
*/


#include "utilft.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <support.h>

#include "providerRegister.h"

static ProviderInfo forceNotFound={"",FORCE_PROVIDER_NOTFOUND};
extern int exFlags;
extern char * configfile;

ProviderInfo *classProvInfoPtr = NULL;
ProviderInfo *defaultProvInfoPtr = NULL;
ProviderInfo *interOpProvInfoPtr = NULL;
ProviderInfo *forceNoProvInfoPtr = &forceNotFound;

static void release(ProviderRegister * br)
{
   ProviderBase *bb = (ProviderBase *) br->hdl;
   free(bb->fn);
   bb->ht->ft->release(bb->ht);
   free(br);
}

static ProviderRegister *clone(ProviderRegister * br)
{
   return NULL;
}

ProviderRegister *newProviderRegister(char *fn)
{
   FILE *in;
   char *dir;
   char fin[1024], *stmt = NULL;
   ProviderInfo *info = NULL;
   int err = 0, n = 0;
   CntlVals rv;
   int id=0;
   int interopFound=0;
   ProviderRegister *br = (ProviderRegister *) malloc(sizeof(ProviderRegister) +
                                                      sizeof(ProviderBase));
   ProviderBase *bb = (ProviderBase *) br + 1;
   
   setupControl(configfile);

   if (getControlChars("registrationDir",&dir)) {
     dir = "/var/lib/sfcb/registration";
   }

   strcpy(fin, dir);
   strcat(fin, "/providerRegister");
   in = fopen(fin, "r");
   if (in == NULL) {
      fprintf(stderr, "--- %s not found\n", fin);
      return NULL;
   }

   br->hdl = bb;
   br->ft = ProviderRegisterFT;
   bb->fn = strdup(fin);
   bb->ht = UtilFactory->newHashTable(61,
               UtilHashTable_charKey | UtilHashTable_ignoreKeyCase);

   while (fgets(fin, 1024, in)) {
      n++;
      if (stmt)
         free(stmt);
      stmt = strdup(fin);
      switch (cntlParseStmt(fin, &rv)) {
      case 0:
         printf("--- registration statement not recognized: \n\t%d: %s\n", n,
                stmt);
         err = 1;
         break;
      case 1:
         if (info) {
            if (classProvInfoPtr==NULL) {
               if (strcmp(info->className,"$ClassProvider$")==0) classProvInfoPtr=info;
            }   
            else if (defaultProvInfoPtr==NULL) {
               if (strcmp(info->className,"$DefaultProvider$")==0) defaultProvInfoPtr=info;
            }   
            else if (interOpProvInfoPtr==NULL) {
               if (strcmp(info->className,"$InterOpProvider$")==0) {
                  if (exFlags & 2) interOpProvInfoPtr=info;
                  else interopFound=1;
               }   
            }   
            bb->ht->ft->put(bb->ht, info->className, info);
         }
         info = (ProviderInfo *) calloc(1, sizeof(ProviderInfo));
         info->className = strdup(rv.id);
         info->id= ++id;
         break;
      case 2:
         if (strcmp(rv.id, "provider") == 0)
            info->providerName = strdup(cntlGetVal(&rv));
         else if (strcmp(rv.id, "location") == 0)
            info->location = strdup(cntlGetVal(&rv));
         else if (strcmp(rv.id, "group") == 0)
            info->group = strdup(cntlGetVal(&rv));
         else if (strcmp(rv.id, "unload") == 0) {
            char *u;
            info->unload = 0;
            while ((u = cntlGetVal(&rv)) != NULL) {
               if (strcmp(u, "never") == 0) {
                  info->unload =-1;
               }   
               else {
                  printf("--- invalid unload specification: \n\t%d: %s\n", n, stmt);
                  err = 1;
               }
            }   
         }   
         else if (strcmp(rv.id, "type") == 0) {
            char *t;
            info->type = 0;
            while ((t = cntlGetVal(&rv)) != NULL) {
               if (strcmp(t, "instance") == 0)
                  info->type |= INSTANCE_PROVIDER;
               else if (strcmp(t, "association") == 0)
                  info->type |= ASSOCIATION_PROVIDER;
               else if (strcmp(t, "method") == 0)
                  info->type |= METHOD_PROVIDER;
               else if (strcmp(t, "indication") == 0)
                  info->type |= INDICATION_PROVIDER;
               else if (strcmp(t, "class") == 0)
                  info->type |= CLASS_PROVIDER;
               else {
                  printf("--- invalid type specification: \n\t%d: %s\n", n, stmt);
                  err = 1;
               }
            }
         }
         else if (strcmp(rv.id, "namespace") == 0) {
            int max=1,next=0;
            char *t;
            info->ns=(char**)malloc(sizeof(char*)*(max+1));
            while ((t = cntlGetVal(&rv)) != NULL) {
               if (next==max) {
                  max++;
                  info->ns=(char**)realloc(info->ns,sizeof(char*)*(max+1));
               }
               info->ns[next]=strdup(t);
               info->ns[++next]=NULL;
            }
         }
         else {
            printf("--- invalid registration statement: \n\t%d: %s\n", n, stmt);
            err = 1;
         }
         break;
      case 3:
         break;
      }
   }

   if (info) {
      bb->ht->ft->put(bb->ht, info->className, info);
   }

   if (classProvInfoPtr==NULL) {
      printf("--- Class provider definition not found\n");
      err=1;
   }
   
   if (defaultProvInfoPtr==NULL) 
      printf("--- Default provider definition not found - no instance repository available\n");
   
   if (interOpProvInfoPtr==NULL) {
      if (exFlags & 2 && interopFound==0)
         printf("--- InterOp provider definition not found - no InterOp support available\n");
      else if (interopFound)    
         printf("--- InterOp provider definition found but not started - no InterOp support available\n");
      interOpProvInfoPtr=&forceNotFound;
   }   
   
   if (err) {
      printf("--- Broker terminated because of previous error(s)\n");
      abort();
   }
   if (stmt) free(stmt);
   return br;
}

static int putProvider(ProviderRegister * br, const char *clsName,
                       ProviderInfo * info)
{
   ProviderBase *bb = (ProviderBase *) br->hdl;
   return bb->ht->ft->put(bb->ht, clsName, info);
}

static ProviderInfo *getProvider(ProviderRegister * br,
                                 const char *clsName, unsigned long type)
{
   ProviderBase *bb = (ProviderBase *) br->hdl;
   ProviderInfo *info = bb->ht->ft->get(bb->ht, clsName);
   if (info && info->type & type)
      return info;
   return NULL;
}

static ProviderInfo *locateProvider(ProviderRegister * br, const char *provName)
{
   ProviderBase *bb = (ProviderBase *) br->hdl;
   HashTableIterator *it;
   char *key = NULL;
   ProviderInfo *info = NULL;

   for (it = bb->ht->ft->getFirst(bb->ht, (void **) &key, (void **) &info);
        key && it && info;
        it = bb->ht->ft->getNext(bb->ht, it, (void **) &key, (void **) &info)) {
      if (strcasecmp(info->providerName, provName) == 0)
         return info;
   }
   return NULL;
}

static void removeProvider(ProviderRegister * br, const char *clsName)
{
   ProviderBase *bb = (ProviderBase *) br->hdl;
   bb->ht->ft->remove(bb->ht, clsName);
}

static Provider_Register_FT ift = {
   1,
   release,
   clone,
   getProvider,
   putProvider,
   removeProvider,
   locateProvider
};

Provider_Register_FT *ProviderRegisterFT = &ift;
