/*
 * classProviderCommon.h
 *
 * (C) Copyright IBM Corp. 2005, 2009
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Common header file for classProvider, classProviderGz, classProviderMem,
 *                        and classProviderSf
 *
*/

#include "utilft.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "constClass.h"
#include "providerRegister.h"
#include "trace.h"
#include "control.h"

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpiftx.h"
#include "cmpimacs.h"
#include "cmpimacsx.h"
#include "objectImpl.h"
#include "mrwlock.h"

static const CMPIBroker *_broker;

extern char * configfile;
extern ProviderRegister *pReg;

typedef struct nameSpaces {
   int next,max,blen;
   char *base;
   char *names[1];
} NameSpaces;

static UtilHashTable *nsHt=NULL;
static pthread_once_t nsHt_once = PTHREAD_ONCE_INIT;

#define Iterator HashTableIterator*
#define NEW(x) ((x *) calloc(1,sizeof(x)))
