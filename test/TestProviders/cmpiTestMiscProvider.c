#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#define _ClassName "TestCMPI_Method"
#define _ClassName_size strlen(_ClassName)
#define _Namespace    "root/cimv2"
#define _PersonClass  "CMPI_TEST_Person"
#  define CMObjectPathToString(p,rc) \
       ((p)->ft->toString((p),(rc)))
#  define CMGetMessage(b,id,def,rc,parms) \
       ((b)->eft->getMessage((b),(id),(def),(rc),parms))

static CMPIBroker *_broker;

#ifdef __GNUC__
# define UINT64_LITERAL(X) ((CMPIUint64)X##ULL)
#else
# define UINT64_LITERAL(X) ((CMPIUint64)X)
#endif

/* ---------------------------------------------------------------------------*/
/*                       CMPI Helper function                        */
/* ---------------------------------------------------------------------------*/



CMPIObjectPath * make_ObjectPath (
    const CMPIBroker *broker,
    const char *ns,
    const char *className)
{
    CMPIObjectPath *objPath = NULL;
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    objPath = CMNewObjectPath (broker, ns, className, &rc);
    CMAddKey (objPath, "ElementName", (CMPIValue *) className, CMPI_chars);
    return objPath;
}

CMPIInstance * make_Instance (const CMPIObjectPath * op)
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

static int
_CMGetMessage (char **result)
{

  CMPIString *str = NULL;
  CMPIStatus rc = { CMPI_RC_OK, NULL };
  str =
    CMGetMessage (_broker, "Common.CIMStatusCode.CIM_ERR_SUCCESS",
                  "CIM_ERR_SUCCESS: Successful.", &rc, 0);
  if (str)
    {
      *result = strdup (CMGetCharsPtr (str,NULL));
    }
  if (rc.rc == CMPI_RC_OK)
    return 0;
  return 1;
}

static int
_CMLogMessage (char **result)
{
  CMPIStatus rc = { CMPI_RC_OK, NULL };
  CMPIString *str = CMNewString (_broker, "CMLogMessage", &rc);
  rc.rc = CMPI_RC_OK;
  rc = CMLogMessage (_broker, 64, _ClassName, "Log message", NULL);
  if (rc.rc == CMPI_RC_ERR_NOT_SUPPORTED) {
    *result=strdup("LogMessage success");
    return 0;
  }
  return 1;
}

static int
_CMTraceMessage (char **result)
{
  CMPIStatus rc = { CMPI_RC_OK, NULL };
  CMPIString *str = CMNewString (_broker, "CMTraceMessage", &rc);
  rc.rc = CMPI_RC_OK;
  rc = CMTraceMessage (_broker, 2, NULL, NULL, NULL);
  if (rc.rc == CMPI_RC_ERR_NOT_SUPPORTED){
    *result=strdup("TraceMessage success");
    return 0;
  }
  return 1;
}

