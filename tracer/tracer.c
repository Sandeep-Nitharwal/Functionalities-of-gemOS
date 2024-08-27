// #include<context.h>
// #include<memory.h>
// #include<lib.h>
// #include<entry.h>
// #include<file.h>
// #include<tracer.h>


// ///////////////////////////////////////////////////////////////////////////
// //// 		Start of Trace buffer functionality 		      /////
// ///////////////////////////////////////////////////////////////////////////

// int is_valid_mem_range(unsigned long buff, u32 count, int access_bit) 
// {	
// 	struct exec_context *ctx = get_current_ctx();
// 	if(ctx==NULL){
// 		return 0;
// 	}
// 	struct vm_area * temp = ctx->vm_area;
// 	while(temp!=NULL){
// 		if(((temp->vm_start) <= buff) && ((temp->vm_end) > buff + count -1 )){
// 			if(((temp->access_flags) & access_bit) == access_bit){
// 				return 1;
// 			}
// 			else{
// 				return 0;
// 			}
// 		}
// 		temp=temp->vm_next;
// 	}
// 	for(int i=0;i<MAX_MM_SEGS;i++){
// 		if((i== MM_SEG_STACK && (((ctx->mms[i]).start) <= buff) && (((ctx->mms[i]).end) > buff + count -1)) || (((ctx->mms[i]).start) <= buff) && (((ctx->mms[i]).next_free) > buff + count -1)){
// 			if((((ctx->mms[i]).access_flags) & access_bit) == access_bit){
// 				return 1;
// 			}
// 			else{
// 				return 0;
// 			}
// 		} 
// 	}
// 	return 0;
// }

// long trace_buffer_close(struct file *filep)
// {
// 	if(filep == NULL || filep->fops==NULL || filep->trace_buffer==NULL || filep->trace_buffer->buf==NULL){
// 		return -EINVAL;
// 	}
// 	struct exec_context* ctx = get_current_ctx();
// 	int fd=0;
// 	while(fd<MAX_OPEN_FILES){
// 		if((ctx->files)[fd]==filep){
// 			(ctx->files)[fd] = NULL;
// 			break;
// 		}
// 		fd=fd+1;
// 	}
// 	os_free(filep->fops,sizeof(struct fileops));
// 	os_page_free(USER_REG, filep->trace_buffer->buf);
// 	os_free(filep->trace_buffer,sizeof(struct trace_buffer_info));
// 	os_free(filep,sizeof(struct file));
// 	filep->fops = NULL;
// 	filep->trace_buffer->buf = NULL;
// 	filep->trace_buffer = NULL;
// 	filep = NULL;
// 	return 0;	
// }

// int trace_buffer_read(struct file *filep, char *buff, u32 count)
// {	
// 	if(filep == NULL || buff == NULL || count<0){
// 		return -EINVAL;
// 	}
// 	if(filep->type!=TRACE_BUFFER || filep->mode == O_WRITE){
//         return -EINVAL;
//     }
// 	if(filep->trace_buffer == NULL || filep->trace_buffer->buf == NULL){
// 		return -EINVAL;
// 	}
// 	if(is_valid_mem_range(((unsigned long)buff),count,2)==0){
// 		return -EBADMEM;
// 	}
// 	u32 i=0;
// 	u32 r_ofset = filep->trace_buffer->read_offset;
// 	u32 w_ofset = filep->trace_buffer->write_offset;
// 	u32 size = filep->trace_buffer->used_size;
// 	if(size>count){
// 		size=count;
// 	}
// 	while(i<size){
// 		buff[i] = (filep->trace_buffer->buf)[(r_ofset+i)%TRACE_BUFFER_MAX_SIZE];
// 		i=i+1;
// 	}
//     filep->trace_buffer->read_offset = (r_ofset + size)%TRACE_BUFFER_MAX_SIZE ;
// 	filep->trace_buffer->used_size -= size; 
// 	filep->offp = filep->trace_buffer->read_offset;
// 	return size;
// }

