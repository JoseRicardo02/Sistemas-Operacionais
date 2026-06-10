#include "student_api.h"

#include "syscall_names.h"
#include "trace_helpers.h"

#include <stdio.h>
#include <sys/syscall.h>

void student_debug_raw_event(const struct syscall_event *ev,
                             char *buf,
                             size_t bufsz)
{
    /*
     * Suporte de depuracao para a Semana 4:
     *
     * Esta funcao existe para inspecionar eventos crus depois que o runtime
     * ja consegue parar em syscalls e preencher struct syscall_event.
     * Ela nao e a formatacao final do projeto.
     *
     * Experimento sugerido:
     * - imprima o nome da syscall;
     * - imprima se o evento e entrada ou saida;
     * - imprima o pid;
     * - em eventos de entrada, observe os argumentos;
     * - em eventos de saida, observe o valor de retorno.
     *
     * Depois compare a saida de:
     *
     *   ./toytrace trace --raw-events -- ./tests/targets/hello_write
     *
     * A pergunta importante da Semana 4 e:
     * por que a mesma syscall aparece duas vezes?
     */
    snprintf(buf, bufsz, "pid=%d %s %s",
             ev->pid,
             syscall_name(ev->syscall_no),
             ev->entering ? "entrada" : "saida");
}

void student_format_event(const struct syscall_event *ev,
                          char *buf,
                          size_t bufsz)
{
    char path[256];
    const char *path_text = "<ilegivel>";

    if (ev == NULL || buf == NULL || bufsz == 0) {
        return;
    }

    switch (ev->syscall_no) {
#ifdef SYS_read
    case SYS_read:
        snprintf(buf, bufsz, "read(%ld, %#lx, %lu) = %ld",
                 (long)ev->args[0],
                 ev->args[1],
                 ev->args[2],
                 ev->ret);
        return;
#endif

#ifdef SYS_write
    case SYS_write:
        snprintf(buf, bufsz, "write(%ld, %#lx, %lu) = %ld",
                 (long)ev->args[0],
                 ev->args[1],
                 ev->args[2],
                 ev->ret);
        return;
#endif

#ifdef SYS_openat
    case SYS_openat:
        if (read_child_string(ev->pid, ev->args[1], path, sizeof(path)) >= 0) {
            path_text = path;
        }

        snprintf(buf, bufsz, "openat(%ld, \"%s\", %#lx, %#lx) = %ld",
                 (long)ev->args[0],
                 path_text,
                 ev->args[2],
                 ev->args[3],
                 ev->ret);
        return;
#endif

#ifdef SYS_execve
    case SYS_execve:
        if (read_child_string(ev->pid, ev->args[0], path, sizeof(path)) >= 0) {
            path_text = path;
        }

        snprintf(buf, bufsz, "execve(\"%s\", ...) = %ld",
                 path_text,
                 ev->ret);
        return;
#endif

#ifdef SYS_exit_group
    case SYS_exit_group:
        snprintf(buf, bufsz, "exit_group(%ld) = %ld",
                 (long)ev->args[0],
                 ev->ret);
        return;
#endif
    }

    snprintf(buf, bufsz, "%s(%#lx, %#lx, %#lx, %#lx, %#lx, %#lx) = %ld",
             syscall_name(ev->syscall_no),
             ev->args[0],
             ev->args[1],
             ev->args[2],
             ev->args[3],
             ev->args[4],
             ev->args[5],
             ev->ret);
}
