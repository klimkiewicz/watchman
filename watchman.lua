--[[
Copyright (c) 2011 Kamil Klimkiewicz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
--]]

require 'cron'
require 'cwatchman'
require 'ev'
require 'ptrace'
require 'signal'
require 'sha2'


local LOOP = ev.Loop.default
local USAGE = ''

local processes = {}
local ordered_processes = {}
local traced_processes = {}


-- Events

local function callbacks_register()
    local register = {
        callbacks = {},
        on_removes = {},
    }

    function register.add(name, callback, on_remove)
        local name_callbacks = register.callbacks[name]

        if (not name_callbacks) then
            name_callbacks = {}
            register.callbacks[name] = name_callbacks
        end

        table.insert(name_callbacks, callback)

        if (on_remove) then
            register.on_removes[callback] = on_remove
        end
    end

    function register.get(name)
        return register.callbacks[name]
    end

    function register.remove(name, callback)
        local name_callbacks = register.callbacks[name]
        if (name_callbacks) then
            if (callback) then
                for i, v in ipairs(name_callbacks) do
                    if (v == callback) then
                        table.remove(name_callbacks, i)
                        local on_remove = register.on_removes[callback]
                        if (on_remove) then
                            on_remove()
                        end
                    end
                end
            else
                for i, v in ipairs(name_callbacks) do
                    local on_remove = register.on_removes[v]
                    if (on_remove) then
                        on_remove()
                    end
                end
                register.callbacks[name] = {}
            end
        end
    end

    function register.clear()
        for name, callbacks in pairs(register.callbacks) do
            register.remove(name)
        end
    end

    return register
end


local path_register = callbacks_register()
local contents_register = callbacks_register()
local time_register = callbacks_register()
local process_register = callbacks_register()


-- TODO: use kqueue on *BSD in place of ptrace

local function handle_initial_stop_signal(pid)
    ev.Child.new(function(loop, child, revents)
        local status = child:getstatus()
        if (status.stopped and status.stop_signal == signal.SIGSTOP) then
            ptrace.continue(child:getrpid())
            child:stop(LOOP)
        end
    end, pid, true):start(LOOP)
end


local function trace_process(proc, pid, co_exec)
    local function next_event(events)
        -- Only wait for given events
        ptrace.set_options(pid, events or {})

        local event = coroutine.yield()

        -- Ignore signals other than SIGTRAP
        while (event == 0) do
            event = coroutine.yield()
        end
        return event
    end

    local function proper_tracer(event)
        assert(event == 0 and pid == proc.pid)

        -- Wait for fork()
        event = next_event({ptrace.PTRACE_O_TRACEFORK})

        -- Get pid of forked process
        local bg_pid = ptrace.event_message(pid)

        -- Forked processes start with SIGSTOP - handle it
        handle_initial_stop_signal(bg_pid)

        -- Wait for exit() - ie. we're getting daemonized
        event = next_event({ptrace.PTRACE_O_TRACEEXIT})

        local exit_status = ptrace.event_message(pid)

        if (exit_status == 0) then
            -- bg_pid becomes our main process now
            proc.pid = bg_pid
            trace_process(proc, bg_pid, co_exec)
        end
    end

    local function child_signal(loop, child, revents)
        local status = child:getstatus()

        if (status.stopped) then
            local stop_signal = status.stop_signal

            if (stop_signal == signal.SIGTERM) then
                -- Remove PTRACE_O_TRACEEXIT option, otherwise we'll get
                -- PTRACE_EVENT_EXIT with exit_status == 0 (as if exit(0)
                -- was issued).
                ptrace.set_options(pid, {})
            end

            local co = traced_processes[pid]
            if (co and coroutine.status(co) ~= 'dead') then
                local event

                if (stop_signal == signal.SIGTRAP) then
                    event = ptrace.event(status.status)
                else
                    event = 0
                end

                coroutine.resume(co, event)
            end

            -- We handle SIGSTOP in handle_initial_stop_signal()
            if (stop_signal ~= signal.SIGSTOP) then
                stop_signal = stop_signal == signal.SIGTRAP and 0 or stop_signal
                ptrace.continue(pid, stop_signal)
            end
        else
            child:stop(LOOP)
            traced_processes[pid] = nil

            if (status.signaled) then
                -- Process terminated abnormally - let's kill the whole
                -- process group not to leave zombies/orphans.
                cwatchman.killpg(pid, signal.SIGKILL)
            end

            if (pid == proc.pid) then
                print('Process ' .. proc.name .. ' terminated.')
                print('Exit status: ' .. tostring(status.exit_status))
                coroutine.resume(co_exec, status.exit_status)
            end
        end
    end

    ev.Child.new(child_signal, pid, true):start(LOOP)
    traced_processes[pid] = coroutine.create(proper_tracer)
    return traced_processes[pid]
