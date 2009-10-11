#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"
#include "trace.h"

#define _ClassName "Test_Query"
#define _ClassName_size strlen(_ClassName)
#define _Namespace    "root/cimv2"
#define CMPI_false 0
#define CMPI_true 1
#define CMEvaluateSelExpUsingAccessor(s,i,p,r) \
                            ((s)->ft->evaluateUsingAccessor((s),(i),(p),(r)))

static CMPIBroker *_broker;

/* ---------------------------------------------------------------------------*/
/*                       CMPI Helper function                        */
/* ---------------------------------------------------------------------------*/

CMPIData instance_accessor (const char *name, void *param)
{

    CMPIData data = { 0, CMPI_null, {0}};
    CMPIStatus rc = { CMPI_RC_OK, NULL};
    const CMPIBroker *broker = (CMPIBroker *) param;

    if (strcmp ("ElementName", name) == 0)
    {
        data.type = CMPI_string;
        data.state = CMPI_goodValue;
        data.value.string = CMNewString (broker, "Test_Query", &rc);
    }
    else if (strcmp ("c", name) == 0 && (strlen(name)==1))
    {
        data.type = CMPI_chars;
        data.state = CMPI_goodValue;
        data.value.chars = "P";
    }
    else if (strcmp ("n64", name) == 0)
    {
        data.type = CMPI_uint64;
        data.state = CMPI_goodValue;
        data.value.uint64 = 64;
    }
    else if (strcmp ("n32", name) == 0)
    {
        data.type = CMPI_uint32;
        data.state = CMPI_goodValue;
        data.value.uint32 = 32;
    }
    else if (strcmp ("n16", name) == 0)
    {
        data.type = CMPI_uint16;
        data.state = CMPI_goodValue;
        data.value.uint16 = 16;
    }
    else if (strcmp ("n8", name) == 0)
    {
        data.type = CMPI_uint8;
        data.state = CMPI_goodValue;
        data.value.uint8 = 8;
    }
    else if (strcmp ("s64", name) == 0)
    {
        data.type = CMPI_sint64;
        data.state = CMPI_goodValue;
        data.value.sint64 = 0xFFFFFFFF;
    }
    else if (strcmp ("s32", name) == 0)
    {
        data.type = CMPI_sint32;
        data.state = CMPI_goodValue;
        data.value.sint32 = 0xDEADBEEF;
    }
    else if (strcmp ("s16", name) == 0)
    {
        data.type = CMPI_sint16;
        data.state = CMPI_goodValue;
        data.value.sint16 = (CMPISint16)0xFFFF;
    }
    else if (strcmp ("s8", name) == 0)
    {
        data.type = CMPI_sint8;
        data.state = CMPI_goodValue;
        data.value.sint8 = (CMPISint8)0xFF;
    }
    else if (strcmp ("r64", name) == 0)
    {
        data.type = CMPI_real64;
        data.state = CMPI_goodValue;
        data.value.real64 = 3.1415678928283;
    }
    else if (strcmp ("r32", name) == 0)
    {
        data.type = CMPI_real32;
        data.state = CMPI_goodValue;
        data.value.real32 = (CMPIReal32)1.23;
    }

    else if (strcmp ("b", name) == 0 && (strlen(name)==1))
    {
        data.type = CMPI_boolean;
        data.state = CMPI_goodValue;
        data.value.boolean = 1;
    }
    else if ((strcmp ("s", name) == 0) && (strlen(name) == 1))
    {
        data.type = CMPI_string;
        data.state = CMPI_goodValue;
        data.value.string = CMNewString (broker, "Hello", &rc);
    }
    else if ((strcmp("d", name) == 0) && strlen(name) == 1)
    {
        data.type = CMPI_dateTime;
        data.state = CMPI_goodValue;
        data.value.dateTime = CMNewDateTime(broker, &rc);
    }
    return data;
}

