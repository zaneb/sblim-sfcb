
/*
 * sfcVersion.h
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
 * Version definition.
 *
*/

#ifndef SFCVERSION_H
#define SFCVERSION_H

#ifdef HAVE_CONFIG_H
#include "config.h"

#define sfcBrokerVersion PACKAGE_VERSION
#define sfcHttpDaemonVersion PACKAGE_VERSION

#else
/* this should never be used - but who knows */
#define sfcBrokerVersion "0.8.1"
#define sfcHttpDaemonVersion "0.8.1"

#endif

#endif