// int trace_buffer_write(struct file *filep, char *buff, u32 count)
// {
// 	if(filep == NULL || buff == NULL || count<0){
// 		return -EINVAL;
// 	}
// 	if(filep->type!=TRACE_BUFFER || filep->mode == O_READ){
//         return -EINVAL;
//     }
// 	if(filep->trace_buffer == NULL || filep->trace_buffer->buf == NULL){
// 		return -EINVAL;
// 	}
// 	if(is_valid_mem_range(((unsigned long)buff),count,1)==0){
//         return -EBADMEM;
//     }
// 	u32 i=0;
// 	u32 r_ofset = filep->trace_buffer->read_offset;
// 	u32 w_ofset = filep->trace_buffer->write_offset;
// 	u32 size = TRACE_BUFFER_MAX_SIZE - filep->trace_buffer->used_size;
// 	if(size>count){
// 		size=count;
// 	}
// 	while(i<size){
// 		(filep->trace_buffer->buf)[(w_ofset+i)%TRACE_BUFFER_MAX_SIZE] = buff[i];
// 		i=i+1;
// 	}
// 	filep->trace_buffer->write_offset = (w_ofset + size)%TRACE_BUFFER_MAX_SIZE ;
//     filep->trace_buffer->used_size += size;
// 	filep->offp = filep->trace_buffer->write_offset;
// 	return size;
// }

// int sys_create_trace_buffer(struct exec_context *current, int mode)
// {
// 	if(current==NULL || current->files == NULL){
// 		return -EINVAL;
// 	}
// 	if(mode!=O_READ && mode!=O_WRITE && mode!=O_RDWR){
// 		return -EINVAL;
// 	}
// 	int i=0;
// 	while(i<MAX_OPEN_FILES){
// 		if((current->files)[i]==NULL){
// 			break;
// 		}
// 		i=i+1;
// 	}
// 	if(i>=MAX_OPEN_FILES){
// 		return -EINVAL;
// 	}
// 	struct file* temp = (struct file*)os_alloc(sizeof(struct file));
// 	if(temp == NULL){
// 		return -ENOMEM;
// 	}
// 	temp->type = TRACE_BUFFER;
// 	temp->mode = mode;
// 	temp->offp = 0;
// 	temp->ref_count = 1;
// 	temp->inode = NULL;

// 	struct trace_buffer_info* buffer = (struct trace_buffer_info*)os_alloc(sizeof(struct trace_buffer_info));
// 	if(buffer == NULL){
// 		return -ENOMEM;
// 	}
// 	buffer->write_offset=0;
// 	buffer->read_offset=0;
// 	buffer->used_size=0;
// 	buffer->buf = (char *)os_page_alloc(USER_REG);
// 	if(buffer->buf == NULL){
// 		return -ENOMEM;
// 	}

// 	struct fileops* operators = (struct fileops*)os_alloc(sizeof(struct fileops));
// 	if(operators == NULL){
// 		return -ENOMEM;
// 	}
// 	operators->read = trace_buffer_read;
// 	operators->write = trace_buffer_write;
// 	operators->close = trace_buffer_close;
// 	operators->lseek = NULL;

// 	temp->fops=operators;
// 	temp->trace_buffer = buffer;
// 	(current->files)[i]=temp;
// 	return i;
// }

// ///////////////////////////////////////////////////////////////////////////
// //// 		Start of strace functionality 		      	      /////
// ///////////////////////////////////////////////////////////////////////////

