#include <types.h>
#include <mmap.h>
#include <fork.h>
#include <v2p.h>
#include <page.h>

/* 
 * You may define macros and other helper functions here
 * You must not declare and use any static/global variables 
 * */
#define bs 4096
#define ONLY_LAST_THREE 0xFFF
#define LAST_ZERO 0xFFFFFFFFFFFFF000
#define READ_ONLY 0xFFFFFFFFFFFFFFF7
#define user_bit_check 0x10
#define write_user_access 0x18

static inline void invlpg(u64 addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}


/**
 * mprotect System call Implementation.
 */
int checker(u64* addr_pointer){
    if((((*addr_pointer) & 0x1) == 0) || (((*addr_pointer) & user_bit_check) == 0) ){
        return -1;
    }
    return 1;
}

int protect_helper(struct exec_context *ctx, struct vm_area* vm_space, u64 addr, int length, int prot)
{
    if (addr >= vm_space->vm_end || addr < vm_space->vm_start) return -1;
    if (addr + length > vm_space->vm_end) return -1;
    if (length % bs) return -1;

    int n = length / bs;

    for (int i=0; i < n; i++, addr += bs)
    {
        u64 offset_l1 = ((addr >> 39) % 512) * 8;
        u64 offset_l2 = ((addr >> 30) % 512) * 8;
        u64 offset_l3 = ((addr >> 21) % 512) * 8;
        u64 offset_l4 = ((addr >> 12) % 512) * 8;

        u64 pgd_base_address = (u64) osmap(ctx->pgd);
        u64* pgd = (u64 *)(pgd_base_address + offset_l1);
        if(checker(pgd)==-1){
            continue;
        }

        u64 pud_base_address = (u64)osmap((*pgd)>>12);
        u64* pud = (u64 *)(pud_base_address + offset_l2);
        if(checker(pud)==-1){
            continue;
        }

        u64 pmd_base_address = (u64)osmap((*pud)>>12);
        u64* pmd = (u64 *)(pmd_base_address + offset_l3);
        if(checker(pmd)==-1){
            continue;
        }

        u64 pte_base_address = (u64)osmap((*pmd)>>12);
        u64* pte = (u64 *)(pte_base_address + offset_l4);
        if(checker(pte)==-1){
            continue;
        }

        if(get_pfn_refcount((*pte) >> 12) != 1) continue;
        if (prot == (PROT_WRITE|PROT_READ)){
            *pte = (*pte | 0x8);
        }
        else if (prot == PROT_READ){
            *pte = ((*pte) & READ_ONLY);
        }
        invlpg(addr);
    }
    return 0;
}