static int _testArrayTypes()
{
    struct array_types
    {
        CMPIType element_type;
        CMPIType typeA;
        char* typeName;
        char* typeAName;
        char* args_name;
    }types_arr[] = {
        {CMPI_uint32,
        CMPI_uint32A,
        "CMPI_uint32",
        "CMPI_uint32A",
        "CMPI_uint32_array"},

        {CMPI_uint16,
        CMPI_uint16A,
        "CMPI_uint16",
        "CMPI_uint16A",
        "CMPI_uint16_array"},

        {CMPI_uint8,
        CMPI_uint8A,
        "CMPI_uint8",
        "CMPI_uint8A",
        "CMPI_uint8_array"},

        {CMPI_uint64,
        CMPI_uint64A,
        "CMPI_uint64",
        "CMPI_uint64A",
        "CMPI_uint64_array"},

        {CMPI_sint32,
        CMPI_sint32A,
        "CMPI_sint32",
        "CMPI_sint32A",
        "CMPI_sint32_array"},

        {CMPI_sint16,
        CMPI_sint16A,
        "CMPI_sint16",
        "CMPI_sint16A",
        "CMPI_sint16_array"},

        {CMPI_sint8,
        CMPI_sint8A,
        "CMPI_sint8",
        "CMPI_sint8A",
        "CMPI_sint8_array"},

        {CMPI_sint64,
        CMPI_sint64A,
        "CMPI_sint64",
        "CMPI_sint64A",
        "CMPI_sint64_array"},

        {CMPI_real32,
        CMPI_real32A,
        "CMPI_real32",
        "CMPI_real32A",
        "CMPI_real32_array"},

        {CMPI_real64,
        CMPI_real64A,
        "CMPI_real64",
        "CMPI_real64A",
        "CMPI_real64_array"},

        {CMPI_char16,
        CMPI_char16A,
        "CMPI_char16",
        "CMPI_char16A",
        "CMPI_char16_array"},

        {CMPI_boolean,
        CMPI_booleanA,
        "CMPI_boolean",
        "CMPI_booleanA",
        "CMPI_boolean_array"},

        {CMPI_string,
        CMPI_stringA,
        "CMPI_string",
        "CMPI_stringA",
        "CMPI_string_array"},

        {CMPI_dateTime,
        CMPI_dateTimeA,
        "CMPI_dateTime",
        "CMPI_dateTimeA",
        "CMPI_dateTime_array"},

        {CMPI_ref,
        CMPI_refA,
        "CMPI_ref",
        "CMPI_refA",
        "CMPI_ref_array"},

        {CMPI_instance,
        CMPI_instanceA,
        "CMPI_instance",
        "CMPI_instanceA",
        "CMPI_instance_array"},
        {CMPI_null,
        CMPI_ARRAY,
        "Invalid",
        "InvalidArray",
        "Invalid_array"}};

    int i ,flag, size;
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    CMPIStatus rc1 = { CMPI_RC_OK, NULL };
    CMPIArray *arr = NULL;
    CMPIString* retNamespace = NULL;
    CMPIString* retClassname = NULL;
    CMPIValue value, value1;
    CMPIData data;
    CMPIData arr_data;
    CMPIData dataInst;
    CMPIData retDataInst;
    CMPIArgs* args_ptr = NULL;
    CMPIObjectPath* objPath = make_ObjectPath(_broker,
        _Namespace,
        _ClassName);
    CMPIUint64 datetime1, datetime2;
    const char* str1;
    const char* str2;

    size = 17;
    for ( i = 0 ; i < size; i++)
    {
        args_ptr = CMNewArgs(_broker, &rc);
        switch(types_arr[i].element_type)
        {
            case CMPI_uint32:
                value.uint32 = 56;
                break;

            case CMPI_uint16:
                value.uint16 = 32;
                break;

            case CMPI_uint8:
                value.uint8 = 56;
                break;

            case CMPI_uint64:
                value.uint64 = 32;
                break;

            case CMPI_sint32:
                value.sint32 = -56;
                break;

            case CMPI_sint16:
                value.sint16 = -32;
                break;

            case CMPI_sint8:
                value.sint8 = -56;
                break;

            case CMPI_sint64:
                value.sint64 = -32;
                break;

            case CMPI_real32:
                value.real32 = (CMPIReal32)-32.78;
                break;

            case CMPI_real64:
                value.real64 = -899.32;
                break;

            case CMPI_char16:
                value.char16 = 'k';
                break;

            case CMPI_string:
                value.string = CMNewString(_broker, "string", &rc);
                break;

            case CMPI_boolean:
                value.boolean = 1;
                break;

            case CMPI_dateTime:
                value.dateTime = CMNewDateTime(_broker, &rc);
                break;

            case CMPI_ref:
                value.ref = CMNewObjectPath (_broker,
                    "root/cimv2",
                    "Sample_Instance",
                    &rc);
                break;

            case CMPI_null:
                value.args = NULL;
                break;

            case CMPI_instance:
                value.inst = make_Instance(objPath);
                value1.uint32 = 20;
                rc = CMSetProperty(value.inst,
                    "Property1",
                    &value1,
                    CMPI_uint32);
                break;
        }

        arr = NULL;
        rc = CMAddArg (args_ptr,
            "EmptyArray",
            (CMPIValue *) &arr,
            types_arr[i].typeA);
        arr = CMNewArray (_broker, 1, types_arr[i].element_type, &rc);

        rc = CMSetArrayElementAt(arr, 0, &value, types_arr[i].element_type);

        rc = CMAddArg (args_ptr,
            types_arr[i].args_name,
            (CMPIValue *) &arr,
            types_arr[i].typeA);

        flag = 1;
        if((types_arr[i].element_type) != CMPI_null)
        {
            data = CMGetArg(args_ptr, types_arr[i].args_name , &rc);

            arr_data = CMGetArrayElementAt(data.value.array, 0, &rc);

            switch(types_arr[i].element_type)
            {
                case CMPI_uint32:
                    if (arr_data.value.uint32 != value.uint32)
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_uint16:
                    if (arr_data.value.uint16 != value.uint16)
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_uint8:
                    if (arr_data.value.uint8 != value.uint8)
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_uint64:
                    if (arr_data.value.uint64 != value.uint64)
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_sint32:
                    if (arr_data.value.sint32 != value.sint32)
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_sint16:
                    if (arr_data.value.sint16 != value.sint16)
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_sint8:
                    if (arr_data.value.sint8 != value.sint8)
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_sint64:
                    if (arr_data.value.sint64 != value.sint64)
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_real32:
                    if (arr_data.value.real32 != value.real32)
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_real64:
                    if (arr_data.value.real64 != value.real64)
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_char16:
                    if (arr_data.value.char16 != value.char16)
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_string:
                    str1 = CMGetCharsPtr(arr_data.value.string, &rc);
                    str2 = CMGetCharsPtr(value.string, &rc1);
                    if ((rc.rc != CMPI_RC_OK) ||
                        (rc1.rc != CMPI_RC_OK) ||
                        strcmp(str1, str2))
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_boolean:
                    if (arr_data.value.boolean != value.boolean)
                    {
                        flag = 0;
                    }
                    break;

                case CMPI_dateTime:
                    datetime1 = CMGetBinaryFormat(arr_data.value.dateTime,
                        &rc);
                    datetime2 = CMGetBinaryFormat(value.dateTime, &rc1);
                    if ((rc.rc != CMPI_RC_OK) ||
                        (rc1.rc != CMPI_RC_OK) ||
                        (datetime1 != datetime2))
                    {
                        flag = 0;
                    }
                    rc = CMRelease(value.dateTime);
                    break;

                case CMPI_ref:
                    retNamespace = CMGetNameSpace(arr_data.value.ref, &rc);
                    retClassname = CMGetClassName(arr_data.value.ref, &rc1);
                    if((rc.rc == CMPI_RC_OK) &&
                        (rc1.rc == CMPI_RC_OK))
                    {
                        str1 = CMGetCharsPtr(retNamespace, &rc);
                        str2 = CMGetCharsPtr(retClassname, &rc1);
                        if ((rc.rc == CMPI_RC_OK) &&
                            (rc1.rc == CMPI_RC_OK))
                        {
                            if ((strcmp(str1, "root/cimv2")) ||
                                (strcmp(str2, "TestCMPI_Instance")))
                            {
                                flag = 0;
                            }
                        }
                        else
                        {
                            flag = 0;
                        }
                    }
                    else
                    {
                        flag = 0;
                    }
                    rc = CMRelease(value.ref);
                    break;

                case CMPI_instance:
                    retDataInst = CMGetProperty(arr_data.value.inst,
                        "Property1", &rc);
                    dataInst = CMGetProperty(value.inst, "Property1", &rc);
                    if (retDataInst.value.uint32 != dataInst.value.uint32)
                    {
                        flag = 0;
                    }
                    rc = CMRelease(value.inst);
                    break;
            }
            if (data.type == types_arr[i].typeA && flag)
            {
            }
        }
        rc = CMRelease(arr);
        rc = CMRelease(args_ptr);
    }
    return flag;
}