// int trace_buff_read(struct file *filep, char *buff, u32 count)
// {	
// 	if(filep == NULL || buff == NULL || count<0){
// 		return -EINVAL;
// 	}
// 	if(filep->type!=TRACE_BUFFER || filep->mode == O_WRITE){
//         return -EINVAL;
//     }
// 	if(filep->trace_buffer == NULL || filep->trace_buffer->buf == NULL){
// 		return -EINVAL;
// 	}
// 	u32 i=0;
// 	u32 r_ofset = filep->trace_buffer->read_offset;
// 	u32 w_ofset = filep->trace_buffer->write_offset;
// 	u32 size = filep->trace_buffer->used_size;
// 	if(size>count){
// 		size=count;
// 	}
// 	while(i<size){
// 		buff[i] = (filep->trace_buffer->buf)[(r_ofset+i)%TRACE_BUFFER_MAX_SIZE];
// 		i=i+1;
// 	}
//     filep->trace_buffer->read_offset = (r_ofset + size)%TRACE_BUFFER_MAX_SIZE ;
// 	filep->trace_buffer->used_size -= size; 
// 	filep->offp = filep->trace_buffer->read_offset;
// 	return size;
// }

// int trace_buff_write(struct file *filep, char *buff, u32 count)
// {
// 	if(filep == NULL || buff == NULL || count<0){
// 		return -EINVAL;
// 	}
// 	if(filep->type!=TRACE_BUFFER || filep->mode == O_READ){
//         return -EINVAL;
//     }
// 	if(filep->trace_buffer == NULL || filep->trace_buffer->buf == NULL){
// 		return -EINVAL;
// 	}
// 	u32 i=0;
// 	u32 r_ofset = filep->trace_buffer->read_offset;
// 	u32 w_ofset = filep->trace_buffer->write_offset;
// 	u32 size = TRACE_BUFFER_MAX_SIZE - filep->trace_buffer->used_size;
// 	if(size>count){
// 		size=count;
// 	}
// 	while(i<size){
// 		(filep->trace_buffer->buf)[(w_ofset+i)%TRACE_BUFFER_MAX_SIZE] = buff[i];
// 		i=i+1;
// 	}
// 	filep->trace_buffer->write_offset = (w_ofset + size)%TRACE_BUFFER_MAX_SIZE ;
//     filep->trace_buffer->used_size += size;
// 	filep->offp = filep->trace_buffer->write_offset;
// 	return size;
// }

// int perform_tracing(u64 syscall_num, u64 param1, u64 param2, u64 param3, u64 param4)
// {
// 	struct exec_context *current = get_current_ctx();
// 	if(syscall_num == SYSCALL_END_STRACE){
// 		return 0;
// 	}
// 	if(current->st_md_base==NULL){
// 		struct strace_head* head = (struct strace_head*)os_alloc(sizeof(struct strace_info));
// 		head->count=0;
// 		head->is_traced=0;
// 		head->next=NULL;
// 		head->last=NULL;
// 		current->st_md_base = head;
// 	}
// 	if((current->st_md_base->is_traced)==0){
// 		return 0;
// 	}
// 	if(current->st_md_base->tracing_mode == FILTERED_TRACING){
// 		struct strace_info* head = current->st_md_base->next;
// 		while(head!=NULL){
// 			if((head->syscall_num)==syscall_num){
// 				break;
// 			}
// 			head=(head->next);
// 		}
// 		if(head==NULL){
// 			return 0;
// 		}
// 	}
	
// 	u64 sn = syscall_num;
// 	struct file* filep = (current->files)[current->st_md_base->strace_fd];
	
// 	if(trace_buff_write(filep, &syscall_num, 8)!=8){
// 		return 0;
// 	}
// 	if(sn==1 || sn==7 || sn==12 || sn==14  || sn==19 || sn==27 || sn==29 || sn==36 ){ // 1 para
// 		if(trace_buff_write(filep, &param1, 8)!=8){
// 			return 0;
// 		}
// 	}
// 	if(sn==4|| sn==8 || sn==9 || sn==17 || sn==23 || sn==28 || sn==37 || sn==40){ // 2 para
// 		if(trace_buff_write(filep, &param1, 8)!=8){
// 			return 0;
// 		}
// 		if(trace_buff_write(filep, &param2, 8)!=8){
// 			return 0;
// 		}
// 	}
// 	if(sn==18|| sn==24 || sn==25 || sn==30 || sn==39 || sn==41){ // 3 para
// 		if(trace_buff_write(filep, &param1, 8)!=8){
// 			return 0;
// 		}
// 		if(trace_buff_write(filep, &param2, 8)!=8){
// 			return 0;
// 		}
// 		if(trace_buff_write(filep, &param3, 8)!=8){
// 			return 0;
// 		}
// 	}
// 	if(sn==16 || sn==35){ //4 para
// 		if(trace_buff_write(filep, &param1, 8)!=8){
// 			return 0;
// 		}
// 		if(trace_buff_write(filep, &param2, 8)!=8){
// 			return 0;
// 		}
// 		if(trace_buff_write(filep, &param3, 8)!=8){
// 			return 0;
// 		}
// 		if(trace_buff_write(filep, &param4, 8)!=8){
// 			return 0;
// 		}
// 	}
//     return 0;
// }

