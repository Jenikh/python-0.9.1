/* Windows implementation of Python 0.9.1's built-in "posix" module. */
#include "allobjects.h"
#include "modsupport.h"

#include <windows.h>
#include <io.h>
#include <direct.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static object *PosixError;

static object *posix_error(void)
{
    return err_errno(PosixError);
}

static object *posix_chdir(object *self, object *args)
{
    object *path;
    if (!getstrarg(args, &path))
        return NULL;
    if (_chdir(getstringvalue(path)) < 0)
        return posix_error();
    INCREF(None);
    return None;
}

static object *posix_chmod(object *self, object *args)
{
    object *path;
    int mode;
    if (!getstrintarg(args, &path, &mode))
        return NULL;
    if (_chmod(getstringvalue(path), mode) < 0)
        return posix_error();
    INCREF(None);
    return None;
}

static object *posix_getcwd(object *self, object *args)
{
    char buf[4096];
    if (!getnoarg(args))
        return NULL;
    if (_getcwd(buf, sizeof(buf)) == NULL)
        return posix_error();
    return newstringobject(buf);
}

static object *posix_link(object *self, object *args)
{
    object *src, *dst;
    if (!getstrstrarg(args, &src, &dst))
        return NULL;
    if (!CreateHardLinkA(getstringvalue(dst), getstringvalue(src), NULL))
        return posix_error();
    INCREF(None);
    return None;
}

static object *posix_listdir(object *self, object *args)
{
    object *path, *result, *name;
    WIN32_FIND_DATAA data;
    HANDLE h;
    char pattern[4096];

    if (!getstrarg(args, &path))
        return NULL;

    if (strlen(getstringvalue(path)) + 3 >= sizeof(pattern)) {
        err_badarg();
        return NULL;
    }

    strcpy(pattern, getstringvalue(path));
    if (pattern[0] == '\0') {
        strcpy(pattern, ".");
    }
    if (pattern[strlen(pattern) - 1] != '\\' &&
        pattern[strlen(pattern) - 1] != '/')
        strcat(pattern, "\\");
    strcat(pattern, "*");

    h = FindFirstFileA(pattern, &data);
    if (h == INVALID_HANDLE_VALUE)
        return posix_error();

    result = newlistobject(0);
    if (result == NULL) {
        FindClose(h);
        return NULL;
    }

    do {
        name = newstringobject(data.cFileName);
        if (name == NULL || addlistitem(result, name) != 0) {
            XDECREF(name);
            DECREF(result);
            FindClose(h);
            return NULL;
        }
        DECREF(name);
    } while (FindNextFileA(h, &data));

    FindClose(h);
    return result;
}

static object *posix_mkdir(object *self, object *args)
{
    object *path;
    int mode;
    if (!getstrintarg(args, &path, &mode))
        return NULL;
    (void)mode;
    if (_mkdir(getstringvalue(path)) < 0)
        return posix_error();
    INCREF(None);
    return None;
}

static object *posix_rename(object *self, object *args)
{
    object *src, *dst;
    if (!getstrstrarg(args, &src, &dst))
        return NULL;
    if (rename(getstringvalue(src), getstringvalue(dst)) < 0)
        return posix_error();
    INCREF(None);
    return None;
}

static object *posix_rmdir(object *self, object *args)
{
    object *path;
    if (!getstrarg(args, &path))
        return NULL;
    if (_rmdir(getstringvalue(path)) < 0)
        return posix_error();
    INCREF(None);
    return None;
}

static object *posix_do_stat(object *self, object *args, int lstat_mode)
{
    object *path, *v;
    struct _stat st;

    if (!getstrarg(args, &path))
        return NULL;

    (void)lstat_mode;
    if (_stat(getstringvalue(path), &st) != 0)
        return posix_error();

    v = newtupleobject(10);
    if (v == NULL)
        return NULL;

#define SET(i, value) settupleitem(v, i, newintobject((long)(value)))
    SET(0, st.st_mode);
    SET(1, st.st_ino);
    SET(2, st.st_dev);
    SET(3, st.st_nlink);
    SET(4, st.st_uid);
    SET(5, st.st_gid);
    SET(6, st.st_size);
    SET(7, st.st_atime);
    SET(8, st.st_mtime);
    SET(9, st.st_ctime);
#undef SET

    if (err_occurred()) {
        DECREF(v);
        return NULL;
    }
    return v;
}

static object *posix_stat(object *self, object *args)
{
    return posix_do_stat(self, args, 0);
}

static object *posix_lstat(object *self, object *args)
{
    /* Python 0.9.1 has no separate Windows lstat semantics. */
    return posix_do_stat(self, args, 1);
}

static object *posix_system(object *self, object *args)
{
    object *command;
    int sts;
    if (!getstrarg(args, &command))
        return NULL;
    sts = system(getstringvalue(command));
    return newintobject((long)sts);
}

static object *posix_umask(object *self, object *args)
{
    int mode, old;
    if (!getintarg(args, &mode))
        return NULL;
    old = _umask(mode);
    return newintobject((long)old);
}

static object *posix_unlink(object *self, object *args)
{
    object *path;
    if (!getstrarg(args, &path))
        return NULL;
    if (remove(getstringvalue(path)) < 0)
        return posix_error();
    INCREF(None);
    return None;
}

static object *posix_utimes(object *self, object *args)
{
    /* The original API passes seconds only. The MSVCRT has no equivalent
       of POSIX utimes(), so keep the operation explicitly unsupported. */
    (void)self;
    (void)args;
    err_setstr(PosixError, "utimes is not supported on this Windows build");
    return NULL;
}

static object *posix_readlink(object *self, object *args)
{
    (void)self;
    (void)args;
    err_setstr(PosixError, "readlink is not supported on this Windows build");
    return NULL;
}

static object *posix_symlink(object *self, object *args)
{
    object *src, *dst;
    DWORD flags = 0;

    if (!getstrstrarg(args, &src, &dst))
        return NULL;

    if (!CreateSymbolicLinkA(getstringvalue(dst), getstringvalue(src), flags))
        return posix_error();

    INCREF(None);
    return None;
}

static struct methodlist posix_methods[] = {
    {"chdir", posix_chdir},
    {"chmod", posix_chmod},
    {"getcwd", posix_getcwd},
    {"link", posix_link},
    {"listdir", posix_listdir},
    {"mkdir", posix_mkdir},
    {"rename", posix_rename},
    {"rmdir", posix_rmdir},
    {"stat", posix_stat},
    {"system", posix_system},
    {"umask", posix_umask},
    {"unlink", posix_unlink},
    {"utimes", posix_utimes},
    {"lstat", posix_lstat},
    {"readlink", posix_readlink},
    {"symlink", posix_symlink},
    {NULL, NULL}
};

void initposix(void)
{
    object *m, *d;

    m = initmodule("posix", posix_methods);
    d = getmoduledict(m);

    PosixError = newstringobject("posix.error");
    if (PosixError == NULL || dictinsert(d, "error", PosixError) != 0)
        fatal("can't define posix.error");
}
