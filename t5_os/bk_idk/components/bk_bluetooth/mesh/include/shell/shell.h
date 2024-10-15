#pragma once
#include <stddef.h>
#include "zephyr.h"


#define shell_help(a) (void)(a)
#define shell_error(shell, ...) BT_ERR(__VA_ARGS__)
#define shell_print(shell, ...) BT_INFO(__VA_ARGS__)

#define SHELL_CMD_HELP_PRINTED	(1)

struct shell_static_args
{
    uint8_t mandatory; /*!< Number of mandatory arguments. */
    uint8_t optional;  /*!< Number of optional arguments. */
};

struct shell
{
//    const char *default_prompt; /*!< shell default prompt. */
//
//    const struct shell_transport *iface; /*!< Transport interface.*/
//    struct shell_ctx *ctx; /*!< Internal context.*/
//
//    struct shell_history *history;
//
//    const enum shell_flag shell_flag;
//
//    const struct shell_fprintf *fprintf_ctx;
//
//    struct shell_stats *stats;
//
//    const struct shell_log_backend *log_backend;
//
//    LOG_INSTANCE_PTR_DECLARE(log);
//
//    const char *thread_name;
//    struct k_thread *thread;
//    k_thread_stack_t *stack;
};

typedef int (*shell_cmd_handler)(const struct shell *shell, size_t argc, char **argv);


struct shell_static_entry
{
    const char *syntax;         /*!< Command syntax strings. */
    const char *help;           /*!< Command help string. */
    const struct shell_cmd_entry *subcmd;   /*!< Pointer to subcommand. */
    shell_cmd_handler handler;      /*!< Command handler. */
    struct shell_static_args args;      /*!< Command arguments. */
};

typedef void (*shell_dynamic_get)(size_t idx, struct shell_static_entry *entry);


struct shell_cmd_entry
{
    bool is_dynamic;
    union union_cmd_entry
    {
        /*!< Pointer to function returning dynamic commands.*/
        shell_dynamic_get dynamic_get;

        /*!< Pointer to array of static commands. */
        const struct shell_static_entry *entry;
    } u;
};


#define SHELL_EXPR_CMD_ARG(_expr, _syntax, _subcmd, _help, _handler, \
                           _mand, _opt) \
{ \
    .syntax = (_expr) ? (const char *)STRINGIFY(_syntax) : "", \
              .help  = (_expr) ? (const char *)_help : NULL, \
                       .subcmd = (const struct shell_cmd_entry *)((_expr) ? \
                                 _subcmd : NULL), \
                                 .handler = (shell_cmd_handler)((_expr) ? _handler : NULL), \
                                            .args = { .mandatory = _mand, .optional = _opt} \
}

#define SHELL_CMD_ARG(syntax, subcmd, help, handler, mand, opt) \
    SHELL_EXPR_CMD_ARG(1, syntax, subcmd, help, handler, mand, opt)




#define SHELL_STATIC_SUBCMD_SET_CREATE(name, ...)           \
    static const struct shell_static_entry shell_##name[] = {   \
        __VA_ARGS__                     \
    };                              \
    static const struct shell_cmd_entry name = {            \
        .is_dynamic = false,                    \
                      .u = { .entry = shell_##name }              \
    }

#define SHELL_SUBCMD_SET_END {NULL}

#define SHELL_CMD_ARG_REGISTER(syntax, subcmd, help, handler, mandatory, optional)

#define SHELL_CMD_REGISTER(syntax, subcmd, help, handler) \
	SHELL_CMD_ARG_REGISTER(syntax, subcmd, help, handler, 0, 0)

int32_t zephry_mesh_shell_run(size_t argc, char **argv);
int32_t zephry_bt_shell_run(size_t argc, char **argv);