long vm_area_mprotect(struct exec_context *current, u64 addr, int length, int prot)
{
    if(!((prot == PROT_READ) || (prot == (PROT_READ|PROT_WRITE)))){
        return -1;
    }
    if(current == NULL || length<0){
        return -1;
    }
    if(addr%4096 != 0){
        return -1;
    }
    struct vm_area* vm_space = current->vm_area;
    u64 size = length - length%bs;
    if(length%bs!=0){
        size = size + bs;
    } 
    u64 end_addr=addr+size;
    if(vm_space==NULL || addr==0 || length==0){
        return 0;
    }
    if(size > 2*1024*1024){
        return -1;
    }
    while(vm_space!=NULL){
        struct vm_area* temp = vm_space->vm_next;
        if(temp==NULL){
            break;
        }
        else if(temp->access_flags ==  prot){
            temp->access_flags = prot;
        }
        else if(temp->vm_start>=addr && temp->vm_start>=end_addr){
            vm_space=vm_space->vm_next;
            continue;
        }
        else if(temp->vm_start>=addr){
            if(temp->vm_end<=end_addr){
                if(protect_helper(current,temp,temp->vm_start,(temp->vm_end - temp->vm_start),prot)<0) return -1;
                temp->access_flags=prot;
            }
            else{
                if(protect_helper(current,temp,temp->vm_start,(end_addr - temp->vm_start),prot)<0) return -1;
                struct vm_area* temp1 = (struct vm_area*)os_alloc(sizeof(struct vm_area));
                temp1->vm_start = end_addr;
                temp1->vm_end = temp->vm_end;
                temp->vm_end = end_addr;
                temp1->vm_next = temp->vm_next;
                temp1->access_flags = temp->access_flags;
                temp->access_flags = prot;

                temp->vm_next = temp1;
                stats->num_vm_area = stats->num_vm_area+1;
                vm_space=vm_space->vm_next;
                break;
            }
        }
        else if(temp->vm_start<addr && temp->vm_end>addr){
            if(temp->vm_end<=end_addr){
                if(protect_helper(current,temp,addr,(temp->vm_end - addr),prot)<0) return -1;
                struct vm_area* temp1 = (struct vm_area*)os_alloc(sizeof(struct vm_area));
                temp1->vm_start = addr;
                temp1->vm_end = temp->vm_end;
                temp->vm_end = addr;
                temp1->vm_next = temp->vm_next;
                temp1->access_flags = prot;

                temp->vm_next = temp1;
                stats->num_vm_area = stats->num_vm_area+1;
                vm_space=vm_space->vm_next;
            }
            else{
                if(protect_helper(current,temp,addr,(end_addr - addr),prot)<0) return -1;
                struct vm_area* temp1 = (struct vm_area*)os_alloc(sizeof(struct vm_area));
                struct vm_area* temp2 = (struct vm_area*)os_alloc(sizeof(struct vm_area));
                temp1->vm_start = end_addr;
                temp1->vm_end = temp->vm_end;
                temp2->vm_start = addr;
                temp2->vm_end = end_addr;
                temp->vm_end = addr;
                temp1->access_flags = temp->access_flags;
                temp2->access_flags = prot;

                temp1->vm_next=temp->vm_next;
                temp2->vm_next = temp1;
                temp->vm_next=temp2;
                stats->num_vm_area = (stats->num_vm_area)+2;
                vm_space=vm_space->vm_next;
                vm_space=vm_space->vm_next;
                break;
            }
        }
        vm_space=vm_space->vm_next;
    }

    // code for merging 
    vm_space = current->vm_area;
    while(vm_space!=NULL){
        struct vm_area* temp = vm_space->vm_next;
        if(temp==NULL){
            break;
        }
        if((vm_space->vm_end == temp->vm_start) && (vm_space->access_flags == temp->access_flags) ){
            vm_space->vm_end = temp->vm_end;
            vm_space->vm_next = temp->vm_next;
            os_free(temp,sizeof(struct vm_area));
            stats->num_vm_area = (stats->num_vm_area)-1;
        }
        else{
            vm_space = vm_space->vm_next;
        }
    }
    return 0;
}

/**
 * mmap system call implementation.
 */

