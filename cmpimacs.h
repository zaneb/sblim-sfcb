
/*
 * cmpimacs.h
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
 * CMPI convenious macros.
 *
*/


#ifndef _CMPIMACS_H_
#define _CMPIMACS_H_

#ifndef DOC_ONLY
#include "cmpidt.h"
#include "cmpift.h"
#endif

#ifdef DOC_ONLY
#define CMPI_INLINE
#endif


#ifdef DOC_ONLY
  /** This macro builds a CMPIStatus object with &lt;rc&gt; as return code and returns
      to the Broker.
      @param rc the CMPI return code
      @return This macro contains a return statement and leaves the function.
   */
 noReturn CMReturn(CMPIrc rc);
#else
#define CMReturn(rc) \
      { CMPIStatus stat={(rc),NULL}; \
         return stat; }
#endif

#ifdef DOC_ONLY
  /** This macro builds a CMPIStatus object with &lt;rc&gt; as return code and &lt;str&gt; as
      message and returns to the Broker.
      @param rc the CMPI return code
      @param str the message as String object
      @return This macro contains a return statement and leaves the function.
   */
 noReturn CMReturnWithString(CMPIrc rc, CMPIString *str);
#else
#define CMReturnWithString(rc,str) \
      { CMPIStatus stat={(rc),(str)}; \
         return stat; }
#endif

#ifdef DOC_ONLY
  /** This macro builds a CMPIStatus object with &lt;rc&gt; as return code and &lt;msg&gt; as
      message and returns to the Broker.
      @param mb Broker this pointer
      @param rc the CMPI return code
      @param msg the message as character string
      @return This macro contains a return statement and leaves the function.
   */
 noReturn CMReturnWithChars(const CMPIBroker *mb, CMPIrc rc, char *msg);
#else
#define CMReturnWithChars(b,rc,chars) \
      { CMPIStatus stat={(rc),NULL}; \
         stat.msg=(b)->eft->newString((b),(chars),NULL); \
         return stat; }
#endif


#ifdef CMPI_INLINE
  /** Initializes status object with rc and NULL message.
      @param st Address of status object
      @param rcp CMPI return code
  */
inline static  void CMSetStatus(CMPIStatus* st, CMPIrc rcp)
      { (st)->rc=(rcp); (st)->msg=NULL; }
#else
#define CMSetStatus(st,rcp) \
      { (st)->rc=(rcp); (st)->msg=NULL; }
#endif


#ifdef CMPI_INLINE
  /** Initializes status object with rc and message.
      @param mb Broker this pointer
      @param st Address of status object
      @param rcp CMPI return code
      @param string Message string
  */
inline static   void CMSetStatusWithString(const CMPIBroker *mb, CMPIStatus *st, CMPIrc rcp,
					   const CMPIString *string)
      { (st)->rc=(rcp); (st)->msg=(string); }
#else
#define CMSetStatusWithString(st,rcp,string) \
      { (st)->rc=(rcp); (st)->msg=(string); }
#endif


#ifdef CMPI_INLINE
  /** Initializes status object with rc and message.
      @param mb Broker this pointer
      @param st Address of status object
      @param rcp CMPI return code
      @param chars Message character string
  */
inline static   void CMSetStatusWithChars(const CMPIBroker *mb, CMPIStatus* st, CMPIrc rcp,
					  const char* chars)
      { (st)->rc=(rcp);
        (st)->msg=(mb)->eft->newString((mb),(chars),NULL); }
#else
#define CMSetStatusWithChars(mb,st,rcp,chars) \
      { (st)->rc=(rcp); \
        (st)->msg=(mb)->eft->newString((mb),(chars),NULL); }
#endif

#ifndef DOC_ONLY
  #ifdef CMPI_PLATFORM_WIN32_IX86_MSVC
	#ifdef __cplusplus
        #define CMPI_EXTERN_C extern "C" __declspec(dllexport)
    #else
	#define  CMPI_EXTERN_C __declspec(dllexport)
    #endif
  #else
	#ifdef __cplusplus
      #define  CMPI_EXTERN_C extern "C"
    #else
      #define CMPI_EXTERN_C
    #endif
  #endif
#endif

#ifdef CMPI_INLINE
  /** Tests for encapsulated NULL object.
      @param obj CMPI Object pointer
  */
  inline static   CMPIBoolean CMIsNullObject(constant void* obj)
       { return ((obj)==NULL || *((void**)(obj))==NULL); }
#else
  #define CMIsNullObject(o)           ((o)==NULL || *((void**)(o))==NULL)
#endif

#ifdef CMPI_INLINE
  /** Tests for nullValue data item.
      @param val Value object
  */
  inline static   CMPIBoolean CMIsNullValue(CMPIData val)
       { return ((val.state) & CMPI_nullValue); }
#else
  #define CMIsNullValue(v)                   ((v.state) & CMPI_nullValue)
#endif

#ifdef CMPI_INLINE
  /** Tests for keyValue data item.
      @param val Value object
  */
  inline static   CMPIBoolean CMIsKeyValue(CMPIData val)
       { return ((val.state) & CMPI_keyValue); }
#else
  #define CMIsKeyValue(v)                     ((v.state) & CMPI_keyValue)
#endif

#ifdef CMPI_INLINE
  /** Tests for keyValue data item.
      @param val Value object
  */
  inline static   CMPIBoolean CMIsArray(CMPIData val)
       { return ((val.type) & CMPI_ARRAY); }
#else
  #define CMIsArray(v)                            ((v.type) & CMPI_ARRAY)
#endif


    // Life-cycle macros

  #define CMClone(o,rc)                        ((o)->ft->clone((o),(rc)))
  #define CMRelease(o)                            ((o)->ft->release((o)))
  #define CMGetCharPtr(s)                                 ((char*)s->hdl)

    // CMPIBroker factory macros

#ifdef CMPI_INLINE
     /** Instance factory service.
         @param mb Broker this pointer
	 @param op ObjectPath containing namespace and classname.
	 @param rc Output: Service return status (suppressed when NULL).
         @return The newly created Instance.
     */
  inline static   CMPIInstance* CMNewInstance(const CMPIBroker *mb, const CMPIObjectPath *op,
                                     CMPIStatus *rc)
       { return ((mb)->eft->newInstance((mb),(op),(rc))); }
#else
  #define CMNewInstance(b,c,rc)     ((b)->eft->newInstance((b),(c),(rc)))
#endif

#ifdef CMPI_INLINE
     /** ObjectPath factory service.
         @param mb Broker this pointer
	 @param ns Namespace
	 @param cn Classname.
	 @param rc Output: Service return status (suppressed when NULL).
         @return The newly created ObjectPath.
     */
  inline static   CMPIObjectPath* CMNewObjectPath(const CMPIBroker *mb, const char *ns, const char *cn,
                                         CMPIStatus *rc)
       { return ((mb)->eft->newObjectPath((mb),(ns),(cn),(rc))); }
#else
  #define CMNewObjectPath(b,n,c,rc) \
                              ((b)->eft->newObjectPath((b),(n),(c),(rc)))
#endif

#ifdef CMPI_INLINE
     /** String container factory service.
         @param mb Broker this pointer
	 @param data String data
	 @param rc Output: Service return status (suppressed when NULL).
         @return The newly created String.
     */
  inline static   CMPIString* CMNewString(const CMPIBroker* mb, const char *data, CMPIStatus *rc)
       { return ((mb)->eft->newString((mb),(data),(rc))); }
#else
  #define CMNewString(b,s,rc)         ((b)->eft->newString((b),(s),(rc)))
#endif

#ifdef CMPI_INLINE
     /** Args container factory service.
         @param mb Broker this pointer
	 @param rc Output: Service return status (suppressed when NULL).
         @return The newly created Args container.
     */
  inline static   CMPIArgs* CMNewArgs(const CMPIBroker* mb, CMPIStatus* rc)
       { return ((mb)->eft->newArgs((mb),(rc))); }
#else
  #define CMNewArgs(b,rc)                   ((b)->eft->newArgs((b),(rc)))
#endif

#ifdef CMPI_INLINE
     /** Array container factory service.
         @param mb Broker this pointer
	 @param max Maximum number of elements
	 @param type Element type
	 @param rc Output: Service return status (suppressed when NULL).
         @return The newly created Array.
     */
  inline static   CMPIArray* CMNewArray(const CMPIBroker* mb, CMPICount max, CMPIType type,
                               CMPIStatus* rc)
       { return ((mb)->eft->newArray((mb),(max),(type),(rc))); }
#else
  #define CMNewArray(b,c,t,rc)     ((b)->eft->newArray((b),(c),(t),(rc)))
#endif

#ifdef CMPI_INLINE
     /** DateTime factory service. Initialized with the time of day.
         @param mb Broker this pointer
	 @param rc Output: Service return status (suppressed when NULL).
         @return The newly created DateTime.
     */
   inline static   CMPIDateTime* CMNewDateTime(const CMPIBroker* mb, CMPIStatus* rc)
        { return ((mb)->eft->newDateTime((mb),(rc))); }
#else
  #define CMNewDateTime(b,rc)           ((b)->eft->newDateTime((b),(rc)))
#endif

#ifdef CMPI_INLINE
     /** DateTime factory service. Initialized from &lt;binTime&gt;.
         @param mb Broker this pointer
	 @param binTime Date/Time definition in binary format in microsecods
	       starting since 00:00:00 GMT, Jan 1,1970.
 	 @param interval Wenn true, defines Date/Time definition to be an interval value
	 @param rc Output: Service return status (suppressed when NULL).
         @return The newly created DateTime.
     */
  inline static   CMPIDateTime* CMNewDateTimeFromBinary
                 (const CMPIBroker* mb, CMPIUint64 binTime, CMPIBoolean interval,
		  CMPIStatus* rc)
       { return ((mb)->eft->newDateTimeFromBinary((mb),(binTime),(interval),(rc))); }
#else
  #define CMNewDateTimeFromBinary(b,d,i,rc) \
                      ((b)->eft->newDateTimeFromBinary((b),(d),(i),(rc)))
#endif

#ifdef CMPI_INLINE
     /** DateTime factory service. Is initialized from &lt;utcTime&gt;.
         @param mb Broker this pointer
	 @param utcTime Date/Time definition in UTC format
	 @param rc Output: Service return status (suppressed when NULL).
         @return The newly created DateTime.
     */
   inline static   CMPIDateTime* CMNewDateTimeFromChars
                 (const CMPIBroker* mb, char* utcTime, CMPIStatus* rc)
	{ return ((mb)->eft->newDateTimeFromChars((mb),(utcTime),(rc))); }
#else
  #define CMNewDateTimeFromChars(b,d,rc) \
                           ((b)->eft->newDateTimeFromChars((b),(d),(rc)))
#endif

