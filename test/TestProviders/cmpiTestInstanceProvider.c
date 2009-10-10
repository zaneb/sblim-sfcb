#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"
#include <string.h>

static const CMPIBroker *_broker;
static CMPIBoolean valid[10];
static CMPICount numOfInst;
static CMPIArray* clone_arr_ptr;
static CMPICount initArraySize = 10;

#define _ClassName "Sample_Instance"

static void initialize()
{
    CMPIStatus rc = { CMPI_RC_OK, NULL};
    CMPIArray *arr_ptr;
    CMPIValue value1, value2, value_inst1, value_inst2;
    CMPIInstance *instance1, *instance2, *instance3;
    CMPIObjectPath *cop1, *cop2, *cop3;
    value1.uint8 = 1;
    value2.string = CMNewString(_broker, "Hello World", &rc);

    /* create a new array to hold the instances created */
    arr_ptr = CMNewArray(_broker, initArraySize, CMPI_instance, &rc);

    /* create object path for instance1 */
    cop1 = CMNewObjectPath(
                _broker,
                "root/cimv2",
                _ClassName,
                &rc);
    /* add key (with its value) to object path to recognize instance uniquely */
    CMAddKey(cop1, "Identifier", (CMPIValue *)&value1, CMPI_uint8);
    /* if created object path is not null then create new instance */
    if(!CMIsNullObject(cop1))
    {

        instance1 = CMNewInstance(_broker,
                        cop1,
                        &rc);
        /* set the properties for newly created instance */
        CMSetProperty(instance1, "Identifier",&value1, CMPI_uint8);
        CMSetProperty(instance1, "Message", &value2, CMPI_string);
        value_inst1.inst = instance1;

        /* assign the created instance to array created */
        rc = CMSetArrayElementAt(
            arr_ptr,
            numOfInst,
            &value_inst1,
            CMPI_instance);
        /* set the validity of instance to be true.
        Validity gets false if instance gets deleted using deleteInstance(). */
        valid[numOfInst] = 1;
        numOfInst++;
    }

    /* create instance 2 and add to array */
    value1.uint8 = 2;
    value2.string = CMNewString(_broker, "Yo Planet", &rc);
    cop2 = CMNewObjectPath(
                _broker,
                "root/cimv2",
                _ClassName,
                &rc);
    CMAddKey(cop2, "Identifier", (CMPIValue *)&value1, CMPI_uint8);
    if(!CMIsNullObject(cop2))
    {

        instance2 = CMNewInstance(_broker,
                        cop2,
                        &rc);
         CMSetProperty(instance2, "Identifier", &value1, CMPI_uint8);
         CMSetProperty(instance2, "Message", &value2, CMPI_string);
         value_inst2.inst = instance2;
         rc = CMSetArrayElementAt(arr_ptr,
            numOfInst,
            &value_inst2,
            CMPI_instance);
         valid[numOfInst] = 1;
         numOfInst++;
    }

    /* create instance 3 and add to array */
    value1.uint8 = 3;
    value2.string = CMNewString(_broker, "Hey Earth", &rc);
    cop3 = CMNewObjectPath(
                _broker,
                "root/cimv2",
                _ClassName,
                &rc);
    CMAddKey(cop3, "Identifier", (CMPIValue *)&value1, CMPI_uint8);
    if(!CMIsNullObject(cop3))
    {

        instance3 = CMNewInstance(_broker,
                        cop3,
                        &rc);
         CMSetProperty(instance3, "Identifier", &value1, CMPI_uint8);
         CMSetProperty(instance3, "Message", &value2, CMPI_string);
         value_inst2.inst = instance3;
         rc = CMSetArrayElementAt(arr_ptr,
            numOfInst,
            &value_inst2,
            CMPI_instance);
         valid[numOfInst] = 1;
         numOfInst++;
    }
    /* clone the array which contains instances. */
    clone_arr_ptr = arr_ptr->ft->clone(arr_ptr, &rc);
}

CMPIStatus TestInstanceProviderCleanup (
    CMPIInstanceMI * mi,
    const CMPIContext * ctx,
    CMPIBoolean  term)
{
    CMRelease(clone_arr_ptr);
    CMReturn (CMPI_RC_OK);
}

