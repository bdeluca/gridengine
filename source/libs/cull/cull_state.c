/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2003 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include "cull_state.h"

#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "pack.h"


/* struct to store ALL state information of cull lib */
struct cull_state_t {
   int               lerrno;            /* cull errno               */
   char              noinit[50];        /* cull error buffer        */
   const lSortOrder* global_sort_order; /* qsort() by-pass argument */
   int               chunk_size;        /* chunk size if packing    */
   const lNameSpace* name_space;        /* name vector              */
};

static pthread_once_t cull_once = PTHREAD_ONCE_INIT;
static pthread_key_t cull_state_key;  

static void cull_once_init(void);
static void cull_state_destroy(void* theState);
static void cull_state_init(struct cull_state_t* theState);


/****** cull_state/cull_mt_init() ************************************************
*  NAME
*     cull_mt_init() -- Initialize CULL for multi threading use.
*
*  SYNOPSIS
*     void cull_mt_init(void) 
*
*  FUNCTION
*     Set up CULL. This function must be called at least once before any of the
*     CULL functions is used. This function is idempotent, i.e. it is safe to
*     call it multiple times.
*
*     Thread local storage for the CULL state information is reserved. 
*
*  INPUTS
*     void - NONE 
*
*  RESULT
*     void - NONE
*
*  NOTES
*     MT-NOTE: cull_mt_init() is MT safe 
*
*******************************************************************************/
void cull_mt_init(void)
{
   pthread_once(&cull_once, cull_once_init);
}

/****** cull_state/state/cull_state_get_????() ************************************
*  NAME
*     cull_state_get_????() - read access to cull state.
*
*  FUNCTION
*     Provides access to thread local storage.
*
******************************************************************************/
int cull_state_get_lerrno(void)
{
   pthread_once(&cull_once, cull_once_init);
   GET_SPECIFIC(struct cull_state_t, cull_state, cull_state_init, cull_state_key, "get_lerrno");
   return cull_state->lerrno;
}

const char *cull_state_get_noinit(void)
{
   pthread_once(&cull_once, cull_once_init);
   GET_SPECIFIC(struct cull_state_t, cull_state, cull_state_init, cull_state_key, "get_noinit");
   return cull_state->noinit;
}

const lSortOrder *cull_state_get_global_sort_order(void)
{
   pthread_once(&cull_once, cull_once_init);
   GET_SPECIFIC(struct cull_state_t, cull_state, cull_state_init, cull_state_key, "get_global_sort_order");
   return cull_state->global_sort_order;
}

int cull_state_get_chunk_size(void)
{
   pthread_once(&cull_once, cull_once_init);
   GET_SPECIFIC(struct cull_state_t, cull_state, cull_state_init, cull_state_key, "get_chunck_size");
   return cull_state->chunk_size;
}

const lNameSpace *cull_state_get_name_space(void)
{
   pthread_once(&cull_once, cull_once_init);
   GET_SPECIFIC(struct cull_state_t, cull_state, cull_state_init, cull_state_key, "get_name_space");
   return cull_state->name_space;
}

/****** cull/list/cull_state_set_????() ************************************
*  NAME
*     cull_state_set_????() - write access to cull state.
*
*  FUNCTION
*     Provides access to thread local storage.
*
******************************************************************************/
void cull_state_set_lerrno( int i)
{
   pthread_once(&cull_once, cull_once_init);
   GET_SPECIFIC(struct cull_state_t, cull_state, cull_state_init, cull_state_key, "set_lerrno");
   cull_state->lerrno = i;
}

void cull_state_set_noinit( char *s)
{
   pthread_once(&cull_once, cull_once_init);
   GET_SPECIFIC(struct cull_state_t, cull_state, cull_state_init, cull_state_key, "set_noinit");
   strcpy(cull_state->noinit, s);
}

void cull_state_set_global_sort_order( const lSortOrder *so)
{
   pthread_once(&cull_once, cull_once_init);
   GET_SPECIFIC(struct cull_state_t, cull_state, cull_state_init, cull_state_key, "set_global_sort_order");
   cull_state->global_sort_order = so;
}

void cull_state_set_chunk_size( int chunk_size)
{
   pthread_once(&cull_once, cull_once_init);
   GET_SPECIFIC(struct cull_state_t, cull_state, cull_state_init, cull_state_key, "set_chunck_size");
   cull_state->chunk_size = chunk_size;
}

void cull_state_set_name_space( const lNameSpace  *ns)
{
   pthread_once(&cull_once, cull_once_init);
   GET_SPECIFIC(struct cull_state_t, cull_state, cull_state_init, cull_state_key, "set_name_space");
   cull_state->name_space = ns;
}

/****** cull_state/cull_once_init() ********************************************
*  NAME
*     cull_once_init() -- One-time CULL initialization.
*
*  SYNOPSIS
*     static cull_once_init(void) 
*
*  FUNCTION
*     Create access key for thread local storage. Register cleanup function.
*
*     This function must be called exactly once.
*
*  INPUTS
*     void - none
*
*  RESULT
*     void - none 
*
*  NOTES
*     MT-NOTE: cull_once_init() is MT safe. 
*
*******************************************************************************/
static void cull_once_init(void)
{
   pthread_key_create(&cull_state_key, cull_state_destroy);
}

/****** cull_state/cull_state_destroy() ****************************************
*  NAME
*     cull_state_destroy() -- Free thread local storage
*
*  SYNOPSIS
*     static void cull_state_destroy(void* theState) 
*
*  FUNCTION
*     Free thread local storage.
*
*  INPUTS
*     void* theState - Pointer to memory which should be freed.
*
*  RESULT
*     static void - none
*
*  NOTES
*     MT-NOTE: cull_state_destroy() is MT safe.
*
*******************************************************************************/
static void cull_state_destroy(void* theState)
{
   free((struct cull_state_t *)theState);
}

/****** cull_state/cull_state_init() *******************************************
*  NAME
*     cull_state_init() -- Initialize CULL state.
*
*  SYNOPSIS
*     static void cull_state_init(struct cull_state_t* theState) 
*
*  FUNCTION
*     Initialize CULL state.
*
*  INPUTS
*     struct cull_state_t* theState - Pointer to CULL state structure.
*
*  RESULT
*     static void - none
*
*  NOTES
*     MT-NOTE: cull_state_init() is MT safe. 
*
*******************************************************************************/
static void cull_state_init(struct cull_state_t* theState)
{
   theState->lerrno = 0;
   theState->noinit[0] = '\0';
   theState->global_sort_order = NULL;
   theState->chunk_size = CHUNK;
   theState->name_space = NULL;
}