static int _testSimpleTypes()
{
    CMPIArgs* args_ptr = NULL;
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    CMPIStatus rc1 = { CMPI_RC_OK, NULL };
    int i, flag, size;
    CMPIValue value;
    CMPIValue value1;
    CMPIData data;
    CMPIData dataInst;
    CMPIData retDataInst;
    CMPIString* retNamespace = NULL;
    CMPIString* retClassname = NULL;
    CMPIObjectPath* objPath = make_ObjectPath(_broker,
        _Namespace,
        _ClassName);
    const char* str1;
    const char* str2;

    struct array_types
    {
        CMPIType element_type;
        char* typeName;
        char* args_name;
    }types_arr[] = {

        {CMPI_instance,
        "CMPI_instance",
        "CMPI_instance"},
        {CMPI_ref,
        "CMPI_ref",
        "CMPI_ref"}};

    size = 2;

    flag = 1;
    for ( i = 0 ; i < size; i++)
    {
        args_ptr = CMNewArgs(_broker, &rc);

        switch(types_arr[i].element_type)
        {
            case CMPI_ref:
                value.ref = CMNewObjectPath (_broker,
                    "root/cimv2",
                    "Sample_Instance",
                    &rc);
                break;

            case CMPI_instance:
                value.inst = make_Instance(objPath);
                value1.uint32 = 20;
                rc = CMSetProperty(value.inst,
                    "Property1",
                    &value1,
                    CMPI_uint32);
                break;
        }
        rc = CMAddArg (args_ptr,
            types_arr[i].args_name,
            (CMPIValue *) &value,
            types_arr[i].element_type);

        data = CMGetArg(args_ptr, types_arr[i].args_name , &rc);

        switch(types_arr[i].element_type)
        {
            case CMPI_ref:
                retNamespace = CMGetNameSpace(data.value.ref, &rc);
                retClassname = CMGetClassName(data.value.ref, &rc1);

                if((rc.rc == CMPI_RC_OK) &&
                    (rc1.rc == CMPI_RC_OK))
                {
                    str1 = CMGetCharsPtr(retNamespace, &rc);
                    str2 = CMGetCharsPtr(retClassname, &rc1);
                    if ((rc.rc == CMPI_RC_OK) &&
                        (rc1.rc == CMPI_RC_OK))
                    {
                        if ((strcmp(str1, "root/cimv2")) ||
                            (strcmp(str2, "Sample_Instance")))
                        {
                            flag = 0;
                        }
                    }
                    else
                    {
                        flag = 0;
                    }
                }
                else
                {
                    flag = 0;
                }
                rc = CMRelease(value.ref);
                break;

            case CMPI_instance:
                retDataInst = CMGetProperty(data.value.inst,
                    "Property1", &rc);
                dataInst = CMGetProperty(value.inst, "Property1", &rc);
                if (retDataInst.value.uint32 != dataInst.value.uint32)
                {
                    flag = 0;
                }
                rc = CMRelease(value.inst);
                break;
        }
        if (data.type == types_arr[i].element_type && flag)
        {
        }
        rc = CMRelease(args_ptr);
    }
    return flag;
}

static int _testErrorPaths()
{
    CMPIArgs* args_ptr = NULL;
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    CMPIValue value;
    char* str = NULL;
    value.inst = NULL;
    args_ptr = CMNewArgs(_broker, &rc);
    rc = CMAddArg (args_ptr,
        "EmptyInstance",
        (CMPIValue *) &value,
        CMPI_instance);
    value.ref = NULL;
    rc = CMAddArg (args_ptr,
        "EmptyRef",
        (CMPIValue *) &value,
        CMPI_ref);
    value.dateTime = NULL;
    rc = CMAddArg (args_ptr,
        "EmptyDatetime",
        (CMPIValue *) &value,
        CMPI_dateTime);
    rc = CMAddArg (args_ptr,
        "EmptyChars",
        (CMPIValue *) str,
        CMPI_chars);
    rc = CMAddArg (args_ptr,
        "EmptyCharsPtrA",
        NULL,
        CMPI_charsptrA);

    value.chars = NULL;
    rc = CMAddArg (args_ptr,
        "EmptyCharsPtr",
        &value,
        CMPI_charsptr);

    value.args = NULL;
    rc = CMAddArg (args_ptr,
        "EmptyArgs",
        (CMPIValue *) &value,
        CMPI_args);

    rc = CMRelease(args_ptr);
    return 1;
}


