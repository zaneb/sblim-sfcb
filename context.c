
/*
 * context.c
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
 * Author:        Frank Scheffler
 * Contributions: Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * CMPIContext implementation.
 *
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native.h"

static struct native_propertyFT propertyFT;

//! Native extension of the CMPIContext data type.
/*!
  This structure stores the information needed to represent contexts for
  CMPI providers.
 */
struct native_context {
   CMPIContext ctx;             /*!< the inheriting data structure  */
   int mem_state;               /*!< states, whether this object is
                                   registered within the memory mangagement or
                                   represents a cloned object */
   struct native_property *entries;     /*!< context content */
   void *data;
};


static struct native_context *__new_empty_context(int);


/****************************************************************************/


static CMPIStatus __cft_release(CMPIContext * ctx)
{
   CMReturn(CMPI_RC_ERR_NOT_SUPPORTED);
}


static CMPIContext *__cft_clone(CMPIContext * ctx, CMPIStatus * rc)
{
   if (rc)
      CMSetStatus(rc, CMPI_RC_ERR_NOT_SUPPORTED);
   return NULL;
}


static CMPIData __cft_getEntry(CMPIContext * ctx,
                               const char *name, CMPIStatus * rc)
{
   struct native_context *c = (struct native_context *) ctx;

   return propertyFT.getDataProperty(c->entries, name, rc);
}


static CMPIData __cft_getEntryAt(CMPIContext * ctx,
                                 unsigned int index,
                                 CMPIString ** name, CMPIStatus * rc)
{
   struct native_context *c = (struct native_context *) ctx;

   return propertyFT.getDataPropertyAt(c->entries, index, name, rc);
}


static unsigned int __cft_getEntryCount(CMPIContext * ctx, CMPIStatus * rc)
{
   struct native_context *c = (struct native_context *) ctx;

   return propertyFT.getPropertyCount(c->entries, rc);
}


static CMPIStatus __cft_addEntry(CMPIContext * ctx,
                                 const char *name,
                                 CMPIValue * value, CMPIType type)
{
   struct native_context *c = (struct native_context *) ctx;

   CMReturn((propertyFT.addProperty(&c->entries,
                                    c->mem_state,
                                    name,
                                    type,
                                    0,
                                    value)) ?
            CMPI_RC_ERR_ALREADY_EXISTS : CMPI_RC_OK);
}


static struct native_context *__new_empty_context(int mm_add)
{
   static CMPIContextFT cft = {
      NATIVE_FT_VERSION,
      __cft_release,
      __cft_clone,
      __cft_getEntry,
      __cft_getEntryAt,
      __cft_getEntryCount,
      __cft_addEntry
   };
   static CMPIContext c = {
      "CMPIContext",
      &cft
   };

   struct native_context *ctx = (struct native_context *)
       tool_mm_alloc(mm_add, sizeof(struct native_context));

   ctx->ctx = c;
   ctx->mem_state = mm_add;

   return ctx;
}



CMPIContext *native_new_CMPIContext(int mem_state, void *data)
{
   struct native_context *ctx;
   ctx=__new_empty_context(mem_state);
   ctx->data=data;
   return (CMPIContext*)ctx; 
}


void native_release_CMPIContext(CMPIContext * ctx)
{
   struct native_context *c = (struct native_context *) ctx;

   if (c->mem_state == TOOL_MM_NO_ADD) {

      c->mem_state = TOOL_MM_ADD;
      tool_mm_add(c);

      propertyFT.release(c->entries);
   }
}

CMPIContext *native_clone_CMPIContext(CMPIContext* ctx) 
{
   CMPIString *name;
   struct native_context *c = (struct native_context *) ctx;
   int i,s;
   CMPIContext *nCtx=native_new_CMPIContext(TOOL_MM_NO_ADD,c->data);
   
   for (i=0,s=ctx->ft->getEntryCount(ctx,NULL); i<s; i++) {
      CMPIData data=ctx->ft->getEntryAt(ctx,i,&name,NULL);
      nCtx->ft->addEntry(nCtx,CMGetCharPtr(name),&data.value,data.type);
   }
   return nCtx;
}


//! Storage container for commonly needed data within native CMPI data types.
/*!
  This structure is used to build linked lists of data containers as needed
  for various native data types.
*/
struct native_property {
	char * name;		        //!< Property identifier.
	CMPIType type;		        //!< Associated CMPIType.
	CMPIValueState state; 	        //!< Current value state.
	CMPIValue value;	        //!< Current value.
	struct native_property * next;	//!< Pointer to next property.
};


/****************************************************************************/

static CMPIData __convert2CMPIData ( struct native_property * prop,
				     CMPIString ** propname )
{
	CMPIData result;

	if ( prop != NULL ) {
		result.type  = prop->type;
		result.state = prop->state;
		result.value = prop->value;

		if ( propname ) {
			*propname  = native_new_CMPIString ( prop->name,
							     NULL );
		}

	} else {
		result.state = CMPI_nullValue;
	}

	return result;
}


/**
 * returns non-zero if already existant
 */