#ifdef CMPI_INLINE
     /** SelectExp factory service. TBD.
         @param mb Broker this pointer
	 @param query The select expression.
	 @param lang The query language.
	 @param projection Output: Projection specification (suppressed when NULL).
	 @param rc Output: Service return status (suppressed when NULL).
         @return The newly created SelectExp.
     */
     inline static   CMPISelectExp* CMNewSelectExp
                 (const CMPIBroker* mb, const char* query, const char* lang,
                  const CMPIArray** projection, CMPIStatus* rc)
	{ return ((mb)->eft->newSelectExp((mb),(query),(lang),(projection),(rc))); }
#else
  #define CMNewSelectExp(b,q,l,p,rc) \
                              ((b)->eft->newSelectExp((b),(q),(l),(p),(rc)))
#endif

#ifdef CMPI_INLINE
     /** Function to determine whether a CIM class is of &lt;type&gt; or any of
         &lt;type&gt; subclasses.
         @param mb Broker this pointer
	 @param op The class path (namespace and classname components).
	 @param type The type to tested for.
	 @param rc Output: Service return status (suppressed when NULL).
         @return True if test successful.
     */
   inline static   CMPIBoolean CMClassPathIsA
                 (const CMPIBroker* mb, const CMPIObjectPath* op, const char* type, CMPIStatus* rc)
	{ return ((mb)->eft->classPathIsA((mb),(op),(type),(rc))); }
#else
  #define CMClassPathIsA(b,p,pn,rc) \
                              ((b)->eft->classPathIsA((b),(p),(pn),(rc)))
#endif

    // Debugging macros

#ifdef CMPI_INLINE
     /** Attempts to transforms an CMPI object to a broker specific string format.
         Intended for debugging purposes only.
         @param mb Broker this pointer
	 @param object A valid CMPI object.
	 @param rc Output: Service return status (suppressed when NULL).
         @return String from representation of &lt;object&gt;.
     */
  inline static   CMPIString* CDToString
                 (const CMPIBroker* mb, const void* object, CMPIStatus* rc)
	{ return ((mb)->eft->toString((mb),(void*)(object),(rc))); }
#else
  #define CDToString(b,o,rc)    ((b)->eft->toString((b),(void*)(o),(rc)))
#endif

#ifdef CMPI_INLINE
     /** Verifies whether &lt;object&gt; is of CMPI type &lt;type&gt;.
         Intended for debugging purposes only.
         @param mb Broker this pointer
	 @param object A valid CMPI object.
	 @param type A string specifying a valid CMPI Object type
	         ("CMPIInstance", "CMPIObjectPath", etc).
	 @param rc Output: Service return status (suppressed when NULL).
         @return True if test successful.
     */
   inline static   CMPIBoolean CDIsOfType
                 (const CMPIBroker* mb, const void* object, const char* type, CMPIStatus* rc)
	{ return ((mb)->eft->isOfType((mb),(void*)(object),(type),(rc))); }
#else
  #define CDIsOfType(b,o,t,rc) \
                             (b)->eft->isOfType((b),(void*)(o),(t),(rc))
#endif

#ifdef CMPI_INLINE
     /** Retrieves the CMPI type of &lt;object&gt;.
         Intended for debugging purposes only.
         @param mb Broker this pointer
	 @param object A valid CMPI object.
	 @param rc Output: Service return status (suppressed when NULL).
         @return CMPI object type.
     */
  inline static   CMPIString* CDGetType
                 (const CMPIBroker* mb, const void* object, CMPIStatus* rc)
	 { return ((mb)->eft->getType((mb),(object),(rc))); }
#else
  #define CDGetType(b,o,rc)      ((b)->eft->getType((b),(void*)(o),(rc)))
#endif

#if defined(CMPI_VER_85)
#ifdef DOC_ONLY
     /** Retrieves translated message. When using as macro, use
         CMFmtArgsX and CMFmtX macros to generate the variable parameter list and ommit
	 the count parameter.
	 @example  CMGetMessage(_broker,"msgid","Test $0 $1",NULL,
	       CMFmtArgs2(CMFmtChars("message"),CMFmtSint(1));
         @param mb Broker this pointer
	 @param msgId The message identifier.
	 @param defMsg The default message. The message can have up to 10 message
	        insert placeholders ($0 through $9). The placeholders will be
		replaced by the corresponding message insert values.
	 @param rc Output: Service return status (suppressed when NULL).
	 @param count The number of message insert values. Ommit when using
	         CMGetMessage as macro.
	 @param ... Up to 10 Message insert values.
	        These are specified using the following macros:
                   CMFmtSint(v)    integer value
                   CMFmtUint(v)    unsigned integer value
                   CMFmtSint64(v)  long integer value
                   CMFmtUint64(v)  long unsigned integer vale
                   CMFmtReal(v)    float or double real value
                   CMFmtBoolean(v) CMPIBoolean value
                   CMFmtChars(v)   char string
                   CMFmtString(v)  CMPIString
         @return the translated message.
     */
  inline static   CMPIString* CMGetMessage
                 (const CMPIBroker* mb, const char *msgId, const char *defMsg, CMPIStatus* rc, unsigned int, ...);
#endif

  #define CMFmtSint(v)    CMPI_sint32,((long int)v)
  #define CMFmtUint(v)    CMPI_uint32,((unsigned long int)v)
  #define CMFmtSint64(v)  CMPI_sint64,((long long int)v)
  #define CMFmtUint64(v)  CMPI_uint64,((unsigned long long int)v)
  #define CMFmtReal(v)    CMPI_real64,((double)v)
  #define CMFmtBoolean(v) CMPI_boolean,((int)v)
  #define CMFmtChars(v)   CMPI_chars,((char*)v)
  #define CMFmtString(v)  CMPI_String,((CMPI_String*)v)

  #define CMFmtArgs0() 0
  #define CMFmtArgs1(v1) \
     1,v1
  #define CMFmtArgs2(v1,v2) \
     2,v1,v2
  #define CMFmtArgs3(v1,v2,v3) \
     3,v1,v2,v3
  #define CMFmtArgs4(v1,v2,v3,v4) \
     4,v1,v2,v3,v4
  #define CMFmtArgs5(v1,v2,v3,v4,v5) \
     5,v1,v2,v3,v4,v5
  #define CMFmtArgs6(v1,v2,v3,v4,v5,v6) \
     6,v1,v2,v3,v4,v5,v6
  #define CMFmtArgs7(v1,v2,v3,v4,v5,v6,v7) \
     7,v1,v2,v3,v4,v5,v6,v7
  #define CMFmtArgs8(v1,v2,v3,v4,v5,v6,v7,v8) \
     8,v1,v2,v3,v4,v5,v6,v7,v8
  #define CMFmtArgs9(v1,v2,v3,v4,v5,v6,v7,v8,v9) \
     9,v1,v2,v3,v4,v5,v6,v7,v8,v9
  #define CMFmtArgs10(v1,v2,v3,v4,v5,v6,v7,v8,v9,v10) \
     10,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10

  #define CMGetMessage(b,id,def,rc,parms)      ((b)->eft->getMessage((b),(id),(def),(rc),parms))
#endif //CMPI_VER_85


#ifdef CMPI_VER_100
#ifdef CMPI_INLINE
       /** IBMKR
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Property value.
      */
   inline static   CMPIStatus CMLogMessage
              (const CMPIBroker *b, int severity, const char *id,
			   const char *text, const CMPIString *string)
	{ return ((b)->eft->logMessage((b),(severity),(id), (text), (string))); }
#else
  #define CMLogMessage(b,severity,id, text, string)      ((b)->eft->logMessage((b),(severity),(id),(text),(string)))
#endif
#endif

#ifdef CMPI_VER_100
#ifdef CMPI_INLINE
       /** IBMKR
	 @return Property value.
      */
   inline static   CMPIStatus CMTraceMessage
              (const CMPIBroker *b, int level, const char *component,
			   const char *text, const CMPIString *string)
	{ return ((b)->eft->logMessage((b),(level),(component), (text), (string))); }
#else
  #define CMTraceMessage(b,level,component, text, string)      ((b)->eft->trace((b),(level),(component),(text),(string)))
#endif
#endif

    // CMPIInstance macros


#ifdef CMPI_INLINE
       /** Gets a named property value.
	 @param inst Instance this pointer.
	 @param name Property name.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Property value.
      */
   inline static   CMPIData CMGetProperty
              (const CMPIInstance* inst, const char* name, CMPIStatus* rc)
	{ return ((inst)->ft->getProperty((inst),(name),(rc))); }
#else
  #define CMGetProperty(i,n,rc)      ((i)->ft->getProperty((i),(n),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Gets a Property value defined by its index.
	 @param inst Instance this pointer.
	 @param index Position in the internal Data array.
	 @param name Output: Returned property name (suppressed when NULL).
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Property value.
      */
   inline static   CMPIData CMGetPropertyAt
              (const CMPIInstance* inst, unsigned int index, CMPIString** name,
	       CMPIStatus* rc)
	{ return ((inst)->ft->getPropertyAt((inst),(index),(name),(rc))); }
#else
 #define CMGetPropertyAt(i,num,s,rc) \
                  ((i)->ft->getPropertyAt((i),(num),(s),(rc)))
#endif

#ifdef CMPI_INLINE
      /** Adds/replaces a named Property.
	 @param inst Instance this pointer.
         @param name Entry name.
         @param value Address of value structure.
         @param type Value type.
	 @return Service return status.
      */
  inline static   CMPIStatus CMSetProperty
              (const CMPIInstance* inst,const  char* name,
               const CMPIValue* value, CMPIType type)
	{ return ((inst)->ft->setProperty((inst),(name),(value),(type))); }
#else
  #define CMSetProperty(i,n,v,t) \
                      ((i)->ft->setProperty((i),(n),(CMPIValue*)(v),(t)))
#endif

#ifdef CMPI_INLINE
      /** Gets the number of properties contained in this Instance.
	 @param inst Instance this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Number of properties.
      */
     inline static   unsigned int CMGetPropertyCount
              (const CMPIInstance* inst, CMPIStatus* rc)
	 { return ((inst)->ft->getPropertyCount((inst),(rc))); }
#else
  #define CMGetPropertyCount(i,rc)   ((i)->ft->getPropertyCount((i),(rc)))
#endif

#ifdef CMPI_INLINE
      /** Generates an ObjectPath out of the namespace, classname and
	  key propeties of this Instance.
	 @param inst Instance this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
         @return the generated ObjectPath.
      */
     inline static   CMPIObjectPath* CMGetObjectPath
              (const CMPIInstance* inst, CMPIStatus* rc)
	{ return ((inst)->ft->getObjectPath((inst),(rc))); }
#else
  #define CMGetObjectPath(i,rc)        ((i)->ft->getObjectPath((i),(rc)))
#endif

#ifdef CMPI_VER_100
#ifdef CMPI_INLINE
	/* IBMKR
      */
     inline static   CMPIStatus CMSetObjectPath
              ( CMPIInstance* inst, const CMPIObjectPath *obj)
	{ return ((inst)->ft->setObjectPath((inst),(obj))); }
