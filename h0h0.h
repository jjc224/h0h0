#pragma once

#ifdef DEBUG
    /* Debug output. */
    #define debug(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

    /* Error output. */
    #define debug_err(fmt, ...) warn(fmt, ##__VA_ARGS__)
#else
    /* Not debugging. */
    #define debug(fmt, ...) 
    #define debug_err(fmt, ...)
#endif
