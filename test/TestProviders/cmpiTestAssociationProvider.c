#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

#define _ClassName "CMPI_TEST_Racing"
#define _ClassName_size strlen(_ClassName)

#define _Namespace "root/cimv2"

static const CMPIBroker *_broker;

/* ---------------------------------------------------------------------------*/
/*                       CMPI Helper function                                 */
/* ---------------------------------------------------------------------------*/

const char * get_assoc_targetClass_Name(
    const CMPIBroker * broker,
    const CMPIObjectPath * ref,
    const char * _RefLeftClass,
    const char * _RefRightClass,
    CMPIStatus * rc)
{
    CMPIString * sourceClass = NULL;
    CMPIObjectPath * op = NULL;

    /* get name of source class */
    sourceClass = CMGetClassName(ref, rc);

    op = CMNewObjectPath(
        broker,
        CMGetCharsPtr(CMGetNameSpace(ref,rc), NULL),
        _RefLeftClass,
        rc );

    if (strcmp(CMGetCharsPtr(sourceClass, NULL),"CMPI_TEST_Person") == 0 )
    {
        return "CMPI_TEST_Vehicle";
    }
    else if ( strcmp(
        CMGetCharsPtr(sourceClass, NULL),
        "CMPI_TEST_Vehicle") == 0 )
    {
        return "CMPI_TEST_Person";
    }
    else
    {
        return NULL;
    }
}


CMPIObjectPath* get_assoc_targetClass_ObjectPath(
    const CMPIBroker* broker,
    const CMPIObjectPath* ref,
    const char* _RefLeftClass,
    const char* _RefRightClass,
    CMPIStatus* rc )
{
    CMPIObjectPath* op = NULL;
    const char* targetName = NULL;

    /* get name of the target class */
    targetName = get_assoc_targetClass_Name(
        broker,
        ref,
        _RefLeftClass,
        _RefRightClass,
        rc);

    if ( targetName != NULL )
    {
        /* create new object path of the target class */
        op = CMNewObjectPath(
            broker,
            CMGetCharsPtr(CMGetNameSpace(ref,rc), NULL),
            targetName,
            rc);
    }
    return op;
}

/* ---------------------------------------------------------------------------*/
/*                      Association Provider Interface                        */
/* ---------------------------------------------------------------------------*/

CMPIStatus TestAssociationProviderAssociationCleanup(
    CMPIAssociationMI* mi,
    const CMPIContext* ctx,
    CMPIBoolean  term)
{

    CMReturn (CMPI_RC_OK);
}

CMPIStatus TestAssociationProviderAssociators(
    CMPIAssociationMI* mi,
    const CMPIContext* ctx,
    const CMPIResult* rslt,
    const CMPIObjectPath* ref,
    const char* _RefLeftClass,
    const char* _RefRightClass,
    const char* role,
    const char* resultRole,
    const char** properties)
{
    CMPIObjectPath * op = NULL;
    CMPIEnumeration * en = NULL;
    CMPIData data ;
    CMPIStatus rc = { CMPI_RC_OK, NULL };

    CMPIString * sourceClass = NULL;

    sourceClass = CMGetClassName(ref,&rc);

    /* get object path of the target class */
    op = get_assoc_targetClass_ObjectPath(
        _broker,
        ref,
        _RefLeftClass,
        _RefRightClass,
        &rc);

    sourceClass = CMGetClassName(op,&rc);

    /* Call to Associators */
    /* upcall to CIMOM; call enumInstances() of the target class */
    en = CBEnumInstances( _broker, ctx, op, NULL, &rc);

    /* as long as instance entries are found in the enumeration */
    while( CMHasNext( en, &rc) )
    {
        /* get the instance */
        data = CMGetNext( en, &rc);

        /* and return the target class instance as result of the
         * associators() call
        */
        CMReturnInstance( rslt, data.value.inst );
    }
    return rc;
}

CMPIStatus TestAssociationProviderAssociatorNames(
    CMPIAssociationMI* mi,
    const CMPIContext* ctx,
    const CMPIResult* rslt,
    const CMPIObjectPath* ref,
    const char* _RefLeftClass,
    const char* _RefRightClass,
    const char* role,
    const char* resultRole)
{
    CMPIObjectPath* op = NULL;
    CMPIObjectPath* rop = NULL;
    CMPIEnumeration* en = NULL;
    CMPIData data ;

    CMPIStatus rc = { CMPI_RC_OK, NULL };

    /* get object path of the target class */
    op = get_assoc_targetClass_ObjectPath(
        _broker,
        ref,
        _RefLeftClass,
        _RefRightClass,
        &rc);

    /* create new object path of association */
    rop = CMNewObjectPath(
        _broker,
        CMGetCharsPtr(CMGetNameSpace(ref,&rc),NULL),
        _ClassName,
        &rc );

    /* upcall to CIMOM; call enumInstanceNames() of the target class */
    en = CBEnumInstanceNames( _broker, ctx, op, &rc);

    /* as long as object path entries are found in the enumeration */
    while( CMHasNext( en, &rc) )
    {
        /* get the object path */
        data = CMGetNext(en, &rc);

        /* and return the target class object path as result of the
         * associatorNames() call
        */
        CMReturnObjectPath( rslt, data.value.ref );
    }
    return rc;
}

