#include <string.h>
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

static const CMPIBroker *_broker;

#define _ClassName "Sample_Method"



CMPIStatus TestMethodProviderMethodCleanup (
    CMPIMethodMI * mi,
    const CMPIContext * ctx,
    CMPIBoolean  term)
{
    CMReturn (CMPI_RC_OK);
}

CMPIStatus TestMethodProviderInvokeMethod (
    CMPIMethodMI * mi,
    const CMPIContext * ctx,
    const CMPIResult * rslt,
    const CMPIObjectPath * ref,
    const char *methodName,
    const CMPIArgs * in,
    CMPIArgs * out)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    CMPIString *className;
    CMPIData data;
    char result[80] = "Hello,";
    const char *strCat;
    const char * name;
    char *argName = "Message";
    CMPIString * str1;
    CMPIString * str2;
    CMPIValue val1, val2;
    /* get the class name from object-path */
    className = CMGetClassName(ref, &rc);
    /* get a pointer to a C char* representation of this String. */
    name = CMGetCharsPtr(className, &rc);

    if(!strcmp(name, _ClassName))
    {
       if(!strcmp ("SayHello", methodName))
       {
            /* gets the number of arguments contained in "in" Args. */
            if(CMGetArgCount(in, &rc) > 0)
            {
                /* gets a Name argument value */
                data = CMGetArg(in, "Name", &rc);
                /*check for data type and not null value of argument value
                  recieved */
                if(data.type == CMPI_string &&  !(CMIsNullValue(data)))
                {
                    strCat = CMGetCharsPtr(data.value.string, &rc);
                    strcat(result, strCat);
//                    strcat(result, "!");
                    /* create the new string to return to client */
                    str1 = CMNewString(_broker, result, &rc);
                    val1.string = str1;
                }
            }
            else
            {
                str1 = CMNewString(_broker, result, &rc);
                val1.string = str1;
            }
            /* create string to add to an array */
            str2 = CMNewString(_broker,"Have a good day", &rc);
            val2.string = str2;
            /* Adds a value of str2 string to out array argument */
            rc = CMAddArg(out, argName, &val2, CMPI_string);
        }
    }
    CMReturnData (rslt, (CMPIValue *) &val1, CMPI_string);
    CMReturnDone (rslt);
    return rc;
}

/* ---------------------------------------------------------------------------*/
/*                              Provider Factory                              */
/* ---------------------------------------------------------------------------*/


CMMethodMIStub (TestMethodProvider,
    TestMethodProvider,
    _broker,
    CMNoHook)

/* ---------------------------------------------------------------------------*/
/*             end of SampleCMPIProvider                      */
/* ---------------------------------------------------------------------------*/

