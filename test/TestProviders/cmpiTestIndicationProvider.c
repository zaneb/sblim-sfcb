#include <stdio.h>
#include <string.h>
#include <time.h>

#define CMPI_VER_86

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

static const CMPIBroker *broker;

unsigned char CMPI_true = 1;
unsigned char CMPI_false = 0;

static int enabled = 0;
static int _nextUID = 0;


static void
generateIndication (const char *methodname, const CMPIContext * ctx)
{

  CMPIInstance *inst;
  CMPIObjectPath *cop;
  CMPIDateTime *dat;
  CMPIArray *ar;
  CMPIStatus rc;
  char buffer[32];

  if (enabled)
    {
      cop =
        CMNewObjectPath (broker, "root/cimv2",
                         "Test_Indication", &rc);
      inst = CMNewInstance (broker, cop, &rc);

      sprintf (buffer, "%d", _nextUID++);
      CMSetProperty (inst, "IndicationIdentifier", buffer, CMPI_chars);

      dat = CMNewDateTime (broker, &rc);
      CMSetProperty (inst, "IndicationTime", &dat, CMPI_dateTime);

      CMSetProperty (inst, "MethodName", methodname, CMPI_chars);

      ar = CMNewArray (broker, 0, CMPI_string, &rc);
      CMSetProperty (inst, "CorrelatedIndications", &ar, CMPI_stringA);

      rc = CBDeliverIndication (broker, ctx, "root/interop", inst);
      if (rc.rc != CMPI_RC_OK)
        {
          fprintf (stderr, "+++ Could not send the indication!\n");
        }
    }
  fprintf (stderr, "+++ generateIndication() done\n");
}

//----------------------------------------------------------
//---
//      Method Provider
//---
//----------------------------------------------------------


CMPIStatus
indProvMethodCleanup (CMPIMethodMI * cThis, const CMPIContext * ctx,
                       CMPIBoolean term)
{
  CMReturn (CMPI_RC_OK);
}

CMPIStatus indProvInvokeMethod
  (CMPIMethodMI * cThis, const CMPIContext * ctx, const CMPIResult * rslt,
   const CMPIObjectPath * cop, const char *method, const CMPIArgs * in,
   CMPIArgs * out)
{
  CMPIValue value;
  fprintf (stderr, "+++ indProvInvokeMethod()\n");

  if (enabled == 0)
    {
      fprintf (stderr, "+++ PROVIDER NOT ENABLED\n");
    }
  else
    {
      generateIndication (method, ctx);
    }

  value.uint32 = 0;
  CMReturnData (rslt, &value, CMPI_uint32);
  CMReturnDone (rslt);
  CMReturn (CMPI_RC_OK);
}

//----------------------------------------------------------
//---
//      Indication Provider
//---
//----------------------------------------------------------


CMPIStatus
indProvIndicationCleanup (CMPIIndicationMI * cThis, const CMPIContext * ctx,
                           CMPIBoolean term)
{
  CMReturn (CMPI_RC_OK);
}

CMPIStatus indProvAuthorizeFilter
  (CMPIIndicationMI * cThis, const CMPIContext * ctx,
   const CMPISelectExp * filter, const char *indType,
   const CMPIObjectPath * classPath, const char *owner)
{

  CMReturn (CMPI_RC_OK);
}

CMPIStatus indProvMustPoll
  (CMPIIndicationMI * cThis, const CMPIContext * ctx,
   const CMPISelectExp * filter, const char *indType,
   const CMPIObjectPath * classPath)
{

  CMReturn (CMPI_RC_OK);
}

CMPIStatus indProvActivateFilter
  (CMPIIndicationMI * cThis, const CMPIContext * ctx,
   const CMPISelectExp * exp, const char *clsName,
   const CMPIObjectPath * classPath, CMPIBoolean firstActivation)
{
  fprintf (stderr, "+++ indProvActivateFilter()\n");
  enabled=1;
  fprintf (stderr, "--- enabled: %d\n", enabled);
  CMReturn (CMPI_RC_OK);
}

CMPIStatus indProvDeActivateFilter
  (CMPIIndicationMI * cThis, const CMPIContext * ctx,
   const CMPISelectExp * filter, const char *clsName,
   const CMPIObjectPath * classPath, CMPIBoolean lastActivation)
{
  fprintf (stderr, "+++ indProvDeActivateFilter\n");
  enabled=0;
  fprintf (stderr, "--- disabled: %d\n", enabled);
  CMReturn (CMPI_RC_OK);
}

CMPIStatus
indProvEnableIndications (CMPIIndicationMI * cThis, const CMPIContext * ctx)
{
  fprintf (stderr, "+++ indProvEnableIndications\n");
  CMReturn (CMPI_RC_OK);
}

CMPIStatus
indProvDisableIndications (CMPIIndicationMI * cThis, const CMPIContext * ctx)
{
  fprintf (stderr, "+++ indProvDisableIndications\n");
  CMReturn (CMPI_RC_OK);
}


//----------------------------------------------------------
//---
//      Provider Factory Stubs
//---
//----------------------------------------------------------

CMMethodMIStub (indProv, TestIndicationProvider, broker, CMNoHook)
//----------------------------------------------------------


CMIndicationMIStub (indProv, TestIndicationProvider, broker, CMNoHook)

//----------------------------------------------------------
