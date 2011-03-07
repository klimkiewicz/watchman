# Watchman

`watchman` is a lightwight, easy-to-use process monitor. It's written in a mix of Lua and C.


## Features

`watchman` has a couple of features you won't find in other similar tools:

* It's event based (we're using great [libev](http://software.schmorp.de/pkg/libev.html) library);
* You match `watch` for such events as: process state changes, file/directory changes, cron-based time events;
* `watchman` watches for changes in its configuration file itself, if a change is detected it's automatically picked up (no running processes are stopped);
* `watchman` uses Linux `ptrace()` facility and can detect daemonizing processes automatically - it doesn't have to track pid-files.


## Overview

Let's say you want to start (and keep it running) the `nginx` web server. You would start with a simple `nginx.lua` configuration file:

    process 'nginx' '/usr/sbin/nginx -c /etc/nginx/conf/nginx.conf'

That's it. Let's start running this configuration file:

    watchman nginx.lua

`nginx` is now running. Let's open another terminal window and try to terminate it:

    killall nginx

You should see the following output in the `watchman` window:

    Process terminated: nginx
    Starting nginx.

As you see, `watchman` detected that the process was killed and restarted it automatically.

Let's add another process for `watchman` to monitor. Without stopping `watchman`, add the following line to `nginx.lua`:

    process 'cat '/bin/cat'

Right after you saved this file you should see the following output in the `watchman` console:

    Starting cat.

`nginx` is of course still running!


## API

## Credits

`Watchman` uses the following great components:

* [libev](http://software.schmorp.de/pkg/libev.html)
* [ragel](http://www.complang.org/ragel/)