static int _testCMPIEnumeration (const CMPIContext* ctx)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    CMPIEnumeration *enum_ptr = NULL;
    CMPIData data;
    unsigned int initCount = 0;
    CMPIObjectPath* objPath = NULL;
    CMPIArray* arr_ptr = NULL;
    CMPICount returnedArraySize;
    void *eptr;
    objPath = make_ObjectPath(_broker, _Namespace, _PersonClass);
    enum_ptr = CBEnumInstances(_broker, ctx, objPath, NULL, &rc);
    if (enum_ptr == NULL)
    {
        return 1;
    }

    arr_ptr = CMToArray(enum_ptr, &rc);
    if (arr_ptr == NULL)
    {
        return 1;
    }

    returnedArraySize = CMGetArrayCount(arr_ptr, &rc);
    while (CMHasNext(enum_ptr, &rc))
    {
        data = CMGetNext(enum_ptr, &rc);
        if (data.type != CMPI_instance)
        {
            return 1;
        }
        initCount++;
    }

    eptr = enum_ptr->hdl;
    enum_ptr->hdl = NULL;

    CMToArray(enum_ptr, &rc);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    CMGetNext(enum_ptr, &rc);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    CMHasNext(enum_ptr, &rc);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }
    enum_ptr->hdl = eptr;
    rc = CMRelease (enum_ptr);
    if (rc.rc != CMPI_RC_OK)
    {
        return 1;
    }

    return 0;
}

static int _testCMPIArray ()
{
  CMPIStatus rc = { CMPI_RC_OK, NULL };
    CMPIArray *arr_ptr = NULL;
    CMPIArray *new_arr_ptr = NULL;
    CMPIData data[3];
    CMPIData clonedData[3];
    CMPIValue value;
    CMPIType initArrayType = CMPI_uint32;
    CMPIType initErrArrayType = CMPI_REAL;
    CMPIType returnedArrayType;
    CMPICount initArraySize = 3;
    CMPICount returnedArraySize;
    CMPIUint32 i;
    CMPIBoolean cloneSuccessful = 0;
    CMPIBoolean getDataSuccessful;
    void *aptr;
    arr_ptr = CMNewArray(_broker, initArraySize, initArrayType, &rc);
    if (arr_ptr == NULL)
    {
        return 1;
    }
    returnedArraySize = CMGetArrayCount(arr_ptr, &rc);
    returnedArrayType = CMGetArrayType(arr_ptr, &rc);

    value.uint32 = 10;
    rc = CMSetArrayElementAt(arr_ptr, 0, &value, initArrayType);
    value.uint32 = 20;
    rc = CMSetArrayElementAt(arr_ptr, 1, &value, initArrayType);
    value.uint32 = 30;
    rc = CMSetArrayElementAt(arr_ptr, 2, &value, initArrayType);

    i = 0;
    while (i < 3)
    {
        data[i] = CMGetArrayElementAt(arr_ptr, i, &rc);
        i++;
    }

    i = 0;
    getDataSuccessful = 1;
    while (i < 3)
    {
        if (data[i].value.uint32 != (i + 1) * 10)
        {
            getDataSuccessful = 0;
            break;
        }
        i++;
    }
    new_arr_ptr = arr_ptr->ft->clone(arr_ptr, &rc);
    i = 0;
    while (i < 3)
    {
        clonedData[i] = CMGetArrayElementAt(new_arr_ptr, i, &rc);
        i++;
    }

    cloneSuccessful = 1;
    for (i = 0; i < initArraySize; i++)
    {
        if (data[i].value.uint32 != clonedData[i].value.uint32)
        {
            cloneSuccessful = 0;
            break;
        }
    }
    rc = new_arr_ptr->ft->release(new_arr_ptr);
    aptr = arr_ptr->hdl;
    arr_ptr->hdl = NULL;

    returnedArraySize = CMGetArrayCount(arr_ptr, &rc);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    returnedArrayType = CMGetArrayType(arr_ptr, &rc);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    rc = CMSetArrayElementAt(arr_ptr, 2, &value, initArrayType);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    CMGetArrayElementAt(arr_ptr, 5, &rc);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }
    arr_ptr->ft->clone(arr_ptr, &rc);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    rc = arr_ptr->ft->release(arr_ptr);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    arr_ptr->hdl = aptr;

    CMGetArrayElementAt(arr_ptr, 5, &rc);
    if (rc.rc != CMPI_RC_ERR_NO_SUCH_PROPERTY)
    {
        return 1;
    }

    rc = CMSetArrayElementAt(arr_ptr, 2, &value, initErrArrayType);
    if (rc.rc != CMPI_RC_ERR_TYPE_MISMATCH)
    {
        return 1;
    }

    rc = arr_ptr->ft->release(arr_ptr);
    return 0;
}

static int _testCMPIcontext (const CMPIContext* ctx)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    CMPIValue value;
    CMPIData data;
    CMPIUint32 count = 0;
    CMPIUint32  count_for_new_context = 0;
    count = CMGetContextEntryCount(ctx, &rc);
    value.uint32 = 40;
    rc = CMAddContextEntry(ctx, "name1", &value, CMPI_uint32);
    value.real32 = (CMPIReal32)40.123;
    rc = CMAddContextEntry(ctx, "name2", &value, CMPI_real32);
    data = CMGetContextEntry(ctx, "name1", &rc);
    data = CMGetContextEntry(ctx, "name2", &rc);
    count_for_new_context = CMGetContextEntryCount(ctx, &rc);
    CMGetContextEntry(ctx, "noEntry", &rc);
    if (rc.rc != CMPI_RC_ERR_NO_SUCH_PROPERTY)
    {
        return 1;
    }

    return 0;
}

