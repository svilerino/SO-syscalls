#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syscall.h>

int main(int argc, char* argv[]) {
	int status;
	pid_t child;

	if (argc <= 1) {
		fprintf(stderr, "Uso: %s commando [argumentos ...]\n", argv[0]);
		exit(1);
	}

	/* Fork en dos procesos */
	child = fork();
	if (child == -1) { perror("ERROR fork"); return 1; }
	if (child == 0) {
		/* Solo se ejecuta en el Hijo */
		if(ptrace(PTRACE_TRACEME, 0, NULL, NULL)){
			perror("[Child]A catastrophical failure has ocurred when trying to PTRACE_TRACEME!");
			exit(1);
		}
		execvp(argv[1], argv+1);
		/* Si vuelve de exec() hubo un error */
		perror("ERROR child exec(...)"); exit(1);
	} else {
		/* Solo se ejecuta en el Padre */
		while(1) {
			if (wait(&status) < 0) { perror("waitpid"); break; }
			if (WIFEXITED(status)) break; /* Proceso terminado */

			int errnum = ptrace(PTRACE_PEEKUSER, child, 4 * EAX, NULL);
			int syscallID = ptrace(PTRACE_PEEKUSER, child, 4*ORIG_EAX, NULL);
			if((errnum==-ENOSYS) &&(syscallID == SYS_kill)){
				printf("Se ha hecho justicia!\n");
				ptrace(PTRACE_KILL, child, NULL, NULL);
				return 0;
			}	
			ptrace(PTRACE_SYSCALL, child, NULL, NULL);
		}
		ptrace(PTRACE_DETACH, child, NULL, NULL);//release the child
	}
	return 0;
}