// int sys_strace(struct exec_context *current, int syscall_num, int action)
// {
// 	if(current==NULL ){
// 		return -EINVAL;
// 	}
// 	if(action!=ADD_STRACE && action!=REMOVE_STRACE){
// 		return -EINVAL;
// 	}
// 	if(current->st_md_base==NULL){
// 		struct strace_head* head = (struct strace_head*)os_alloc(sizeof(struct strace_head));
// 		if(head == NULL){
// 			return -EINVAL;
// 		}
// 		head->count=0;
// 		head->is_traced=0;
// 		head->next=NULL;
// 		head->last=NULL;
// 		current->st_md_base = head;
// 	}
// 	if(action == ADD_STRACE){
// 		if(current->st_md_base->count >= STRACE_MAX){
// 			return -EINVAL;
// 		}
// 		struct strace_info* head = current->st_md_base->next;
// 		while(head!=NULL){
// 			if((head->syscall_num)==syscall_num){
// 				break;
// 			}
// 			head=(head->next);
// 		}
// 		if(head!=NULL){
// 			return -EINVAL;
// 		}
// 	}
	
// 	if(action == ADD_STRACE){
// 		struct strace_info* head = (struct strace_info* )os_alloc(sizeof(struct strace_info));
// 		if(head == NULL){
// 			return -EINVAL;
// 		}
// 		head->syscall_num = syscall_num;
// 		head->next = NULL;
// 		if((current->st_md_base->last)==NULL){
// 			current->st_md_base->next=head;
// 			current->st_md_base->last=head;
// 		}
// 		else{
// 			head->next = current->st_md_base->next;
// 			current->st_md_base->next=head;
// 		}
// 		(current->st_md_base->count)+=1;
// 	}
// 	else{
// 		if(current->st_md_base->count == 0){
// 			return -EINVAL;
// 		}
// 		struct strace_info* head = current->st_md_base->next;
// 		struct strace_info* temp = head;
// 		struct strace_info* temp2 = head;
// 		while(temp!=NULL){
// 			if(temp->syscall_num == syscall_num){
// 				if(temp==head){
// 					if(temp == current->st_md_base->last){
// 						current->st_md_base->next=NULL;
// 						current->st_md_base->last=NULL;
// 					}
// 					else{
// 						current->st_md_base->next=temp->next;
// 					}
// 				}
// 				else{
// 					if(temp == current->st_md_base->last){
// 						current->st_md_base->last=temp2;
// 					}
// 					temp2->next=temp->next;
// 				}
// 				os_free(temp,sizeof(struct strace_info));
// 				(current->st_md_base->count)-=1;
// 				return 0;
// 			}
// 			temp2 = temp;
// 			temp = temp->next;
// 		}
// 		return -EINVAL;
// 	}
//     return 0;
// }

