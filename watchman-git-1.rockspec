package = 'watchman'
version = 'git-1'

source = {
    url = 'git://github.com/miGlanz/watchman.git',
}

description = {
    summary = 'Event-based process monitor',
    license = 'MIT/X11',
    homepage = "http://github.com/miGlanz/watchman",
}

supported_platforms = {
    'linux',
}

dependencies = {
    'lua >= 5.1',
    'lua-ev',
    'sha2',
}

build = {
    type = 'builtin',
    modules = {
        watchman = 'watchman.lua',
        cron = {
            sources = {'lcron.c'},
        },
        ptrace = {
            sources = {'lptrace.c'},
        },
        cwatchman = {
            sources = {'lcwatchman.c'},
        },
    },
    install = {
        bin = {'watchman.lua'},
    },
}

-- vim: set ft=lua:
