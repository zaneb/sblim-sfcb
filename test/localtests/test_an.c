#include "cimc/cimc.h"
#include "CimClientLib/cmci.h"
#include "CimClientLib/native.h"
#include <unistd.h>
#include <stdlib.h>
#include "CimClientLib/cmcimacs.h"

extern char *value2Chars(CMPIType type, CMPIValue * value);

int main()
{
    CIMCEnv *ce;
    char *msg = NULL;
    int rc;
    CIMCStatus status;
    CIMCClient *client;
    CIMCObjectPath *op;
    CMPIConstClass* clas ;
    CIMCEnumeration *enumeration;
    CIMCString *path;
    CMPIString * classname ;
    CIMCData data;
    CMPIData cdata ;
    char        *cim_host, *cim_host_passwd, *cim_host_userid , *cim_host_port;
    int i = 0 ;
    int retc = 0;
    int count = 0;
    int numproperties = 0;
    CMPIString * propertyname;
    char *cv;

    /* Setup a connection to the CIMOM */
    cim_host = getenv("CIM_HOST");
    if (cim_host == NULL)
                cim_host = "localhost";
    cim_host_userid = getenv("CIM_HOST_USERID");
    if (cim_host_userid == NULL)
                cim_host_userid = "root";
    cim_host_passwd = getenv("CIM_HOST_PASSWD");
    if (cim_host_passwd == NULL)
                cim_host_passwd = "password";
    cim_host_port = getenv("CIM_HOST_PORT");
          if (cim_host_port == NULL)
             cim_host_port = "5988";

    printf(" Testing AssociatorNames \n") ;
    printf(" using SfcbLocal interface : host = %s userid = %s\n",
                          cim_host,cim_host_userid) ;
    ce = NewCIMCEnv("SfcbLocal",0,&rc,&msg);

    if(ce == NULL) {
      printf(" local connect failed call to NewCIMCEnv rc = %d , message = [%s] \n",retc,msg) ;
      return 1;
    }

    client = ce->ft->connect(ce, cim_host , "http", cim_host_port, cim_host_userid, cim_host_passwd , &status);

    op = ce->ft->newObjectPath(ce, "root/cimv2", "TEST_Person" , &status); 
    
    CMAddKey(op, "name", "Mike", CMPI_chars);
    enumeration = client->ft->associatorNames( client,
        op,                             // The parent object?
        "TEST_Lineage",                 // assocClass
        NULL,                           // resultClass
        NULL,                           // role
        NULL,                           // resultRole
        &status);                       // return code

    /* Print the results */
    printf( "associatorNames() rc=%d, msg=%s\n",
            status.rc, (status.msg)? (char *)status.msg->hdl : NULL);
    if (!status.rc)
    {
        printf("result(s):\n");
        while (enumeration->ft->hasNext(enumeration, NULL))
        {
            data = enumeration->ft->getNext(enumeration, NULL);
            showObjectPath(data.value.ref);
        }
    }

    if (enumeration) CMRelease(enumeration);
    if (op) CMRelease(op);
    if (status.msg) CMRelease(status.msg);

    return 0;

}