/**
    Enumerate ObjectPaths of Instances serviced by this provider.
*/
CMPIStatus TestInstanceProviderEnumInstanceNames (
    CMPIInstanceMI * mi,
    const CMPIContext * ctx,
    const CMPIResult * rslt,
    const CMPIObjectPath * ref)
{
    CMPIStatus rc = {CMPI_RC_OK, NULL};
    unsigned int j = 0;
    CMPIObjectPath *local;
    CMPIData data;
    /* get the element from array containing created instances. */
    for (j = 0; j < numOfInst; j++)
    {
        /*check for validity of Instance, that its not deleted */
        if(valid[j] == 1)
        {
            /* get element(instance) from array */
            data = CMGetArrayElementAt(clone_arr_ptr, j, &rc);
            /* get object-path of that instance */
            local = CMGetObjectPath(data.value.inst, &rc);
            /* return object-path */
            CMReturnObjectPath(rslt,local);
        }
    }
    CMReturnDone (rslt);
    CMReturn (CMPI_RC_OK);
}

/**
    Enumerate the Instances serviced by this provider.
*/
CMPIStatus TestInstanceProviderEnumInstances (
    CMPIInstanceMI * mi,
    const CMPIContext * ctx,
    const CMPIResult * rslt,
    const CMPIObjectPath * op,
    const char **properties)
{
    CMPIStatus rc = {CMPI_RC_OK, NULL};
    unsigned int j = 0;
    CMPIData data;
    /* get the element from array containing created instances */
    for (j = 0; j < numOfInst; j++)
    {
        /* check for validity of Instance, that its not deleted */
        if(valid[j] == 1)
        {
            /* get element(instance) from array */
            data = CMGetArrayElementAt(clone_arr_ptr, j, &rc);
            /* return the instance */
            CMReturnInstance(rslt, data.value.inst);
        }
    }
    CMReturnDone (rslt);
    CMReturn (CMPI_RC_OK);
}

/**
    Get the Instances defined by object-path op.
*/
CMPIStatus TestInstanceProviderGetInstance (
    CMPIInstanceMI * mi,
    const CMPIContext * ctx,
    const CMPIResult * rslt,
    const CMPIObjectPath * op,
    const char **properties)
{
    CMPIData data,key1,key2;
    CMPIObjectPath *local;
    unsigned int j = 0;
    CMPIBoolean flag = 0;
    CMPIStatus rc = {CMPI_RC_OK, NULL};
    /* get the kay from object-path */
    key1 = CMGetKey(op, "Identifier", &rc);
    /* get the element from array containing created instances */
    for(j = 0; j < numOfInst; j++)
    {
        /* check for validity of Instance, that its not deleted */
        if (valid[j] == 1)
        {
            /* get element(instance) from array */
            data = CMGetArrayElementAt(clone_arr_ptr, j, &rc);

            /* get object-path of instance */
            local = CMGetObjectPath(data.value.inst, &rc);

            /* get key from this object-path */
            key2 = CMGetKey(local, "Identifier", &rc);

            /* compare key values.
               If they match return instance */
            if (key1.value.uint8 == key2.value.uint8)
            {
                CMReturnInstance(rslt, data.value.inst);
                flag =1;
            }
        }
    }
    /* key values did not match so throw exception */
    if(!flag)
    {
        CMReturn (CMPI_RC_ERR_NOT_FOUND);
    }
    CMReturnDone (rslt);
    CMReturn (CMPI_RC_OK);
}