CMPIStatus TestAssociationProviderReferences(
    CMPIAssociationMI * mi,
    const CMPIContext * ctx,
    const CMPIResult * rslt,
    const CMPIObjectPath * ref,
    const char *resultClass,
    const char *role ,
    const char** properties)
{
    CMPIInstance * ci = NULL;
    CMPIObjectPath * op = NULL;
    CMPIObjectPath * rop = NULL;
    CMPIEnumeration * en = NULL;
    CMPIData data ;

    const char * targetName = NULL;
    char * _thisClassName;
    char * _RefLeftClass = NULL;
    char * _RefRightClass = NULL;

    CMPIStatus rc = {CMPI_RC_OK, NULL};

    _thisClassName=_ClassName;

    /* get object path of the target class */
    op = get_assoc_targetClass_ObjectPath(
        _broker,
        ref,
        _RefLeftClass,
        _RefRightClass,
        &rc);


    /* create new object path of association */
    rop = CMNewObjectPath(
        _broker,
        CMGetCharsPtr(CMGetNameSpace(ref,&rc),NULL),
        _thisClassName,
        &rc );

    /* upcall to CIMOM; call enumInstanceNames() of the target class */
    en = CBEnumInstanceNames( _broker, ctx, op, &rc);

    /* as long as object path entries are found in the enumeration */
    while( CMHasNext( en, &rc) )
    {
        /* get the object path */
        data = CMGetNext( en, &rc);
        {
            /* create new instance of the association */
            ci = CMNewInstance( _broker, rop, &rc);

            /* get name of the target class */
            targetName = get_assoc_targetClass_Name(
                _broker,
                ref,
                _RefLeftClass,
                _RefRightClass,
                &rc);

            /* set the properties of the association instance depending on the
             * constellation of the source class (parameter ref) and the target
             * class (see targetName)
            */
            if(strcmp (targetName, "CMPI_TEST_Person")== 0)
            {
                CMSetProperty(
                    ci,
                    "model",
                    (CMPIValue*)&(data.value.ref),
                    CMPI_ref );
                CMSetProperty( ci, "driver", (CMPIValue*)&(ref), CMPI_ref );
            }
            else if( strcmp( targetName,"CMPI_TEST_Vehicle") == 0 )
            {
                CMSetProperty(
                    ci,
                    "model",
                    (CMPIValue*)&(data.value.ref),
                    CMPI_ref );
                CMSetProperty( ci, "driver", (CMPIValue*)&(ref), CMPI_ref );
            }

            CMReturnInstance( rslt, ci );
        }
    }

    return rc;
}

CMPIStatus TestAssociationProviderReferenceNames(
    CMPIAssociationMI * mi,
    const CMPIContext * ctx,
    const CMPIResult * rslt,
    const CMPIObjectPath * ref,
    const char* resultClass,
    const char* role)
{
    CMPIInstance * ci = NULL;
    CMPIObjectPath * op = NULL;
    CMPIObjectPath * rop = NULL;
    CMPIObjectPath * cop = NULL;
    CMPIEnumeration * en = NULL;
    CMPIData data ;

    const char * targetName = NULL;
    const char * _thisClassName;
    const char * _RefLeftClass = NULL;
    const char * _RefRightClass = NULL;

    CMPIStatus rc = {CMPI_RC_OK, NULL};
    _thisClassName=_ClassName;


    /* get object path of the target class */
    op = get_assoc_targetClass_ObjectPath(
        _broker,
        ref,
        _RefLeftClass,
        _RefRightClass,
        &rc);

    /* create new object path of association */
    rop = CMNewObjectPath(
        _broker,
        CMGetCharsPtr(CMGetNameSpace(ref,&rc),NULL),
        _thisClassName,
        &rc );

    /* upcall to CIMOM; call enumInstanceNames() of the target class */
    en = CBEnumInstanceNames( _broker, ctx, op, &rc);

    /* as long as object path entries are found in the enumeration */
    while( CMHasNext( en, &rc) )
    {
        /* get the object path */
        data = CMGetNext( en, &rc);

        /* create new instance of the association */
        ci = CMNewInstance( _broker, rop, &rc);

        /* get name of the target class */
        targetName = get_assoc_targetClass_Name(
            _broker,
            ref,
            _RefLeftClass,
            _RefRightClass,
            &rc);

        /* set the properties of the association instance depending on the
         * constellation of the source class (parameter ref) and the target
         * class (see targetName)
        */

        if(strcmp (targetName, "CMPI_TEST_Person")== 0)
        {
            CMSetProperty(
                ci,
                "model",
                (CMPIValue*)&(data.value.ref),
                CMPI_ref );
            CMSetProperty( ci, "driver", (CMPIValue*)&(ref), CMPI_ref );
        }
        else if( strcmp( targetName,"CMPI_TEST_Vehicle") == 0 )
        {
            CMSetProperty(
                ci,
                "model",
                (CMPIValue*)&(data.value.ref),
                CMPI_ref );
            CMSetProperty( ci, "driver", (CMPIValue*)&(ref), CMPI_ref );
        }

        /* get object path of association instance */
        cop = CMGetObjectPath(ci,&rc);

        /* set namespace in object path of association */
        CMSetNameSpace(cop,CMGetCharsPtr(CMGetNameSpace(ref,&rc),NULL));

        /* and return the association object path as result of the
         * referenceNames() call
        */
        CMReturnObjectPath( rslt, cop );
    }

    return rc;
}



/* ---------------------------------------------------------------------------*/
/*                              Provider Factory                              */
/* ---------------------------------------------------------------------------*/

CMAssociationMIStub(
    TestAssociationProvider,
    TestAssociationProvider,
    _broker,
    CMNoHook)

/* ---------------------------------------------------------------------------*/
/*             end of TestCMPIProvider                      */
/* ---------------------------------------------------------------------------*/

