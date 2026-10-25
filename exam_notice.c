#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_STUDENTS 3
#define BUF_SIZE 256

static volatile sig_atomic_t interrupted = 0;

static void sigint_handler(int sig)
{
(void)sig;
const char msg[] = "\n\n[INVIGILATOR] Ctrl+C detected! Exam session interrupted.\n";
write(STDOUT_FILENO, msg, sizeof(msg) - 1);
interrupted = 1;
}

static void student_process(int id, int fd)
{
char buf[BUF_SIZE];

signal(SIGINT, SIG_IGN);

FILE *in = fdopen(fd, "r");
if (in ==NULL) 
{
perror("fdopen");
exit(1);
}

printf(" [Student %d | PID %d] Logged in and waiting for notices...\n", id, getpid());
fflush(stdout);

while (fgets(buf, sizeof(buf), in) != NULL)
{
printf(" [Student %d | PID %d] NOTICE RECEIVED: %s", id, getpid(), buf);
fflush(stdout);
}

printf(" [Student %d | PID %d] Exam session ended. Logging out.\n", id, getpid());
fflush(stdout);
fclose(in);
exit(0);
}

int main(void)
{
int pipes[NUM_STUDENTS][2];
pid_t pids[NUM_STUDENTS];
char input[BUF_SIZE];
int i, j;

struct sigaction sa;
memset(&sa, 0, sizeof(sa));
sa.sa_handler = sigint_handler;
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;
sigaction(SIGINT, &sa, NULL);

printf("ONLINE EXAM NOTICE SYSTEM\n");
printf("===================================\n");
printf("Invigilator PID: %d\n", getpid());
printf("Type a notice and press Enter to send it to all students.\n");
printf("Type 'exit' to end the exam, or press Ctrl+C to interrupt.\n\n");

for (i =0; i < NUM_STUDENTS; i++)
{
if (pipe(pipes[i]) == -1)
{
perror("pipe");
exit(1);
}
}
for (i = 0; i < NUM_STUDENTS; i++)
{
pids[i] = fork();

if (pids[i] == 0)
{
for (j = 0; j < NUM_STUDENTS; j++) {
close(pipes[j][1]);
if (j != i)
close(pipes[j][0]);
}
student_process(i + 1, pipes[i][0]);
}
}

for (i = 0; i < NUM_STUDENTS; i++)
close(pipes[i][0]);
sleep(1);

while (!interrupted)
{
printf("\n[INVIGILATOR] Enter notice: ");
fflush(stdout);

if (fgets(input, sizeof(input), stdin) == NULL)
break;

if (strcmp(input, "exit\n") == 0) {
snprintf(input, sizeof(input), "Exam has ended. Please submit your answers.\n");
for (i = 0; i < NUM_STUDENTS; i++)
write(pipes[i][1], input, strlen(input));
break;
}

if (input[0] == '\n')
continue;

for (i = 0; i < NUM_STUDENTS; i++)
write(pipes[i][1], input, strlen(input));
usleep(200000);
}

if (interrupted)
{
const char*msg = "Exam session interrupted by invigilator.\n";
for (i = 0; i < NUM_STUDENTS; i++)
write(pipes[i][1], msg, strlen(msg));
}

for (i = 0; i < NUM_STUDENTS; i++)
close(pipes[i][1]);

for (i = 0; i < NUM_STUDENTS; i++)
waitpid(pids[i], NULL, 0);

printf("\n[INVIGILATOR] All students logged out. Program terminated.\n");
return 0;
}
