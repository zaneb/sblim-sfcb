

/*

package org.pegasus.jmpi.tests.JMPI_TestPropertyTypes;

import org.pegasus.jmpi.*;
import java.util.*;


public class JMPI_TestPropertyTypes implements InstanceProvider, MethodProvider, EventProvider
{
   CIMOMHandle ch;

   Vector pathes=new Vector();
   Vector instances=new Vector();

   public JMPI_TestPropertyTypes() {
   }

   public void cleanup()
                throws CIMException {
   }

   public void initialize(CIMOMHandle ch)
                throws CIMException {
      this.ch=ch;

      CIMInstance instance1=new CIMInstance("JMPI_TestPropertyTypes");
      CIMObjectPath cop1=new CIMObjectPath("JMPI_TestPropertyTypes","test/static");

      CMSetProperty(ci1,"CreationClassName",
         new CIMValue(new String("JMPI_TestPropertyTypes")));
      CMAddKey(op1,"CreationClassName",
         new CIMValue(new String("JMPI_TestPropertyTypes")));
      CMSetProperty(ci1,"InstanceId",
          new CIMValue(new UnsignedInt64("1")));
      CMAddKey(op1,"InstanceId",
          new CIMValue(new UnsignedInt64("1")));
      CMSetProperty(ci1,"PropertyString",
         new CIMValue(new String("JMPI_TestPropertyTypes_Instance1")));
      CMSetProperty(ci1,"PropertyUint8",
          new CIMValue(new UnsignedInt8((short)120)));
      CMSetProperty(ci1,"PropertyUint16",
          new CIMValue(new UnsignedInt16((int)1600)));
      CMSetProperty(ci1,"PropertyUint32",
          new CIMValue(new UnsignedInt32((long)3200)));
      CMSetProperty(ci1,"PropertyUint64",
          new CIMValue(new UnsignedInt64("6400")));
      CMSetProperty(ci1,"PropertySint8",
          new CIMValue(new Byte((byte)-120)));
      CMSetProperty(ci1,"PropertySint16",
          new CIMValue(new Short((short)-1600)));
      CMSetProperty(ci1,"PropertySint32",
          new CIMValue(new Integer(-3200)));
      CMSetProperty(ci1,"PropertySint64",
          new CIMValue(new Long(-6400)));
      CMSetProperty(ci1,"PropertyBoolean",
          new CIMValue(new Boolean(true)));
      CMSetProperty(ci1,"PropertyReal32",
          new CIMValue(new Float(1.12345670123)));
      CMSetProperty(ci1,"PropertyReal64",
          new CIMValue(new Double(1.12345678906543210123)));
      CMSetProperty(ci1,"PropertyDatetime",
         new CIMValue(new CIMDateTime("20010515104354.000000:000")));

      instances.addElement(instance1);
      pathes.addElement(cop1);

      CIMInstance instance2=new CIMInstance("JMPI_TestPropertyTypes");
      CIMObjectPath cop2=new CIMObjectPath("JMPI_TestPropertyTypes","test/static");

      instance2.setProperty("CreationClassName",
         new CIMValue(new String("JMPI_TestPropertyTypes")));
      cop2.addKey("CreationClassName",
         new CIMValue(new String("JMPI_TestPropertyTypes")));
      instance2.setProperty("InstanceId",
          new CIMValue(new UnsignedInt64("2")));
      cop2.addKey("InstanceId",
          new CIMValue(new UnsignedInt64("2")));
      instance2.setProperty("PropertyString",
         new CIMValue(new String("JMPI_TestPropertyTypes_Instance2")));
      instance2.setProperty("PropertyUint8",
          new CIMValue(new UnsignedInt8((short)122)));
      instance2.setProperty("PropertyUint16",
          new CIMValue(new UnsignedInt16((int)1602)));
      instance2.setProperty("PropertyUint32",
          new CIMValue(new UnsignedInt32((long)3202)));
      instance2.setProperty("PropertyUint64",
          new CIMValue(new UnsignedInt64("6402")));
      instance2.setProperty("PropertySint8",
          new CIMValue(new Byte((byte)-122)));
      instance2.setProperty("PropertySint16",
          new CIMValue(new Short((short)-1602)));
      instance2.setProperty("PropertySint32",
          new CIMValue(new Integer(-3202)));
      instance2.setProperty("PropertySint64",
          new CIMValue(new Long(-6402)));
      instance2.setProperty("PropertyBoolean",
          new CIMValue(new Boolean(false)));
      instance2.setProperty("PropertyReal32",
          new CIMValue(new Float(2.12345670123)));
      instance2.setProperty("PropertyReal64",
          new CIMValue(new Double(2.12345678906543210123)));
      instance2.setProperty("PropertyDatetime",
         new CIMValue(new CIMDateTime("20010515104354.000000:000")));

      instances.addElement(instance2);
      pathes.addElement(cop2);
   }

   int findObjectPath(CIMObjectPath path) {
      String p=path.toString();
      for (int i=0; i<pathes.size(); i++) {
        if (((CIMObjectPath)pathes.elementAt(i)).toString().equalsIgnoreCase(p))
            return i;
      }
      return -1;
   }


   void testPropertyTypesValue(CIMInstance instanceObject)
                       throws CIMException {

      Vector properties=instanceObject.getProperties();

      int PropertyCount=properties.size();

      for (int j=0; j<PropertyCount; j++) {
         CIMProperty property=(CIMProperty)properties.elementAt(j);
         String propertyName=property.getName();
         CIMValue propertyValue=property.getValue();
         Object value=propertyValue.getValue();

         int type=property.getType().getType();

         switch (type) {
         case CIMDataType.UINT8:
            if (!(value instanceof UnsignedInt8))
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            if (((UnsignedInt8)value).intValue() >= 255)
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            break;
         case CIMDataType.UINT16:
            if (!(value instanceof UnsignedInt16))
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            if (((UnsignedInt16)value).intValue() >= 10000)
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            break;
         case CIMDataType.UINT32:
            if (!(value instanceof UnsignedInt32))
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            if (((UnsignedInt32)value).intValue() >= 10000000)
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            break;
         case CIMDataType.UINT64:
            if (!(value instanceof UnsignedInt64))
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            if (((UnsignedInt64)value).longValue() >= 1000000000)
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            break;
         case CIMDataType.SINT8:
            if (!(value instanceof Byte))
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            if (((Byte)value).intValue() <= -120)
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            break;
         case CIMDataType.SINT16:
           if (!(value instanceof Short))
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            if (((Short)value).intValue() < -10000)
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            break;
         case CIMDataType.SINT32:
            if (!(value instanceof Integer))
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            if (((Integer)value).intValue() <= -10000000)
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            break;
         case CIMDataType.SINT64:
            if (!(value instanceof Long))
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            if (((Long)value).intValue() <= -1000000000)
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            break;
         case CIMDataType.REAL32:
            if (!(value instanceof Float))
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            if (((Float)value).floatValue() >= 10000000.32)
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            break;
         case CIMDataType.REAL64:
           if (!(value instanceof Double))
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            if (((Double)value).doubleValue() >= 1000000000.64)
               throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);
            break;
         default: ;
         }
      }
   }

   public CIMObjectPath createInstance(CIMObjectPath op,
                               CIMInstance ci)
                        throws CIMException {

      // ensure the Namespace is valid
      if (!op.getNameSpace().equalsIgnoreCase("test/static"))
         throw new CIMException(CIMException.CIM_ERR_INVALID_NAMESPACE);

      // ensure the class existing in the specified namespace
      if (!op.getObjectName().equalsIgnoreCase("JMPI_TestPropertyTypes"))
         throw new CIMException(CIMException.CIM_ERR_INVALID_CLASS);

      // ensure the InstanceId key is valid
      CIMProperty prop=ci.getProperty("InstanceId");
      if (prop==null)
         throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);

      CIMObjectPath rop=new CIMObjectPath("JMPI_TestPropertyTypes","test/static");
      rop.addKey("InstanceId",prop.getValue());
      rop.addKey("CreationClassName", new CIMValue("JMPI_TestPropertyTypes"));

      // ensure the property values are valid
      testPropertyTypesValue(ci);

      // Determine if a property exists in the class
      if (ci.getProperty("PropertyUint8")==null)
         throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);

      // ensure the requested object do not exist
      if (findObjectPath(op) >= 0)
         throw new CIMException(CIMException.CIM_ERR_ALREADY_EXISTS);

      return rop;
   }



   public CIMInstance getInstance(CIMObjectPath op,
                               CIMClass cc,
                               boolean localOnly)
                        throws CIMException {
      // ensure the InstanceId key is valid
      Vector keys=op.getKeys();
      int i;
      for (i=0; i<keys.size() && !((CIMProperty)keys.elementAt(i)).getName().
             equalsIgnoreCase("InstanceId");
           i++);

      if (i==keys.size())
         throw new CIMException(CIMException.CIM_ERR_INVALID_PARAMETER);

      // ensure the Namespace is valid
      if (!op.getNameSpace().equalsIgnoreCase("test/static"))
         throw new CIMException(CIMException.CIM_ERR_INVALID_NAMESPACE);

      // ensure the class existing in the specified namespace
      if (!op.getObjectName().equalsIgnoreCase("JMPI_TestPropertyTypes"))
         throw new CIMException(CIMException.CIM_ERR_INVALID_CLASS);

      // ensure the request object exists
      int index=findObjectPath(op);
      if (index<0)
         throw new CIMException(CIMException.CIM_ERR_NOT_FOUND);

      return (CIMInstance)instances.elementAt(index);
   }



   public void setInstance(CIMObjectPath op,
                               CIMInstance cimInstance)
                        throws CIMException  {

      // ensure the Namespace is valid
      if (!op.getNameSpace().equalsIgnoreCase("test/static"))
         throw new CIMException(CIMException.CIM_ERR_INVALID_NAMESPACE);

      // ensure the class existing in the specified namespace
      if (!op.getObjectName().equalsIgnoreCase("JMPI_TestPropertyTypes"))
         throw new CIMException(CIMException.CIM_ERR_INVALID_CLASS);

      // ensure the property values are valid
      testPropertyTypesValue(cimInstance);

      // ensure the request object exists
      int index=findObjectPath(op);
      if (index<0)
         throw new CIMException(CIMException.CIM_ERR_NOT_FOUND);
   }



   public void deleteInstance(CIMObjectPath op)
                        throws CIMException  {
      // ensure the Namespace is valid
      if (!op.getNameSpace().equalsIgnoreCase("test/static"))
         throw new CIMException(CIMException.CIM_ERR_INVALID_NAMESPACE);

      // ensure the class existing in the specified namespace
      if (!op.getObjectName().equalsIgnoreCase("JMPI_TestPropertyTypes"))
         throw new CIMException(CIMException.CIM_ERR_INVALID_CLASS);

      // ensure the request object exists
      int index=findObjectPath(op);
      if (index<0)
         throw new CIMException(CIMException.CIM_ERR_NOT_FOUND);
   }



   public Vector enumInstances(CIMObjectPath op,
                               boolean deep,
                               CIMClass cimClass)
                        throws CIMException {
      // ensure the Namespace is valid
      if (!op.getNameSpace().equalsIgnoreCase("test/static"))
         throw new CIMException(CIMException.CIM_ERR_INVALID_NAMESPACE);

      // ensure the class existing in the specified namespace
      if (!op.getObjectName().equalsIgnoreCase("JMPI_TestPropertyTypes"))
         throw new CIMException(CIMException.CIM_ERR_INVALID_CLASS);

      return pathes;
   }



   public Vector enumInstances(CIMObjectPath op,
                               boolean deep,
                               CIMClass cimClass,
                               boolean localOnly)
                        throws CIMException {
      // ensure the Namespace is valid
      if (!op.getNameSpace().equalsIgnoreCase("test/static"))
         throw new CIMException(CIMException.CIM_ERR_INVALID_NAMESPACE);

      // ensure the class existing in the specified namespace
      if (!op.getObjectName().equalsIgnoreCase("JMPI_TestPropertyTypes"))
         throw new CIMException(CIMException.CIM_ERR_INVALID_CLASS);

      return instances;
   }



   public Vector execQuery(CIMObjectPath op,
                               String queryStatement,
                               int ql,
                               CIMClass cimClass)
                        throws CIMException {
      return null;
   }




   public CIMValue invokeMethod(CIMObjectPath op,
                               String method,
                               Vector in,
                               Vector out)
                        throws CIMException {

      if (method.equalsIgnoreCase("SayHello"))
         return new CIMValue(new String("hello"));

      throw new CIMException(CIMException.CIM_ERR_METHOD_NOT_AVAILABLE);
   }




    public void authorizeFilter(SelectExp filter,
                                String eventType,
                                CIMObjectPath classPath,
                                String owner)
                        throws CIMException {
    }

    public boolean mustPoll(SelectExp filter,
                            String eventType,
                            CIMObjectPath classPath)
                        throws CIMException {
        return false;
    }


    public void activateFilter(SelectExp filter,
                               String eventType,
                               CIMObjectPath classPath,
                               boolean firstActivation)
                        throws CIMException {
    }


    public void deActivateFilter(SelectExp filter,
                                 String eventType,
                                 CIMObjectPath classPath,
                                 boolean lastActivation)
                        throws CIMException {
    }


}

*/



