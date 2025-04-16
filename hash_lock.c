#include "hash_lock.h"

#include <stdio.h>
#include <stdlib.h>

void hashInit(hash_lock_t* bucket) {
    for(int i=0; i < HASHNUM ;i++){
        pthread_mutex_init(&bucket->table[i].mutex,NULL);
        bucket->table[i].head=NULL;
    }
}

int getValue(hash_lock_t* bucket, int key) {
    int index=HASH(key);
    pthread_mutex_lock(&bucket->table[index].mutex);
    
    Hnode* p=bucket->table[index].head;
    while(p){
        if(p->key==key){
            int val=p->value;
            pthread_mutex_unlock(&bucket->table[index].mutex);
            return val;
        }
        p=p->next;
    }

    pthread_mutex_unlock(&bucket->table[index].mutex);
    return -1;


  
}

void insert(hash_lock_t* bucket, int key, int value) {
    int index=HASH(key);
    pthread_mutex_lock(&bucket->table[index].mutex);

    Hnode* p =bucket->table[index].head;
    while (p)
    {
        if(p->key==key){
            p->value=value;
            pthread_mutex_unlock(&bucket->table[index].mutex);
            return;
        }
        p=p->next;
    }
    


    Hnode* hash =malloc(sizeof(Hnode));
    if(hash==NULL){
        perror("malloc() failed!\n");
        return -1;
    }
    hash->value=value;
    hash->key=key;
    hash->next=bucket->table[index].head;
    bucket->table[index].head=hash;

    pthread_mutex_unlock(&bucket->table[index].mutex);
  
}

int setKey(hash_lock_t* bucket, int key, int new_key) {
    int old_index=HASH(key);
    int new_index=HASH(new_key);

    if(old_index<new_key){
        pthread_mutex_lock(&bucket->table[old_index].mutex);
        pthread_mutex_lock(&bucket->table[new_index].mutex); 
    }
    else if(old_index>new_key){
        pthread_mutex_lock(&bucket->table[new_index].mutex); 
        pthread_mutex_lock(&bucket->table[old_index].mutex);
    }
    else{
        pthread_mutex_lock(&bucket->table[old_index].mutex);
    }

    Hnode* pre=NULL;
    Hnode* p=bucket->table[old_index].head;
    while(p){
        if(p->key==key){
            if(pre!=NULL){
                pre->next=p->next;
            }else{
                bucket->table[old_index].head=p->next;
            }


            if(old_index!=new_index){
                p->next=bucket->table[new_index].head;
                bucket->table[new_index].head=p;
            }
            p->key=new_key;

            if(old_index!=new_index){
                pthread_mutex_lock(&bucket->table[new_index].mutex); 
            }
            pthread_mutex_lock(&bucket->table[old_index].mutex); 

            return 0;



        }

        pre=p;
        p=p->next;
    }


    if(old_index!=new_index){
        pthread_mutex_lock(&bucket->table[new_index].mutex); 
    }
    pthread_mutex_lock(&bucket->table[old_index].mutex); 

    return -1;

  
}