#else
  #define CMSetObjectPath(i,obj)        ((i)->ft->getObjectPath((i),(obj)))
#endif
#endif

#ifdef CMPI_INLINE
      /** Directs CMPI to ignore any setProperty operations for this
	  instance for any properties not in this list.
	 @param inst Instance this pointer.
	 @param propertyList If not NULL, the members of the array define one
	     or more Property names to be accepted by setProperty operations.
	 @param keys array of key properties. This array must be specified.
	 @return Service return status.
      */
     inline static   CMPIStatus CMSetPropertyFilter
              (CMPIInstance* inst, const char** propertyList, char **keys)
	{ return ((inst)->ft->setPropertyFilter((inst),(propertyList),(keys))); }
#else
  #define CMSetPropertyFilter(i,pl,k) ((i)->ft->setPropertyFilter((i),(pl),(k)))
#endif



   // CMPIObjectPath macros


#ifdef CMPI_INLINE
       /** Set/replace the hostname component.
	 @param op ObjectPath this pointer.
	 @param hn The hostname string
	 @return Service return status.
      */
     inline static   CMPIStatus CMSetHostname
              (CMPIObjectPath* op, const char* hn)
	{ return ((op)->ft->setHostname((op),(hn))); }
#else
  #define CMSetHostname(p,n)              ((p)->ft->setHostname((p),(n)))
#endif

#ifdef CMPI_INLINE
       /** Get the hostname component.
	 @param op ObjectPath this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return The hostname component.
      */
   inline static   CMPIString* CMGetHostname
              (const CMPIObjectPath* op,CMPIStatus* rc)
	{ return ((op)->ft->getHostname((op),(rc))); }
#else
  #define CMGetHostname(p,rc)            ((p)->ft->getHostname((p),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Set/replace the namespace component.
	 @param op ObjectPath this pointer.
	 @param ns The namespace string
	 @return Service return status.
      */
   inline static   CMPIStatus CMSetNameSpace
              (CMPIObjectPath* op,const char* ns)
	 { return ((op)->ft->setNameSpace((op),(ns))); }
#else
  #define CMSetNameSpace(p,n)            ((p)->ft->setNameSpace((p),(n)))
#endif

#ifdef CMPI_INLINE
       /** Get the namespace component.
	 @param op ObjectPath this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return The namespace component.
      */
   inline static   CMPIString* CMGetNameSpace
              (const CMPIObjectPath* op, CMPIStatus* rc)
	{ return ((op)->ft->getNameSpace((op),(rc))); }
#else
  #define CMGetNameSpace(p,rc)          ((p)->ft->getNameSpace((p),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Set/replace the classname component.
	 @param op ObjectPath this pointer.
	 @param cn The hostname string
	 @return Service return status.
      */
   inline static   CMPIStatus CMSetClassName
                 (CMPIObjectPath* op, const char* cn)
	{ return ((op)->ft->setClassName((op),(cn))); }
#else
  #define CMSetClassName(p,n)            ((p)->ft->setClassName((p),(n)))
#endif

#ifdef CMPI_INLINE
       /** Get the classname component.
	 @param op ObjectPath this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return The classname component.
      */
   inline static   CMPIString* CMGetClassName
              (const CMPIObjectPath* op,CMPIStatus* rc)
	{ return ((op)->ft->getClassName((op),(rc))); }
#else
  #define CMGetClassName(p,rc)          ((p)->ft->getClassName((p),(rc)))
#endif

#ifdef CMPI_INLINE
      /** Adds/replaces a named key property.
	 @param op ObjectPath this pointer.
         @param name Key property name.
         @param value Address of value structure.
         @param type Value type.
	 @return Service return status.
      */
   inline static   CMPIStatus CMAddKey
              (CMPIObjectPath* op, const char* name,
               const CMPIValue * value, const CMPIType type)
	{ return ((op)->ft->addKey((op),(name),(value),(type))); }
#else
  #define CMAddKey(p,n,v,t) \
                           ((p)->ft->addKey((p),(n),(CMPIValue*)(v),(t)))
#endif

#ifdef CMPI_INLINE
       /** Gets a named key property value.
	 @param op ObjectPath this pointer.
	 @param name Key property name.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Entry value.
      */
   inline static   CMPIData CMGetKey
              (const CMPIObjectPath* op, const char* name, CMPIStatus* rc)
	{ return ((op)->ft->getKey((op),(name),(rc))); }
#else
  #define CMGetKey(p,n,rc)                ((p)->ft->getKey((p),(n),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Gets a key property value defined by its index.
	 @param op ObjectPath this pointer.
	 @param index Position in the internal Data array.
	 @param name Output: Returned property name (suppressed when NULL).
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Data value.
      */
   inline static   CMPIData CMGetKeyAt
              (const CMPIObjectPath* op,unsigned int index, CMPIString** name,
	       CMPIStatus* rc)
	{ return ((op)->ft->getKeyAt((op),(index),(name),(rc))); }
#else
  #define CMGetKeyAt(p,i,n,rc)          ((p)->ft->getKeyAt((p),(i),(n),(rc)))
#endif


#ifdef CMPI_INLINE
      /** Gets the number of key properties contained in this ObjectPath.
	 @param op ObjectPath this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Number of properties.
      */
   inline static   unsigned int CMGetKeyCount
              (const CMPIObjectPath* op, CMPIStatus* rc)
	{ return ((op)->ft->getKeyCount((op),(rc))); }
#else
  #define CMGetKeyCount(p,rc)            ((p)->ft->getKeyCount((p),(rc)))
#endif

#ifdef CMPI_INLINE
      /** Set/replace namespace and classname components from &lt;src&gt;.
	 @param op ObjectPath this pointer.
	 @param src Source input.
	 @return Service return status.
      */
   inline static   CMPIStatus CMSetNameSpaceFromObjectPath
              (CMPIObjectPath* op, const CMPIObjectPath* src)
	{ return ((op)->ft->setNameSpaceFromObjectPath((op),(src))); }
#else
  #define CMSetNameSpaceFromObjectPath(p,s) \
                           ((p)->ft->setNameSpaceFromObjectPath((p),(s)))
#endif

#ifdef CMPI_INLINE
      /** Set/replace hostname, namespace and classname components from &lt;src&gt;.
	 @param op ObjectPath this pointer.
	 @param src Source input.
	 @return Service return status.
      */
   inline static   CMPIStatus CMSetHostAndNameSpaceFromObjectPath
              (CMPIObjectPath* op,
               const CMPIObjectPath* src)
	{ return ((op)->ft->setHostAndNameSpaceFromObjectPath((op),(src))); }
#else
  #define CMSetHostAndNameSpaceFromObjectPath(p,s) \
                     ((p)->ft->setHostAndNameSpaceFromObjectPath((p),(s)))
#endif

#if defined(CMPI_VER_86)
#ifdef CMPI_INLINE
      /** Set/replace hostname, namespace and classname components from &lt;src&gt;.
	 @param op ObjectPath this pointer.
	 @param src Source input.
	 @return Service return status.
      */
   inline static   CMPIString* CMObjectPathToString
              (const CMPIObjectPath* op, CMPIStatus* rc)
	{ return ((op)->ft->toString((op),(rc))); }
#else
  #define CMObjectPathToString(p,rc) \
                     ((p)->ft->toString((p),(rc)))
#endif
#endif

    // CMPIArray macros


#ifdef CMPI_INLINE
      /** Gets the number of elements contained in this Array.
	 @param ar Array this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Number of elements.
      */
   inline static   CMPICount CMGetArrayCount
             (const CMPIArray* ar, CMPIStatus* rc)
	{ return ((ar)->ft->getSize((ar),(rc))); }
#else
   #define CMGetArrayCount(a,rc)             ((a)->ft->getSize((a),(rc)))
#endif

#ifdef CMPI_INLINE
      /** Gets the element type.
	 @param ar Array this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Number of elements.
      */
   inline static   CMPIType CMGetArrayType
             (const CMPIArray* ar, CMPIStatus* rc)
	{ return ((ar)->ft->getSimpleType((ar),(rc))); }
#else
   #define CMGetArrayType(a,rc)        ((a)->ft->getSimpleType((a),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Gets an element value defined by its index.
	 @param ar Array this pointer.
	 @param index Position in the internal Data array.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Element value.
      */
   inline static   CMPIData CMGetArrayElementAt
             (const CMPIArray* ar, CMPICount index, CMPIStatus* rc)
	{ return ((ar)->ft->getElementAt((ar),(index),(rc))); }