/*
 * dataType.c
 *
 * (C) Copyright IBM Corp. 2005
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Common Public License from
 * http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
 *
 * Author:   Adrian Schuur    
 *
 * Description:
 *
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cmpidt.h"
#include "cmpift.h"
#include "cmpimacs.h"

static CMPIObjectPath **ops=NULL;
static CMPIInstance **cis=NULL;
static int iCount=0;

static CMPIBroker *_broker;

static CMPIInstance *getInstance(CMPIObjectPath *op, int *n, CMPIStatus *rc)
{
   CMPIString *icn,*cn;
   CMPIUint64 iid,id;
   int i;
   
   icn=CMGetKey(op,"CreationClassName",rc).value.string;
   if (icn==NULL) return NULL;
   iid=CMGetKey(op,"InstanceId",rc).value.uint64;
   if (rc->rc) return NULL;
   
   for (i=0; i<iCount; i++) {
      if (ops[i]) {
         cn=CMGetKey(op,"CreationClassName",rc).value.string;
         if (cn==NULL) continue;
         id=CMGetKey(op,"InstanceId",rc).value.uint64;
         if (rc->rc) continue;
         if (strcasecmp(CMGetCharPtr(icn),CMGetCharPtr(cn))==0 && iid==id) {
            if (n) *n=i;
            return cis[i];
         }   
      }
   }
   
   return NULL;
}

static void addInstance(CMPIObjectPath *op, CMPIInstance *ci)
{
   if (iCount) {
      ops=(CMPIObjectPath**)realloc(ops,(iCount+1)*sizeof(CMPIObjectPath*));
      cis=(CMPIInstance**)realloc(cis,(iCount+1)*sizeof(CMPIInstance*));
   }
   else {
      ops=(CMPIObjectPath**)malloc(sizeof(CMPIObjectPath*));
      cis=(CMPIInstance**)malloc(sizeof(CMPIInstance*));
   }
   
   ops[iCount]=CMClone(op,NULL);
   cis[iCount]=CMClone(ci,NULL);
   iCount++;
}

CMPIrc testPropertyTypesValue(CMPIInstance *ci)
{
   int i,n=CMGetPropertyCount(ci,NULL);
   CMPIData d;

   for (i=0; i<n; i++) {
      d=CMGetPropertyAt(ci,i,NULL,NULL);
      switch (d.type) {
      case CMPI_uint8: 
         if (d.value.uint8<=127) return CMPI_RC_OK;
         return CMPI_RC_ERR_INVALID_PARAMETER;
      case CMPI_uint16: 
         if (d.value.uint16<=10000) return CMPI_RC_OK;
         return CMPI_RC_ERR_INVALID_PARAMETER;
      case CMPI_uint32: 
         if (d.value.uint32<=10000000) return CMPI_RC_OK;
         return CMPI_RC_ERR_INVALID_PARAMETER;
      case CMPI_uint64: 
         if (d.value.uint64<=1000000000) return CMPI_RC_OK;
         return CMPI_RC_ERR_INVALID_PARAMETER;
         
      case CMPI_sint8: 
         if (d.value.sint8<=-127) return CMPI_RC_OK;
         return CMPI_RC_ERR_INVALID_PARAMETER;
      case CMPI_sint16: 
         if (d.value.sint16<=-10000) return CMPI_RC_OK;
         return CMPI_RC_ERR_INVALID_PARAMETER;
      case CMPI_sint32: 
         if (d.value.sint32<=-10000000) return CMPI_RC_OK;
         return CMPI_RC_ERR_INVALID_PARAMETER;
      case CMPI_sint64: 
         if (d.value.sint64<=-1000000000) return CMPI_RC_OK;
         return CMPI_RC_ERR_INVALID_PARAMETER;
      case CMPI_real32: 
         if (d.value.real32<=10000000.32) return CMPI_RC_OK;
         return CMPI_RC_ERR_INVALID_PARAMETER;
      case CMPI_real64: 
         if (d.value.real64<=1000000000.64) return CMPI_RC_OK;
         return CMPI_RC_ERR_INVALID_PARAMETER;
      }
   }
   
   return CMPI_RC_OK;
}
   
   
static void init(CMPIBroker *broker)
{
   CMPIInstance *ci1,*ci2;
   CMPIObjectPath *op1,*op2;
   CMPIUint64 id1=1,id2=2;
   
   CMPIUint8  uint8_1=121,  uint8_2=122;
   CMPIUint16 uint16_1=1601,uint16_2=1602;
   CMPIUint32 uint32_1=3201,uint32_2=2302;
   CMPIUint64 uint64_1=6401,uint64_2=6402;
   
   CMPISint8  sint8_1=-121,  sint8_2=-122;
   CMPISint16 sint16_1=-1601,sint16_2=-1602;
   CMPISint32 sint32_1=-3201,sint32_2=-2302;
   CMPISint64 sint64_1=-6401,sint64_2=-6402;
   
   CMPIBoolean booleanTrue=1,booleanFalse=0;
   CMPIReal32 real32_1=1.12345670123,real32_2=2.12345670123;
   CMPIReal64 real64_1=1.6412345678906543210123,real64_2=2.6412345678906543210123;
   
   CMPIDateTime *dateTime=CMNewDateTimeFromChars(_broker,"20010515104354.000000:000",NULL);
 
   op1=CMNewObjectPath(broker,"root/tests","TST_DataTypes",NULL);
   ci1=CMNewInstance(broker,op1,NULL);

   CMSetProperty(ci1,"CreationClassName","TST_DataTypes",CMPI_chars);
   CMAddKey(op1,"CreationClassName","TST_DataTypes",CMPI_chars);
   CMSetProperty(ci1,"InstanceId",&id1,CMPI_uint64);
   CMAddKey(op1,"InstanceId",&id1,CMPI_uint64);
      
   CMSetProperty(ci1,"PropertyString","Test_DataTypes_Instance1",CMPI_chars);
      
   CMSetProperty(ci1,"PropertyUint8",&uint8_1,CMPI_uint8);
   CMSetProperty(ci1,"PropertyUint16",&uint16_1,CMPI_uint16);
   CMSetProperty(ci1,"PropertyUint32",&uint32_1,CMPI_uint32);
   CMSetProperty(ci1,"PropertyUint64",&uint64_1,CMPI_uint64);
      
   CMSetProperty(ci1,"PropertySint8",&sint8_1,CMPI_sint8);
   CMSetProperty(ci1,"PropertySint16",&sint16_1,CMPI_sint16);
   CMSetProperty(ci1,"PropertySint32",&sint32_1,CMPI_sint32);
   CMSetProperty(ci1,"PropertySint64",&sint64_1,CMPI_sint64);

   CMSetProperty(ci1,"PropertyBoolean",&booleanTrue,CMPI_boolean);
   CMSetProperty(ci1,"PropertyReal32",&real32_1,CMPI_real32);
   CMSetProperty(ci1,"PropertyReal64",&real64_1,CMPI_real64);
   CMSetProperty(ci1,"PropertyDatetime",&dateTime,CMPI_dateTime);

   addInstance(op1,ci1);
   
   op2=CMNewObjectPath(broker,"root/tests","TST_DataTypes",NULL);
   ci2=CMNewInstance(broker,op2,NULL);

   CMSetProperty(ci2,"CreationClassName","TST_DataTypes",CMPI_chars);
   CMAddKey(op2,"CreationClassName","TST_DataTypes",CMPI_chars);
   CMSetProperty(ci2,"InstanceId",&id2,CMPI_uint64);
   CMAddKey(op2,"InstanceId",&id2,CMPI_uint64);
      
   CMSetProperty(ci2,"PropertyString","Test_DataTypes_Instance2",CMPI_chars);
      
   CMSetProperty(ci2,"PropertyUint8",&uint8_2,CMPI_uint8);
   CMSetProperty(ci2,"PropertyUint16",&uint16_2,CMPI_uint16);
   CMSetProperty(ci2,"PropertyUint32",&uint32_2,CMPI_uint32);
   CMSetProperty(ci2,"PropertyUint64",&uint64_2,CMPI_uint64);
      
   CMSetProperty(ci2,"PropertySint8",&sint8_2,CMPI_sint8);
   CMSetProperty(ci2,"PropertySint16",&sint16_2,CMPI_sint16);
   CMSetProperty(ci2,"PropertySint32",&sint32_2,CMPI_sint32);
   CMSetProperty(ci2,"PropertySint64",&sint64_2,CMPI_sint64);

   CMSetProperty(ci2,"PropertyBoolean",&booleanFalse,CMPI_boolean);
   CMSetProperty(ci2,"PropertyReal32",&real32_2,CMPI_real32);
   CMSetProperty(ci2,"PropertyReal64",&real64_2,CMPI_real64);
   CMSetProperty(ci2,"PropertyDatetime",&dateTime,CMPI_dateTime);

   addInstance(op2,ci2);
   
}

CMPIStatus dataTypesCleanup( CMPIInstanceMI * mi, 
           CMPIContext * ctx) { 

  CMReturn(CMPI_RC_OK);
}


CMPIStatus dataTypesEnumInstanceNames( CMPIInstanceMI * mi, 
  CMPIContext * ctx, CMPIResult * rslt, CMPIObjectPath * ref) 
{ 
  
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  CMPIString *ns=CMGetNameSpace(ref,NULL);
  int i;
  
  if (strcasecmp(CMGetCharPtr(ns),"root/tests"))
     rc.rc=CMPI_RC_ERR_INVALID_NAMESPACE;
     
  else for (i=0; i<iCount; i++) {
     CMReturnObjectPath(rslt,ops[i]);
  }   
  
  return rc;
}

CMPIStatus dataTypesEnumInstances( CMPIInstanceMI * mi, 
  CMPIContext * ctx, CMPIResult * rslt, CMPIObjectPath * ref, char ** properties) 
{ 
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  CMPIString *ns=CMGetNameSpace(ref,NULL);
  int i;
  
  if (strcasecmp(CMGetCharPtr(ns),"root/tests"))
     rc.rc=CMPI_RC_ERR_INVALID_NAMESPACE;
     
  else for (i=0; i<iCount; i++) {
     CMReturnInstance(rslt,cis[i]);
  }   
  
  return rc;
}

CMPIStatus dataTypesGetInstance( CMPIInstanceMI * mi, 
  CMPIContext * ctx, CMPIResult * rslt, CMPIObjectPath * cop, char ** properties) 
{   
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  CMPIString *ns=CMGetNameSpace(cop,NULL);
  CMPIInstance *ci;
  int i;
  
  if (strcasecmp(CMGetCharPtr(ns),"root/tests"))
     rc.rc=CMPI_RC_ERR_INVALID_NAMESPACE;
     
  else {
     ci=getInstance(cop,&i,&rc);
     if (ci) CMReturnInstance(rslt,ci);
  }
  
  return rc;
}



CMPIStatus dataTypesCreateInstance( CMPIInstanceMI * mi, 
   CMPIContext * ctx, CMPIResult * rslt, CMPIObjectPath * cop, CMPIInstance * ci) 
{
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  CMPIString *ns=CMGetNameSpace(cop,NULL);
  CMPIInstance *cci;
  int i;
  
  if (strcasecmp(CMGetCharPtr(ns),"root/tests"))
     rc.rc=CMPI_RC_ERR_INVALID_NAMESPACE;
     
  else {
     cci=getInstance(cop,&i,&rc);
     if (cci) rc.rc=CMPI_RC_ERR_ALREADY_EXISTS;
     else  {
        rc.rc=testPropertyTypesValue(ci);
        if (rc.rc) return rc;

        if (CMGetProperty(ci,"PropertyUint8",&rc).state) {
            rc.rc=CMPI_RC_ERR_INVALID_PARAMETER;
            return rc;
        } 

        addInstance(cop,ci);
        CMReturnObjectPath(rslt,cop);
     }
  }
  
  return rc;
}

CMPIStatus dataTypesSetInstance( CMPIInstanceMI * mi, CMPIContext * ctx, 
   CMPIResult * rslt, CMPIObjectPath * cop, CMPIInstance * ci, char **properties) 
{
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  CMPIString *ns=CMGetNameSpace(cop,NULL);
  CMPIInstance *cci;
  int i;
  
  if (strcasecmp(CMGetCharPtr(ns),"root/tests"))
     rc.rc=CMPI_RC_ERR_INVALID_NAMESPACE;
     
  else {
     rc.rc=testPropertyTypesValue(ci);
     if (rc.rc) return rc;

     if (CMGetProperty(ci,"PropertyUint8",&rc).state) {
         rc.rc=CMPI_RC_ERR_INVALID_PARAMETER;
         return rc;
     } 

     cci=getInstance(cop,&i,&rc);
     if (cci) cis[i]=CMClone(ci,NULL);
  }
  
  return rc;
}


CMPIStatus dataTypesDeleteInstance( CMPIInstanceMI * mi, 
   CMPIContext * ctx, CMPIResult * rslt, CMPIObjectPath * cop) 
{
  CMPIStatus rc = {CMPI_RC_OK, NULL};
  CMPIString *ns=CMGetNameSpace(cop,NULL);
  CMPIInstance *cci;
  int i;
  
  if (strcasecmp(CMGetCharPtr(ns),"root/tests"))
     rc.rc=CMPI_RC_ERR_INVALID_NAMESPACE;
     
  else {
     cci=getInstance(cop,&i,&rc);
     if (cci) {
        ops[i]=NULL;
        cis[i]=NULL;
     }
  }
  
  return rc;
}


CMPIStatus dataTypesExecQuery( CMPIInstanceMI * mi, CMPIContext * ctx, 
   CMPIResult * rslt, CMPIObjectPath * ref, char * lang, char * query) 
{
  CMPIStatus rc = {CMPI_RC_ERR_NOT_SUPPORTED, NULL};
  CMPIString *ns=CMGetNameSpace(ref,NULL);
  
  if (strcasecmp(CMGetCharPtr(ns),"root/tests"))
     rc.rc=CMPI_RC_ERR_INVALID_NAMESPACE;
     
  return rc;
}



CMInstanceMIStub(dataTypes,dataTypes,_broker,init(_broker));