CMPIBoolean evalute_selectcond (const CMPISelectCond * cond,
                                CMPIAccessor *accessor,
                                void *parm)
{
    CMPIStatus rc_String = { CMPI_RC_OK, NULL};
    CMPIStatus rc = { CMPI_RC_OK, NULL};
    int sub_type;
    CMPISelectCond *selectcond_clone = NULL;
    CMPISubCond *subcnd = NULL;
    CMPISubCond *subcnd_clone = NULL;
    /* Predicate operations */
    CMPICount pred_cnt;
    unsigned int pred_idx;
    CMPIPredicate *pred = NULL;
    CMPIPredicate *pred2 = NULL;
    CMPIPredicate *pred_clone = NULL;
    CMPIType pred_type;
    CMPIPredOp pred_op;
    CMPIString *left_side = NULL;
    CMPIString *right_side = NULL;
    CMPICount cnt;
    unsigned int idx;

    if (cond != NULL)
    {
        selectcond_clone = CMClone(cond, NULL);
        if (selectcond_clone)
        {
            CMRelease(selectcond_clone);
        }
        cnt = CMGetSubCondCountAndType (cond, &sub_type, &rc);

        /* Parsing the disjunctives */
        for (idx = 0; idx < cnt; idx++)
        {
            subcnd = CMGetSubCondAt (cond, idx, &rc);

            /* Try to copy it */
            subcnd_clone = CMClone (subcnd, &rc);
            if (subcnd_clone)
                CMRelease (subcnd_clone);
            pred_cnt = CMGetPredicateCount (subcnd, &rc);

            /* Parsing throught conjuctives */
            for (pred_idx = 0; pred_idx < pred_cnt; pred_idx++)
            {
                pred = CMGetPredicateAt (subcnd, pred_idx, &rc);

                pred_clone = CMClone (pred, &rc);
                if (pred_clone)
                    CMRelease (pred_clone);

                rc = CMGetPredicateData (pred,
                                         &pred_type,
                                         &pred_op, &left_side, &right_side);
                // LS has the name. Get the predicate using another mechanism.
                pred2 =
                CMGetPredicate (subcnd, CMGetCharsPtr (left_side, &rc_String),
                                &rc);
            }
        }
    }
    return 0;
}



CMPIBoolean evaluate(const CMPISelectExp *se,
                     const CMPIInstance *inst,
                     CMPIAccessor *inst_accessor,
                     void *parm )
{
    CMPIStatus rc_Eval = { CMPI_RC_OK, NULL};
    CMPIStatus rc = { CMPI_RC_OK, NULL};
    CMPISelectCond *doc_cond = NULL;
    CMPISelectCond *cod_cond = NULL;
    CMPIBoolean evalRes = CMPI_false;
    CMPIBoolean evalResAccessor = CMPI_false;
    CMPIBoolean evalCOD = CMPI_false;
    CMPIBoolean evalDOC = CMPI_false;

    if (se)
    {
        evalRes = CMEvaluateSelExp (se, inst, &rc_Eval);

        evalResAccessor =
        CMEvaluateSelExpUsingAccessor (se, inst_accessor, parm,
                                       &rc_Eval);
        doc_cond = CMGetDoc (se, &rc);
        evalDOC=evalute_selectcond (doc_cond, inst_accessor, parm);
        cod_cond = CMGetCod (se, &rc);
        evalCOD=evalute_selectcond (cod_cond, inst_accessor, parm);
    }

    return(evalRes|evalResAccessor|evalDOC|evalCOD);
}


static CMPIObjectPath *
make_ObjectPath (const CMPIBroker * broker, const char *ns, const char *class)
{
  CMPIObjectPath *objPath = NULL;
  CMPIStatus rc = { CMPI_RC_OK, NULL };
  objPath = CMNewObjectPath (broker, ns, class, &rc);
  CMAddKey (objPath, "ElementName", (CMPIValue *) class, CMPI_chars);
  return objPath;
}

static CMPIInstance *
make_Instance (CMPIObjectPath * op)
{

  CMPIStatus rc = { CMPI_RC_OK, NULL };
  CMPIInstance *ci = NULL;
  ci = CMNewInstance (_broker, op, &rc);
  if (rc.rc == CMPI_RC_ERR_NOT_FOUND)
    {
      return NULL;
    }
  return ci;
}

 int
