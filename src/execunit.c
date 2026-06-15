#include "execunit.h"

static struct cmd *init_cmd()
{
    struct cmd *ptr = malloc(sizeof(struct cmd));
    ptr->next = NULL;
    ptr->pid = 0;
    ptr->arg = NULL;
    return ptr;
}

void init_unit(struct exec_unit *ut)
{
    ut->flags = 0;
    ut->input = ut->output = NULL;
    ut->first = ut->last = NULL;
}

void clear_unit(struct exec_unit *ut)
{
    if (ut->input != NULL)
        free(ut->input);
    if (ut->output != NULL)
        free(ut->output);
    while (ut->first != NULL)
    {
        struct cmd *ptr = ut->first;
        ut->first = ut->first->next;
        free(ptr);
    }
    init_unit(ut);
}

void unit_add_cmd(struct exec_unit *ut, char **arg)
{
    if (ut->first == NULL)
    {
        ut->first = ut->last = init_cmd();
    }
    else
    {
        ut->last->next = init_cmd();
        ut->last = ut->last->next;
    }

    ut->last->arg = arg;
}
