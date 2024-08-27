#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char * argv[])
{
        unsigned long int value = atol(argv[argc-1]);
        unsigned long int square_value = value*value;
        if(argc==2){
                printf("%lu\n",square_value);
                exit(0);
        }
        int rc = fork();
        if(rc<0){
                printf("Unable to execute\n");
                exit(-1);
        }
        else if(rc==0){
                char buffer[256];
                sprintf(buffer,"%lu",square_value);
                
                char new_program_path[256];
                strcpy(new_program_path, "./");
                strcat(new_program_path, argv[1]);

		char *newp[argc];
		for(int i=1;i<argc-1;i++){
			newp[i]=argv[i+1];
		}
		newp[0]=new_program_path;
		newp[argc-2]=buffer;
		newp[argc-1]=NULL;
                execvp(newp[0],newp);
                
                printf("Unable to execute\n");
                exit(-1);
        }
        else{
                int w= wait(NULL);
        }
        exit(0);
}