static int __addProperty ( struct native_property ** prop,
			   int mm_add,
			   const char * name,
			   CMPIType type,
			   CMPIValueState state, 
			   CMPIValue * value )
{
	CMPIValue v;

	if ( *prop == NULL ) {
		struct native_property * tmp = *prop =
			(struct native_property *)
			tool_mm_alloc ( mm_add,
					sizeof ( struct native_property ) );
  
		tmp->name = strdup ( name );

		if ( mm_add == TOOL_MM_ADD ) tool_mm_add ( tmp->name );

		if ( type == CMPI_chars ) {

			type = CMPI_string;
			v.string = native_new_CMPIString ( (char *) value,
							   NULL );
			value = &v;
		}

		tmp->type  = type;

		if ( type != CMPI_null ) {
			tmp->state = state;

			if ( mm_add == TOOL_MM_ADD ) {

				tmp->value = *value;
			} else {
			
				CMPIStatus rc;
				tmp->value = native_clone_CMPIValue ( type,
								      value,
								      &rc );
				// what if clone() fails???
			}
		} else tmp->state = CMPI_nullValue;

		return 0;
	}
	return ( strcmp ( (*prop)->name, name ) == 0 ||
		 __addProperty ( &( (*prop)->next ), 
				 mm_add,
				 name, 
				 type,
				 state, 
				 value ) );
}


/**
 * returns -1 if non-existant
 */
static int __setProperty ( struct native_property * prop, 
			   int mm_add,
			   const char * name, 
			   CMPIType type,
			   CMPIValue * value )
{
	CMPIValue v;
	if ( prop == NULL ) {
		return -1;
	}

	if ( strcmp ( prop->name, name ) == 0 ) {

		CMPIStatus rc;

		if ( ! ( prop->state & CMPI_nullValue ) )
			native_release_CMPIValue ( prop->type, &prop->value );

		if ( type == CMPI_chars ) {

			type = CMPI_string;
			v.string = native_new_CMPIString ( (char *) value,
							   NULL );
			value = &v;
		}

		prop->type  = type;

		if ( type != CMPI_null ) {
			prop->value =
				( mm_add == TOOL_MM_ADD )?
				*value:
				native_clone_CMPIValue ( type, value, &rc );

			// what if clone() fails ???

		} else prop->state = CMPI_nullValue;

		return 0;
	}
	return __setProperty ( prop->next, mm_add, name, type, value);
}


static struct native_property * __getProperty ( struct native_property * prop, 
						const char * name )
{
	if ( ! prop || ! name ) {
		return NULL;
	}
	return ( strcmp ( prop->name, name ) == 0 )?
		prop: __getProperty ( prop->next, name );
}


static CMPIData __getDataProperty ( struct native_property * prop,
				    const char * name,
				    CMPIStatus * rc )
{
	struct native_property * p = __getProperty ( prop, name );

	if ( rc ) CMSetStatus ( rc,
				( p )?
				CMPI_RC_OK:
				CMPI_RC_ERR_NO_SUCH_PROPERTY );

	return __convert2CMPIData ( p, NULL );
}


static struct native_property * __getPropertyAt
( struct native_property * prop, unsigned int pos )
{
	if ( ! prop ) {
		return NULL;
	}

	return ( pos == 0 )?
		prop: __getPropertyAt ( prop->next, --pos );
}


static CMPIData __getDataPropertyAt ( struct native_property * prop, 
				      unsigned int pos,
				      CMPIString ** propname,
				      CMPIStatus * rc )
{
	struct native_property * p = __getPropertyAt ( prop, pos );

	if ( rc ) CMSetStatus ( rc,
				( p )?
				CMPI_RC_OK:
				CMPI_RC_ERR_NO_SUCH_PROPERTY );

	return __convert2CMPIData ( p, propname );
}


static CMPICount __getPropertyCount ( struct native_property * prop,
				      CMPIStatus * rc )
{
	CMPICount c = 0;

	if ( rc ) CMSetStatus ( rc, CMPI_RC_OK );

	while ( prop != NULL ) {
		c++;
		prop = prop->next;
	}

	return c;
}


static void __release ( struct native_property * prop )
{
	for ( ; prop; prop = prop->next ) {
		tool_mm_add ( prop );
		tool_mm_add ( prop->name );
		native_release_CMPIValue ( prop->type, &prop->value );
	}
}


static struct native_property * __clone ( struct native_property * prop,
					  CMPIStatus * rc )
{
	struct native_property * result;
	CMPIStatus tmp;

	if ( prop == NULL ) {

		if ( rc ) CMSetStatus ( rc, CMPI_RC_OK );
		return NULL;
	}

	result = 
		(struct native_property * )
		tool_mm_alloc ( TOOL_MM_NO_ADD,
				sizeof ( struct native_property ) );

	result->name  = strdup ( prop->name );
	result->type  = prop->type;
	result->state = prop->state;
	result->value = native_clone_CMPIValue ( prop->type,
						 &prop->value,
						 &tmp );

	if ( tmp.rc != CMPI_RC_OK ) {

		result->state = CMPI_nullValue;
	}
  
	result->next  = __clone ( prop->next, rc );
	return result;
}


/**
 * Global function table to access native_property helper functions.
 */
static struct native_propertyFT propertyFT = {
	__addProperty,
	__setProperty,
	__getDataProperty,
	__getDataPropertyAt,
	__getPropertyCount,
	__release,
	__clone
};
