#include <stdio.h>
#include <sys/mman.h>

static unsigned long * head = NULL;

void *memalloc(unsigned long size) 
{
    if(size == 0){
        return NULL;
    }
    size = size+8;
    if(size<24){
        size = 24;
    }
    if(size%8!=0){
        size=size+8-size%8;
    }
    unsigned long *ptr = head;
    unsigned long *ans = NULL;
    unsigned long *next= NULL;
    unsigned long *prev= NULL;

    while (ptr != NULL) {
        unsigned long ptr_size = *(ptr);     
        if (ptr_size >= size) {
            if (ptr_size >= size + 24) {

                next = *(ptr+1);
                prev = *(ptr+2);
                if(next!=NULL){
                    *(next+2)=prev;
                }
                if(prev!=NULL){
                    *(prev+1)=next;
                }
                ans=ptr;
                *ans=size;

                ptr=ptr+size/8;
                *ptr=ptr_size-size;
                *(ptr+1)=head;
                *(ptr+2)=NULL;
                if(head!=NULL){
                    *(head+2)=ptr;
                }
                head=ptr;
            }
            else{
                ans=ptr;
                *ans=ptr_size;
                
                next = *(ptr+1);
                prev = *(ptr+2);
                if(next!=NULL){
                    *(next+2)=prev;
                }
                if(prev!=NULL){
                    *(prev+1)=next;
                }
            }
            ans=ans+1;
            return ans;
        }
        ptr=*(ptr+1);
    }
    unsigned long int total_size;
    unsigned long int M_B = 4*1024*1024; 
    if(size%M_B!=0){
        total_size=M_B*(1+size/M_B);
    }
    else{
        total_size=size;
    }
    unsigned long * new_ptr;
    new_ptr = mmap(NULL, total_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
    if(new_ptr == MAP_FAILED){
        return NULL;
    }
    unsigned long new_ptr_size = total_size;
    if (new_ptr_size >= size + 24) {
        ans=new_ptr;
        *ans=size;
        new_ptr=new_ptr+size/8;
        *new_ptr=new_ptr_size-size;
        *(new_ptr+1)=head;
        *(new_ptr+2)=NULL;
        if(head!=NULL){
            *(head+2)=new_ptr;
        }
        head=new_ptr;
    }
    else{
        ans=new_ptr;
        *ans=new_ptr_size;
    }
    ans=ans+1;
    return ans;
}

int memfree(void *ptr)
{
	if(ptr==NULL){
        return 0;
    }
    unsigned long * new_ptr = ptr;
    new_ptr=new_ptr-1;
    unsigned long * temp = NULL;
    unsigned long * temp1 = NULL;
    unsigned long * left = head;
    unsigned long * right = head;
    while(left!=NULL){
        if(*(left)/8+left == new_ptr){
            temp=*(left+2);
            temp1=*(left+1);
            if(temp!=NULL){
                *(temp+1) = temp1;
            }
            if(temp1!=NULL){
                *(temp1+2)=temp;
            }
            break;
        }
        left = *(left+1);
    }
    while(right!=NULL){
        if( right == new_ptr+*(right)/8){
            temp=*(right+2);
            temp1=*(right+1);
            if(temp!=NULL){
                *(temp+1) = temp1;
            }
            if(temp1!=NULL){
                *(temp1+2)=temp;
            }
            break;
        }
        right = *(right+1);
    }
    if(left!=NULL && right!=NULL){
        *(left)=*(left)+ *(new_ptr) + *(right);
        new_ptr = left;
    }
    else if(left!=NULL){
        *(left)=*(left)+ *(new_ptr);
        new_ptr = left;
    }
    else if(right!=NULL){
        *(new_ptr) = *(new_ptr) + *(right); 
    }
    *(new_ptr+1)=head;
    *(head+2)=new_ptr;
    *(new_ptr+2)=NULL;
    head=new_ptr;
    return 0;
}