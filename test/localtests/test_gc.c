#include "cimc/cimc.h"
#include "CimClientLib/cmci.h"
#include "CimClientLib/native.h"
#include <unistd.h>
#include <stdlib.h>

#include "show.h"
int main()
{
    CIMCEnv *ce;
    char *msg = NULL;
    int rc;
    CIMCStatus status;
    CIMCClient *client;
    CMPIObjectPath * op;
    CMPIConstClass * class;
    char 	*cim_host, *cim_host_passwd, *cim_host_userid , *cim_host_port;
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

    printf(" Testing getClass \n") ;

    printf(" checking with bad input : host = %s userid = %s\n",
                          cim_host,cim_host_userid) ;
    ce = NewCIMCEnv("bogus",0,&rc,&msg);
    if (rc == 3) {
        printf (" bad input caught OK,\n\tmsg = %s\n\n",msg);
    } else {
        printf (" bad input NOT caught.\n");
    }

    printf(" using SfcbLocal interface : host = %s userid = %s\n",
                          cim_host,cim_host_userid) ;
    ce = NewCIMCEnv("SfcbLocal",0,&rc,&msg);

    if (rc == 0 ) {
	
        client = ce->ft->connect(ce, cim_host , "http", cim_host_port, cim_host_userid, cim_host_passwd , &status);

        op = (CMPIObjectPath *)ce->ft->newObjectPath(ce, "root/cimv2", "CIM_ComputerSystem" , &status); 
        class =(CMPIConstClass *) client->ft->getClass(client,(CIMCObjectPath *) op, CMPI_FLAG_IncludeQualifiers, NULL, &status);
	 
        /* Print the results */
        printf( "getClass() rc=%d, msg=%s\n", 
            status.rc, (status.msg)? (char *)status.msg->hdl : NULL);

        if (!status.rc) {
            printf("result:\n");
            showClass(class);
        }

        if (class) class->ft->release((CMPIConstClass *)class);
        if (op) op->ft->release(op);

        if (client) client->ft->release(client);
        if(ce) ce->ft->release(ce);
        if (status.msg) CMRelease(status.msg);
    
        return 0;
    } else {
        printf ("Call to NewCIMCEnv failed, rc = %d\n\tmsg = %s\n",rc,msg);
        return 1;
    }
}