#else
   #define CMGetArrayElementAt(a,n,rc) \
                                    ((a)->ft->getElementAt((a),(n),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Sets an element value defined by its index.
	 @param ar Array this pointer.
	 @param index Position in the internal Data array.
         @param value Address of value structure.
         @param type Value type.
	 @return Service return status.
      */
   inline static   CMPIStatus CMSetArrayElementAt
             (CMPIArray* ar, CMPICount index, const CMPIValue* value, CMPIType type)
	{ return ((ar)->ft->setElementAt((ar),(index),(value),(type))); }
#else
   #define CMSetArrayElementAt(a,n,v,t) \
                     ((a)->ft->setElementAt((a),(n),(CMPIValue*)(v),(t)))
#endif



    // CMPIArgs macros


#ifdef CMPI_INLINE
      /** Adds/replaces a named argument.
	 @param as Args this pointer.
         @param name Argument name.
         @param value Address of value structure.
         @param type Value type.
	 @return Service return status.
      */
   inline static   CMPIStatus CMAddArg
              (CMPIArgs* as, char* name ,void* value,
               CMPIType type)
	{ return  ((as)->ft->addArg((as),(name),(CMPIValue*)(value),(type))); }
#else
  #define CMAddArg(a,n,v,t) \
                           ((a)->ft->addArg((a),(n),(CMPIValue*)(v),(t)))
#endif

#ifdef CMPI_INLINE
       /** Gets a named argument value.
	 @param as Args this pointer.
	 @param name Argument name.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Argument value.
      */
  inline static   CMPIData CMGetArg
              (const CMPIArgs* as, const char* name, CMPIStatus* rc)
	{ return  ((as)->ft->getArg((as),(name),(rc))); }
#else
  #define CMGetArg(a,n,rc)                ((a)->ft->getArg((a),(n),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Gets a Argument value defined by its index.
	 @param as Args this pointer.
	 @param index Position in the internal Data array.
	 @param name Output: Returned argument name (suppressed when NULL).
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Argument value.
      */
   inline static   CMPIData CMGetArgAt
              (const CMPIArgs* as, unsigned int index, CMPIString** name,
	       CMPIStatus* rc)
	{ return  ((as)->ft->getArgAt((as),(index),(name),(rc))); }
#else
  #define CMGetArgAt(a,p,n,rc)       ((a)->ft->getArgAt((a),(p),(n),(rc)))
#endif

#ifdef CMPI_INLINE
      /** Gets the number of arguments contained in this Args.
	 @param as Args this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Number of properties.
      */
   inline static   unsigned int CMGetArgCount
              (const CMPIArgs* as, CMPIStatus* rc)
	{ return  ((as)->ft->getArgCount((as),(rc))); }
#else
  #define CMGetArgCount(a,rc)            ((a)->ft->getArgCount((a),(rc)))
#endif



    // CMPIString Macros


#ifdef CMPI_INLINE
       /** Get a pointer to a C char* representation of this String.
	 @param st String this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Pointer to char* representation.
      */
   inline static   char* CMGetCharsPtr
             (const CMPIString* st, CMPIStatus* rc)
	{ return  ((st)->ft->getCharPtr((st),(rc))); }
#else
  #define CMGetCharsPtr(st,rc)              ((st)->ft->getCharPtr((st),(rc)))
#endif



    // CMPIDateTime macros


#ifdef CMPI_INLINE
       /** Get DateTime setting in UTC string format.
	 @param dt DateTime this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return DateTime as UTC string.
      */
   inline static   CMPIString* CMGetStringFormat
             (const CMPIDateTime* dt, CMPIStatus* rc)
	{ return ((dt)->ft->getStringFormat((dt),(rc))); }
#else
  #define CMGetStringFormat(d,rc)    ((d)->ft->getStringFormat((d),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Get DateTime setting in binary format (in microsecods
	       starting since 00:00:00 GMT, Jan 1,1970).
	 @param dt DateTime this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return DateTime in binary.
      */
   inline static   CMPIUint64 CMGetBinaryFormat
             (const CMPIDateTime* dt, CMPIStatus* rc)
	{ return ((dt)->ft->getBinaryFormat((dt),(rc))); }
#else
  #define CMGetBinaryFormat(d,rc)    ((d)->ft->getBinaryFormat((d),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Tests whether DateTime is an interval value.
	 @param dt DateTime this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return True if interval value.
      */
   inline static   CMPIBoolean CMIsInterval
              (const CMPIDateTime* dt, CMPIStatus* rc)
	{ return ((dt)->ft->isInterval((dt),(rc))); }
#else
  #define CMIsInterval(d,rc)              ((d)->ft->isInterval((d),(rc)))
#endif


    // CMPIEnumeration Macros


#ifdef CMPI_INLINE
       /** Get the next element of this Enumeration.
	 @param en Enumeration this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Element value.
      */
   inline static   CMPIData CMGetNext
             (const CMPIEnumeration* en, CMPIStatus* rc)
	{ return ((en)->ft->getNext((en),(rc))); }
#else
  #define CMGetNext(n,rc)                    ((n)->ft->getNext((n),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Test for any elements left in this Enumeration.
	 @param en Enumeration this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return True or false.
      */
   inline static   CMPIBoolean CMHasNext
             (const CMPIEnumeration* en, CMPIStatus* rc)
	{ return ((en)->ft->hasNext((en),(rc))); }
#else
  #define CMHasNext(n,rc)                    ((n)->ft->hasNext((n),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Convert this Enumeration into an Array.
	 @param en Enumeration this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return The Array.
      */
   inline static   CMPIArray* CMToArray
             (const CMPIEnumeration* en, CMPIStatus* rc)
	{ return ((en)->ft->toArray((en),(rc))); }
#else
  #define CMToArray(n,rc)                   ((n)->ft->toArray((n),(rc)))
#endif


   // CMPIResult Macros


#ifdef CMPI_INLINE
       /** Return a value/type pair.
	 @param rslt Result this pointer.
	 @param value Address of a Value object.
	 @param type Type of the Value object.
	 @return Service return status.
      */
  inline static   CMPIStatus CMReturnData
              (const CMPIResult* rslt, const CMPIValue* value, const CMPIType type)
	{ return ((rslt)->ft->returnData((rslt),(value),(type))); }
#else
  #define CMReturnData(r,v,t) \
                           ((r)->ft->returnData((r),(CMPIValue*)(v),(t)))
#endif

#ifdef CMPI_INLINE
       /** Return a Instance object.
	 @param rslt Result this pointer.
	 @param inst Instance to be returned.
	 @return Service return status.
      */
   inline static   CMPIStatus CMReturnInstance
              (const CMPIResult* rslt,const CMPIInstance* inst)
	{ return ((rslt)->ft->returnInstance((rslt),(inst))); }
#else
  #define CMReturnInstance(r,i)        ((r)->ft->returnInstance((r),(i)))
#endif

#ifdef CMPI_INLINE
       /** Return a ObjectPath object..
	 @param rslt Result this pointer.
	 @param ref ObjectPath to be returned.
	 @return Service return status.
      */
   inline static   CMPIStatus CMReturnObjectPath
              (const CMPIResult* rslt, const CMPIObjectPath* ref)
	{ return ((rslt)->ft->returnObjectPath((rslt),(ref))); }
#else
  #define CMReturnObjectPath(r,o)    ((r)->ft->returnObjectPath((r),(o)))
#endif

#ifdef CMPI_INLINE
       /** Indicates no further data to be returned.
	 @param rslt Result this pointer.
	 @return Service return status.
      */
   inline static   CMPIStatus CMReturnDone
              (const CMPIResult* rslt)
	{ return ((rslt)->ft->returnDone((rslt))); }
#else
  #define CMReturnDone(r)                      ((r)->ft->returnDone((r)))
#endif



    // CMPIContext Macros


#ifdef CMPI_INLINE
       /** Gets a named Context entry value.
	 @param ctx Context this pointer.
	 @param name Context entry name.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Entry value.
      */
   inline static   CMPIData CMGetContextEntry
              (const CMPIContext* ctx, const char* name, CMPIStatus* rc)
	{ return ((ctx)->ft->getEntry((ctx),(name),(rc))); }
#else
  #define CMGetContextEntry(c,n,rc)  \
                                 ((c)->ft->getEntry((c),(n),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Gets a Context entry value defined by its index.
	 @param ctx Context this pointer.
	 @param index Position in the internal Data array.
	 @param name Output: Returned Context entry name (suppressed when NULL).
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Entry value.
      */
   inline static   CMPIData CMGetContextEntryAt
              (const CMPIContext* ctx, unsigned int index, CMPIString** name,
	       CMPIStatus* rc)
	{ return ((ctx)->ft->getEntryAt((ctx),(index),(name),(rc))); }
#else
  #define CMGetContextEntryAt(c,p,n,rc) \
                         ((c)->ft->getEntryAt((c),(p),(n),(rc)))
#endif

#ifdef CMPI_INLINE
      /** Gets the number of entries contained in this Context.
	 @param ctx Context this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Number of entries.
      */
   inline static   unsigned int CMGetContextEntryCount
              (const CMPIContext* ctx, CMPIStatus* rc)
	{ return ((ctx)->ft->getEntryCount((ctx),(rc))); }
#else
  #define CMGetContextEntryCount(c,rc) \
                                ((c)->ft->getEntryCount((c),(rc)))
#endif

#ifdef CMPI_INLINE
      /** adds/replaces a named Context entry
	 @param ctx Context this pointer.
         @param name Entry name.
         @param value Address of value structure.
         @param type Value type.
	 @return Service return status.
      */
   inline static   CMPIStatus CMAddContextEntry
              (const CMPIContext* ctx, const char* name, const CMPIValue* value, const CMPIType type)
	{ return ((ctx)->ft->addEntry((ctx),(name),(value),(type))); }
#else
  #define CMAddContextEntry(c,n,v,t) \
                  ((c)->ft->addEntry((c),(n),(CMPIValue*)(v),(t)))
#endif



    // CMPISelectExp macros



#ifdef CMPI_INLINE
       /** Return the select expression in string format.
	 @param se SelectExp this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return The select expression.
      */
   inline static   CMPIString* CMGetSelExpString
              (const CMPISelectExp* se, CMPIStatus* rc)
	{ return ((se)->ft->getString((se),(rc))); }
#else
  #define CMGetSelExpString(s,rc)          ((s)->ft->getString((s),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Evaluate the instance using this select expression.
	 @param se SelectExp this pointer.
	 @param inst Instance to be evaluated.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return True or false incicator.
      */
   inline static   CMPIBoolean CMEvaluateSelExp
              (const CMPISelectExp* se, const CMPIInstance* inst, CMPIStatus* rc)
	{ return ((se)->ft->evaluate((se),(inst),(rc))); }
#else
  #define CMEvaluateSelExp(s,i,r)        ((s)->ft->evaluate((s),(i),(r)))
#endif

#if defined(CMPI_VER_87)
#ifdef CMPI_INLINE
       /** Evaluate this select expression by using a data value accessor routine. .
	 @param se SelectExp this pointer.
	 @param accessor Data accessor routine to be used.
	 @param parm Data accessor parameter.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return True or false incicator.
      */
   inline static   CMPIBoolean CMEvaluateSelExpUsingAccessor
              (const CMPISelectExp* se, CMPIAccessor* accessor, void *parm, CMPIStatus* rc)
	{ return ((se)->ft->evaluateUsingAccessor((se),(accessor),(parm),(rc))); }
#else
  #define CMEvaluateSelExpUsingAccessor(s,i,p,r) \
                            ((s)->ft->evaluateUsingAccessor((s),(i),(p),(r)))
#endif
#endif

#ifdef CMPI_INLINE
       /** Return the select expression as disjunction of conjunctions.
	 @param se SelectExp this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return The disjunction.
      */
   inline static   CMPISelectCond* CMGetDoc
              (const CMPISelectExp* se, CMPIStatus* rc)
	{ return ((se)->ft->getDOC((se),(rc))); }
#else
  #define CMGetDoc(s,rc)                      ((s)->ft->getDOC((s),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Return the select expression as conjunction of disjunctions.
	 @param se SelectExp this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return The conjunction.
      */
   inline static   CMPISelectCond* CMGetCod
              (const CMPISelectExp* se, CMPIStatus* rc)
	{ return ((se)->ft->getCOD((se),(rc))); }
#else
  #define CMGetCod(s,rc)                      ((s)->ft->getCOD((s),(rc)))
#endif



    // CMPISelectCond macros



#ifdef CMPI_INLINE
       /** Return the number of sub conditions that are partof this SelectCond.
           Optionally, the SelectCond type (COD or DOC) will be returned.
	 @param sc SelectCond this pointer.
	 @param type Output: SelectCond type (suppressed when NULL).
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Number of SubCond elements.
      */
   inline static   CMPICount CMGetSubCondCountAndType
              (const CMPISelectCond* sc, int* type, CMPIStatus* rc)
	{ return ((sc)->ft->getCountAndType((sc),(type),(rc))); }
#else
  #define CMGetSubCondCountAndType(c,t,rc) \
                                ((c)->ft->getCountAndType((c),(t),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Return a SubCond element based on its index.
	 @param sc SelectCond this pointer.
	 @param index Position in the internal SubCoind array.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return The indexed SubCond element.
      */
   inline static   CMPISubCond* CMGetSubCondAt
              (const CMPISelectCond* sc, unsigned int index, CMPIStatus* rc)
	{ return ((sc)->ft->getSubCondAt((sc),(index),(rc))); }
#else
  #define CMGetSubCondAt(c,p,rc)    ((c)->ft->getSubCondAt((c),(p),(rc)))
#endif



    // CMPISubCond macros



#ifdef CMPI_INLINE
       /** Return the number of predicates that are part of sub condition.
	 @param sc SubCond this pointer.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Number of Predicate elements.
      */
   inline static   CMPICount CMGetPredicateCount
              (const CMPISubCond* sc, CMPIStatus* rc)
	{ return ((sc)->ft->getCount((sc),(rc))); }
#else
  #define CMGetPredicateCount(s,rc)         ((s)->ft->getCount((s),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Return a Predicate element based on its index.
	 @param sc SubCond this pointer.
	 @param index Position in the internal Predicate array.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return The indexed Predicate element.
      */
   inline static   CMPIPredicate* CMGetPredicateAt
              (const CMPISubCond* sc, unsigned int index, CMPIStatus* rc)
	{ return ((sc)->ft->getPredicateAt((sc),(index),(rc))); }
#else
   #define CMGetPredicateAt(s,p,rc) \
                                  ((s)->ft->getPredicateAt((s),(p),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Return a named Predicate element.
	 @param sc SubCond this pointer.
	 @param name Predicate name (property name).
	 @param rc Output: Service return status (suppressed when NULL).
	 @return The named Predicate element.
      */
   inline static   CMPIPredicate* CMGetPredicate
              (const CMPISubCond* sc, const char* name, CMPIStatus* rc)
	{ return ((sc)->ft->getPredicate((sc),(name),(rc))); }
#else
  #define CMGetPredicate(s,n,rc)    ((s)->ft->getPredicate((s),(n),(rc)))
#endif



    // CMPIPredicate macros


#ifdef CMPI_INLINE
       /** Get the predicate components.
	 @param pr Predicate this pointer.
	 @param type Property type.
	 @param op Predicate operation.
	 @param lhs Left hand side of predicate.
	 @param rhs Right hand side of predicate.
	 @return Service return status.
      */
   inline static   CMPIStatus CMGetPredicateData
              (const CMPIPredicate* pr, CMPIType* type,
               CMPIPredOp* op, CMPIString** lhs, CMPIString** rhs)
	{ return ((pr)->ft->getData((pr),(type),(op),(lhs),(rhs))); }
#else
  #define CMGetPredicateData(p,t,o,n,v) \
                                  ((p)->ft->getData((p),(t),(o),(n),(v)))
#endif

#if defined(CMPI_VER_87) && !defined(CMPI_VER_100)
#ifdef CMPI_INLINE
       /** Evaluate the predicate using a specific value.
	 @param pr Predicate this pointer.
	 @param type Property type.
	 @param value Address of value structure.
	 @param type Value type.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Evaluation result.
      */
   inline static   int CMEvaluatePredicate
              (CMPIPredicate* pr, void* value,
               CMPIType type, CMPIStatus* rc)
	{ return ((pr)->ft->evaluate((pr),(CMPIValue*)(value),(type),(rc))); }
#else
  #define CMEvaluatePredicate(p,v,t,rc) \
                                  ((p)->ft->evaluate((p),(CMPIValue*)(v),(t),(rc)))
#endif
#endif

#if defined(CMPI_VER_100)
#ifdef CMPI_INLINE
       /** Evaluate the predicate using a specific value.
IBMKR
	 @param pr Predicate this pointer.
	 @param type Property type.
	 @param value Address of value structure.
	 @param type Value type.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Evaluation result.
      */
   inline static   int CMEvaluatePredicateUsingAccessor
              (const CMPIPredicate* pr, CMPIAccessor *accessor, void* parm,
               CMPIStatus* rc)
	{ return ((pr)->ft->evaluateUsingAccessor((pr),(accessor), (parm), (rc))); }
#else
  #define CMEvaluatePredicateUsingAccessor(p,a,parm,rc) \
                                  ((p)->ft->evaluateUsingAccessor((p),(a),(parm),(rc)))
#endif

#endif // CMPI_VER_100



    // CMPIBroker Macros



#ifdef CMPI_INLINE
     /** 32 bits describing CMPI features supported by this CIMOM.
         See CMPI_MB_Class_x and CMPI_MB_Supports_xxx flags.
     */
   inline static   unsigned long CBGetClassification(const CMPIBroker* mb)
	{ return ((mb)->bft->brokerClassification); }
#else
  #define CBGetClassification(b)         ((b)->bft->brokerClassification)
#endif

#ifdef CMPI_INLINE
     /** CIMOM version as defined by CIMOM
     */
   inline static   int CBBrokerVersion(const CMPIBroker* mb)
	{ return ((mb)->bft->brokerVersion); }
#else
  #define CBBrokerVersion(b)                    ((b)->bft->brokerVersion)
#endif

#ifdef CMPI_INLINE
     /** CIMOM name
     */
   inline static   char* CBBrokerName(CMPIBroker* mb)
	{ return ((mb)->bft->brokerName); }
#else
  #define CBBrokerName(b)                          ((b)->bft->brokerName)
#endif


#ifdef CMPI_INLINE
     /** This function prepares the CMPI run time system to accept
         a thread that will be using CMPI services. The returned
	 CMPIContext object must be used by the subsequent attachThread()
	 and detachThread() invocations.
	 @param mb Broker this pointer.
	 @param ctx Old Context object
	 @return New Context object to be used by thread to be attached.
     */
   inline static   CMPIContext* CBPrepareAttachThread
                (const CMPIBroker* mb, const CMPIContext* ctx)
	{ return ((mb)->bft->prepareAttachThread((mb),(ctx))); }
#else
  #define CBPrepareAttachThread(b,c) \
                                 ((b)->bft->prepareAttachThread((b),(c)))
#endif

#ifdef CMPI_INLINE
      /** This function informs the CMPI run time system that the current
         thread with Context will begin using CMPI services.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @return Service return status.
     */
   inline static   CMPIStatus CBAttachThread
                (const CMPIBroker* mb, const CMPIContext* ctx)
	{ return ((mb)->bft->attachThread((mb),(ctx))); }
#else
  #define CBAttachThread(b,c)           ((b)->bft->attachThread((b),(c)))
#endif

#ifdef CMPI_INLINE
      /** This function informs the CMPI run time system that the current thread
         will not be using CMPI services anymore. The Context object will be
	 freed during this operation.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @return Service return status.
     */
   inline static   CMPIStatus CBDetachThread
                (const CMPIBroker* mb, const CMPIContext* ctx)
	{ return ((mb)->bft->detachThread((mb),(ctx))); }
#else
  #define CBDetachThread(b,c)           ((b)->bft->detachThread((b),(c)))
#endif



#ifdef CMPI_INLINE
      /** This function requests delivery of an Indication. The CIMOM will
         locate pertinent subscribers and notify them about the event.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param ns Namespace
	 @param ind Indication Instance
	 @return Service return status.
     */
   inline static   CMPIStatus CBDeliverIndication
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const char* ns, const CMPIInstance* ind)
	{ return ((mb)->bft->deliverIndication((mb),(ctx),(ns),(ind))); }
#else
  #define CBDeliverIndication(b,c,n,i) \
                           ((b)->bft->deliverIndication((b),(c),(n),(i)))
#endif

#ifdef CMPI_INLINE
      /** Enumerate Instance Names of the class (and subclasses) defined by &lt;op&gt;.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op ObjectPath containing namespace and classname components.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Enumeration of ObjectPathes.
     */
   inline static   CMPIEnumeration* CBEnumInstanceNames
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op, CMPIStatus* rc)
	{ return ((mb)->bft->enumerateInstanceNames((mb),(ctx),(op),(rc))); }
#else
  #define CBEnumInstanceNames(b,c,p,rc) \
                          ((b)->bft->enumerateInstanceNames((b),(c),(p),(rc)))
#endif

#ifdef CMPI_INLINE
     /** Enumerate Instances of the class (and subclasses) defined by &lt;op&gt;.
         Instance structure and inheritance scope can be controled using the
	 CMPIInvocationFlags entry in &lt;ctx&gt;.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op ObjectPath containing namespace and classname components.
	 @param properties If not NULL, the members of the array define one or more Property
	     names. Each returned Object MUST NOT include elements for any Properties
	     missing from this list
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Enumeration of Instances.
     */
   inline static   CMPIEnumeration* CBEnumInstances
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op, const char** properties, CMPIStatus* rc)
	{ return ((mb)->bft->enumerateInstances((mb),(ctx),(op),(properties),(rc))); }
#else
  #define CBEnumInstances(b,c,p,pr,rc) \
                         ((b)->bft->enumerateInstances((b),(c),(p),(pr),(rc)))
#endif

#ifdef CMPI_INLINE
      /** Get Instance using &lt;op&gt; as reference. Instance structure can be
         controled using the CMPIInvocationFlags entry in &lt;ctx&gt;.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op ObjectPath containing namespace, classname and key components.
	 @param properties If not NULL, the members of the array define one or more Property
	     names. Each returned Object MUST NOT include elements for any Properties
	     missing from this list
	 @param rc Output: Service return status (suppressed when NULL).
	 @return The Instance.
     */
   inline static   CMPIInstance* CBGetInstance
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op, const char** properties, CMPIStatus* rc)
	{ return ((mb)->bft->getInstance((mb),(ctx),(op),(properties),(rc))); }
#else
  #define CBGetInstance(b,c,p,pr,rc) \
                           ((b)->bft->getInstance((b),(c),(p),(pr),(rc)))
#endif

#ifdef CMPI_INLINE
      /** Create Instance from &lt;inst&gt; using &lt;op&gt; as reference.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op ObjectPath containing namespace, classname and key components.
	 @param inst Complete instance.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return The assigned instance reference.
     */
   inline static   CMPIObjectPath* CBCreateInstance
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op, const CMPIInstance* inst, CMPIStatus* rc)
	{ return ((mb)->bft->createInstance((mb),(ctx),(op),(inst),(rc))); }
#else
  #define CBCreateInstance(b,c,p,i,rc) \
                             ((b)->bft->createInstance((b),(c),(p),(i),(rc)))
#endif

#if defined(CMPI_VER_90)
 
  #ifdef CMPI_INLINE
      /** Replace an existing Instance from &lt;inst&gt; using &lt;op&gt; as reference.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op ObjectPath containing namespace, classname and key components.
	 @param inst Complete instance.
	 @param properties Specifies which properties to set. All properties will be ste if NULL.
	 @return Service return status.
     */
   #if defined(CMPI_VER_100)
   inline static   CMPIStatus CBModifyInstance
                (const CMPIBroker* mb, const CMPIContext* ctx,
		 		const CMPIObjectPath* op, const CMPIInstance* inst, const char** properties)
	{ return ((mb)->bft->modifyInstance((mb),(ctx),(op),(inst),(properties))); }
   #else // CMPI_VER_100
   inline static   CMPIStatus CBSetInstance
                (const CMPIBroker* mb, const CMPIContext* ctx,
		 const CMPIObjectPath* op, const CMPIInstance* inst, const char** properties)
	{ return ((mb)->bft->setInstance((mb),(ctx),(op),(inst),(properties))); }
   #endif // CMPI_VER_!00
  #else // CMPI_INLINE
    #if defined(CMPI_VER_100)
      #define CBModifyInstance(b,c,p,i,pr)      ((b)->bft->modifyInstance((b),(c),(p),(i),(pr)))
  #else
    #define CBSetInstance(b,c,p,i,pr)      ((b)->bft->setInstance((b),(c),(p),(i),(pr)))
    #endif //CMPI_VER_100
  #endif
#else
  #ifdef CMPI_INLINE
      /** Replace an existing Instance from &lt;inst&gt; using &lt;op&gt; as reference.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op ObjectPath containing namespace, classname and key components.
	 @param inst Complete instance.
	 @return Service return status.
     */
   inline static   CMPIStatus CBSetInstance
                (CMPIBroker* mb, CMPIContext* ctx,
		 CMPIObjectPath* op, CMPIInstance* inst)
	{ return ((mb)->bft->setInstance((mb),(ctx),(op),(inst))); }
  #else
  #define CBSetInstance(b,c,p,i)      ((b)->bft->setInstance((b),(c),(p),(i)))
  #endif
#endif //version 90

#ifdef CMPI_INLINE
      /** Delete an existing Instance using &lt;op&gt; as reference.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op ObjectPath containing namespace, classname and key components.
	 @return Service return status.
     */
   inline static   CMPIStatus CBDeleteInstance
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op)
	{ return ((mb)->bft->deleteInstance((mb),(ctx),(op))); }
#else
  #define CBDeleteInstance(b,c,p)  ((b)->bft->deleteInstance((b),(c),(p)))
#endif

#ifdef CMPI_INLINE
      /** Query the enumeration of instances of the class (and subclasses) defined
         by &lt;op&gt; using &lt;query&gt; expression.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op ObjectPath containing namespace and classname components.
	 @param query Query expression
	 @param lang Query Language
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Resulting eumeration of Instances.
     */
   inline static   CMPIEnumeration* CBExecQuery
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op, const char* query, const char* lang, CMPIStatus* rc)
	{ return ((mb)->bft->execQuery((mb),(ctx),(op),(query),(lang),(rc))); }
