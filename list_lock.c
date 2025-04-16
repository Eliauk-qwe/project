#include "list_lock.h"

#include <stdio.h>
#include <stdlib.h>

void listInit(list_lock_t* list) {
   list->head=NULL;
   pthread_mutex_init(&list->mutex,NULL);
   pthread_cond_init(&list->cond,NULL);
}

void producer(list_lock_t* list, DataType value) {
    LNode*  resource = malloc(sizeof(LNode));
    if(resource==NULL){
        perror("malloc() failed!\n");
        exit(1);
    }
    resource->value=value;

    pthread_mutex_lock(&list->mutex);
    resource->next=list->head;
    list->head=resource;
    pthread_cond_signal(&list->cond);
    pthread_mutex_unlock(&list->mutex);


  
}

void consumer(list_lock_t* list) {
    pthread_mutex_lock(&list->mutex);
    while(list->head==NULL){
        pthread_cond_wait(&list->cond,&list->mutex);
    }
    LNode* t =list->head;
    list->head=t->next;
    pthread_mutex_unlock(&list->mutex);
    free(t);
   
}

int getListSize(list_lock_t* list) {
    int count=0;
    pthread_mutex_lock(&list->mutex);
    LNode* p=list->head;
    while(p){
        count++;
        p=p->next;
    }
    pthread_mutex_unlock(&list->mutex);
    return count;


  
}