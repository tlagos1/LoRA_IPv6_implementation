#include <stdint.h>     /* C99 types */
#include <stdbool.h>    /* bool type */
#include <stdio.h>      /* printf fprintf */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "vector.h"

node *getMinimumRightChildDelete(node **);

void btree_init(node *t)
{
    t->ID=5;
    t->right=t->left=NULL;
}

void insert(node **tree,int ID)
{
    node *Aux = NULL;
    if(!(*tree))
    {
        Aux = (node *)malloc(sizeof(node));
        Aux->left = Aux->right = NULL;
        Aux->ID = ID;
        *tree = Aux;
        return;
    }
    if(ID < (*tree)->ID)
    {
        insert(&(*tree)->left, ID);
    }
    if(ID > (*tree)->ID)
    {
        insert(&(*tree)->right, ID);
    }
}

void deleteInTree(node **tree, int value)
{
    if(value == (*tree)->ID)
    {
        node *Aux2 = (node *)malloc(sizeof(node));

        if((*tree)->right == NULL && (*tree)->left == NULL)
        {
            (*tree) = NULL;
        }

        else if((*tree)->right != NULL && (*tree)->left != NULL)
        {
            Aux2->left = (*tree)->left;
            Aux2->right = (*tree)->right; 
            (*tree) = getMinimumRightChildDelete(&(*tree)->right);

            if((*tree)->ID == Aux2->right->ID)
            {
                (*tree)->left = Aux2->left;
            }
            else
            {
                (*tree)->right = Aux2->right;
                (*tree)->left = Aux2->left;
            }
        }

        else if((*tree)->right != NULL && (*tree)->left == NULL)
        {
            (*tree) = (*tree)->right;
        }

        else if((*tree)->right == NULL && (*tree)->left != NULL)
        {
            (*tree) = (*tree)->left;
        }
    }
    else if(value > (*tree)->ID)
    {
         deleteInTree(&(*tree)->right,value);
    }
    else if(value < (*tree)->ID)
    {
         deleteInTree(&(*tree)->left,value);
    }         
}

node *getMinimumRightChildDelete(node **tree)
{
    if((*tree)->left == NULL)
    {

        node *Aux = NULL;

        if((*tree)->right !=NULL)
        {
            Aux = (node *)malloc(sizeof(node));
            Aux = (*tree);
            (*tree) = (*tree)->right;
            return Aux;
        }
        else
        {
            Aux = (node *)malloc(sizeof(node));
            Aux = (*tree);
            (*tree) = NULL;
            return Aux;
        }    
    }
    else
    {
       return getMinimumRightChildDelete(&(*tree)->left);
    }
}


node *searchInTree(node **tree,int ID)
{
    if((*tree)->ID > ID)
    {
        print_preorder(&(*tree)->left);
    }
    else if((*tree)->ID < ID)
    {
        print_preorder(&(*tree)->right);
    }    
    else if((*tree)->value == ID)
    {
        return (*tree);
    }

}