static int _testCMPIDateTime ()
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };

    CMPIBoolean isInterval = 0;
    CMPIBoolean interval = 0;
    CMPIBoolean cloneSuccessful = 0;
    CMPIBoolean binaryDateTimeEqual = 0;

    CMPIDateTime *dateTime = NULL;
    CMPIDateTime *new_dateTime = NULL;
    CMPIDateTime *clonedDateTime = NULL;
    CMPIDateTime *dateTimeFromBinary = NULL;

    CMPIUint64 dateTimeInBinary = UINT64_LITERAL(1150892800000000); 
    CMPIUint64 returnedDateTimeInBinary = 0;

    CMPIString* stringDate = NULL;
    CMPIString* clonedStringDate = NULL;

    const char *normalString = NULL;
    const char *clonedString = NULL;
    void *dtptr;

    dateTime = CMNewDateTime(_broker, &rc);
    if (dateTime == NULL)
    {
        return 1;
    }

    dateTimeFromBinary = CMNewDateTimeFromBinary(
        _broker, dateTimeInBinary, interval, &rc);
    returnedDateTimeInBinary = CMGetBinaryFormat(dateTimeFromBinary, &rc);
    if (dateTimeInBinary == returnedDateTimeInBinary)
    {
        binaryDateTimeEqual = 1;
    }
    isInterval = CMIsInterval(dateTime, &rc);
    interval = 1;
    new_dateTime = CMNewDateTimeFromBinary(
        _broker, dateTimeInBinary, interval,&rc);
    isInterval = CMIsInterval(new_dateTime, &rc);
    clonedDateTime = dateTime->ft->clone(dateTime, &rc);
    stringDate = CMGetStringFormat(dateTime, &rc);
    clonedStringDate = CMGetStringFormat(clonedDateTime, &rc);
    rc = clonedDateTime->ft->release(clonedDateTime);
    normalString = CMGetCharsPtr(stringDate, &rc);
    clonedString = CMGetCharsPtr(clonedStringDate, &rc);
    if (strcmp(normalString,clonedString) == 0)
    {
        cloneSuccessful = 1;
    }
    dtptr = dateTime->hdl;
    dateTime->hdl = NULL;

    CMGetBinaryFormat(dateTime, &rc);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    dateTime->ft->clone(dateTime, &rc);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    CMGetStringFormat(dateTime, &rc);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    rc = dateTime->ft->release(dateTime);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    dateTime->hdl = dtptr;
    rc = dateTime->ft->release(dateTime);
    return 0;
}

static int _testCMPIInstance ()
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };

    CMPIInstance* instance = NULL;
    CMPIInstance* clonedInstance = NULL;
    CMPIObjectPath* objPath = NULL;
    CMPIObjectPath* newObjPath = NULL;
    CMPIObjectPath* returnedObjPath = NULL;

    CMPIData returnedData1;
    CMPIData returnedData2;
    CMPIData clonedData1;

    CMPIString* returnedName = NULL;
    unsigned int count = 0;
    const char* name1 = "firstPropertyName";
    CMPIValue value1;
    const char* name2 = "secondPropertyName";
    CMPIValue value2;
    CMPIType type = CMPI_uint64;
    CMPIBoolean dataEqual = 0;
    CMPIBoolean objectPathEqual = 0;
    CMPIBoolean cloneSuccessful = 0;
    CMPIString* beforeObjPath = NULL;
    CMPIString* afterObjPath = NULL;
    const char* beforeString = NULL;
    const char* afterString = NULL;
    objPath = make_ObjectPath(_broker, _Namespace, _ClassName);
    instance = make_Instance(objPath);
    value1.uint32 = 10;
    rc = CMSetProperty(instance, name1, &value1, type);
    value2.uint32 = 20;
    rc = CMSetProperty(instance, name2, &value2, type);
    count = CMGetPropertyCount(instance, &rc);
    returnedData1 = CMGetProperty(instance, name1, &rc);
    if (returnedData1.value.uint32 == 10)
    {
        dataEqual = 1 ;
    }
    returnedData2 = CMGetPropertyAt(instance, 2, &returnedName, &rc);
    if (returnedData2.value.uint32 == 20)
    {
        dataEqual = 1 ;
    }
    newObjPath = make_ObjectPath(_broker, _Namespace, _ClassName);
    returnedObjPath = CMGetObjectPath(instance, &rc);
    beforeObjPath = CMObjectPathToString(returnedObjPath, &rc);
    beforeString = CMGetCharsPtr(beforeObjPath, &rc);
    rc = CMSetNameSpace(newObjPath, "newNamespace");
    rc = CMSetObjectPath(instance, newObjPath);
    returnedObjPath = CMGetObjectPath(instance, &rc);
    afterObjPath = CMObjectPathToString(returnedObjPath, &rc);
    afterString = CMGetCharsPtr(afterObjPath,&rc);
    afterString = CMGetCharsPtr(CMGetNameSpace(returnedObjPath, &rc), &rc);
    if (strcmp("newNamespace",afterString) == 0)
    {
        objectPathEqual = 1;
    }
    clonedInstance = instance->ft->clone(instance, &rc);
    clonedData1 = CMGetProperty(clonedInstance, name1, &rc);
    rc = clonedInstance->ft->release(clonedInstance);
    if (returnedData1.value.uint32 == clonedData1.value.uint32)
    {
        cloneSuccessful = 1;
    }
    else
    {
        cloneSuccessful = 0;
    }
    CMGetProperty(instance, "noProperty", &rc);
    if (rc.rc != CMPI_RC_ERR_NO_SUCH_PROPERTY)
    {
        return 1;
    }

    CMGetPropertyAt(instance, 100, &returnedName, &rc);
    if (rc.rc != CMPI_RC_ERR_NO_SUCH_PROPERTY)
    {
        return 1;
    }
    rc = instance->ft->release(instance);
    return 0;
}