long vm_area_map(struct exec_context *current, u64 addr, int length, int prot, int flags)
{
    if(!addr && flags==MAP_FIXED){
        return -1;
    }
    if(!((prot == PROT_READ) || (prot == (PROT_READ|PROT_WRITE)))){
        return -1;
    }
    if(!(flags==0 || flags == MAP_FIXED)){
        return -1;
    }
    if(current == NULL || length<0){
        return -1;
    }
    if(addr%4096 != 0){
        return -1;
    }
    struct vm_area* vm_space = current->vm_area;
    u64 size = length - length%bs;
    u64 return_addr;
    if(length%bs!=0){
        size = size + bs;
    } 
    if(size > 2*1024*1024){
        return -1;
    }
    if(vm_space==NULL){
        vm_space = (struct vm_area*)os_alloc(sizeof(struct vm_area));
        vm_space->vm_start = MMAP_AREA_START;
        vm_space->vm_end = bs + MMAP_AREA_START;
        vm_space->vm_next = NULL;
        vm_space->access_flags = 0;
        current->vm_area=vm_space;

        stats->num_vm_area = stats->num_vm_area+1;
    }
    if(addr){
        while(vm_space!=NULL){
            if(vm_space->vm_end <= addr && ((vm_space->vm_next == NULL) || ((vm_space->vm_next->vm_start)  >= (addr + size)))){
                return_addr=addr;
                if(vm_space->access_flags == prot && (vm_space->vm_end == addr)){
                    vm_space->vm_end = vm_space->vm_end + size;
                    if(vm_space->vm_next!=NULL && (vm_space->vm_next->vm_start == vm_space->vm_end && vm_space->vm_next->access_flags==prot)){
                        struct vm_area* temp = vm_space->vm_next;
                        vm_space->vm_end = temp->vm_end;
                        vm_space->vm_next = temp->vm_next;
                        os_free(temp,sizeof(struct vm_area));
                        stats->num_vm_area = stats->num_vm_area-1;
                    }
                }
                else{
                    if(vm_space->vm_next!=NULL && ((vm_space->vm_next->vm_start == (addr + size)) && vm_space->vm_next->access_flags==prot)){
                        struct vm_area* temp = vm_space->vm_next;
                        temp->vm_start = addr;
                    }
                    else{
                        struct vm_area* temp = (struct vm_area*)os_alloc(sizeof(struct vm_area));
                        temp->vm_start = addr;
                        temp->vm_end = temp->vm_start + size;
                        temp->vm_next = vm_space->vm_next;
                        temp->access_flags = prot;

                        vm_space->vm_next = temp;
                        stats->num_vm_area = stats->num_vm_area+1;
                    }
                }
                return return_addr; //return address
            }
            vm_space=vm_space->vm_next;
        }
    }
    if(flags==MAP_FIXED){
        return -1;
    }
    // check that below while loop always work
    vm_space = current->vm_area;
    while(vm_space!=NULL){
        if((vm_space->vm_next == NULL) || ((vm_space->vm_next->vm_start) >=  (vm_space->vm_end + size))){
            return_addr=vm_space->vm_end; //return address
            if(vm_space->access_flags == prot){
                vm_space->vm_end = vm_space->vm_end + size;
                if(vm_space->vm_next!=NULL && ((vm_space->vm_next->vm_start == vm_space->vm_end) && (vm_space->vm_next->access_flags==prot))){
                    struct vm_area* temp = vm_space->vm_next;
                    vm_space->vm_end = temp->vm_end;
                    vm_space->vm_next = temp->vm_next;
                    os_free(temp,sizeof(struct vm_area));
                    stats->num_vm_area = stats->num_vm_area-1;
                }
            }
            else{
                if((vm_space->vm_next!=NULL) && ((vm_space->vm_next->vm_start == (vm_space->vm_end + size)) && (vm_space->vm_next->access_flags==prot))){
                    struct vm_area* temp = vm_space->vm_next;
                    temp->vm_start = vm_space->vm_end;
                }
                else{
                    struct vm_area* temp = (struct vm_area*)os_alloc(sizeof(struct vm_area));
                    temp->vm_start = vm_space->vm_end;
                    temp->vm_end = temp->vm_start + size;
                    temp->vm_next = vm_space->vm_next;
                    temp->access_flags = prot;

                    vm_space->vm_next = temp;
                    stats->num_vm_area = stats->num_vm_area+1;
                }
            }
            return return_addr; //return address
        }
        vm_space = vm_space->vm_next;
    }
    return -1;
}

/**
 * munmap system call implemenations
 */
