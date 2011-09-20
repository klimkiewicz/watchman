# Watchman

`watchman` is a lightwight, easy-to-use process monitor. It's written in a mix of Lua and C.


## Installation

You will need the following components first:

1. [Lua 5.1](http://lua.org)
2. [LuaRocks](http://luarocks.org)
3. [libev](http://software.schmorp.de/pkg/libev.html)

Then simply do:

```bash
sudo luarocks install lua_signal
sudo luarocks install https://raw.github.com/miGlanz/watchman/master/lua-ev-scm-1.rockspec
sudo luarocks install https://raw.github.com/miGlanz/watchman/master/watchman-git-1.rockspec
```


## Features

`watchman` has a couple of features you won't find in other similar tools:

* It's event based (we're using great [libev](http://software.schmorp.de/pkg/libev.html) library);
* You may `watch` for such events as: process state changes, file/directory changes, cron-based time events;
* `watchman` watches for changes in its configuration file itself, if a change is detected it's automatically picked up (no running processes are stopped);
* `watchman` uses Linux `ptrace()` facility and can detect daemonizing processes automatically - it doesn't have to track pid-files.


## Sample script

```lua
NGINX_CONF = '/etc/nginx/conf/nginx.conf'

-- Launch nginx daemon (and respawn it if it goes down)
nginx = process 'nginx' { '/usr/sbin/nginx -c ' .. NGINX_CONF }

-- Reload nginx (by sending SIGHUP) if the config file changes
watch_contents(NGINX_CONF, nginx.reload)
```

## Overview

Let's say you want to start (and keep it running) the `nginx` web server. You would start with a simple `nginx.lua` configuration file:

```lua
nginx = process 'nginx' '/usr/sbin/nginx -c /etc/nginx/conf/nginx.conf'
```

That's it. Let's start running this configuration file:

```bash
watchman nginx.lua
```

`nginx` is now running. Let's open another terminal window and try to terminate it:

```bash
killall nginx
```

You should see the following output in the `watchman` window:

    Process terminated: nginx
    Starting nginx.

As you see, `watchman` detected that the process was killed and restarted it automatically.

Let's add another process for `watchman` to monitor. Without stopping `watchman`, add the following line to `nginx.lua`:

```lua
process 'cat' '/bin/cat'
```

Right after you saved this file you should see the following output in the `watchman` console:

    Starting cat.

`nginx` is of course still running!

What if we wanted to automatically reload `nginx` when the configuration file is modified? `nginx` can reload its configuration while running. But it doesn't autodetect modifications to this file. Fortunately `watchman` can do this pretty well. Simply add the following line to our `nginx.lua`:

```lua
watch_contents('/etc/nginx/conf/nginx.conf', nginx.reload)
```

Now, whenever we modify `nginx.conf` the `nginx` process will get `SIGHUP` signal.


## API

These are functions you may use in `watchman` configuration files:

* **process(process_name)(process_options)**
* **watch_process(process, callback)**
* **unwatch_process(process [, callback])**
* **watch_path(path, callback)**
* **unwatch_path(path [, callback])**
* **watch_contents(path, callback)**
* **unwatch_contents(path [, callback])**
* **watch_cron(cron_spec, callback)**
* **unwatch_cron(cron_spec [, callback])**

## Credits

`watchman` uses the following great components:

* [libev](http://software.schmorp.de/pkg/libev.html)
* [lua-ev](https://github.com/brimworks/lua-ev)
* [ragel](http://www.complang.org/ragel/)


## License

`watchman` is free software and uses the same [license](http://www.opensource.org/licenses/mit-license.html) as Lua 5.1.
