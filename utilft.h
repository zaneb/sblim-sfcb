
/*
 * util.h
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
 * Author:        Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Utilitysupport.
 *
*/


#ifndef _UTILFT_H_
#define _UTILFT_H_

#include "providerRegister.h"
#include "hashtable.h"

#ifdef __cplusplus
extern "C" {
#endif

   struct _Util_HashTable_FT;
   typedef struct _Util_HashTable_FT Util_HashTable_FT;

   struct _UtilHashTable {
      void *hdl;
      Util_HashTable_FT *ft;
   };
   typedef struct _UtilHashTable UtilHashTable;

   struct _Util_HashTable_FT {
      int version;
      void (*release)
       (UtilHashTable * ht);
      UtilHashTable *(*clone)
       (UtilHashTable * ht);
      void (*clear)
       (UtilHashTable * ht);
      int (*containsKey)
       (const UtilHashTable * ht, const void *key);
      int (*containsValue)
       (const UtilHashTable * ht, const void *value);
      int (*put)
       (UtilHashTable * ht, const void *key, void *value);
      void *(*get)
       (const UtilHashTable * ht, const void *key);
      void (*remove)
       (UtilHashTable * ht, const void *key);
      int (*isEmpty)
       (const UtilHashTable * ht);
      int (*size)
       (const UtilHashTable * ht);
      int (*buckets)
       (const UtilHashTable * ht);
      void (*rehash)
       (UtilHashTable * ht, int buckets);

      HashTableIterator *(*getFirst)
       (UtilHashTable * ht, void **key, void **value);
      HashTableIterator *(*getNext)
       (UtilHashTable * ht, HashTableIterator * iterator, void **key,
        void **value);

      void (*setKeyCmpFunction)
       (UtilHashTable * ht, int (*keycomp) (const void *k1, const void *k2));
      void (*setValueCmpFunction)
       (UtilHashTable * ht, int (*keycomp) (const void *v1, const void *v2));
      void (*setHashFunction)
       (UtilHashTable * ht, unsigned long (*hashFunction) (const void *key));
      void (*setReleaseFunctions)
       (UtilHashTable * ht, void (*keyRelease) (void *key),
        void (*valueRelease) (void *value));
   };

#define UtilHashTable_charKey 1
#define UtilHashTable_CMPIStringKey 2
#define UtilHashTable_ignoreKeyCase 4
#define UtilHashTable_managedKey 8
#define UtilHashTable_charValue 16
#define UtilHashTable_CMPIStyleValue 32
#define UtilHashTable_ignoreValueCase 64
#define UtilHashTable_managedValue 128


   struct _Util_List_FT;
   typedef struct _Util_List_FT Util_List_FT;

   struct _UtilList {
      void *hdl;
      Util_List_FT *ft;
      int  mem_state;
   };
   typedef struct _UtilList UtilList;

   struct _Util_List_FT {
      int version;
      void (*release)
       (UtilList * ul);
      UtilList *(*clone)
       (UtilList * ul);
      void (*clear)
       (UtilList * ul);
      unsigned long (*size)
       (UtilList * ul);
      int (*isEmpty)
       (UtilList * ul);
      int (*contains)
       (UtilList * ul, const void *elm);
      void (*append)
       (UtilList * ul, const void *elm);
      void (*prepend)
       (UtilList * ul, const void *elm);
      void (*add)
       (UtilList * ul, const void *elm);
      void *(*getFirst)
       (UtilList * ul);
      void *(*getLast)
       (UtilList * ul);
      void *(*getNext)
       (UtilList * ul);
      void *(*getPrevious)
       (UtilList * ul);
      void *(*getCurrent)
       (UtilList * ul);
      void *(*removeFirst)
       (UtilList * ul);
      void *(*removeLast)
       (UtilList * ul);
      void *(*removeCurrent)
       (UtilList * ul);
      void *(*removeThis)
       (UtilList * ul, void *elm);
   };



   struct _Provider_Register_FT;
   typedef struct _Provider_Register_FT Provider_Register_FT;

   struct _ProviderRegister {
      void *hdl;
      Provider_Register_FT *ft;
   };
   typedef struct _ProviderRegister ProviderRegister;

   typedef struct _ProviderBase {
      char *fn;
      UtilHashTable *ht;
   } ProviderBase;

   struct _Provider_Register_FT {
      int version;
      void (*release) (ProviderRegister * br);
      ProviderRegister *(*clone) (ProviderRegister * br);
      ProviderInfo *(*getProvider) (ProviderRegister * br,
                                    const char *clsName, unsigned long type);
      int (*putProvider) (ProviderRegister * br, const char *clsName,
                          ProviderInfo * info);
      void (*removeProvider) (ProviderRegister * br, const char *clsName);
      ProviderInfo *(*locateProvider) (ProviderRegister * br,
                                       const char *provName);
   };

   extern Provider_Register_FT *ProviderRegisterFT;
   struct _Provider_Register_FT;


/*
   typedef struct _Class_Register_FT Class_Register_FT;
   struct _ClassRegister {
      void *hdl;
      Class_Register_FT *ft;
   };
   typedef struct _ClassRegister ClassRegister;

   typedef struct _ClassBase {
      char *fn;
      UtilHashTable *ht;
      UtilHashTable *it;
   } ClassBase;

   struct _Class_Register_FT {
      int version;
      void (*release) (ClassRegister * br);
      ClassRegister *(*clone) (ClassRegister * br);
      CMPIConstClass *(*getClass) (ClassRegister * br, const char *clsName);
      int (*putClass) (ClassRegister * br, CMPIConstClass * cls);
      UtilList *(*getChildren) (ClassRegister * br, const char *className);
   };

   extern Class_Register_FT *ClassRegisterFT;

*/

   typedef struct _Util_StringBuffer_FT Util_StringBuffer_FT;
   struct _UtilStringBuffer {
      void *hdl;
      Util_StringBuffer_FT *ft;
      int max, len;
   };
   typedef struct _UtilStringBuffer UtilStringBuffer;

   struct _Util_StringBuffer_FT {
      int version;
      void (*release) (UtilStringBuffer * sb);
      UtilStringBuffer *(*clone) (UtilStringBuffer * sb);
      const char *(*getCharPtr) (UtilStringBuffer * sb);
      unsigned int (*getSize) (UtilStringBuffer * sb);
      void (*appendChars) (UtilStringBuffer * sb, const char *chars);
      void (*appendString) (UtilStringBuffer * sb, CMPIString * string);
      void (*reset) (UtilStringBuffer * sb);
   };



   struct _Util_Factory_FT;
   typedef struct _Util_Factory_FT Util_Factory_FT;

   struct _Util_Factory_FT {
      int version;
      UtilHashTable *(*newHashTableDefault) (long buckets);
      UtilHashTable *(*newHashTable) (long buckets, long opt);
      UtilList *(*newList) ();
      ProviderRegister *(*newProviderRegister) (char *fn);
      UtilStringBuffer *(*newStrinBuffer) (int s);
   };

   extern Util_Factory_FT *UtilFactory;


#ifdef __cplusplus
}
#endif
#endif                          // _UTILFT_H_