long unmap_pfn(struct exec_context *ctx, struct vm_area* vm_space, u64 addr, int length)
{
    if (addr >= vm_space->vm_end || addr < vm_space->vm_start) return -1;
    if (addr + length > vm_space->vm_end) return -1;
    if (length % bs) return -1;
    int n = (length / bs);

    for (int i=0; i < n; i++, addr += bs)
    {
        u64 offset_l1 = ((addr >> 39) % 512) * 8;
        u64 offset_l2 = ((addr >> 30) % 512) * 8;
        u64 offset_l3 = ((addr >> 21) % 512) * 8;
        u64 offset_l4 = ((addr >> 12) % 512) * 8;

        u64 pgd_base_address = (u64) osmap(ctx->pgd);
        u64* pgd = (u64 *)(pgd_base_address + offset_l1);
        if(checker(pgd)==-1){
            continue;
        }

        u64 pud_base_address = (u64)osmap((*pgd)>>12);
        u64* pud = (u64 *)(pud_base_address + offset_l2);
        if(checker(pud)==-1){
            continue;
        }

        u64 pmd_base_address = (u64)osmap((*pud)>>12);
        u64* pmd = (u64 *)(pmd_base_address + offset_l3);
        if(checker(pmd)==-1){
            continue;
        }

        u64 pte_base_address = (u64)osmap((*pmd)>>12);
        u64* pte = (u64 *)(pte_base_address + offset_l4);
        if(checker(pte)==-1){
            continue;
        }

        put_pfn((*pte) >> 12);
        if (get_pfn_refcount((*pte) >> 12) == 0) os_pfn_free(USER_REG, ((*pte) >> 12));
        (*pte) = 0x0;
        invlpg(addr);
    }
    return 0;
}


long vm_area_unmap(struct exec_context *current, u64 addr, int length)
{
    if(current == NULL || length<0){
        return -1;
    }
    if(addr%4096 != 0){
        return -1;
    }
    struct vm_area* vm_space = current->vm_area;
    u64 size = length - length%bs;
    if(length%bs!=0){
        size = size + bs;
    } 
    if(size > 2*1024*1024){
        return -1;
    }
    u64 end_addr = addr+size;
    if(vm_space==NULL || addr==0 || length==0){
        return 0;
    }
    while(vm_space!=NULL){
        struct vm_area* temp = vm_space->vm_next;
        if(temp==NULL){
            break;
        }
        if(temp->vm_start>=addr && temp->vm_start>=end_addr){
            vm_space=vm_space->vm_next;
            continue;
        }
        else if(temp->vm_start>=addr){
            if(temp->vm_end<=end_addr){
                if(unmap_pfn(current, temp, temp->vm_start, temp->vm_end - temp->vm_start)<0) return -1;
                vm_space->vm_next = temp->vm_next;
                os_free(temp,sizeof(struct vm_area));
                stats->num_vm_area = stats->num_vm_area-1;
            }
            else{
                if(unmap_pfn(current, temp, temp->vm_start, end_addr - temp->vm_start)<0) return -1;
                temp->vm_start=end_addr;
                vm_space=vm_space->vm_next;
            }
        }
        else if(temp->vm_start<addr && temp->vm_end>addr){
            if(temp->vm_end<=end_addr){
                if(unmap_pfn(current, temp, addr, temp->vm_end - addr)<0) return -1;
                temp->vm_end=addr;
                vm_space=vm_space->vm_next;
            }
            else{
                if(unmap_pfn(current, temp, addr, end_addr - addr)<0) return -1;
                struct vm_area* temp1 = (struct vm_area*)os_alloc(sizeof(struct vm_area));
                temp1->vm_start = end_addr;
                temp1->vm_end = temp->vm_end;
                temp->vm_end = addr;
                temp1->vm_next = temp->vm_next;
                temp1->access_flags = temp->access_flags;

                temp->vm_next = temp1;
                stats->num_vm_area = stats->num_vm_area+1;
                break;
            }
        }
        else{
            vm_space=vm_space->vm_next;
        }
    }
    return 0;
}



/**
 * Function will invoked whenever there is page fault for an address in the vm area region
 * created using mmap
 */

int solve(u64* addr_pointer){
    *addr_pointer = ((*addr_pointer) | 0x1);
    *addr_pointer = ((*addr_pointer) & ONLY_LAST_THREE);
    u64 temp = (u64) os_pfn_alloc(OS_PT_REG);
    if (!temp) return -1;
    temp = (u64) osmap(temp);
    *addr_pointer = ((*addr_pointer) | temp);
    return 1;
}