_setProperty (CMPIInstance * ci, const char *p)
{
  CMPIValue val;
  const char *property;
  CMPIStatus rc = { CMPI_RC_OK, NULL };
  if (strncmp (p, _ClassName, _ClassName_size) == 0)
    {
      property = p + _ClassName_size + 1;
    }
  else
    property = p;

  if ((strncmp (property, "ElementName", 11) == 0)
      && (strlen (property) == 11))
    {
      rc =
        CMSetProperty (ci, "ElementName", (CMPIValue *) _ClassName,
                       CMPI_chars);
    }

  else if ((strncmp (property, "s", 1) == 0) && (strlen (property) == 1))
    {
      rc = CMSetProperty (ci, "s", (CMPIValue *) "Hello", CMPI_chars);
    }
  else if ((strncmp (property, "c", 1) == 0) && (strlen (property) == 1))
    {
      rc = CMSetProperty (ci, "c", (CMPIValue *) "c", CMPI_char16);
    }

  else if ((strncmp (property, "n32", 3) == 0) && (strlen (property) == 3))
    {
      val.uint32 = 32;
      rc = CMSetProperty (ci, "n32", &val, CMPI_uint32);
    }
  else if ((strncmp (property, "n64", 3) == 0) && (strlen (property) == 3))
    {
      val.uint64 = 64;
      rc = CMSetProperty (ci, "n64", &val, CMPI_uint64);
    }
  else if ((strncmp (property, "n16", 3) == 0) && (strlen (property) == 3))
    {
      val.uint16 = 16;
      rc = CMSetProperty (ci, "n16", &val, CMPI_uint16);
    }
  else if ((strncmp (property, "n8", 2) == 0) && (strlen (property) == 2))
    {
      val.uint8 = 8;
      rc = CMSetProperty (ci, "n8", &val, CMPI_uint8);
    }

  else if ((strncmp (property, "r32", 3) == 0) && (strlen (property) == 3))
    {
      val.real32 = (CMPIReal32)1.23;
      rc = CMSetProperty (ci, "r32", &val, CMPI_real32);
    }
  else if ((strncmp (property, "r64", 3) == 0) && (strlen (property) == 3))
    {
      val.real64 = 3.1415678928283;
      rc = CMSetProperty (ci, "r64", &val, CMPI_real64);
    }

  else if ((strncmp (property, "s64", 3) == 0) && (strlen (property) == 3))
    {
      val.sint64 = 0xFFFFFFF;
      rc = CMSetProperty (ci, "s64", &val, CMPI_sint64);
    }
  else if ((strncmp (property, "s32", 3) == 0) && (strlen (property) == 3))
    {
      val.sint32 = 0xDEADBEEF;
      rc = CMSetProperty (ci, "s32", &val, CMPI_sint32);
    }
  else if ((strncmp (property, "s16", 3) == 0) && (strlen (property) == 3))
    {
      val.sint16 = (CMPISint16)0xFFFF;
      rc = CMSetProperty (ci, "s16", &val, CMPI_sint16);
    }
  else if ((strncmp (property, "s8", 2) == 0) && (strlen (property) == 2))
    {
      val.sint8 = (CMPISint8)0xFF;
      rc = CMSetProperty (ci, "s8", &val, CMPI_sint8);
    }

  else if ((strncmp (property, "b", 1) == 0) && (strlen (property) == 1))
    {
      val.boolean = 1;
      rc = CMSetProperty (ci, "b", &val, CMPI_boolean);
    }
  else if ((strncmp (property, "d", 1) == 0) && (strlen (property) == 1))
    {
      val.dateTime = CMNewDateTime (_broker, &rc);
      rc = CMSetProperty (ci, "d", &val, CMPI_dateTime);
    }
  else if ((strncmp (property, "*", 1) == 0) && (strlen (property) == 1))
    {
      _setProperty (ci, "ElementName");
      _setProperty (ci, "s");
      _setProperty (ci, "c");
      _setProperty (ci, "n64");
      _setProperty (ci, "n32");
      _setProperty (ci, "n16");
      _setProperty (ci, "n8");
      _setProperty (ci, "s64");
      _setProperty (ci, "s32");
      _setProperty (ci, "s16");
      _setProperty (ci, "s8");
      _setProperty (ci, "r64");
      _setProperty (ci, "r32");
      _setProperty (ci, "d");
      _setProperty (ci, "b");
    }
  else
    {
      return 1;
    }
  return 0;
}
static CMPISelectExp* construct_instance(
    const CMPIBroker* _broker,
    const char* query,
    const char* lang,
    CMPIInstance* inst)
{
  CMPIStatus rc = { CMPI_RC_OK, NULL };
  CMPISelectExp *se_def = NULL;
  unsigned int idx;
  CMPIData data;
  CMPIArray *projection = NULL;
  CMPICount cnt = 0;
  int rc_setProperty = 0;

  se_def = CMNewSelectExp (_broker, query, lang, &projection, &rc);
  if (se_def)
    {
      if (projection)
        {
          cnt = CMGetArrayCount (projection, &rc);
          for (idx = 0; idx < cnt; idx++)
            {
              data = CMGetArrayElementAt (projection, idx, &rc);
              if (data.type == CMPI_chars)
              {
                  rc_setProperty = _setProperty (inst, data.value.chars);
                  if (rc_setProperty)
                  {
                      goto error;
                  }
              }
              if (data.type == CMPI_string)
                {
                  rc_setProperty =
                    _setProperty (inst, CMGetCharsPtr (data.value.string, &rc));
                  if (rc_setProperty)
                  {
                      goto error;
                  }
                }
            }

        }
      else
        {
          _setProperty (inst, "*");
        }
    }
exit:
  return se_def;
error:
  CMRelease(se_def);
  se_def = NULL;
  goto exit;
}

