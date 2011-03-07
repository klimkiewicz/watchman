#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

/*
 * setpgid(pid, pgid)
 * Set process group ID for job control.
 *
 * [-0, +1, v]
 */
static int l_setpgid(lua_State *L) {
    pid_t pid = luaL_checkint(L, 1);
    pid_t pigd = luaL_checkint(L, 2);

    lua_pushinteger(L, setpgid(pid, pigd));

    return 1;
}

/*
 * setpgrp()
 * Set the process group ID.
 *
 * [-0, +0, v]
 */
static int l_setpgrp(lua_State *L) {
    setpgrp();

    return 0;
}

/*
 * chdir(path)
 * Change working directory.
 *
 * [-0, +0, v]
 */
static int l_chdir(lua_State *L) {
    chdir(luaL_checkstring(L, 1));

    return 0;
}

/*
 * umask(cmask)
 * Set and get the file mode creation mask.
 *
 * [-0, +1, v]
 */
static int l_umask(lua_State *L) {
    lua_pushinteger(L, umask(luaL_checkint(L, 1)));

    return 1;
}


/*
 * exec(path, [args])
 * Execute a file.
 *
 * [-0, +1, v]
 */
static int l_exec(lua_State *L) {
	const char *path = luaL_checkstring(L, 1);
    int i, n = lua_gettop(L);
    char **argv = lua_newuserdata(L, (n+1)*sizeof(char*));
    argv[0] = (char*)path;
    for (i=1; i<n; i++) {
        argv[i] = (char*)luaL_checkstring(L, i+1);
    }
    argv[n] = NULL;
    lua_pushinteger(L, execv(path, argv));

    return 1;
}

/*
 * sleep(seconds)
 * Sleep for the specified number of seconds.
 *
 * [-0, +1, v]
 */
static int l_sleep(lua_State *L) {
    lua_pushinteger(L, sleep(luaL_checkint(L, 1)));

    return 1;
}

/*
 * fork()
 * Create a new process.
 *
 * [-0, +1, v]
 */
static int l_fork(lua_State *L) {
    lua_pushinteger(L, fork());

    return 1;
}

/*
 * vfork()
 * Create a new process; share virtual memory.
 *
 * [-0, +1, v]
 */
static int l_vfork(lua_State *L) {
    lua_pushinteger(L, vfork());

    return 1;
}

/*
 * kill(pid, sig)
 * Send a signal to a process or a group of processes.
 *
 * [-0, +0, v]
 */
static int l_kill(lua_State *L) {
    pid_t pid = luaL_checkint(L, 1);
    int sig = luaL_checkint(L, 2);

    kill(pid, sig);

    return 0;
}

/*
 * killpg(pgrp, sig)
 * Send a signal to a process group.
 *
 * [-0, +0, v]
 */
static int l_killpg(lua_State *L) { /* killpg(pgrp, sig) */
    pid_t pgrp = luaL_checkint(L, 1);
    int sig = luaL_checkint(L, 2);

    killpg(pgrp, sig);

    return 0;
}

static const luaL_reg R[] = {
    {"setpgid", l_setpgid},
    {"setpgrp", l_setpgrp},
    {"chdir", l_chdir},
    {"exec", l_exec},
    {"sleep", l_sleep},
    {"fork", l_fork},
    {"vfork", l_vfork},
    {"kill", l_kill},
    {"killpg", l_killpg},
    {NULL, NULL},
};

LUALIB_API int luaopen_cwatchman (lua_State *L) {
    luaL_register(L, "cwatchman", R);

    return 1;
}