end


ProcessState = {
    RUNNING = 'running',
    STOPPED = 'stopped',
    STARTING = 'starting',
    STOPPING = 'stopping',
    FAILED = 'failed',
}


local function hash_process(proc)
    return sha2.sha256hex(table.concat {
        proc.name, proc.command,
    })
end


function process(name)
    return function(options)
        if (type(options) == 'string') then
            options = {options}
        end

        local started = false

        local Process = {
            name = name,
            command = assert(options[1], 'Process command missing'),
            state = ProcessState.STOPPED,
            default_state = options.default_state or ProcessState.RUNNING,
            pid = 0,
        }
        Process.hash = hash_process(Process)

        local function fire_events()
            local callbacks = process_register.get(Process.name)
            if (callbacks) then
                for i, callback in ipairs(callbacks) do
                    callback(Process)
                end
            end
        end

        local state_transitions = {
            -- state_transitions[desired_state][current_state] -> action
            [ProcessState.RUNNING] = {
                [ProcessState.STOPPED] = function() Process.start() end,
                [ProcessState.STOPPING] = function()
                    -- wait for process until it's ProcessState.STOPPED
                    local function stop_handler()
                        if (Process.state == ProcessState.STOPPED) then
                            Process.start()
                            unwatch_process(Process, stop_handler)
                        end
                    end
                    watch_process(Process, stop_handler)
                end,
                [ProcessState.FAILED] = function() Process.start() end,
            },
            [ProcessState.STOPPED] = {
                [ProcessState.RUNNING] = function() Process.stop() end,
                [ProcessState.STARTING] = function() Process.stop() end,
            },
        }

        local function change_state(new_state)
            if (Process.state ~= new_state) then
                Process.state = new_state
                fire_events()
            end
        end

        function Process:change_to_state(state)
            -- perform an action that will bring the Process into
            -- desired state
            local fun = state_transitions[state][self.state]
            if (fun) then fun() end
        end

        local function exec()
            change_state(ProcessState.STARTING)
            pid = cwatchman.fork()
            if (pid == -1) then
                change_state(ProcessState.FAILED)
                print('error')
            elseif (pid == 0) then
                -- Let's make forked process a process group leader
                -- (it's handy when we need to kill the process and its
                -- children).
                cwatchman.setpgrp()

                ptrace.traceme()

                -- PTRACE_TRACEME will result in a signal SIGTRAP to the process
                -- every time an `exec` is called - we don't have to issue
                -- explicit SIGSTOP.
                local ret = cwatchman.exec('/bin/sh', '-c', Process.command)
                -- if everything's fine we don't ever get here
                print(ret)
            else
                Process.pid = pid
                change_state(ProcessState.RUNNING)
                print('Process ' .. Process.name .. ' started.')
                trace_process(Process, pid, coroutine.running())
                local exit_status = coroutine.yield()
                change_state(ProcessState.STOPPED)
                Process.pid = 0
                return exit_status
            end
        end

        local co_exec = coroutine.create(function()
            while started do
                local exit_status = exec()
                print('exit_status: ' .. tostring(exit_status))
                if (exit_status ~= 0) then
                    break
                end
                cwatchman.sleep(1)
            end
        end)

        function Process.start()
            if (Process.pid == 0) then
                started = true
                coroutine.resume(co_exec)
            end
        end

        function Process.stop()
            if (Process.pid) then
                started = false
                change_state(ProcessState.STOPPING)
                cwatchman.kill(Process.pid, signal.SIGTERM)

                -- SIGKILL the process if it's still active after 5 seconds
                ev.Timer.new(function(loop, timer, revents)
                    if (Process.pid > 0) then
                        cwatchman.kill(Process.pid, signal.SIGKILL)
                    end
                end, 5):start(LOOP)
            end
        end
        
        function Process.reload()
            if (Process.pid) then
                cwatchman.kill(Process.pid, signal.SIGHUP)
            end
        end

        function Process.restart()
            if (Process.state == ProcessState.RUNNING) then
                local function stop_handler(event)
                    if (event.process.state == ProcessState.STOPPED) then
                        event.process.start()
                        unwatch_process(event.process, stop_handler)
                    end
                end

                watch_process(Process, stop_handler)
                Process.stop()
            end
        end

        processes[Process.name] = Process
        table.insert(ordered_processes, Process)

        return Process
    end
end


-- Watch functions