// int sys_read_strace(struct file *filep, char *buff, u64 count)
// {
// 	if(filep==NULL || count<0){
// 		return -EINVAL;
// 	}
// 	struct exec_context *current = get_current_ctx();
// 	int j=0;
// 	int sn =0;
// 	int offset=0,val=0;
// 	while(j<count){
// 		val=trace_buff_read(filep, buff+offset, 8);
// 		if(val<0){
// 			return -EINVAL;
// 		}
// 		if(val!=8){
// 			return offset;
// 		}
// 		sn = (((u64)buff[offset + 7] << 56) + ((u64)buff[offset + 6] << 48) +
// 			((u64)buff[offset + 5] << 40) + ((u64)buff[offset + 4] << 32) +
// 			((u64)buff[offset + 3] << 24) + ((u64)buff[offset + 2] << 16) +
// 			((u64)buff[offset + 1] << 8) + (u64)buff[offset]);
// 		offset=offset+val;
// 		if(sn==1 || sn==7 || sn==12 || sn==14  || sn==19 || sn==27 || sn==29 || sn==36){ // 1 para
// 			val=trace_buff_read(filep, buff+offset, 8);
// 			if(val<0){
// 				return -EINVAL;
// 			}
// 			if(val!=8){
// 				return offset;
// 			}
// 			offset=offset+val;
// 		}
// 		if(sn==4|| sn==8 || sn==9 || sn==17 || sn==23 || sn==28 || sn==37 || sn==40){ // 2 para
// 			int k=1;
// 			while(k<=2){
// 				val=trace_buff_read(filep, buff+offset, 8);
// 				if(val<0){
// 					return -EINVAL;
// 				}
// 				if(val!=8){
// 					return offset;
// 				}
// 				k++;
// 				offset=offset+val;
// 			}
// 		}
// 		if(sn==18|| sn==24 || sn==25 || sn==30 || sn==39 || sn==41){ // 3 para
// 			int k=1;
// 			while(k<=3){
// 				val=trace_buff_read(filep, buff+offset, 8);
// 				if(val<0){
// 					return -EINVAL;
// 				}
// 				if(val!=8){
// 					return offset;
// 				}
// 				k++;
// 				offset=offset+val;
// 			}
// 		}
// 		if(sn==16 || sn==35){ //4 para
// 			int k=1;
// 			while(k<=4){
// 				val=trace_buff_read(filep, buff+offset, 8);
// 				if(val<0){
// 					return -EINVAL;
// 				}
// 				if(val!=8){
// 					return offset;
// 				}
// 				k++;
// 				offset=offset+val;
// 			}
// 		}
// 		j++;
// 	}
//     return offset;
// }

// int sys_start_strace(struct exec_context *current, int fd, int tracing_mode)
// {
// 	if(current == NULL){
// 		return -EINVAL;
// 	}
// 	if(tracing_mode!=FULL_TRACING && tracing_mode!=FILTERED_TRACING){
// 		return -EINVAL;
// 	}
// 	if(current->st_md_base == NULL){
// 		struct strace_head* head = (struct strace_head*)os_alloc(sizeof(struct strace_head));
// 		if(head == NULL){
// 			return -EINVAL;
// 		}
// 		head->count=0;
// 		head->is_traced=1;
// 		head->strace_fd=fd;
// 		head->next=NULL;
// 		head->last=NULL;
// 		head->tracing_mode=tracing_mode;
// 		current->st_md_base = head;
// 	}
// 	else{
// 		current->st_md_base->is_traced=1;
// 		current->st_md_base->strace_fd=fd;
// 		current->st_md_base->tracing_mode=tracing_mode;
// 	}
// 	return 0;
// }

// int sys_end_strace(struct exec_context *current)
// {
// 	if(current==NULL || current->st_md_base==NULL){
// 		return -EINVAL;
// 	}
// 	struct strace_info* temp = current->st_md_base->next;
// 	struct strace_info* temp2 = NULL;
// 	while(temp!=NULL){
// 		temp2=temp->next;
// 		os_free(temp,sizeof(struct strace_info));
// 		temp=temp2;
// 	}
// 	current->st_md_base->count=0;
// 	current->st_md_base->is_traced=0;
// 	current->st_md_base->next=NULL;
// 	current->st_md_base->last=NULL;
// 	os_free(current->st_md_base,sizeof(struct strace_head));
// 	current->st_md_base=NULL;
// 	return 0;
// }



