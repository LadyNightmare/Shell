/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c 
#include <string.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

job* jobs;

void sighandler (int signal); // We create the handler, we later develop it

void printRat();
void printTux();


// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void) {
	ignore_terminal_signals();
	jobs = new_list("Background processes");
	signal(SIGCHLD, sighandler);
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */
	new_process_group(getpid()); //We create the process group for the father

	//printRat();

	while (1) {   /* Program terminates normally inside get_command() after ^D is typed*/

		//Recorrer la lista e imprimir los que están hechos

		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command

		if(args[0]!=NULL) {

			if (strcmp(args[0], "cd") == 0) {

				chdir(args[1]);

			}else if (strcmp(args[0], "tux") == 0) {

				printTux();


			} else if (strcmp(args[0], "jobs") == 0) {


				print_job_list(jobs);

			} else if(strcmp(args[0], "fg") == 0) {

				job* job;

				if (args [1] == NULL) {

					job = get_item_bypos(jobs, 1);

				} else {

					job = get_item_bypos(jobs, atoi(args[1]));

				}

				set_terminal(job->pgid);

				//We change its state to foreground

				job->state = FOREGROUND;

				//We get the id of the group and we send the signal to continue (SIGCONT)


				printf("%s\n", job->command);

				killpg(job->pgid, 18);

				int pid = waitpid(job->pgid, &status, WUNTRACED);

				set_terminal(getpid());

				//We analyze its status and if it's finished a pastar Maricarmen
				//and if not, pues a pararlo

				status_res = analyze_status(status, &info);

				if (status_res == EXITED || status_res == SIGNALED) {

					delete_job(jobs, job);

				} else if (status_res == SUSPENDED) {

					job->state = STOPPED;

				}

			} else if (strcmp(args[0], "bg") == 0) {

				job* job;

				if (args [1] == NULL) {

					job = get_item_bypos(jobs, 1);

				} else {

					job = get_item_bypos(jobs, atoi(args[1]));

				}

				//We change the state of the job to background and we start it again

				job->state = BACKGROUND;
				killpg(job->pgid, 18);

			} else {

				pid_fork = fork(); //We create the child

							if(!pid_fork) { //This is the child

								new_process_group(getpid()); //We create the process group for the child

								if(!background) { //If not in background

									set_terminal(getpid()); //We give the control of the terminal to the child process in foreground

								}

								restore_terminal_signals();
								execvp(args[0], args); //we execute the command
								printf("Error, command not found: %s\n", args[0]); //execvp has failed so it goes on
								exit(-1);


							} else { //This is the father

								new_process_group(pid_fork);

								if (!background) { //If it's in background


									set_terminal(pid_fork); //After the child has finished, we recover the control of the terminal

									waitpid(pid_fork, &status, WUNTRACED); //Wait for the child [0 to work normally]

									set_terminal(getpid());

									status_res = analyze_status(status, &info);
									if (info != 255) {

										printf("Foregroung pid: %d, command: %s, %s, info: %d\n", pid_fork, args [0], status_strings[status_res], info);

									}

									if (status_res == SUSPENDED) {

										add_job(jobs, new_job(pid_fork, args[0], STOPPED));

									}

								}else {

									add_job(jobs, new_job(pid_fork, args[0], BACKGROUND));

									printf("Background job running... pid: %d, command: %s\n", pid_fork, args [0]);

								}

							}
			}

		}

		/* the steps are:
			 (1) fork a child process using fork() V
			 (2) the child process will invoke execvp() V
			 (3) if background == 0, the parent will wait, otherwise continue V
			 (4) Shell shows a status message for processed command V
			 (5) loop returns to get_commnad() function V
		*/

	} // end while

}

void sighandler (int signal) {

	block_SIGCHLD();

	int pos = 0;

	int status, info;
	enum status status_res;

	//We get the first job

	job* job = get_item_bypos(jobs, 1);

	while(job != NULL) {

		if(job->state != FOREGROUND) {

			int pid = waitpid(job->pgid, &status, WUNTRACED | WNOHANG);

			if (pid == job->pgid) {

				//If the pid is the one of the process that has sent the signal, we analyze its status

				status_res = analyze_status(status, &info);

				if (status_res == EXITED || status_res == SIGNALED) {

					//If it's finished, we delete it from the list

					job->state = STOPPED;
					delete_job(jobs, job);
					printf("Job %s has finished its execution.\n", job->command);

				} else if (status_res == SUSPENDED) {

					//If it's suspended, we change its state

					job->state = STOPPED;
					printf("Job %s has been suspended.\n", job->command);

				}


				//We get the next job

				++pos;
				job = get_item_bypos(jobs, pos);

			} else {

				//We get the next job

				++pos;
				job = get_item_bypos(jobs, pos);

			}

		} else {

			//We get the next job

			++pos;
			job = get_item_bypos(jobs, pos);

		}


	}

	unblock_SIGCHLD();

}

/*void printRat () {

	printf("         __             _,-'~^'-.\n"
"       _// )      _,-'~`         `.\n"
"     .' ( /`'-,-'`                 ;\n"
"    / 6                             ;\n"
"   /           ,             ,-'     ;\n"
"  (,__.--.      \           /        ;\n"
"   //'   /`-.\   |          |        `._________\n"
"     _.-'_/`  )  )--...,,,___\     \-----------,)\n"
"   ((('~` _.-'.-'           __`-.   )         //\n"
"         ((('`             (((---~'`         //\n"
"                                            ((________________\n"
"                                            `----''''---^^^```\n");

}*/

void printTux () {

	printf("    ....,,\n"
"        .::o::;'          .....\n"
"       ::::::::        .::::o:::.,\n"
"      .::' `:::        :::::::''\n"
"      :::     :       ::'   `.\n"
"     .:::     :       :'      ::\n"
"    .:::       :     ,:       `::\n"
"   .' :        :`. .' :        :::\n"
"  .' .:        :  :  .:        : :\n"
"  : ::'        ::. :' :        : :\n"
"  :: :         :`: :  :        :`:\n"
"  :  :         :  ''  :        : '\n"
"_.---:         :___   :        :\n"
"     :        :`   `--:        :\n"
"      : :---: :        : :---: :`---.\n"
"      '```  '```      '''   ''''\n");

}
