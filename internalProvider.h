
/*
 * $Id$
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
 * Author:        Viktor Mihajlovski <mihajlov@de.ibm.com>
 * Contributions: 
 *
 * Description:
 *
 * Header file for the internal provider external functions
 *
*/

#ifndef INTERNAL_PROVIDER_H
#define INTERNAL_PROVIDER_H

CMPIStatus InternalProviderEnumInstanceNames(CMPIInstanceMI * mi,
					     const CMPIContext * ctx, 
					     const CMPIResult * rslt, 
					     const CMPIObjectPath * ref);
CMPIStatus InternalProviderEnumInstances(CMPIInstanceMI * mi,
					 const CMPIContext * ctx, 
					 const CMPIResult * rslt, 
					 const CMPIObjectPath * ref, 
					 const char **properties);
CMPIInstance *internalProviderGetInstance(const CMPIObjectPath * cop, 
					  CMPIStatus *rc);
CMPIStatus InternalProviderCreateInstance(CMPIInstanceMI * mi,
					  const CMPIContext * ctx, 
					  const CMPIResult * rslt, 
					  const CMPIObjectPath * cop, 
					  const CMPIInstance * ci);
CMPIStatus InternalProviderGetInstance(CMPIInstanceMI * mi,
				       const CMPIContext * ctx,
				       const CMPIResult * rslt,
				       const CMPIObjectPath * cop, 
				       const char **properties);
CMPIStatus InternalProviderDeleteInstance(CMPIInstanceMI * mi,
					  const CMPIContext * ctx, 
					  const CMPIResult * rslt, 
					  const CMPIObjectPath * cop);
extern UtilList *SafeInternalProviderEnumInstances(CMPIInstanceMI * mi,
						   const CMPIContext * ctx, 
						   const CMPIObjectPath * ref, 
						   const char **properties, 
						   CMPIStatus *st,int ignprov);
extern char *internalProviderNormalizeObjectPath(const CMPIObjectPath *cop);

#endif