// ///////////////////////////////////////////////////////////////////////////
// //// 		Start of ftrace functionality 		      	      /////
// ///////////////////////////////////////////////////////////////////////////


// long do_ftrace(struct exec_context *ctx, unsigned long faddr, long action, long nargs, int fd_trace_buffer)
// {
// 	if(ctx == NULL){
// 		return -EINVAL;
// 	}
// 	if(ctx->ft_md_base == NULL){
// 		ctx->ft_md_base = os_alloc(sizeof(struct ftrace_head));
// 		if(ctx->ft_md_base==NULL){
// 			return -EINVAL;
// 		}
// 		ctx->ft_md_base->count = 0;
// 		ctx->ft_md_base->next = NULL;
// 		ctx->ft_md_base->last = NULL;
// 	}

// 	if(action == ADD_FTRACE){
// 		if(ctx->ft_md_base->count >= FTRACE_MAX){
// 			return -EINVAL;
// 		}
// 		struct ftrace_info* head = ctx->ft_md_base->next;
// 		while(head!=NULL){
// 			if(head->faddr == faddr){
// 				return -EINVAL;
// 			}
// 			head = head->next;
// 		}
// 		head = ctx->ft_md_base->next;
// 		struct ftrace_info* temp = os_alloc(sizeof(struct ftrace_info));
// 		if(temp==NULL){
// 			return -EINVAL;
// 		}
// 		temp->faddr = faddr;
// 		temp->num_args = nargs;
// 		temp->fd = fd_trace_buffer;
// 		temp->capture_backtrace = 0;
// 		temp->next = NULL;
		
// 		if(head == NULL){
// 			ctx->ft_md_base->last = temp;
// 			ctx->ft_md_base->next = temp;
// 		}
// 		else{
// 			temp->next = ctx->ft_md_base->next;
// 			ctx->ft_md_base->next = temp;
// 		}
// 		ctx->ft_md_base->count +=1;
// 		return 0;
// 	}

// 	else if(action == REMOVE_FTRACE){
// 		struct ftrace_info* head = ctx->ft_md_base->next;
// 		struct ftrace_info* temp = ctx->ft_md_base->next;
// 		if(ctx->ft_md_base->count == 0){
// 			return -EINVAL;
// 		}
// 		while(temp!=NULL && temp->next!=NULL){
// 			if(temp->next->faddr == faddr){
// 				break;
// 			}
// 			temp = temp->next;
// 		}
// 		if(head->faddr == faddr){
// 			if(ctx->ft_md_base->last == head){
// 				ctx->ft_md_base->next = NULL;
// 				ctx->ft_md_base->last = NULL;
// 			}
// 			else{
// 				ctx->ft_md_base->next = ctx->ft_md_base->next->next;
// 			}
// 			u8 * ins = head->faddr;
// 			if((*(ins)==INV_OPCODE && *(ins+1)==INV_OPCODE && *(ins+2)==INV_OPCODE && *(ins+3)==INV_OPCODE)){
// 				do_ftrace(ctx, head->faddr, DISABLE_FTRACE, head->num_args, head->fd);
// 			}
// 			os_free(head,sizeof(struct ftrace_info));
// 		}
// 		else if(temp->next!=NULL){
// 			head = temp->next;
// 			temp->next = temp->next->next;
// 			if(temp->next == ctx->ft_md_base->last){
// 				ctx->ft_md_base->last = temp;
// 			}
// 			u8 * ins = head->faddr;
// 			if((*(ins)==INV_OPCODE && *(ins+1)==INV_OPCODE && *(ins+2)==INV_OPCODE && *(ins+3)==INV_OPCODE)){
// 				do_ftrace(ctx, head->faddr, DISABLE_FTRACE, head->num_args, head->fd);
// 			}
// 			os_free(head,sizeof(struct ftrace_info));
// 		}
// 		else{
// 			return -EINVAL;
// 		}
// 		ctx->ft_md_base->count -=1;
// 		return 0;
// 	}

