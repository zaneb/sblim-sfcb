#include <string.h>
#include <stdio.h>
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

static const CMPIBroker *_broker;

#define _ClassName "Sample_Method"

static char *paramType(CMPIType type)
{
   switch (type & ~CMPI_ARRAY) {
   case CMPI_chars:
   case CMPI_string:
   case CMPI_instance:
      return "string";
   case CMPI_sint64:
      return "sint64";
   case CMPI_uint64:
      return "uint64";
   case CMPI_sint32:
      return "sint32";
   case CMPI_uint32:
      return "uint32";
   case CMPI_sint16:
      return "sint16";
   case CMPI_uint16:
      return "uint16";
   case CMPI_uint8:
      return "uint8";
   case CMPI_sint8:
      return "sint8";
   case CMPI_boolean:
      return "boolean";
   case CMPI_char16:
      return "char16";
   case CMPI_real32:
      return "real32";
   case CMPI_real64:
      return "real64";
   case CMPI_dateTime:
      return "datetime";
   case CMPI_ref:
      return "reference";
   }
   printf("Invalid data type %d %x\n",(int) type, (int) type);
   return "*??*";
}




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
        } else if (!strcmp("CheckArrayNoType", methodName)) {
            data = CMGetArg(in, "IntArray", &rc);
            CMPIType atype=data.value.array->ft->getSimpleType(data.value.array,&rc); 
            sprintf(result,"Datatype is %s",paramType(atype));
            str1 = CMNewString(_broker, result, &rc);
            val1.string = str1;
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