int solve2(u64* addr_pointer){
    *addr_pointer = ((*addr_pointer) | 0x1);
    *addr_pointer = ((*addr_pointer) & ONLY_LAST_THREE);
    u64 temp = (u64) os_pfn_alloc(USER_REG);
    if (!temp) return -1;
    temp = (u64) osmap(temp);
    *addr_pointer = ((*addr_pointer) | temp);
    return 1;
}

long vm_area_pagefault(struct exec_context *current, u64 addr, int error_code)
{
    struct vm_area* vm_space = current->vm_area;
    while(vm_space != NULL)
    {
        if (vm_space->vm_start <= addr && addr < vm_space->vm_end)
        {
            if (error_code == 7 && vm_space->access_flags == PROT_READ) return -1;
            if (error_code == 6 && vm_space->access_flags == PROT_READ) return -1;

            u64 offset_l1 = ((addr >> 39) % 512) * 8;
            u64 offset_l2 = ((addr >> 30) % 512) * 8;
            u64 offset_l3 = ((addr >> 21) % 512) * 8;
            u64 offset_l4 = ((addr >> 12) % 512) * 8;

            u64 pgd_base_addr = (u64) osmap(current->pgd);
            u64* pgd =(u64*)(pgd_base_addr+offset_l1);
            *pgd = ((*pgd) | (write_user_access));

            if(((*pgd)&(0x1)) == 0){
                if(solve(pgd)==-1){
                    return -1;
                }
            }

            u64 pud_base_addr = (u64) osmap(((*pgd) >> 12));
            u64* pud =(u64*)(pud_base_addr+offset_l2);
            *pud = ((*pud) | (write_user_access));

            if(((*pud)&(0x1)) == 0){
                if(solve(pud)==-1){
                    return -1;
                }
            }

            u64 pmd_base_addr = (u64) osmap(((*pud) >> 12));
            u64* pmd =(u64*)(pmd_base_addr+offset_l3);
            *pmd = ((*pmd) | (write_user_access));

            if(((*pmd)&(0x1)) == 0){
                if(solve(pmd)==-1){
                    return -1;
                }
            }

            u64 pte_base_addr = (u64) osmap(((*pmd) >> 12));
            u64* pte =(u64*)(pte_base_addr+offset_l4);
            if(vm_space->access_flags == (PROT_READ|PROT_WRITE)){
                *pte = ((*pte) | (write_user_access));
            }
            else{
                *pte = ((*pte) | (write_user_access));
                *pte = ((*pte) & (READ_ONLY));
            }

            if(((*pte)&(0x1)) == 0){
                if(solve2(pte)==-1){
                    return -1;
                }
            }

            if (error_code == 7)
            {   
                handle_cow_fault(current, addr, vm_space->access_flags);
            }
            return 1;
        }

        vm_space = vm_space->vm_next;
    }
    return -1;
}


/**
 * cfork system call implemenations
 * The parent returns the pid of child process. The return path of
 * the child process is handled separately through the calls at the 
 * end of this function (e.g., setup_child_context etc.)
 */
