/*
 * Copyright (c) 2011 Kamil Klimkiewicz
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <sys/ptrace.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

static int l_traceme(lua_State *L) { /* traceme() */
    ptrace(PTRACE_TRACEME, 0, 0, 0);
    return 0;
}

static int l_set_options(lua_State *L) { /* set_options(pid, options) */
    int i;
    long options = 0;
    int pid = luaL_checkint(L, 1);
    int options_len;

    luaL_checktype(L, 2, LUA_TTABLE);
    options_len = lua_objlen(L, 2);

    for (i=0; i < options_len; i++) {
        lua_rawgeti(L, 2, i + 1);
        options |= luaL_checkint(L, -1);
        lua_pop(L, 1);
    }

    ptrace(PTRACE_SETOPTIONS, pid, 0, options);
    return 0;
}

static int l_continue(lua_State *L) { /* continue(pid, signum=0) */
    int pid = luaL_checkint(L, 1);
    int signum = luaL_optint(L, 2, 0);

    ptrace(PTRACE_CONT, pid, 0, signum);
    return 0;
}

static int l_event(lua_State *L) { /* event(status) */
    int status, event;

    status = luaL_checkint(L, 1);
    event = (status >> 16) & 0xffff;

    lua_pushinteger(L, event);

    return 1;
}

static int l_event_message(lua_State *L) { /* event_message(pid) */
    int pid = luaL_checkint(L, 1);
    unsigned long data;

    ptrace(PTRACE_GETEVENTMSG, pid, 0, &data);

    lua_pushnumber(L, data);

    return 1;
}

static int l_detach(lua_State *L) { /* detach(pid, signum=0) */
    int pid = luaL_checkint(L, 1);
    int signum = luaL_optint(L, 2, 0);

    ptrace(PTRACE_DETACH, pid, 0, signum);

    return 0;
}

static const luaL_reg R[] = {
    {"traceme", l_traceme},
    {"set_options", l_set_options},
    {"continue", l_continue},
    {"event", l_event},
    {"event_message", l_event_message},
    {"detach", l_detach},
    {NULL, NULL},
};

#define set_const(value)            \
    lua_pushliteral(L, #value);     \
    lua_pushnumber(L, value);       \
    lua_settable(L, -3)

LUALIB_API int luaopen_ptrace (lua_State *L) {
    luaL_register(L, "ptrace", R);

    /* options */
    set_const(PTRACE_O_TRACESYSGOOD);
    set_const(PTRACE_O_TRACEFORK);
    set_const(PTRACE_O_TRACEVFORK);
    set_const(PTRACE_O_TRACECLONE);
    set_const(PTRACE_O_TRACEEXEC);
    set_const(PTRACE_O_TRACEVFORKDONE);
    set_const(PTRACE_O_TRACEEXIT);

    /* events */
    set_const(PTRACE_EVENT_FORK);
    set_const(PTRACE_EVENT_VFORK);
    set_const(PTRACE_EVENT_CLONE);
    set_const(PTRACE_EVENT_EXEC);
    set_const(PTRACE_EVENT_VFORK_DONE);
    set_const(PTRACE_EVENT_EXIT);

    return 1;
}
