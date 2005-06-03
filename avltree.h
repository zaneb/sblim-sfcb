
/*
 * avltree.h
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
 * Author:       Sebastian Seyrich <seyrich@de.ibm.com>
 *
 * Description:
 * AVL-Tree Datatype
 *
 * 
 *
*/


#ifndef AVL_H
#define AVL_H
#define AVL_STACK_SIZE 1000

struct avlTree;
typedef struct avlTree AvlTree;
struct avlNode;
typedef struct avlNode AvlNode;


typedef struct avlNode {
    void *item;
    struct avlNode *left, *right;
    signed char balance;
};

typedef struct avlTree {
    AvlNode *root;
    int n;
    int (* compar)(const void * a, const void *b);
    AvlNode **stack;
    void (*free)(AvlTree **this);
    void* (*insert)(AvlTree *this, void *item);
    void* (*find)(AvlTree *this, void *item);
    void* (*findMin)(AvlTree *this);
    void* (*delete)(AvlTree *this, void *item);
    void* (*deleteMin)(AvlTree *this);
    signed char *pathInfo;  
};


AvlTree *newAvlTree(int (* compar)(const void *, const void *));


#endif