static int _testCMPIObjectPath ()
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };

    CMPIObjectPath* objPath = NULL;
    CMPIObjectPath* clonedObjPath = NULL;
    CMPIObjectPath* otherObjPath = NULL;
    CMPIObjectPath *fakeObjPath  = NULL;

    const char* hostName = "HOSTNAME";
    const char* nameSpace = "root/dummy";
    const char* className = "classname";

    CMPIString* returnedHostname = NULL;
    CMPIBoolean equalHostname = 0;
    CMPIString* returnedNamespace = NULL;
    CMPIBoolean equalNamespace = 0;
    CMPIString* returnedClassName;
    CMPIBoolean equalClassName = 0;
    CMPIString* returnedObjectPath;
    CMPIBoolean cloneSuccessful = 0;
    CMPIBoolean getKeySuccessful = 0;
    CMPIBoolean getKeyCountSuccessful = 0;
    CMPIBoolean getKeyAtSuccessful = 0;
    CMPIBoolean getKeyAtErrorPathSuccessful = 0;
    const char* objectPath1 = NULL;
    const char* objectPath2 = NULL;
    CMPIData data;
    CMPIValue value;
    unsigned int keyCount = 0;
    void *opptr;
    objPath = make_ObjectPath(_broker, _Namespace, _ClassName);
    rc = CMSetHostname(objPath, hostName);
    returnedHostname = CMGetHostname(objPath, &rc);
    if (strcmp(hostName,CMGetCharsPtr(returnedHostname,&rc)) == 0)
    {
        equalHostname = 1;
    }
    rc = CMSetNameSpace(objPath, nameSpace);
    returnedNamespace = CMGetNameSpace(objPath, &rc);
    if (strcmp(nameSpace, CMGetCharsPtr(returnedNamespace, &rc)) == 0)
    {
        equalNamespace = 1;
    }
    rc = CMSetClassName(objPath, className);
    returnedClassName = CMGetClassName(objPath, &rc);
    if (strcmp(className,CMGetCharsPtr(returnedClassName, &rc)) == 0)
    {
        equalClassName = 1;
    }
    otherObjPath = make_ObjectPath(_broker, _Namespace, _ClassName);
    returnedNamespace = CMGetNameSpace(otherObjPath, &rc);
    rc = CMSetNameSpaceFromObjectPath(otherObjPath, objPath);
    returnedNamespace = CMGetNameSpace(otherObjPath, &rc);
    if (strcmp(nameSpace,CMGetCharsPtr(returnedNamespace, &rc)) == 0)
    {
        equalNamespace = 1;
    }
    returnedHostname = CMGetHostname(otherObjPath, &rc);
    rc = CMSetHostAndNameSpaceFromObjectPath(otherObjPath,objPath);
    returnedHostname = CMGetHostname(otherObjPath, &rc);
    if (strcmp(hostName,CMGetCharsPtr(returnedHostname,&rc)) == 0)
    {
        equalHostname = 1;
    }
    returnedObjectPath = CMObjectPathToString(objPath, &rc);
    objectPath1 = CMGetCharsPtr(returnedObjectPath, &rc);
    clonedObjPath = objPath->ft->clone(objPath, &rc);
    returnedObjectPath = CMObjectPathToString(clonedObjPath, &rc);
    rc = clonedObjPath->ft->release(clonedObjPath);
    objectPath2 = CMGetCharsPtr(returnedObjectPath, &rc);
    if (strcmp(objectPath1,objectPath2) == 0)
    {
        cloneSuccessful = 1;
    }
    else
    {
        cloneSuccessful = 0;
    }
    fakeObjPath = CMNewObjectPath (_broker, "root#cimv2",
        "Sample_Instance", &rc);
    rc = CMAddKey (fakeObjPath, "ElementName",
        (CMPIValue *) "Fake", CMPI_chars);
    rc = CMAddKey (otherObjPath, "ElementName1",
        (CMPIValue *) "otherObjPath", CMPI_chars);
    data = CMGetKey(fakeObjPath, "ElementName", &rc);
    if (strcmp(CMGetCharsPtr(data.value.string, &rc),"Fake") == 0)
    {
        getKeySuccessful = 1;
    }
    keyCount = CMGetKeyCount(fakeObjPath, &rc);
    if (keyCount == 1)
    {
        getKeyCountSuccessful = 1;
    }
    data = CMGetKeyAt(fakeObjPath, 0, NULL, &rc);
    if (rc.rc == 0)
    {
        getKeyAtSuccessful = 1;
    }
    value.uint16 = 67;
    rc = CMAddKey (fakeObjPath, "Numeric_key_unsigned",
        (CMPIValue *) &value, CMPI_uint16);
    data = CMGetKey(fakeObjPath, "Numeric_key_unsigned", &rc);
    value.sint16 = -67;
    rc = CMAddKey (fakeObjPath, "Numeric_key_signed",
        (CMPIValue *) &value, CMPI_sint16);
    data = CMGetKey(fakeObjPath, "Numeric_key_signed", &rc);
    value.boolean = 1;
    rc = CMAddKey (fakeObjPath, "Boolean_key",
        (CMPIValue *) &value, CMPI_boolean);
    data = CMGetKey(fakeObjPath, "Boolean_key", &rc);
    CMGetKeyAt(objPath, 500, NULL, &rc);
    if (rc.rc != CMPI_RC_ERR_NO_SUCH_PROPERTY)
    {
        return 1;
    }
    rc = objPath->ft->release(objPath);
    rc = fakeObjPath->ft->release(fakeObjPath);
    return 0;
}