/**
    Create Instance from inst, using object-path op as reference.
*/
CMPIStatus TestInstanceProviderCreateInstance (
    CMPIInstanceMI * mi,
    const CMPIContext * ctx,
    const CMPIResult * rslt,
    const CMPIObjectPath * cop,
    const CMPIInstance * ci)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    CMPIInstance * inst;
    CMPIValue value_inst;
    CMPIData key1,key2, retInst;
    CMPIObjectPath *obp;
    unsigned int j = 0;
    if(ci)
    {
        /* clone the instance to be added to the array */
        inst = CMClone(ci, &rc);
        key1 = CMGetProperty(inst, "Identifier", &rc);
        for (j=0; j < numOfInst ; j++)
        {
            /* check for validity of Instance, that its not deleted */
            if (valid[j] == 1)
            {
                /* get element(instance) from array */
                retInst = CMGetArrayElementAt(clone_arr_ptr, j, &rc);
                /* get object-path of instance */
                obp = CMGetObjectPath(retInst.value.inst, &rc);
                /* get key from this object-path */
                key2 = CMGetKey(obp, "Identifier", &rc);
                /*compare key values.
                  If they match throw exception as two instance with same key
                  properties cannot exists. */
                if(key1.value.uint8 == key2.value.uint8)
                {
                    CMReturn (CMPI_RC_ERR_ALREADY_EXISTS);
                }
            }
        }
        value_inst.inst = inst;
        /* If instance doesnot exists in array add it */
        rc = CMSetArrayElementAt(
            clone_arr_ptr,
            numOfInst,
            &value_inst,
            CMPI_instance);
        valid[numOfInst]=1;
        numOfInst++;
        /* return object-path of instance */
        CMReturnObjectPath(rslt, cop);
        CMReturnDone(rslt);
    }
    else
    {
        CMReturn (CMPI_RC_ERR_NOT_SUPPORTED);
    }
    CMReturn (CMPI_RC_OK);
}

/**
    Replace an existing Instance from inst, using object-path op as reference.
*/
CMPIStatus TestInstanceProviderModifyInstance  (
    CMPIInstanceMI * mi,
    const CMPIContext * ctx,
    const CMPIResult * rslt,
    const CMPIObjectPath * cop,
    const CMPIInstance * ci,
    const char **properties)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    CMPIInstance * inst;
    CMPIValue val1, val2;
    CMPIData key1, key2, retData1, retInst;
    CMPIObjectPath *obp;
    unsigned int j = 0, flag = 0;
    if(ci)
    {
        inst = CMClone(ci, &rc);
        /* get key from the object-path */
        key1 = CMGetKey(cop, "Identifier", &rc);
        val1.uint8 = key1.value.uint8;
        /* get the value of Message property */
        retData1 = CMGetProperty(inst, "Message", &rc);
        val2.string = retData1.value.string;
        for (j=0; j < numOfInst; j++)
        {
            /* check for validity of Instance, that its not deleted */
            if (valid[j] == 1)
            {
                /* get element(instance) from array */
                retInst = CMGetArrayElementAt(clone_arr_ptr, j, &rc);
                /* get object-path of instance */
                obp = CMGetObjectPath(retInst.value.inst, &rc);
                /* get key from this object-path */
                key2 = CMGetKey(obp, "Identifier", &rc);
                /*compare key values.
                  If they match then set the properties received from client */
                if(key1.value.uint8 == key2.value.uint8)
                {
                    CMSetProperty(
                        retInst.value.inst,
                        "Message",
                        &val2,
                        CMPI_string);
                    flag = 1;
                }
            }
        }
        CMRelease(inst);
        /*If match fails, throw exception, as instance to be mmodified is not
          found */
        if(!flag)
        {
            CMReturn (CMPI_RC_ERR_NOT_FOUND);
        }
    }
    CMReturnDone (rslt);
    CMReturn (CMPI_RC_OK);
}

CMPIStatus TestInstanceProviderDeleteInstance (
    CMPIInstanceMI * mi,
    const CMPIContext * ctx,
    const CMPIResult * result,
    const CMPIObjectPath * cop)
{

    CMPIStatus rc = { CMPI_RC_OK, NULL };
    CMPIData key1, key2, retInst;
    CMPIObjectPath *obp;
    unsigned int j = 0, flag = 0;
    /* get key from the object-path */
    key1 = CMGetKey(cop, "Identifier", &rc);
    for (j=0; j < numOfInst ; j++)
    {
        /*check for validity of Instance, that its not deleted */
        if(valid[j] == 1)
        {
            /*get element(instance) from array */
            retInst = CMGetArrayElementAt(clone_arr_ptr, j, &rc);

            /*get object-path of instance */
            obp = CMGetObjectPath(retInst.value.inst, &rc);

            /*get key from this object-path */
            key2 = CMGetKey(obp, "Identifier" , &rc);

            /*compare key values.
              If they match, release the object(instance)
              Also set its validity to zero, marking it as deleted */
            if (key1.value.uint8 == key2.value.uint8)
            {
                if(retInst.value.inst)
                {
                    flag =1;
                    CMRelease(retInst.value.inst);
                    CMSetArrayElementAt(
                        clone_arr_ptr,
                        j,
                        &retInst.value.inst,
                        CMPI_null);
                    valid[j] = 0;
                }
            }
        }
    }
    if(!flag)
    {
        CMReturn (CMPI_RC_ERR_NOT_FOUND);
    }
    CMReturn (CMPI_RC_OK);
}