int fork_helper(struct exec_context *parent, struct exec_context *child, u64 addr, u64 end_addr)
{
    int length = end_addr - addr;
    if (length % bs) return -1;
    int pg_cnt = length / bs;

    for (int i=0; i < pg_cnt; i++, addr += bs)
    {
        u64 pgd_offset = ((addr >> 39) % 512) * 8;
        u64 pud_offset = ((addr >> 30) % 512) * 8;
        u64 pmd_offset = ((addr >> 21) % 512) * 8;
        u64 pte_offset = ((addr >> 12) % 512) * 8;

        u64 pgd_base_address = (u64) osmap(parent->pgd);
        u64* pgd = (u64 *)(pgd_base_address + pgd_offset);
        if(checker(pgd)==-1){
            continue;
        }

        u64 pud_base_address = (u64)osmap((*pgd)>>12);
        u64* pud = (u64 *)(pud_base_address + pud_offset);
        if(checker(pud)==-1){
            continue;
        }

        u64 pmd_base_address = (u64)osmap((*pud)>>12);
        u64* pmd = (u64 *)(pmd_base_address + pmd_offset);
        if(checker(pmd)==-1){
            continue;
        }

        u64 pte_base_address = (u64)osmap((*pmd)>>12);
        u64* pte = (u64 *)(pte_base_address + pte_offset);
        if(checker(pte)==-1){
            continue;
        }

        *pte = ((*pte) & READ_ONLY);
        u64 child_pgd_base_address = (u64) osmap(child->pgd);
        u64* child_pgd = (u64 *)(child_pgd_base_address + pgd_offset);
        *child_pgd = ((*child_pgd) | 0x18);

        if (((*child_pgd) & 0x1) == 0)
        {
            if(solve(child_pgd)== -1){
                return -1;
            }
        } 


        u64 child_pud_base_address = (u64)osmap((*child_pgd) >> 12);
        u64* child_pud = (u64 *)(child_pud_base_address + pud_offset);

        *child_pud = ((*child_pud)| 0x18);

        if ((*child_pud & 0x1) == 0)
        {
            if(solve(child_pud)== -1){
                return -1;
            }
        } 


        u64 child_pmd_base_address = (u64) osmap((*child_pud) >> 12);
        u64* child_pmd = (u64 *)(child_pmd_base_address + pmd_offset);

        *child_pmd = ((*child_pmd )| 0x18);

        if ((*child_pmd & 0x1) == 0)
        {
            if(solve(child_pmd)== -1){
                return -1;
            }
        } 

        u64 child_pte_base_address = (u64) osmap((*child_pmd) >> 12);
        u64* child_pte = (u64 *)(child_pte_base_address + pte_offset);

        *child_pte = *pte; 
        get_pfn((*pte) >> 12);
    }
    return 0;
}

long do_cfork(){
    u32 pid;
    struct exec_context *new_ctx = get_new_ctx();
    struct exec_context *ctx = get_current_ctx();
    /* Do not modify above lines
    * 
    * */   
    /*--------------------- Your code [start]---------------*/
    
    
    pid = new_ctx->pid;
    new_ctx->ppid = ctx->pid;
    new_ctx->pgd = (u64) os_pfn_alloc(OS_PT_REG);
    if((new_ctx->pgd) == 0){
        return -1;
    }
    new_ctx->type = ctx->type;
    new_ctx->used_mem = ctx->used_mem;
    new_ctx->regs = ctx->regs;
    new_ctx->pending_signal_bitmap = ctx->pending_signal_bitmap;
    new_ctx->ticks_to_sleep = ctx->ticks_to_sleep;
    new_ctx->alarm_config_time = ctx->alarm_config_time;
    new_ctx->ticks_to_alarm = ctx->ticks_to_alarm;
    new_ctx->ctx_threads = ctx->ctx_threads;

    for (int i=0;i<MAX_MM_SEGS;i++){
        (new_ctx->mms)[i] = (ctx->mms)[i];
    }
    for (int i=0;i<CNAME_MAX;i++){
        (new_ctx->name)[i] = (ctx->name)[i];
    }
    for (int i=0;i<MAX_SIGNALS;i++){
        (new_ctx->sighandlers)[i] = (ctx->sighandlers)[i];
    }
    for (int i=0;i<MAX_OPEN_FILES;i++){
        (new_ctx->files)[i] = (ctx->files)[i];
    }
        
    struct vm_area* temp = ctx->vm_area;
    struct vm_area* new_temp;
    int flag = 0;

    while(temp!=NULL)
    {
        if(flag == 0){
            new_ctx->vm_area = (struct vm_area*)os_alloc(sizeof(struct vm_area));
            new_temp = new_ctx->vm_area;
            flag=1;
        }
        else{
            new_temp->vm_next = (struct vm_area*)os_alloc(sizeof(struct vm_area));
            new_temp = new_temp->vm_next;
        }
        new_temp->access_flags = temp->access_flags;
        new_temp->vm_start = temp->vm_start;
        new_temp->vm_end = temp->vm_end;
        temp = temp->vm_next;
    }
    new_temp->vm_next = NULL;
    

    if (fork_helper(ctx, new_ctx, new_ctx->mms[MM_SEG_CODE].start, new_ctx->mms[MM_SEG_CODE].next_free) < 0) return -1;
    if (fork_helper(ctx, new_ctx, new_ctx->mms[MM_SEG_RODATA].start, new_ctx->mms[MM_SEG_RODATA].next_free) < 0) return -1;
    if (fork_helper(ctx, new_ctx, new_ctx->mms[MM_SEG_DATA].start, new_ctx->mms[MM_SEG_DATA].next_free) < 0) return -1;
    if (fork_helper(ctx, new_ctx, new_ctx->mms[MM_SEG_STACK].start, new_ctx->mms[MM_SEG_STACK].end) < 0) return -1;

    new_temp = new_ctx->vm_area;

    while(new_temp!=NULL)
    {   
        if (fork_helper(ctx, new_ctx, new_temp->vm_start, new_temp->vm_end) < 0) return -1;
        new_temp = new_temp->vm_next;
    }
    
    /*--------------------- Your code [end] ----------------*/

    /*
    * The remaining part must not be changed
    */
    copy_os_pts(ctx->pgd, new_ctx->pgd);
    do_file_fork(new_ctx);
    setup_child_context(new_ctx);
    return pid;
}


