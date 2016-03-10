#pragma once

typedef const int (*libcall)();

/* Libcall hash. */
struct lc_hash
{
    char name[64];
    libcall callback;
};

/* Dummy libcall to fall back on. */
const int dummy()   __attribute__((visibility("hidden")));
const int dummy()
{
    debug("dummy()\n");
    return 0;
}

/* Get libcall by name. */
libcall getlibcall(const char *name)
{
    /* Libcall table. */
    static struct lc_hash *libcalls = NULL;
    static size_t num_libcalls = 0;

    /* Search for libcall. */
    for (int i = 0; i < num_libcalls; i++)
    {
        if (strcmp(name, libcalls[i].name) == 0)
        {
            /* Get original address. */ 
            if (libcalls[i].callback == NULL)
            {
                if ((libcalls[i].callback = dlsym(RTLD_NEXT, name)) == NULL)
                    return dummy;
            }

            return libcalls[i].callback;
        }
    }

    /* Memoize libcalls. */
    struct lc_hash *temp;
    temp = realloc(libcalls, (num_libcalls + 1) * sizeof(struct lc_hash));
    if (temp != NULL)
    {
        libcalls = temp;

        if ((libcalls[num_libcalls].callback = dlsym(RTLD_NEXT, name)) != NULL)
        {
            strncpy(libcalls[num_libcalls].name, name, 64);
            return libcalls[num_libcalls++].callback;
        }
    }

    return dummy;
}