#else
  #define CBExecQuery(b,c,p,l,q,rc) \
                          ((b)->bft->execQuery((b),(c),(p),(l),(q),(rc)))
#endif


#ifdef CMPI_INLINE
      /** Enumerate instances associated with the Instance defined by the &lt;op&gt;.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op Source ObjectPath containing namespace, classname and key components.
	 @param assocClass If not NULL, MUST be a valid Association Class name.
	    It acts as a filter on the returned set of Objects by mandating that
	    each returned Object MUST be associated to the source Object via an
	    Instance of this Class or one of its subclasses.
	 @param resultClass If not NULL, MUST be a valid Class name.
	    It acts as a filter on the returned set of Objects by mandating that
	    each returned Object MUST be either an Instance of this Class (or one
	    of its subclasses).
	 @param role If not NULL, MUST be a valid Property name.
	    It acts as a filter on the returned set of Objects by mandating
	    that each returned Object MUST be associated to the source Object
	    via an Association in which the source Object plays the specified role
	    (i.e. the name of the Property in the Association Class that refers
	    to the source Object MUST match the value of this parameter).
	 @param resultRole If not NULL, MUST be a valid Property name.
	    It acts as a filter on the returned set of Objects by mandating
	    that each returned Object MUST be associated to the source Object
	    via an Association in which the returned Object plays the specified role
	    (i.e. the name of the Property in the Association Class that refers to
	    the returned Object MUST match the value of this parameter).
	 @param properties If not NULL, the members of the array define one or more Property
	     names. Each returned Object MUST NOT include elements for any Properties
	     missing from this list
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Enumeration of Instances.
     */
   inline static   CMPIEnumeration* CBAssociators
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op, const char* assocClass, const char* resultClass,
		 const char* role, const char* resultRole, const char** properties, CMPIStatus* rc)
	{ return ((mb)->bft->associators((mb),(ctx),(op),(assocClass),(resultClass),
	                                 (role),(resultRole),(properties),(rc))); }
