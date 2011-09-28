#include "linkedList.h"

void initList(DLL *list,
              int (*compare)(void *, void *),
              void (*freeData)(void *),
              int (*map)(void *data))
{
    list->head = NULL;
    list->size = 0;
    list->compare = compare;
    list->freeData = freeData;
    list->map = map;
}

void freeList(DLL* list){
    if(list!=NULL){
        while(list->size>0){
            removeNodeAt(list, 0);
        }
        free(list);
    }
}

void insertNode(DLL *list, void *data)
{
    Node *new = malloc(sizeof(Node));
    new->data = data;

    if (list->head == NULL) {
        new->prev = NULL;
        new->next = NULL;
        list->head = new;
    } else {
        Node *ref = list->head;
        while(ref->next != NULL) {
            ref = ref->next;
        }
        ref->next = new;
        new->prev = ref;
        new->next = NULL;
    }
    list->size++;
}

void removeNode(DLL *list, Node *deadNode)
{
    Node *ref = deadNode;
    Node *head = list->head;
    if (head == NULL) {
        return;
    }

    if(ref->prev == NULL) {
        //Remove from beginning
        list->head = ref->next;
    } else if(ref->next == NULL) {
        //Remove from end
        ref->prev->next = NULL;
    } else {
        ref->prev->next = ref->next;
        ref->next->prev = ref->prev;
    }
    (*list).freeData(ref->data);
    free(ref);
    list->size--;
}

Node *searchList( DLL *list, void *target )
{
    Node *ref=list->head;
    while(ref!=NULL){
        logger(LogDebug, "comparing\n");
        if (list->compare(ref->data, target) == 0) {
             return ref;
        } else {
             ref = ref->next;
        }
    }
    return NULL;

}

void *getNodeDataAt(DLL *list, int idx)
{
    Node *ref = getNodeAt(list, idx);
    return (ref == NULL) ? NULL : ref->data;
}

void removeNodeAt(DLL *list, int idx)
{
    removeNode(list, getNodeAt(list, idx));
}

Node *getNodeAt(DLL *list, int idx)
{
    if(idx < 0 || idx >= list->size) {
        return NULL;
    } else {
        int i;
        Node *ref = list->head;
        for(i = 0; i < idx; i++) {
            ref = ref->next;
        }
        return ref;
    }
}

void mapNode(DLL *list)
{
    Node *ref = list->head;
    if(list->map == NULL) {
        return;
    }
    while(ref != NULL) {
        Node *next = ref->next;
        if(!list->map(ref->data)) {
            removeNode(list, ref);
        }
        ref = next;
    }
}

int compareInt(void *data1, void *data2)
{
    return (int)((intptr_t)data1 - (intptr_t)data2);
}

void freeInt(void *data)
{
    data = data;
}
