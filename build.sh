cd src

gcc -Wall -g \
    exec.c \
    chunicmd.c \
    execunit.c \
    parser.c \
    constant.c \
    edit.c \
    ppchars.c \
    tools/chuni_tty.c \
    tools/chuni_wait.c \
    tools/tools.c \
    ../main.c -o ../main