#else
  #define CBAssociators(b,c,p,acl,rcl,r,rr,pr,rc) \
      ((b)->bft->associators((b),(c),(p),(acl),(rcl),(r),(rr),(pr),(rc)))
#endif

#ifdef CMPI_INLINE
      /** Enumerate ObjectPaths associated with the Instance defined by &lt;op&gt;.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op Source ObjectPath containing namespace, classname and key components.
	 @param assocClass If not NULL, MUST be a valid Association Class name.
	    It acts as a filter on the returned set of Objects by mandating that
	    each returned Object MUST be associated to the source Object via an
	    Instance of this Class or one of its subclasses.
	 @param resultClass If not NULL, MUST be a valid Class name.
	    It acts as a filter on the returned set of Objects by mandating that
	    each returned Object MUST be either an Instance of this Class (or one
	    of its subclasses).
	 @param role If not NULL, MUST be a valid Property name.
	    It acts as a filter on the returned set of Objects by mandating
	    that each returned Object MUST be associated to the source Object
	    via an Association in which the source Object plays the specified role
	    (i.e. the name of the Property in the Association Class that refers
	    to the source Object MUST match the value of this parameter).
	 @param resultRole If not NULL, MUST be a valid Property name.
	    It acts as a filter on the returned set of Objects by mandating
	    that each returned Object MUST be associated to the source Object
	    via an Association in which the returned Object plays the specified role
	    (i.e. the name of the Property in the Association Class that refers to
	    the returned Object MUST match the value of this parameter).
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Enumeration of ObjectPaths.
     */
   inline static   CMPIEnumeration* CBAssociatorNames
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op, const char* assocClass, const char* resultClass,
		 const char* role, const char* resultRole, CMPIStatus* rc)
	{ return ((mb)->bft->associatorNames((mb),(ctx),(op),
	             (assocClass),(resultClass),(role),(resultRole),(rc))); }
#else
  #define CBAssociatorNames(b,c,p,acl,rcl,r,rr,rc) \
       ((b)->bft->associatorNames((b),(c),(p),(acl),(rcl),(r),(rr),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Enumerates the association instances that refer to the instance defined by
           &lt;op&gt;.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op Source ObjectPath containing namespace, classname and key components.
	 @param resultClass If not NULL, MUST be a valid Class name.
	    It acts as a filter on the returned set of Objects by mandating that
	    each returned Object MUST be either an Instance of this Class (or one
	    of its subclasses).
	 @param role If not NULL, MUST be a valid Property name.
	    It acts as a filter on the returned set of Objects by mandating
	    that each returned Object MUST be associated to the source Object
	    via an Association in which the source Object plays the specified role
	    (i.e. the name of the Property in the Association Class that refers
	    to the source Object MUST match the value of this parameter).
	 @param properties If not NULL, the members of the array define one or more Property
	     names. Each returned Object MUST NOT include elements for any Properties
	     missing from this list
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Enumeration of ObjectPaths.
     */
   inline static   CMPIEnumeration* CBReferences
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op, const char* resultClass ,const char* role ,
		 const char** properties, CMPIStatus* rc)
	{ return ((mb)->bft->references((mb),(ctx),(op),
	             (resultClass),(role),(properties),(rc))); }
#else
  #define CBReferences(b,c,p,acl,r,pr,rc) \
       ((b)->bft->references((b),(c),(p),(acl),(r),(pr),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Enumerates the association ObjectPaths that refer to the instance defined by
           &lt;op&gt;.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op Source ObjectPath containing namespace, classname and key components.
	 @param resultClass If not NULL, MUST be a valid Class name.
	    It acts as a filter on the returned set of Objects by mandating that
	    each returned Object MUST be either an Instance of this Class (or one
	    of its subclasses).
	 @param role If not NULL, MUST be a valid Property name.
	    It acts as a filter on the returned set of Objects by mandating
	    that each returned Object MUST be associated to the source Object
	    via an Association in which the source Object plays the specified role
	    (i.e. the name of the Property in the Association Class that refers
	    to the source Object MUST match the value of this parameter).
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Enumeration of ObjectPaths.
       */
   inline static   CMPIEnumeration* CBReferenceNames
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op, const char* resultClass ,const char* role,
                 CMPIStatus* rc)
	{ return ((mb)->bft->referenceNames((mb),(ctx),(op),(resultClass),(role),(rc))); }
#else
  #define CBReferenceNames(b,c,p,acl,r,rc) \
       ((b)->bft->referenceNames((b),(c),(p),(acl),(r),(rc)))
#endif


#ifdef CMPI_INLINE
       /** Invoke a named, extrinsic method of an Instance
         defined by the &lt;op&gt; parameter.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op ObjectPath containing namespace, classname and key components.
	 @param method Method name
	 @param in Input parameters.
	 @param out Output parameters.
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Method return value.
      */
   inline static   CMPIData CBInvokeMethod
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op, const char* method,
		 	     const CMPIArgs* in, CMPIArgs* out, CMPIStatus* rc)
	{ return ((mb)->bft->invokeMethod((mb),(ctx),(op),(method),(in),(out),(rc))); }
#else
  #define CBInvokeMethod(b,c,p,m,ai,ao,rc) \
                 ((b)->bft->invokeMethod((b),(c),(p),(m),(ai),(ao),(rc)))
#endif

#ifdef CMPI_INLINE
       /** Set the named property value of an Instance defined by the &lt;op&gt; parameter.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op ObjectPath containing namespace, classname and key components.
	 @param name Property name
	 @param value Value.
	 @param type Value type.
	 @return Service return status.
      */
     inline static   CMPIStatus CBSetProperty
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op, const char* name , 
				 const CMPIValue *value, CMPIType type)
	{ return ((mb)->bft->setProperty((mb),(ctx),(op),(name),
	                 (CMPIValue*)(value),(type))); }
#else
  #define CBSetProperty(b,c,p,n,v,t) \
             ((b)->bft->setProperty((b),(c),(p),(n),(CMPIValue*)(v),(t)))
#endif

#ifdef CMPI_INLINE
       /** Get the named property value of an Instance defined by the &lt;op&gt; parameter.
	 @param mb Broker this pointer.
	 @param ctx Context object
	 @param op ObjectPath containing namespace, classname and key components.
	 @param name Property name
	 @param rc Output: Service return status (suppressed when NULL).
	 @return Property value.
      */
   inline static   CMPIData CBGetProperty
                (const CMPIBroker* mb, const CMPIContext* ctx,
                 const CMPIObjectPath* op, const char* name, CMPIStatus* rc)
	{ return (mb)->bft->getProperty((mb),(ctx),(op),(name),(rc)); }
