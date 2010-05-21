/*
 * cimslp.h
 *
 * (C) Copyright IBM Corp. 2006
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:        Sven Schuetz <sven@de.ibm.com>
 * Contributions:
 *
 * Description:
 *
 * Control functions, main if running standlone, or slpAgent
 * function if running in sfcb
 *
 */

#ifndef _cimslp_h
#define _cimslp_h

#include <slp.h>
#include "cimslpCMPI.h"
#include "cimslpSLP.h"

int             slpAgent();
int             slppid;
void            setUpDefaults(cimomConfig * cfg);
void            setUpTimes(int *slpLifeTime, int *sleepTime);
void            freeCFG(cimomConfig * cfg);

#endif
/* MODELINES */
/* DO NOT EDIT BELOW THIS COMMENT */
/* Modelines are added by 'make pretty' */
/* -*- Mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* vi:set ts=2 sts=2 sw=2 expandtab: */
