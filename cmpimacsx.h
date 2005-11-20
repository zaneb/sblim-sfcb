
/*
 * cmpimacsx.h
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
 * CMPI extended convenious macros.
 *
*/


#ifndef _CMPIMACSX_H_
#define _CMPIMACSX_H_

#ifndef DOC_ONLY
#include "cmpidt.h"
#include "cmpift.h"
#include "cmpiftx.h"
#endif

#ifdef DOC_ONLY
#define CMPI_INLINE
#endif

/*
	-----------------  C provider factories ---------------------
*/

#ifdef DOC_ONLY
       /** This macro generates the function table and initialization stub
           for a class provider. The initialization routine &lt;pn&gt;Create_ClassMI
	   is called when this provider module is loaded by the broker.
	   This macro is for CMPI providers written in plain C.
	 @param pfx The prefix for all mandatory class provider functions.
	        This is a character string without quotes.
	        Mandatory functions are: &lt;pfx&gt;Cleanup, &lt;pfx&gt;EnumClassNames,
                &lt;pfx&gt;EnumClasses, &lt;pfx&gt;GetClass, &lt;pfx&gt;CreateClass,
                &lt;pfx&gt;SetClass and &lt;pfx&gt;DeleteClass.
	 @param pn The provider name under which this provider is registered.
	        This is a character string without quotes.
	 @param broker The name of the broker variable used by this macro to store
	               the CMPIBroker pointer
	 @param hook A statement that is executed within &lt;pn&gt;Create_ClassMI routine.
	             This enables you to perform additional initialization functions and
		     is normally a function call like furtherInit(broker) or CMNoHook.
		     Use CMNoHook if no further intialization is required.
	 @return The function table of this Class provider.
      */
   CMPIClassMI* CMClassMIStub(chars pfx, chars pn,
         CMPIBroker *broker, statement hook);
#else
  #define CMClassMIStub(pfx,pn,broker,hook) \
  static CMPIClassMIFT clsMIFT__={ \
   CMPICurrentVersion, \
   CMPICurrentVersion, \
   "Class" #pn, \
   pfx##Cleanup, \
   pfx##EnumClassNames, \
   pfx##EnumClasses, \
   pfx##GetClass, \
   pfx##CreateClass, \
   pfx##SetClass, \
   pfx##DeleteClass, \
  }; \
  CMPI_EXTERN_C \
  CMPIClassMI* pn##_Create_ClassMI(CMPIBroker* brkr,CMPIContext *ctx) { \
   static CMPIClassMI mi={ \
      NULL, \
      &clsMIFT__, \
   }; \
   broker=brkr; \
   hook; \
   return &mi;  \
  }
#endif

#endif // _CMPIMACSX_H_
