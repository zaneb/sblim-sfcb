
/*
 * cmpiftx.h
 *
 * (C) Copyright IBM Corp. 2005
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:        Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * CMPI extdended function tables.
 *
*/

#ifndef _CMPIFTX_H_
#define _CMPIFTX_H_

#include "constClass.h"
#include "cmpidt.h"

#ifdef __cplusplus
extern "C" {
#endif

   //---------------------------------------------------
   //--
   //	_CMPIClassMI Class Provider object
   //--
   //---------------------------------------------------


   /** This structure represents a Class provider.
   */
   typedef struct _CMPIClassMIFT CMPIClassMIFT;
   typedef struct _CMPIClassMI {

       /** Opaque pointer to Provider specific implementation data.
       */
      void *hdl;

       /** Pointer to the Class Provider Function Table.
       */
      CMPIClassMIFT *ft;
   } CMPIClassMI;



   //---------------------------------------------------
   //--
   //	_CMPIClassMIFT Function Table
   //--
   //---------------------------------------------------


   /** This structure is a table of pointers providing access to Class
       provider functions. This table must be returend during initialization
       by the provider.
   */
   struct _CMPIClassMIFT {

       /** Function table version
       */
     int ftVersion;

       /** Provider version
       */
     int miVersion;

       /** Provider name
       */
     char *miName;

       /** Cleanup is called prior to unloading of the provider.
	 @param mi Provider this pointer.
	 @param ctx Invocation Context
	 @return Function return status.
      */
     CMPIStatus (*cleanup)
            (CMPIClassMI* mi, CMPIContext* ctx);

       /** Enumerate ObjectPaths of Classes serviced by this provider.
	 @param mi Provider this pointer.
	 @param ctx Invocation Context.
	 @param rslt Result data container.
	 @param op ObjectPath containing namespace and classname components.
	 @return Function return status.
      */
     CMPIStatus (*enumClassNames)
             (CMPIClassMI* mi, CMPIContext* ctx, CMPIResult* rslt,
              CMPIObjectPath* op);

       /** Enumerate the Classes serviced by this provider.
	 @param mi Provider this pointer.
	 @param ctx Invocation Context.
	 @param rslt Result data container.
	 @param op ObjectPath containing namespace and classname components.
	 @return Function return status.
      */
     CMPIStatus (*enumClasses)
             (CMPIClassMI* mi, CMPIContext* ctx, CMPIResult* rslt,
              CMPIObjectPath* op);

       /** Get the Class defined by &lt;op&gt;.
	 @param mi Provider this pointer.
	 @param ctx Invocation Context.
	 @param rslt Result data container.
	 @param op ObjectPath containing namespace, classname and key components.
	 @param properties If not NULL, the members of the array define one or
	     more Property names. Each returned Object MUST NOT include elements
	     for any Properties missing from this list.
	 @return Function return status.
      */
     CMPIStatus (*getClass)
             (CMPIClassMI* mi, CMPIContext* ctx, CMPIResult* rslt,
              CMPIObjectPath* op, char** properties);

       /** Create Class from &lt;cls&gt; using &lt;op&gt; as reference.
	 @param mi Provider this pointer.
	 @param ctx Invocation Context.
	 @param rslt Result data container.
	 @param op ObjectPath containing namespace and classname components.
	 @param cls The Class.
	 @return Function return status.
      */
     CMPIStatus (*createClass)
             (CMPIClassMI* mi, CMPIContext* ctx, CMPIResult* rslt,
              CMPIObjectPath* op, CMPIConstClass* cls);

       /** Replace an existing Class from &lt;cls&gt; using &lt;op&gt; as reference.
	 @param mi Provider this pointer.
	 @param ctx Invocation Context.
	 @param rslt Result data container.
	 @param op ObjectPath containing namespace and classname components.
	 @param cls The Class.
	 @return Function return status.
      */
     CMPIStatus (*setClass)
             (CMPIClassMI* mi, CMPIContext* ctx, CMPIResult* rslt,
              CMPIObjectPath* op, CMPIConstClass* cls);

       /** Delete an existing Class defined by &lt;op&gt;.
	 @param mi Provider this pointer.
	 @param ctx Invocation Context.
	 @param rslt Result data container.
	 @param op ObjectPath containing namespace and classname components.
	 @return Function return status.
      */
     CMPIStatus (*deleteClass)
             (CMPIClassMI* mi, CMPIContext* ctx, CMPIResult* rslt,
              CMPIObjectPath* op);
  };



#ifdef __cplusplus
 };
#endif

#endif // _CMPIFTX_H_