function watch_path(path, callback)
    local stat = ev.Stat.new(function(loop, stat, revents)
        local data = stat:getdata()
        callback(data.path, data.attr, data.prev)
    end, path)
    stat:start(LOOP)

    -- TODO: normalize path
    path_register.add(path, callback, function()
        stat:stop(LOOP)
    end)
end

function unwatch_path(path, callback)
    path_register.remove(path, callback)
end

function watch_contents(path, callback, ignore_missing)
    local function file_hash()
        local f = io.open(path)
        if (f) then
            local contents = f:read('*a')
            f:close()
            return sha2.sha256hex(contents)
        else
            return ''
        end
    end

    local last_hash = file_hash()

    local stat = ev.Stat.new(function(loop, stat, revents)
        local data = stat:getdata()

        if (not ignore_missing or data.attr.nlink > 0) then
            local hash = file_hash()
            if (hash ~= last_hash) then
                last_hash = hash
                callback(data.path, data.attr, data.prev)
            end
        end
    end, path)
    stat:start(LOOP)

    contents_register.add(path, callback, function()
        stat:stop(LOOP)
    end)
end

function unwatch_contents(path, callback)
    contents_register.remove(path, callback)
end

function watch_cron(cron_spec, callback)
    local seconds = cron.next(cron_spec)
    if (seconds > 0) then
        local timer = ev.Timer.new(function(loop, timer, revents)
            callback()
            local seconds = cron.next(cron_spec)
            if (seconds > 0) then
                timer:again(LOOP, seconds)
            end
        end, seconds)
        timer:start(LOOP)

        time_register.add(cron_spec, callback, function()
            timer:stop(LOOP)
        end)
    end
end

function unwatch_cron(cron_spec, callback)
    time_register.remove(cron_spec, callback)
end

function watch_process(process, callback)
    if (type(process) == 'table') then
        process = process.name
    elseif (type(process) ~= 'string') then
        error('Invalid type for watch_process parameter: ' .. type(process))
    end

    process_register.add(process, callback)
end

function unwatch_process(process, callback)
    process_register.remove(process.name, callback)
end


-- Load and execute the configuration file

if (not arg[1]) then
    print(USAGE)
    os.exit(1)
end

local config = loadfile(arg[1])
local config_env = {
    process = process,
    watch_path = watch_path,
    unwatch_path = unwatch_path,
    watch_contents = watch_contents,
    watch_cron = watch_cron,
    unwatch_cron = unwatch_cron,
    watch_process = watch_process,
    unwatch_process = unwatch_process,
    ProcessState = ProcessState,
    print = print,
    require = require,
    tostring = tostring,
}

if (config) then
    setfenv(config, config_env)
    config()
else
    print('error')
end


-- Watch config file and reload if necessary

local function reload_process(current_processes)
    return function(name)
        return function(options)
            local existing_process = current_processes[name]
            local new_process = process(name)(options)

            if (existing_process) then
                if (existing_process.hash == new_process.hash) then
                    processes[name] = existing_process
                    return existing_process
                else
                    watch_process(existing_process, function()
                        if (existing_process.state == ProcessState.STOPPED) then
                            new_process:change_to_state(
                                new_process.default_state)
                        end
                    end)
                    existing_process.stop()
                end
            else
                new_process:change_to_state(new_process.default_state)
            end

            return new_process
        end
    end
end

local function config_changed()
    print(arg[1] .. ' file changed - reloading')

    local current_processes = {}
    for k, v in pairs(processes) do
        current_processes[k] = v
    end

    processes = {}

    -- Remove all existing watches
    path_register.clear()
    contents_register.clear()
    time_register.clear()
    process_register.clear()

    local config = loadfile(arg[1])
    if (config) then
        config_env['process'] = reload_process(current_processes)
        setfenv(config, config_env)
        config()
    end

    -- Stop services that are not defined in the new config file
    for k, v in pairs(current_processes) do
        if (not processes[k]) then
            v.stop()
        end
    end

    -- Since we removed all watches we have to register ourselves again
    watch_contents(arg[1], config_changed, true)
end

watch_contents(arg[1], config_changed, true)


-- Start defined processes
ev.Idle.new(function(loop, idle, revents)
    for i, proc in ipairs(ordered_processes) do
        proc:change_to_state(proc.default_state)
    end
    idle:stop(LOOP)
end):start(LOOP)


local function signal_handler(loop, signal, revents)
    for k, v in pairs(processes) do
        if (v.state ~= ProcessState.STOPPED) then
            print('Stopping ' .. v.name)
            v.stop()
        end
    end
    signal:stop(LOOP)
    os.exit(0)
end


ev.Signal.new(signal_handler, signal.SIGINT):start(LOOP)
ev.Signal.new(signal_handler, signal.SIGTERM):start(LOOP)


LOOP:loop()
