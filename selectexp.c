
/*
 * selectexp.c
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
 * Author:        Frank Scheffler
 * Contributions: Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * CMPISelectExp implementation.
 *
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native.h"
#include "queryOperation.h"
#include "selectexp.h"

QLStatement *parseQuery(int mode, char *query, char *lang, int *rc);
static NativeSelectExp *__new_exp(int node,
         const char *queryString, const char *language,
         CMPIArray ** projection, CMPIStatus * rc);
extern CMPIValue queryGetValue(QLPropertySource* src, char* name, QLOpd *type);
extern CMPISelectCond *TrackedCMPISelectCond(CMPIArray *conds, int type, CMPIStatus * rc);


static NativeSelectExp *__new_exp(int, const char *,  const char *,
                                          CMPIArray **, CMPIStatus *);


/*****************************************************************************/

static CMPIStatus __eft_release(CMPISelectExp * exp)
{
   NativeSelectExp *e = (NativeSelectExp *) exp;

   if (e->mem_state && e->mem_state != MEM_RELEASED) {
      free(e->queryString);
      free(e->language);
      memUnlinkEncObj(e->mem_state);
      e->mem_state = MEM_RELEASED;
      free(e);
      CMReturn(CMPI_RC_OK);
   }
   CMReturn(CMPI_RC_ERR_FAILED);
}


static CMPISelectExp *__eft_clone(CMPISelectExp * exp, CMPIStatus * rc)
{
   NativeSelectExp *e = (NativeSelectExp *) exp;

   return (CMPISelectExp *) __new_exp(MEM_NOT_TRACKED,
         e->queryString, e->language, NULL, rc);
}



CMPIBoolean __eft_evaluate(CMPISelectExp * exp,
                           CMPIInstance * inst, CMPIStatus * rc)
{
   int irc;
   NativeSelectExp *e = (NativeSelectExp *) exp;
   struct qlPropertySource src={inst,queryGetValue};
   
   if (rc) CMSetStatus(rc, CMPI_RC_OK);
   if (e->qs->where==NULL) return 1;
   
   irc=e->qs->where->ft->evaluate(e->qs->where,&src);
   return irc;
}


CMPIString *__eft_getString(CMPISelectExp * exp, CMPIStatus * rc)
{
   NativeSelectExp *e = (NativeSelectExp *) exp;
   return native_new_CMPIString(e->queryString,rc);
}


CMPISelectCond *__eft_getDOC(CMPISelectExp * exp, CMPIStatus * rc)
{
   NativeSelectExp *e = (NativeSelectExp *) exp;
   PredicateDisjunction *pc=NULL;   
   
   if (e->qs->where) 
      pc=e->qs->where->ft->getPredicateDisjunction(e->qs->where);

   return TrackedCMPISelectCond(pc,0,rc);       
}


CMPISelectCond *__eft_getCOD(CMPISelectExp * exp, CMPIStatus * rc)
{
   NativeSelectExp *e = (NativeSelectExp *) exp;
   PredicateConjunction *pc=NULL;   
   
   if (e->qs->where) 
      pc=e->qs->where->ft->getPredicateConjunction(e->qs->where);

   return TrackedCMPISelectCond(pc,1,rc);       
}

CMPIBoolean __eft_evaluateUsingAccessor(CMPISelectExp * se,
                                        CMPIAccessor * accessor,
                                        void *parm, CMPIStatus * rc)
{
   if (rc)
      rc->rc = CMPI_RC_ERR_NOT_SUPPORTED;
   return 0;
}

static CMPISelectExpFT eft = {
      NATIVE_FT_VERSION,
      __eft_release,
      __eft_clone,
      __eft_evaluate,
      __eft_getString,
      __eft_getDOC,
      __eft_getCOD,
      __eft_evaluateUsingAccessor
};

static CMPISelectExp eFt = {
   "CMPISelectExp",
   &eft
};

static NativeSelectExp *__new_exp(int mode, const char *queryString,
               const char *language, CMPIArray ** projection, CMPIStatus * rc)
{               
   int state,irc;
   NativeSelectExp exp,*tExp;
   
   memset(&exp, 0, sizeof(exp));
   exp.exp = eFt;
   
   exp.qs=parseQuery(mode,(char*)queryString, (char*)language, &irc);
   if (irc) {
      if (rc) CMSetStatus(rc, CMPI_RC_ERR_INVALID_QUERY);
      return NULL;
   }   

   exp.queryString = strdup(queryString);
   exp.language = strdup(language);
   
   if (projection) {
      char **list=exp.qs->spNames;
      int i;
      CMPIArray *ar=
        *projection=TrackedCMPIArray(exp.qs->spNext,CMPI_string, NULL);
      for (i=0; *list; list++,i++)
         CMSetArrayElementAt(ar,i,(CMPIValue*)*list,CMPI_chars);
   }

   tExp=memAddEncObj(mode, &exp, sizeof(exp),&state);
   tExp->mem_state=state;
   
   if (rc) CMSetStatus(rc, CMPI_RC_OK);
   return tExp;
}


CMPISelectExp *TrackedCMPISelectExp(const char *queryString,
                                        const char *language,
                                        CMPIArray ** projection,
                                        CMPIStatus * rc)
{
   return (CMPISelectExp*) __new_exp(MEM_TRACKED,
          queryString, language, projection, rc);
}

CMPISelectExp *NewCMPISelectExp(const char *queryString,
                                        const char *language,
                                        CMPIArray ** projection,
                                        CMPIStatus * rc)
{
   return (CMPISelectExp*) __new_exp(MEM_NOT_TRACKED,
          queryString, language, projection, rc);
}

CMPISelectExp *TempCMPISelectExp(QLStatement *qs)
{
   NativeSelectExp *exp=(NativeSelectExp*)calloc(1,sizeof(NativeSelectExp));
   exp->exp = eFt;   
   exp->qs=qs;
   
   return exp;
}


/****************************************************************************/

/*** Local Variables:  ***/
/*** mode: C           ***/
/*** c-basic-offset: 8 ***/
/*** End:              ***/