CMPIStatus TestInstanceProviderExecQuery (
    CMPIInstanceMI * mi,
    const CMPIContext * ctx,
    const CMPIResult * rslt,
    const CMPIObjectPath * referencePath,
    const char *query,
    const char *lang)
{
    CMPIStatus rc = { CMPI_RC_OK, NULL };
    CMPIStatus rc_Eval = { CMPI_RC_OK, NULL };
    CMPIStatus rc_Clone = { CMPI_RC_OK, NULL };
    CMPIStatus rc_Array = { CMPI_RC_OK, NULL };
    CMPISelectExp *se_def = NULL;
    CMPICount cnt = 0;
    CMPIArray *projection = NULL;
    unsigned int j = 0;
    CMPIBoolean evalRes;
    CMPIData arr_data;
    CMPIInstance *instance1;
    CMPIObjectPath *cop1;
    CMPIData data;
    CMPIValue value1;
    const char* prop_name;
    CMPIData retProp;

    /*create the select expression */
    se_def = CMNewSelectExp (_broker, query, lang, &projection, &rc_Clone);
    if (se_def)
    {
        /*loop over instances in array to evaluate for requested properties */
        for (j = 0; j < numOfInst ; j++)
        {
            /*check for validity of Instance,that its not deleted */
            if(valid[j] == 1)
            {
                /*get the element from array */
                arr_data = CMGetArrayElementAt(
                    clone_arr_ptr,
                    j,
                    &rc);
                /*Evaluate the instance using this select expression */
               evalRes = CMEvaluateSelExp(se_def,arr_data.value.inst, &rc_Eval);
                if (evalRes)
                {
                    /*check if any of properties are requested */
                    if (projection)
                    {
                        /*get number of properties requested */
                        cnt = CMGetArrayCount (projection, &rc_Array);
                        /*if count is not equal to number of properties of
                          instance */
                        if(cnt == 1)
                        {
                            /*check for the properties, requested */
                            data = CMGetArrayElementAt(
                                projection,
                                0,
                                &rc_Array);
                            prop_name = CMGetCharsPtr (
                                data.value.string,
                                &rc);
                            /*create the new instance that has to be returned */
                            cop1 = CMNewObjectPath(
                                _broker,
                                "root/cimv2",
                                _ClassName,
                                &rc);

                            instance1 = CMNewInstance(_broker,
                                cop1,
                                &rc);

                            /*if property name is "Identifier",
                               gets its value from instance */
                            if(!strcmp(prop_name, "Identifier"))
                            {
                                retProp = CMGetProperty(
                                    arr_data.value.inst,
                                        "Identifier",
                                        &rc);
                                value1.uint8 = retProp.value.uint8;
                                CMSetProperty(
                                    instance1,
                                    "Identifier",
                                    (CMPIValue *)&value1,
                                    CMPI_uint8);
                            }
                            /*if property name is "Message",
                              gets its value from instance */
                            if(!strcmp(prop_name, "Message"))
                            {
                                retProp = CMGetProperty(
                                    arr_data.value.inst,
                                    "Message",
                                    &rc);
                                    value1.string = retProp.value.string;
                                    CMSetProperty(
                                        instance1,
                                        "Message",
                                        (CMPIValue *)&value1,
                                        CMPI_string);
                            }
                            /*if the query is evaluated return instance */
                            CMReturnInstance (rslt, instance1);
                        }
                    }
                    else
                    {
                        CMReturnInstance(rslt, arr_data.value.inst);
                    }
                }
            }
        }
    }

    CMReturnDone (rslt);
    CMReturn (CMPI_RC_OK);
}


/* ---------------------------------------------------------------------------*/
/*                              Provider Factory                              */
/* ---------------------------------------------------------------------------*/


CMInstanceMIStub(
    TestInstanceProvider,
    TestInstanceProvider,
    _broker,
    initialize())

/* ---------------------------------------------------------------------------*/
/*             end of SampleCMPIProvider                      */
/* ---------------------------------------------------------------------------*/