/* ---------------------------------------------------------------------------*/
/*                      Instance Provider Interface                           */
/* ---------------------------------------------------------------------------*/

CMPIStatus
TestExecQueryProviderCleanup (CMPIInstanceMI * mi, CMPIContext * ctx)
{

  CMReturn (CMPI_RC_OK);
}

CMPIStatus
TestExecQueryProviderEnumInstanceNames (CMPIInstanceMI * mi,
                                            CMPIContext * ctx,
                                            CMPIResult * rslt,
                                            CMPIObjectPath * ref)
{

  CMReturn (CMPI_RC_ERR_NOT_SUPPORTED);
}

CMPIStatus
TestExecQueryProviderEnumInstances (CMPIInstanceMI * mi,
                                            CMPIContext * ctx,
                                            CMPIResult * rslt,
                                            CMPIObjectPath * ref,
                                            char **properties)
{

  CMReturn (CMPI_RC_ERR_NOT_SUPPORTED);
}

CMPIStatus
TestExecQueryProviderGetInstance (CMPIInstanceMI * mi,
                                      CMPIContext * ctx,
                                      CMPIResult * rslt,
                                      CMPIObjectPath * cop, char **properties)
{
  CMReturn (CMPI_RC_ERR_NOT_SUPPORTED);
}

CMPIStatus
TestExecQueryProviderCreateInstance (CMPIInstanceMI * mi,
                                         CMPIContext * ctx,
                                         CMPIResult * rslt,
                                         CMPIObjectPath * cop,
                                         CMPIInstance * ci)
{
  CMReturn (CMPI_RC_ERR_NOT_SUPPORTED);
}

CMPIStatus
TestExecQueryProviderSetInstance (CMPIInstanceMI * mi,
                                      CMPIContext * ctx,
                                      CMPIResult * rslt,
                                      CMPIObjectPath * cop,
                                      CMPIInstance * ci, char **properties)
{
  CMReturn (CMPI_RC_ERR_NOT_SUPPORTED);
}

CMPIStatus TestExecQueryProviderModifyInstance  (
    CMPIInstanceMI * mi,
    const CMPIContext * ctx,
    const CMPIResult * rslt,
    const CMPIObjectPath * cop,
    const CMPIInstance * ci,
    const char **properties)
{
  CMReturn (CMPI_RC_ERR_NOT_SUPPORTED);
}

CMPIStatus
TestExecQueryProviderDeleteInstance (CMPIInstanceMI * mi,
                                         CMPIContext * ctx,
                                         CMPIResult * rslt,
                                         CMPIObjectPath * cop)
{
  CMReturn (CMPI_RC_ERR_NOT_SUPPORTED);
}

CMPIStatus
TestExecQueryProviderExecQuery (CMPIInstanceMI * mi,
                                    CMPIContext * ctx,
                                    CMPIResult * rslt,
                                    CMPIObjectPath * ref,
                                    char *query, char *lang)
{

  CMPISelectExp *se_def = NULL;
  CMPIBoolean evalRes;
  CMPIInstance *inst = NULL;
  CMPIObjectPath *objPath = NULL;

  objPath = make_ObjectPath (_broker, _Namespace, _ClassName);
  inst = make_Instance (objPath);
  se_def=construct_instance(_broker, query, lang, inst);
  evalRes = evaluate(se_def, inst,  instance_accessor, (void *)_broker);
  if (evalRes)
  {
       CMReturnInstance (rslt, inst);
       CMReturnDone (rslt);
  }
  CMReturn (CMPI_RC_OK);
}


/* ---------------------------------------------------------------------------*/
/*                              Provider Factory                              */
/* ---------------------------------------------------------------------------*/

CMInstanceMIStub (TestExecQueryProvider,
                  TestExecQueryProvider, _broker, CMNoHook);


/* ---------------------------------------------------------------------------*/
/*             end of TestCMPIProvider                      */
/* ---------------------------------------------------------------------------*/