#else
  #define CBGetProperty(b,c,p,n,rc) \
             (b)->bft->getProperty((b),(c),(p),(n),(rc))
#endif


#ifndef DOC_ONLY
   // MI factory stubs

  #define CMNoHook 
#endif

/*
	-----------------  C provider factories ---------------------
*/

#ifdef DOC_ONLY
       /** This macro generates the function table and initialization stub
           for an instance provider. The initialization routine &lt;pn&gt;Create_InstanceMI
	   is called when this provider module is loaded by the broker.
	   This macro is for CMPI providers written in plain C.
	 @param pfx The prefix for all mandatory association provider functions.
	        This is a character string without quotes.
	        Mandatory functions are: &lt;pfx&gt;Cleanup, &lt;pfx&gt;EnumInstanceNames,
                &lt;pfx&gt;EnumInstances, &lt;pfx&gt;GetInstance, &lt;pfx&gt;CreateInstance,
                &lt;pfx&gt;SetInstance, &lt;pfx&gt;DeleteInstance and &lt;pfx&gt;ExecQuery.
	 @param pn The provider name under which this provider is registered.
	        This is a character string without quotes.
	 @param broker The name of the broker variable used by this macro to store
	               the CMPIBroker pointer
	 @param hook A statement that is executed within &lt;pn&gt;Create_InstanceMI routine.
	             This enables you to perform additional initialization functions and
		     is normally a function call like furtherInit(broker) or CMNoHook.
		     Use CMNoHook if no further intialization is required.
	 @return The function table of this instance provider.
      */
   CMPIInstanceMI* CMInstanceMIStub(chars pfx, chars pn,
         CMPIBroker *broker, statement hook);
#else

  #if defined(CMPI_VER_100)
    #define CMInstanceMIStubChange(pfx) pfx##ModifyInstance
  #else
    #define CMInstanceMIStubChange(pfx) pfx##SetInstance
  #endif

  #define CMInstanceMIStub(pfx,pn,broker,hook) \
  static CMPIInstanceMIFT instMIFT__={ \
   CMPICurrentVersion, \
   CMPICurrentVersion, \
   "instance" #pn, \
   pfx##Cleanup, \
   pfx##EnumInstanceNames, \
   pfx##EnumInstances, \
   pfx##GetInstance, \
   pfx##CreateInstance, \
   CMInstanceMIStubChange(pfx), \
   pfx##DeleteInstance, \
   pfx##ExecQuery, \
  }; \
  CMPI_EXTERN_C \
  CMPIInstanceMI* pn##_Create_InstanceMI(const CMPIBroker* brkr,const CMPIContext *ctx, CMPIStatus *rc) { \
   static CMPIInstanceMI mi={ \
      NULL, \
      &instMIFT__, \
   }; \
   broker=brkr; \
   hook; \
   return &mi;  \
  }

#endif

#ifdef DOC_ONLY
       /** This macro generates the function table and initialization stub
           for an association provider. The initialization routine
	   &lt;pn&gt;Create_AssociationMI
	   is called when this provider module is loaded by the broker.
	   This macro is for CMPI providers written in plain C.
	 @param pfx The prefix for all mandatory instance provider functions.
	        This is a character string without quotes.
	        Mandatory functions are: &lt;pfx&gt;AssociationCleanup,
                &lt;pfx&gt;Associators, &lt;pfx&gt;AssociatorNames, &lt;pfx&gt;References and
                &lt;pfx&gt;ReferenceNames.
	 @param pn The provider name under which this provider is registered.
	        This is a character string without quotes.
	 @param broker The name of the broker variable used by this macro to store
	               the CMPIBroker pointer
	 @param hook A statement that is executed within &lt;pn&gt;Create_AssociationMI routine.
	             This enables you to perform additional initialization functions and
		     is normally a function call like furtherInit(broker) or CMNoHook.
		     Use CMNoHook if no further intialization is required.
	 @return The function table of this association provider.
      */
   CMPIAssociationMI* CMAssociationMIStub(chars pfx, chars pn,
         CMPIBroker *broker, statement hook);
#else
  #define CMAssociationMIStub(pfx,pn,broker,hook) \
  static CMPIAssociationMIFT assocMIFT__={ \
   CMPICurrentVersion, \
   CMPICurrentVersion, \
   "association" #pn, \
   pfx##AssociationCleanup, \
   pfx##Associators, \
   pfx##AssociatorNames, \
   pfx##References, \
   pfx##ReferenceNames, \
  }; \
  CMPI_EXTERN_C \
  CMPIAssociationMI* pn##_Create_AssociationMI(const CMPIBroker* brkr,const CMPIContext *ctx,  CMPIStatus *rc) { \
   static CMPIAssociationMI mi={ \
      NULL, \
      &assocMIFT__, \
   }; \
   broker=brkr; \
   hook; \
   return &mi;  \
  }
#endif

#ifdef DOC_ONLY
       /** This macro generates the function table and initialization stub
           for a method provider. The initialization routine &lt;pn&gt;Create_MethodMI
	   is called when this provider module is loaded by the broker.
	   This macro is for CMPI providers written in plain C.
	 @param pfx The prefix for all mandatory method provider functions.
	        This is a character string without quotes.
	        Mandatory functions are: &lt;pfx&gt;MthodCleanup and &lt;pfx&gt;InvokeMethod.
	 @param pn The provider name under which this provider is registered.
	        This is a character string without quotes.
	 @param broker The name of the broker variable used by this macro to store
	               the CMPIBroker pointer
	 @param hook A statement that is executed within &lt;pn&gt;Create_MethodMI routine.
	             This enables you to perform additional initialization functions and
		     is normally a function call like furtherInit(broker) or CMNoHook.
		     Use CMNoHook if no further intialization is required.
	 @return The function table of this method provider.
      */
   CMPIMethodMI* CMMethodMIStub(chars pfx, chars pn,
         CMPIBroker *broker, statement hook);
#else
  #define CMMethodMIStub(pfx,pn,broker,hook) \
  static CMPIMethodMIFT methMIFT__={ \
   CMPICurrentVersion, \
   CMPICurrentVersion, \
   "method" #pn, \
   pfx##MethodCleanup, \
   pfx##InvokeMethod, \
  }; \
  CMPI_EXTERN_C \
  CMPIMethodMI* pn##_Create_MethodMI(const CMPIBroker* brkr, const CMPIContext *ctx,  CMPIStatus *rc) { \
   static CMPIMethodMI mi={ \
      NULL, \
      &methMIFT__, \
   }; \
   broker=brkr; \
   hook; \
   return &mi; \
  }
#endif

#ifdef DOC_ONLY
       /** This macro generates the function table and initialization stub
           for a property provider. The initialization routine &lt;pn&gt;Create_PropertyMI
	   is called when this provider module is loaded by the broker.
	   This macro is for CMPI providers written in plain C.
	 @param pfx The prefix for all mandatory property provider functions.
	        This is a character string without quotes.
	        Mandatory functions are: &lt;pfx&gt;PropertyCleanup, &lt;pfx&gt;SetProperty and
                &lt;pfx&gt;GetProperty.

	 @param pn The provider name under which this provider is registered.
	        This is a character string without quotes.
	 @param broker The name of the broker variable used by this macro to store
	               the CMPIBroker pointer
	 @param hook A statement that is executed within &lt;pn&gt;Create_PropertyMI routine.
	             This enables you to perform additional initialization functions and
		     is normally a function call like furtherInit(broker) or CMNoHook.
		     Use CMNoHook if no further intialization is required.
	 @return The function table of this property provider.
      */
   CMPIPropertyMI* CMPropertyMIStub(chars pfx, chars pn,
         CMPIBroker *broker, statement hook);
#else
  #define CMPropertyMIStub(pfx,pn,broker,hook) \
  static CMPIPropertyMIFT propMIFT__={ \
   CMPICurrentVersion, \
   CMPICurrentVersion, \
   "property" #pn, \
   pfx##PropertyCleanup, \
   pfx##SetProperty, \
   pfx##GetProperty, \
  }; \
  CMPI_EXTERN_C \
  CMPIPropertyMI* pn##_Create_PropertyMI(const CMPIBroker* brkr,const CMPIContext *ctx,  CMPIStatus *rc) { \
   static CMPIPropertyMI mi={ \
      NULL, \
      &propMIFT__, \
   }; \
   broker=brkr; \
   hook; \
   return &mi; \
  }
#endif

#ifdef DOC_ONLY
       /** This macro generates the function table and initialization stub
           for an indication provider. The initialization routine &lt;pn&gt;Create_IndicationMI
	   is called when this provider module is loaded by the broker.
	   This macro is for CMPI providers written in plain C.
	 @param pfx The prefix for all mandatory indication provider functions.
	        This is a character string without quotes.
	        Mandatory functions are: &lt;pfx&gt;IndicationCleanup, &lt;pfx&gt;AuthorizeFilter,
                &lt;pfx&gt;MustPoll, &lt;pfx&gt;ActivateFilter and &lt;pfx&gt;DeActivateFilter.
	 @param pn The provider name under which this provider is registered.
	        This is a character string without quotes.
	 @param broker The name of the broker variable used by this macro to store
	               the CMPIBroker pointer
	 @param hook A statement that is executed within &lt;pn&gt;Create_IndicationMI routine.
	             This enables you to perform additional initialization functions and
		     is normally a function call like furtherInit(broker) or CMNoHook.
		     Use CMNoHook if no further intialization is required.
	 @return The function table of this indication provider.
      */
   CMPIIndicationMI* CMIndicationMIStub(chars pfx, chars pn,
         CMPIBroker *broker, statement hook);
#else

  #if defined(CMPI_VER_86)
    #define CMIndicationMIStubExtensions(pfx) pfx##EnableIndications, \
       pfx##DisableIndications,
  #else
    #define CMIndicationMIStubExtensions(pfx)
  #endif

  #define CMIndicationMIStub(pfx,pn,broker,hook) \
  static CMPIIndicationMIFT indMIFT__={ \
   CMPICurrentVersion, \
   CMPICurrentVersion, \
   "Indication" #pn, \
   pfx##IndicationCleanup, \
   pfx##AuthorizeFilter, \
   pfx##MustPoll, \
   pfx##ActivateFilter, \
   pfx##DeActivateFilter, \
   CMIndicationMIStubExtensions(pfx) \
 }; \
  CMPI_EXTERN_C \
  CMPIIndicationMI* pn##_Create_IndicationMI(const CMPIBroker* brkr,const CMPIContext *ctx,CMPIStatus *rc) { \
   static CMPIIndicationMI mi={ \
      NULL, \
      &indMIFT__, \
   }; \
   broker=brkr; \
   hook; \
   return &mi; \
 }