/* Cow fault handling, for the entire user address space
 * For address belonging to memory segments (i.e., stack, data) 
 * it is called when there is a CoW violation in these areas. 
 *
 * For vm areas, your fault handler 'vm_area_pagefault'
 * should invoke this function
 * */
long handle_cow_fault(struct exec_context *current, u64 addr, int access_flags)
{
    invlpg(addr);
    u64 offset_l1 = ((addr >> 39) % 512) * 8;
    u64 offset_l2 = ((addr >> 30) % 512) * 8;
    u64 offset_l3 = ((addr >> 21) % 512) * 8;
    u64 offset_l4 = ((addr >> 12) % 512) * 8;

    u64 pgd_base_address = (u64) osmap(current->pgd);
    u64* pgd = (u64 *)(pgd_base_address + offset_l1);
    if(checker(pgd) == -1){
        return -1;
    }

    u64 pud_base_address = (u64)osmap((*pgd)>>12);
    u64* pud = (u64 *)(pud_base_address + offset_l2);
    if(checker(pud) == -1){
        return -1;
    }

    u64 pmd_base_address = (u64)osmap((*pud)>>12);
    u64* pmd = (u64 *)(pmd_base_address + offset_l3);
    if(checker(pmd) == -1){
        return -1;
    }

    u64 pte_base_address = (u64)osmap((*pmd)>>12);
    u64* pte = (u64 *)(pte_base_address + offset_l4);
    if(checker(pte) == -1){
        return -1;
    }
    
    int ref_cnt = get_pfn_refcount((*pte) >> 12);

    if (ref_cnt < 0){
        return -1;
    }
    else if(ref_cnt == 0){
        os_pfn_free(USER_REG, ((*pte) >> 12));
        return 1;
    }
    if (ref_cnt == 1) 
    {
        *pte = (*pte | 0x8);
        return 1;
    }

    put_pfn((*pte) >> 12);
    u64 source_addr = (u64) osmap((*pte) >> 12);

    *pte = ((*pte )| write_user_access);
    *pte = ((*pte )| 0x1);
    *pte = ((*pte) & ONLY_LAST_THREE);
    u64 temp = (u64) os_pfn_alloc(USER_REG);
    if (!temp) return -1;
    temp = (u64) osmap(temp);
    *pte = ((*pte )| temp);

    u64 dest_addr = temp;
    memcpy((char *)dest_addr, (char *)source_addr, bs);
    return 1;
}