// 	else if(action == ENABLE_FTRACE){
// 		struct ftrace_info* temp = ctx->ft_md_base->next;
// 		while(temp!=NULL){
// 			if(temp->faddr == faddr){
// 				break;
// 			}
// 			temp = temp->next;
// 		}
// 		if(temp==NULL){
// 			return -EINVAL;
// 		}
// 		u8 * ind = faddr;
// 		if(*(ind)==INV_OPCODE && *(ind+1)==INV_OPCODE && *(ind+2)==INV_OPCODE && *(ind+3)==INV_OPCODE){
// 			return 0;
// 		}
// 		u64* add = (u64 *)temp->faddr;
// 		u64 de_add = *(add);
// 		u8 * ins = add;
// 		(temp->code_backup)[0]=*(ins);
// 		(temp->code_backup)[1]=*(ins+1);
// 		(temp->code_backup)[2]=*(ins+2);
// 		(temp->code_backup)[3]=*(ins+3);
// 		*(ins) = INV_OPCODE;
// 		*(ins+1) = INV_OPCODE;
// 		*(ins+2) = INV_OPCODE;
// 		*(ins+3) = INV_OPCODE;
// 		de_add = *(add);
// 		return 0;
// 	}

// 	else if(action == DISABLE_FTRACE){
// 		struct ftrace_info* temp = ctx->ft_md_base->next;
// 		while(temp!=NULL){
// 			if(temp->faddr == faddr){
// 				break;
// 			}
// 			temp = temp->next;
// 		}
// 		if(temp==NULL){
// 			return -EINVAL;
// 		}
// 		u8 * ins = faddr;
// 		if(*(ins)==INV_OPCODE && *(ins+1)==INV_OPCODE && *(ins+2)==INV_OPCODE && *(ins+3)==INV_OPCODE){
// 			u64* add = (u64 *)temp->faddr;
// 			u64 de_add = *(add);
// 			u8 * ins = add;
// 			*(ins)=(temp->code_backup)[0];
// 			*(ins+1)=(temp->code_backup)[1];
// 			*(ins+2)=(temp->code_backup)[2];
// 			*(ins+3)=(temp->code_backup)[3];
// 			de_add = *(add);
// 		}
// 		return 0;
// 	}

// 	else if(action == ENABLE_BACKTRACE){
// 		struct ftrace_info* temp = ctx->ft_md_base->next;
// 		while(temp!=NULL){
// 			if(temp->faddr == faddr){
// 				break;
// 			}
// 			temp = temp->next;
// 		}
// 		if(temp==NULL){
// 			return -EINVAL;
// 		}
// 		u8 * ins = faddr;
// 		if(!(*(ins)==INV_OPCODE && *(ins+1)==INV_OPCODE && *(ins+2)==INV_OPCODE && *(ins+3)==INV_OPCODE)){
// 			u64* add = (u64 *)temp->faddr;
// 			u64 de_add = *(add);
// 			u8 * ins = add;
// 			(temp->code_backup)[0]=*(ins);
// 			(temp->code_backup)[1]=*(ins+1);
// 			(temp->code_backup)[2]=*(ins+2);
// 			(temp->code_backup)[3]=*(ins+3);
// 			*(ins) = INV_OPCODE;
// 			*(ins+1) = INV_OPCODE;
// 			*(ins+2) = INV_OPCODE;
// 			*(ins+3) = INV_OPCODE;
// 			de_add = *(add);
// 		}
// 		temp->capture_backtrace = 1;
// 	}

