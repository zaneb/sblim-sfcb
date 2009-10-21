#include "cimc/cimc.h"
#include "CimClientLib/cmci.h"
#include "CimClientLib/native.h"
#include <unistd.h>
#include <stdlib.h>
#include "CimClientLib/cmcimacs.h"

int main()
{
        CIMCEnv *ce;
    char *msg = NULL;
    int rc;
    CIMCStatus status;
    CIMCClient *client;
    CIMCObjectPath *op, *objectpath_r;
    CMPIConstClass* clas ;
    CMPIInstance *instance;
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

    printf(" Testing Get instance \n") ;
    printf(" using SfcbLocal interface : host = %s userid = %s\n",
                          cim_host,cim_host_userid) ;
    ce = NewCIMCEnv("SfcbLocal",0,&rc,&msg);
    if(ce == NULL) {
      printf(" local connect failed call to NewCIMCEnv rc = %d , message = [%s] \n",retc,msg) ;
      return 1;
    }

    client = ce->ft->connect(ce, cim_host , "http", cim_host_port, cim_host_userid, cim_host_passwd , &status);

    op = ce->ft->newObjectPath(ce, "root/cimv2", "Linux_ComputerSystem" , &status);

    CMAddKey(op, "CreationClassName", "Linux_ComputerSystem", CMPI_chars);
    CMAddKey(op, "Name", "localhost.localdomain", CMPI_chars);
    instance = client->ft->getInstance(client, op, 0, NULL, &status);

    /* Print the results */
    printf( "getInstance() rc=%d, msg=%s\n",
            status.rc, (status.msg)? (char *)status.msg->hdl : NULL);
    if (!status.rc) {
        printf("result:\n");
        showInstance(instance);
    }

    if (instance) CMRelease(instance);
    if (op) CMRelease(op);
    if (status.msg) CMRelease(status.msg);

    return 0;
}