static int _testCMPIResult (const CMPIResult *rslt)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };

    CMPIValue value;
    CMPIType type;
    const CMPIObjectPath* objPath = NULL;
    CMPIBoolean returnDataSuccessful = 0;
    value.uint32 = 10;
    type = CMPI_uint32;
    rc = CMReturnData(rslt, &value, type);
    if (rc.rc == CMPI_RC_OK)
    {
        returnDataSuccessful = 1;
    }
    objPath = make_ObjectPath(_broker, _Namespace, _ClassName);
    rc = CMReturnObjectPath(rslt, objPath);
    rc = CMReturnDone(rslt);
    rc = CMReturnData(rslt, NULL, type);
    if (rc.rc != CMPI_RC_ERR_INVALID_PARAMETER)
    {
        return 1;
    }

    return 0;
}

static int _testCMPIString()
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };

    CMPIString* string = NULL;
    CMPIString* clonedString = NULL;
    const char* actual_string = NULL;
    const char* cloned_string = NULL;
    const char *data = "dataString";
    CMPIBoolean cloneSuccessful = 0;
    void *string_ptr;
    string = CMNewString(_broker, data, &rc);
    actual_string = CMGetCharsPtr(string, &rc);
    clonedString = string->ft->clone(string, &rc);
    cloned_string = CMGetCharsPtr(clonedString, &rc);
    if (strcmp(actual_string,cloned_string) == 0)
    {
        cloneSuccessful = 1;
    }
    rc = clonedString->ft->release(clonedString);
    string_ptr = string->hdl;
    string->hdl = NULL;

    string->ft->clone(string, &rc);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    rc = string->ft->release(string);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    CMGetCharsPtr(string, &rc);
    if (rc.rc != CMPI_RC_ERR_INVALID_HANDLE)
    {
        return 1;
    }

    string->hdl = string_ptr;
    rc = string->ft->release(string);
    return 0;
}

static int _testCMPIArgs()
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };

    CMPIArgs* args = NULL;
    CMPIArgs* clonedArgs = NULL;
    CMPIUint32 count = 0;
    CMPIType type = CMPI_uint32;
    char *arg1 = "arg1";
    char *arg2 = "arg2";
    CMPIValue value;
    CMPIData data;
    CMPIData clonedData;
    CMPIBoolean cloneSuccessful = 0;
    CMPIBoolean getArgCountSuccessful = 0;
    void *args_ptr;
    args = CMNewArgs(_broker, &rc);
    value.uint32 = 10;
    rc = CMAddArg(args, arg1, &value, type);
    count = CMGetArgCount(args, &rc);
    if (count == 1)
    {
        getArgCountSuccessful = 1;
    }
    value.uint32 = 20;
    rc = CMAddArg(args, arg2, &value, type);
    count = CMGetArgCount(args, &rc);
    if (count == 2)
    {
        getArgCountSuccessful = 1;
    }
    data = CMGetArg(args, arg2, &rc);
    rc = CMAddArg(args, arg1, &value, type);
    clonedArgs = args->ft->clone(args, &rc);
    clonedData = CMGetArg(clonedArgs, arg2, &rc);
    rc = clonedArgs->ft->release(clonedArgs);
    if (data.value.uint32 == clonedData.value.uint32)
    {
        cloneSuccessful = 1;
    }
    rc = args->ft->release(args);
    return 0;
}

/* ---------------------------------------------------------------------------*/
/*                        Method Provider Interface                           */
/* ---------------------------------------------------------------------------*/
CMPIStatus
TestMiscProviderMethodCleanup (CMPIMethodMI * mi, CMPIContext * ctx)
{
  CMReturn (CMPI_RC_OK);
}

