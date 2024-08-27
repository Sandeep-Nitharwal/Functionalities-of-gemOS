#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

unsigned long solve(char path[4096]){
	DIR *d= opendir(path);
	unsigned long size = 0;
	if(d==NULL){
		perror("Unable to execute\n");
		exit(-1);
	}
	struct dirent *d_add;
	while((d_add = readdir(d))!=NULL){
		if (strcmp(d_add->d_name, "..") == 0) {
            continue;
        }
		struct stat st;
		char file_path[4096];
		snprintf(file_path,sizeof(file_path),"%s/%s",path,d_add->d_name);
		if(stat(file_path,&st) == -1){
			perror("Unable to execute\n");
			exit(-1);
		}
		if(S_ISREG(st.st_mode) || strcmp(d_add->d_name, ".") == 0  ){
			size = size + st.st_size;
		}
		else if(S_ISLNK(st.st_mode)){
			char link_target[4096];
            ssize_t link_size = readlink(file_path, link_target, sizeof(link_target) - 1);
            if (link_size < 0) {
                perror("Unable to execute\n");
                exit(-1);
            }
            link_target[link_size] = '\0';
			size = size + solve(link_target);
		}
		else{
			size=size+solve(file_path);
		}
	}
	closedir(d);
	return size;
}

int main(int argc , char *argv[])
{
	DIR *d= opendir(argv[1]);
	unsigned long size = 0;
	if(d==NULL){
		perror("Unable to execute\n");
		exit(-1);
	}
	struct dirent *d_add;
	while((d_add = readdir(d))!=NULL){
		if (strcmp(d_add->d_name, "..") == 0 ) {
            continue;
        }
		struct stat st;
		char file_path[4096];
		snprintf(file_path,sizeof(file_path),"%s/%s",argv[1],d_add->d_name);
		if(stat(file_path,&st) == -1){
			perror("Unable to execute\n");
			exit(-1);
		}
		if(S_ISREG(st.st_mode) || strcmp(d_add->d_name,".") == 0){
			size = size + st.st_size;
		}
		else if(S_ISLNK(st.st_mode)){
			char link_target[4096];
            ssize_t link_size = readlink(file_path, link_target, sizeof(link_target) - 1);
            if (link_size < 0) {
                perror("Unable to execute\n");
                exit(-1);
            }
            link_target[link_size] = '\0';
			size = size + solve(link_target);
		}
		else{
			int fd[2];
			pid_t childpid;
			if (pipe(fd) < 0) {
				perror("Unable to execute\n");
				exit(-1);
			}
			childpid = fork();
			if(childpid<0){
				perror("Unable to execute\n");
				exit(-1);
			}
			else if(childpid==0){
				close(fd[0]);
				close(1);
				dup(fd[1]);
				printf("%lu/n",solve(file_path));
			}
			else{
				wait(NULL);
				close(fd[1]);
				int l;
				char read_val[80];
				l = read(fd[0],read_val,sizeof(read_val));
				unsigned long int val = atol(read_val);
				size=size+val;
			}
		}
	}
	closedir(d);
	printf("%lu\n",size);
	return 0;
}
