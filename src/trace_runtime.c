#include "trace_runtime.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#if !defined(__x86_64__)
#error "Este runtime didatico suporta apenas Linux x86_64."
#endif

#define WAIT_ERROR -1
#define WAIT_FINISHED 0
#define WAIT_SYSCALL 1
#define WAIT_OTHER_STOP 2

static void fill_event_from_regs(pid_t pid,
                                 int entering,
                                 const struct user_regs_struct *regs,
                                 struct syscall_event *ev)
{
    /*
     * TODO Semana 4:
     *
     * Preencha struct syscall_event usando os registradores x86_64.
     *
     * Dicas:
     * - regs->orig_rax contem o numero da syscall.
     * - regs->rax contem o retorno, valido na saida.
     * - os seis argumentos ficam em rdi, rsi, rdx, r10, r8 e r9.
     * - ev->entering deve copiar o parametro entering.
     */
    memset(ev, 0, sizeof(*ev));
    ev->pid = pid;
    ev->entering = entering;
}

static pid_t launch_tracee(char *const argv[])
{
    /*
     * TODO Semana 2:
     *
     * Crie o processo monitorado.
     *
     * Fluxo esperado:
     * - fork()
     * - no filho:
     *   - ptrace(PTRACE_TRACEME, ...)
     *   - raise(SIGSTOP)
     *   - execvp(argv[0], argv)
     * - no pai:
     *   - retornar o pid do filho
     *
     * Em erro, imprima uma mensagem com perror() e retorne -1.
     */
    fprintf(stderr, "erro: TODO Semana 2: implementar launch_tracee()\n");
    return -1;
}

static int wait_for_initial_stop(pid_t child)
{
    /*
     * TODO Semana 2:
     *
     * O filho chama raise(SIGSTOP) antes de executar o programa alvo.
     * O pai precisa esperar essa parada inicial com waitpid().
     *
     * Retorne 0 se o filho parou como esperado, -1 em erro.
     */
    fprintf(stderr, "erro: TODO Semana 2: implementar wait_for_initial_stop()\n");
    return -1;
}

static int configure_trace_options(pid_t child)
{
    long result;

    result = ptrace(PTRACE_SETOPTIONS,
                    child,
                    0,
                    PTRACE_O_TRACESYSGOOD);

    if (result < 0) {
        perror("ptrace(PTRACE_SETOPTIONS)");
        return -1;
    }

    return 0;
}

static int resume_until_next_syscall(pid_t child, int signal_to_deliver)
{
    long result;

    result = ptrace(PTRACE_SYSCALL,
                    child,
                    0,
                    signal_to_deliver);

    if (result < 0) {
        perror("ptrace(PTRACE_SYSCALL)");
        return -1;
    }

    return 0;
}

static int wait_for_syscall_stop(pid_t child,
                                 int *status,
                                 int *signal_to_deliver)
{
    pid_t waited;
    int signal_number;

    *signal_to_deliver = 0;

    waited = waitpid(child, status, 0);

    if (waited < 0) {
        perror("waitpid");
        return WAIT_ERROR;
    }

    if (WIFEXITED(*status) || WIFSIGNALED(*status)) {
        return WAIT_FINISHED;
    }

    if (WIFSTOPPED(*status)) {
        signal_number = WSTOPSIG(*status);

        if (signal_number & 0x80) {
            return WAIT_SYSCALL;
        }

        if (signal_number == SIGTRAP) {
            return WAIT_OTHER_STOP;
        }

        *signal_to_deliver = signal_number;
        return WAIT_OTHER_STOP;
    }

    return WAIT_OTHER_STOP;
}

int trace_program(char *const argv[],
                  trace_observer_fn observer,
                  void *userdata)
{
    pid_t child;
    int status = 0;
    int entering = 1;
    int signal_to_deliver = 0;

    if (argv == NULL || argv[0] == NULL) {
        fprintf(stderr, "erro: programa alvo ausente\n");
        return -1;
    }

    child = launch_tracee(argv);
    if (child < 0) {
        return -1;
    }

    if (wait_for_initial_stop(child) < 0) {
        return -1;
    }

    if (configure_trace_options(child) < 0) {
        return -1;
    }

    if (resume_until_next_syscall(child, 0) < 0) {
        return -1;
    }

    while (1) {
        struct user_regs_struct regs;
        struct syscall_event ev;
        int stop_kind;

        stop_kind = wait_for_syscall_stop(child, &status, &signal_to_deliver);
        if (stop_kind == WAIT_ERROR) {
            return -1;
        }
        if (stop_kind == WAIT_FINISHED) {
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            }
            return 0;
        }
        if (stop_kind == WAIT_OTHER_STOP) {
            if (resume_until_next_syscall(child, signal_to_deliver) < 0) {
                return -1;
            }
            continue;
        }

        /*
         * TODO Semana 4:
         *
         * Use PTRACE_GETREGS para preencher regs.
         * Depois chame fill_event_from_regs() e observer().
         */
        memset(&regs, 0, sizeof(regs));
        fill_event_from_regs(child, entering, &regs, &ev);
        if (observer != NULL) {
            observer(&ev, userdata);
        }

        entering = !entering;

        if (resume_until_next_syscall(child, 0) < 0) {
            return -1;
        }
    }
}