CMPIStatus
TestMiscProviderInvokeMethod (CMPIMethodMI * mi,
                                    CMPIContext * ctx,
                                    CMPIResult * rslt,
                                    CMPIObjectPath * ref,
                                    char *methodName,
                                    CMPIArgs * in, CMPIArgs * out)
{
  CMPIString *class = NULL;
  CMPIStatus rc = { CMPI_RC_OK, NULL };
  CMPIData data;
  CMPIString *argName = NULL;
  CMPIInstance *instance = NULL;
  CMPIInstance *paramInst = NULL;
  unsigned int arg_cnt = 0, index = 0;
  CMPIValue value;
  char *result = NULL;

  if (strncmp ("testReturn", methodName, strlen ("testReturn")) == 0)
  {
      value.uint32 = 2;
      CMReturnData (rslt, &value, CMPI_uint32);
      CMReturnDone (rslt);
      return rc;
  }

  class = CMGetClassName (ref, &rc);
  if (strncmp(
      CMGetCharsPtr (class,NULL),
      _ClassName,
      strlen (_ClassName)) == 0)
    {
      if (strncmp ("TestCMPIBroker", methodName, strlen ("TestCMPIBroker")) ==
          0)
        {
          // Parse the CMPIArgs in to figure out which operation it is.
          // There are six of them:
          //   ValueMap { "1", "2", "3"},
          //   Values {"CMGetMessage","CMLogMessage","CDTraceMessage"}]
          //    uint32 Operation,
          //    [OUT]string Result);
          data = CMGetArg (in, "Operation", &rc);

          if (data.type == CMPI_uint32)
            {
              switch (data.value.uint32)
                {
                case 1:
                  value.uint32 = _CMGetMessage (&result);
                  break;
                case 2:
                  value.uint32 = _CMLogMessage (&result);
                  break;
                case 3:
                  value.uint32 = _CMTraceMessage (&result);
                  break;
                case 4:
                    value.uint32 = _testCMPIEnumeration (ctx);
                    result=strdup("_testCMPIEnumeration ");
                    break;
                case 5:
                    value.uint32 = _testCMPIArray ();
                    result=strdup("_testCMPIArray ");
                    break;
                case 6:
                    value.uint32 = _testCMPIcontext (ctx);
                    result=strdup("_testCMPIContext ");
                    break;
                case 7:
                    value.uint32 = _testCMPIDateTime ();
		    result=strdup("_testCMPIDateTime ");
                    break;
                case 8:
                    value.uint32 = _testCMPIInstance ();
		    result=strdup("_testCMPIInstance ");
                    break;
                case 9:
                    value.uint32 = _testCMPIObjectPath ();
		    result=strdup("_testCMPIObjectPath ");
                    break;
                case 10:
                    value.uint32 = _testCMPIResult (rslt);
		    result=strdup("_testCMPIResult ");
                    break;
                case 11:
                    value.uint32 = _testCMPIString ();
		    result=strdup("_testCMPIString ");
                    break;
                case 12:
                    value.uint32 = _testCMPIArgs ();
		    result=strdup("_testCMPIArgs ");
                    break;
                default:
                  break;
                }
              CMReturnData (rslt, &value, CMPI_uint32);
              CMReturnDone (rslt);
              rc = CMAddArg (out, "Result", (CMPIValue *) result, CMPI_chars);
              free (result);
            }
          else                 
            {
              value.uint32 = 1;
              CMReturnData (rslt, &value, CMPI_uint32);
              CMReturnDone (rslt);
            }
        }

      else if (strncmp ("returnString", methodName, strlen ("returnString"))
               == 0)
        {
          result = strdup ("Returning string");
          CMReturnData (rslt, (CMPIValue *) result, CMPI_chars);
          CMReturnDone (rslt);
          free(result);
        }
      else if (strncmp ("returnUint32", methodName, strlen ("returnUint32"))
               == 0)
        {
          value.uint32 = 42;

          CMReturnData (rslt, &value, CMPI_uint32);
          CMReturnDone (rslt);
        }
      else if (
          strncmp("returnDateTime", methodName, strlen("returnDateTime")) == 0)
        {
          CMPIUint64 ret_val = 0;
          CMPIStatus dateTimeRc={CMPI_RC_OK, NULL};

          CMPIDateTime *dateTime = CMNewDateTime(_broker, &dateTimeRc);
          // Checking the date.
          ret_val = CMGetBinaryFormat (dateTime, &dateTimeRc);
          if (ret_val == 0) {} 

          CMReturnData (rslt, (CMPIValue *) & dateTime, CMPI_dateTime);
          CMReturnDone (rslt);
        }
    else if (
        strncmp("testArrayTypes", methodName, strlen ("testArrayTypes"))== 0)
    {
        value.uint32 = _testArrayTypes();
        CMReturnData (rslt, &value, CMPI_uint32);
        CMReturnDone (rslt);
    }
    else if (
        strncmp("testErrorPaths", methodName, strlen ("testErrorPaths")) == 0)
    {
        value.uint32 = _testErrorPaths();
        CMReturnData (rslt, &value, CMPI_uint32);
        CMReturnDone (rslt);
    }
    else if (
        strncmp("testSimpleTypes", methodName, strlen ("testSimpleTypes")) == 0)
    {
        value.uint32 = _testSimpleTypes();
        CMReturnData (rslt, &value, CMPI_uint32);
        CMReturnDone (rslt);
    }
    else if (strncmp ("methodNotInMof", methodName, strlen ("methodNotInMof"))
        == 0)
    {
        value.uint32 = 42;
        CMReturnData (rslt, &value, CMPI_uint32);
        CMReturnDone (rslt);
    }
      else
        {
          CMSetStatusWithChars (_broker, &rc,
                                CMPI_RC_ERR_NOT_FOUND, methodName);
        }
    }
  return rc;
}

/* ---------------------------------------------------------------------------*/
/*                              Provider Factory                              */
/* ---------------------------------------------------------------------------*/


CMMethodMIStub (TestMiscProvider,
                TestMiscProvider, _broker, CMNoHook)

/* ---------------------------------------------------------------------------*/
/*             end of TestCMPIProvider                      */
/* ---------------------------------------------------------------------------*/