#endif

/*
	-----------------  C++ provider factories ---------------------
*/

#ifdef DOC_ONLY
       /** This macro generates the function table and initialization stub
           for an instance provider. The initialization routine &lt;pn&gt;Create_IndicationMI
	   is called when this provider module is loaded by the broker.
	   This macro is for CMPI providers written in C++ using the Cmpi* classes.
	 @param cn The C++ class name of this instance provider
	        (a subclass of CmpiInstanceMI).
	        This is a character string without quotes.
	 @param pn The provider name under which this provider is registered.
	        This is a character string without quotes.
	 @return The function table of this instance provider.
      */
   CMPIInstanceMI* CMInstanceMIFactory(chars cn, chars pn);
#else
 #define CMInstanceMIFactory(cn,pn) \
 CMPI_EXTERN_C \
 CMPIInstanceMI* pn##_Create_InstanceMI(const CMPIBroker* broker, const CMPIContext *ctxp,  CMPIStatus *rc) { \
   static CMPIInstanceMIFT instMIFT={ \
    CMPICurrentVersion, \
    CMPICurrentVersion, \
    "instance" #pn, \
    (CMPIStatus(*)(CMPIInstanceMI*,const CMPIContext*,CMPIBoolean))CmpiBaseMI::driveBaseCleanup, \
    CmpiInstanceMI::driveEnumInstanceNames, \
    CmpiInstanceMI::driveEnumInstances, \
    CmpiInstanceMI::driveGetInstance, \
    CmpiInstanceMI::driveCreateInstance, \
    CmpiInstanceMI::driveSetInstance, \
    CmpiInstanceMI::driveDeleteInstance, \
    CmpiInstanceMI::driveExecQuery, \
   }; \
   static CMPIInstanceMI mi; \
   fprintf(stderr,"--- _Create_InstanceMI() broker: %p\n",broker); \
   CmpiContext ctx((CMPIContext*)ctxp); \
   mi.ft=&instMIFT; \
   CmpiBaseMI *provider=base##pn.getBaseMI(); \
   if (provider == 0) {\
     provider = new cn(CmpiBroker((CMPIBroker*)broker),ctx); \
     provider->setProviderBase(&base##pn); \
       provider->initialize(ctx); \
     base##pn.setBaseMI(provider); \
    } \
   mi.hdl=provider; \
   base##pn.incUseCount(); \
    return &mi; \
 }
#endif

#ifdef DOC_ONLY
       /** This macro generates the function table and initialization stub
           for an association provider. The initialization routine
	   &lt;pn&gt;Create_AssociationMI
	   is called when this provider module is loaded by the broker.
	   This macro is for CMPI providers written in C++ using the Cmpi* classes.
	 @param cn The C++ class name of this instance provider
	        (a subclass of CmpiInstanceMI).
	        This is a character string without quotes.
	 @param pn The provider name under which this provider is registered.
	        This is a character string without quotes.
	 @return The function table of this instance provider.
      */
   CMPIAssociationMI* CMAssociationMIFactory(chars cn, chars pn);
#else
 #define CMAssociationMIFactory(cn,pn) \
 CMPI_EXTERN_C \
 CMPIAssociationMI* pn##_Create_AssociationMI(const CMPIBroker* broker, const CMPIContext *ctxp,  CMPIStatus *rc) { \
   static CMPIAssociationMIFT assocMIFT={ \
    CMPICurrentVersion, \
    CMPICurrentVersion, \
    "association" #pn, \
    (CMPIStatus(*)(CMPIAssociationMI*,const CMPIContext*,CMPIBoolean))CmpiBaseMI::driveBaseCleanup, \
    CmpiAssociationMI::driveAssociators, \
    CmpiAssociationMI::driveAssociatorNames, \
    CmpiAssociationMI::driveReferences, \
    CmpiAssociationMI::driveReferenceNames, \
  }; \
   static CMPIAssociationMI mi; \
   fprintf(stderr,"--- _Create_AssociationMI() broker: %p\n",broker); \
   CmpiContext ctx((CMPIContext*)ctxp); \
   mi.ft=&assocMIFT; \
   CmpiBaseMI *provider=base##pn.getBaseMI(); \
   if (provider == 0) {\
     provider = new cn(CmpiBroker((CMPIBroker*)broker),ctx); \
     provider->setProviderBase(&base##pn); \
       provider->initialize(ctx); \
     base##pn.setBaseMI(provider); \
    } \
   mi.hdl=provider; \
   base##pn.incUseCount(); \
    return &mi; \
 }
#endif

#ifdef DOC_ONLY
       /** This macro generates the function table and initialization stub
           for an method provider. The initialization routine
	   &lt;pn&gt;Create_MethodMI is called when this provider module is loaded
	   by the broker.
	   This macro is for CMPI providers written in C++ using the Cmpi* classes.
	 @param cn The C++ class name of this method provider
	        (a subclass of CmpiMethodMI).
	        This is a character string without quotes.
	 @param pn The provider name under which this provider is registered.
	        This is a character string without quotes.
	 @return The function table of this association provider.
      */
   CMPIMethodMI* CMMethodMIFactory(chars cn, chars pn);
#else
 #define CMMethodMIFactory(cn,pn) \
 CMPI_EXTERN_C \
 CMPIMethodMI* pn##_Create_MethodMI(const CMPIBroker* broker, const CMPIContext *ctxp,  CMPIStatus *rc) { \
   static CMPIMethodMIFT methMIFT={ \
    CMPICurrentVersion, \
    CMPICurrentVersion, \
    "method" #pn, \
    (CMPIStatus(*)(CMPIMethodMI*,const CMPIContext*, CMPIBoolean))CmpiBaseMI::driveBaseCleanup, \
    CmpiMethodMI::driveInvokeMethod, \
   }; \
   static CMPIMethodMI mi; \
   fprintf(stderr,"--- _Create_MethodMI() broker: %p\n",broker); \
   CmpiContext ctx((CMPIContext*)ctxp); \
   mi.ft=&methMIFT; \
   CmpiBaseMI *provider=base##pn.getBaseMI(); \
   if (provider == 0) {\
     provider = new cn(CmpiBroker((CMPIBroker*)broker),ctx); \
     provider->setProviderBase(&base##pn); \
       provider->initialize(ctx); \
     base##pn.setBaseMI(provider); \
    } \
   mi.hdl=provider; \
   base##pn.incUseCount(); \
    return &mi; \
 }
#endif

#ifdef DOC_ONLY
       /** This macro generates the function table and initialization stub
           for a property provider. The initialization routine &lt;pn&gt;Create_PropertyMI
	   is called when this provider module is loaded by the broker.
	   This macro is for CMPI providers written in C++ using the Cmpi* classes.
	 @param cn The C++ class name of this method provider
	        (a subclass of CmpiMethodMI).
	        This is a character string without quotes.
	 @param pn The provider name under which this provider is registered.
	        This is a character string without quotes.
	 @return The function table of this association provider.
      */
   CMPIPropertyMI* CMPropertyMIFactory(chars cn, chars pn):
#else
 #define CMPropertyMIFactory(cn,pn) \
 CMPI_EXTERN_C \
 CMPIMethodMI* pn##_Create_PropertyMI(const CMPIBroker* broker, const CMPIContext *ctxp,  CMPIStatus *rc) { \
   static CMPIPropertyMIFT propMIFT={ \
    CMPICurrentVersion, \
    CMPICurrentVersion, \
    "property" #pn, \
    (CMPIStatus(*)(CMPIPropertyMI*,const CMPIContext*,CMPIBoolean))CmpiBaseMI::driveBaseCleanup, \
    CmpiPropertyMI::driveSetProperty, \
    CmpiPropertyMI::driveGetProperty, \
   }; \
   static CMPIPropertyMI mi; \
   fprintf(stderr,"--- _Create_MethodMI() broker: %p\n",broker); \
   CmpiContext ctx((CMPIContext*)ctxp); \
   mi.ft=&propMIFT; \
   CmpiBaseMI *provider=base##pn.getBaseMI(); \
   if (provider == 0) {\
     provider = new cn(CmpiBroker((CMPIBroker*)broker),ctx); \
     provider->setProviderBase(&base##pn); \
       provider->initialize(ctx); \
     base##pn.setBaseMI(provider); \
    } \
   mi.hdl=provider; \
   base##pn.incUseCount(); \
   return &mi; \
 }
#endif


#ifdef DOC_ONLY
       /** This macro generates the function table and initialization stub
           for an indication provider. The initialization routine &lt;pn&gt;Create_IndicationMI
	   is called when this provider module is loaded by the broker.
	   This macro is for CMPI providers written in C++ using the Cmpi* classes.
	 @param cn The C++ class name of this indication provider
	        (a subclass of CmpiIndicationMI).
	        This is a character string without quotes.
	 @param pn The provider name under which this provider is registered.
	        This is a character string without quotes.
	 @return The function table of this association provider.
      */
   CMPIIndicationMI* CMIndicationMIFactory(chars cn, chars pn);
#else

  #if defined(CMPI_VER_86)
    #define CMIndicationMIFactoryExtensions CmpiIndicationMI::driveEnableIndications, \
       CmpiIndicationMI::driveDisableIndications,
  #else
    #define CMIndicationMIFactoryExtensions
  #endif

#define CMIndicationMIFactory(cn,pn) \
 CMPI_EXTERN_C \
 CMPIIndicationMI* pn##_Create_IndicationMI(const CMPIBroker* broker, const CMPIContext *ctxp,  CMPIStatus *rc) { \
   static CMPIIndicationMIFT indMIFT={ \
    CMPICurrentVersion, \
    CMPICurrentVersion, \
    "indication" #pn, \
    (CMPIStatus(*)(CMPIIndicationMI*,const CMPIContext*,CMPIBoolean))CmpiBaseMI::driveBaseCleanup, \
    CmpiIndicationMI::driveAuthorizeFilter, \
    CmpiIndicationMI::driveMustPoll, \
    CmpiIndicationMI::driveActivateFilter, \
    CmpiIndicationMI::driveDeActivateFilter, \
    CMIndicationMIFactoryExtensions \
   }; \
   static CMPIIndicationMI mi; \
   fprintf(stderr,"--- _Create_IndicationMI() broker: %p\n",broker); \
   CmpiContext ctx((CMPIContext*)ctxp); \
   mi.ft=&indMIFT; \
   CmpiBaseMI *provider=base##pn.getBaseMI(); \
   if (provider == 0) {\
     provider = new cn(CmpiBroker((CMPIBroker*)broker),ctx); \
     provider->setProviderBase(&base##pn); \
       provider->initialize(ctx); \
     base##pn.setBaseMI(provider); \
    } \
   mi.hdl=provider; \
   base##pn.incUseCount(); \
  return &mi; \
 }
#endif

#define CMProviderBase(pn) \
   CmpiProviderBase base##pn;

#endif // _CMPIMACS_H_
