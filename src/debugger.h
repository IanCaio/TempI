#ifndef __debugger_h__
#define __debugger_h__

#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef NO_DEBUG
#define D_debug(M, ...)
#else
#define D_debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define D_check_errno() (errno==0 ? "None" : strerror(errno))

#define D_log_error(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, D_check_errno(), ##__VA_ARGS__)

#define D_log_warning(M, ...) fprintf(stderr, "[WARNING] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, D_check_errno(), ##__VA_ARGS__)

#define D_log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d:) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define D_check(A, M, ...) do {if(!(A)) {D_log_error(M, ##__VA_ARGS__); errno=0; goto error;}} while(0)

#define D_sentinel(M, ...) do {D_log_error(M, ##__VA_ARGS__); errno=0; goto error;} while(0)

#define D_check_pointer(A) D_check((A), "Pointer " #A " is NULL.")

#define D_check_debug(A, M, ...) do {if(!(A)) {D_debug(M, ##__VA_ARGS__); errno=0; goto error;}} while(0)

#endif
