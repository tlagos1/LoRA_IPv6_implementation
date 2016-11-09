#ifndef LORA_GW_IPV6
#define LORA_GW_IPV6

typedef struct btree {
    int ID;
    struct btree *right, *left;
} node;

void insertInTree(node **,int);
void deleteInTree(node **,int);
node *searchInTree(node **, int);


#endif