// 	else if(action == DISABLE_BACKTRACE){
// 		struct ftrace_info* temp = ctx->ft_md_base->next;
// 		while(temp!=NULL){
// 			if(temp->faddr == faddr){
// 				break;
// 			}
// 			temp = temp->next;
// 		}
// 		if(temp==NULL){
// 			return -EINVAL;
// 		}
// 		u8 * ins = faddr;
// 		if((*(ins)==INV_OPCODE && *(ins+1)==INV_OPCODE && *(ins+2)==INV_OPCODE && *(ins+3)==INV_OPCODE)){
// 			u64* add = (u64 *)temp->faddr;
// 			u64 de_add = *(add);
// 			u8 * ins = add;
// 			*(ins)=(temp->code_backup)[0];
// 			*(ins+1)=(temp->code_backup)[1];
// 			*(ins+2)=(temp->code_backup)[2];
// 			*(ins+3)=(temp->code_backup)[3];
// 			de_add = *(add);
// 		}
// 		temp->capture_backtrace = 0;
// 	}

// 	else{
// 		return -EINVAL;
// 	}
//     return 0;
// }

// long handle_ftrace_fault(struct user_regs *regs)
// {
//     struct exec_context* ctx = get_current_ctx();
// 	if(ctx->ft_md_base==NULL){
// 		return -EINVAL;
// 	}
// 	struct ftrace_info* head = ctx->ft_md_base->next;
// 	u64 faddr = regs->entry_rip;
// 	while(head!=NULL){
// 		if(head->faddr == faddr){
// 			break;
// 		}
// 		head=head->next;
// 	}
// 	if(head==NULL){
// 		return -EINVAL;
// 	}

// 	if(ctx->files == NULL){
// 		return -EINVAL;
// 	}
// 	struct file* filep = (ctx->files)[head->fd];
// 	if(filep == NULL){
// 		return -EINVAL;
// 	}
// 	int c=head->num_args;
// 	trace_buff_write(filep,&(regs->entry_rip),8);
// 	if(c>=1){
// 		trace_buff_write(filep,&(regs->rdi),8);
// 	}
// 	if(c>=2){
// 		trace_buff_write(filep,&(regs->rsi),8);
// 	}
// 	if(c>=3){
// 		trace_buff_write(filep,&(regs->rdx),8);
// 	}
// 	if(c>=4){
// 		trace_buff_write(filep,&(regs->rcx),8);
// 	}
// 	if(c>=5){
// 		trace_buff_write(filep,&(regs->r8),8);
// 	}
// 	if(c>=6){
// 		trace_buff_write(filep,&(regs->r9),8);
// 	}

// 	regs->entry_rsp = regs->entry_rsp - 8;
//     *((u64 *)(regs->entry_rsp)) = regs->rbp;
// 	regs->rbp = regs->entry_rsp;
// 	regs->entry_rip = regs->entry_rip + 4; 

// 	if(head->capture_backtrace==1){
// 		trace_buff_write(filep,&(faddr),8);
// 		u64 *rbp = regs->rbp;
// 		u64 r_add = *(rbp+1);
// 		while(r_add != END_ADDR){
// 			trace_buff_write(filep,&(r_add),8);
// 			rbp = *(rbp);
// 			r_add = *(rbp+1);
// 		}
// 	}
// 	u64 dlim = DELIM;
// 	trace_buff_write(filep,&(dlim),8);
// 	return 0;
// }

// int sys_read_ftrace(struct file *filep, char *buff, u64 count)
// {
// 	if(filep==NULL || count<0){
// 		return -EINVAL;
// 	}

// 	int j=0;
// 	u64 sn =0;
// 	int offset=0,val=0;
// 	while(j<count){
// 		val=trace_buff_read(filep, buff+offset, 8);
// 		if(val<0){
// 			return -EINVAL;
// 		}
// 		offset=offset+val;
// 		if(val!=8){
// 			return offset;
// 		}
// 		while(1){
// 			val = trace_buff_read(filep,&(sn),8);
// 			if(val<0){
// 				return -EINVAL;
// 			}
// 			if(sn == DELIM){
// 				break;
// 			}
// 			*((u64 *)(buff+offset)) = sn;
// 			offset=offset+val;
// 			if(val!=8){
// 				return offset;
// 			}
// 		}
// 		j++;
// 	}
//     return offset;
// }