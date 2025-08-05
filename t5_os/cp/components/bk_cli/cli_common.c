// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include "cli_common.h"
#include "cli.h"

void print_help(const char *progname, void **argtable)
{
    arg_dstr_t ds = arg_dstr_create();
    if (!ds) {
        printf("Failed to create arg_dstr\n");
        return;
    }

    arg_print_glossary_gnu_ds(ds, argtable);

    const char *help_str = arg_dstr_cstr(ds);
    if (help_str) {
        char *help_copy = strdup(help_str);
        if (help_copy) {
            char *line = strtok(help_copy, "\n");
            while (line != NULL) {
                BK_DUMP_OUT("%s\n", line);
                line = strtok(NULL, "\n");
            }
            free(help_copy);
        }
    } else {
        printf("Failed to generate help string\n");
    }

    arg_dstr_destroy(ds);
}

void common_cmd_handler(int argc, char **argv, void **argtable, int argtable_size, void (*handler)(void **argtable))
{
    struct arg_end *end = (struct arg_end *)argtable[argtable_size - 1];

    // Check if argtable is initialized successfully
    if (arg_nullcheck(argtable) != 0) {
        printf("Memory allocation error\n");
        return;
    }

    // Parse command-line arguments
    int nerrors = arg_parse(argc, argv, argtable);

    // If help is requested, print help information
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    if (help && help->count > 0) {
        print_help(argv[0], argtable);
        arg_freetable(argtable, argtable_size);  // Use the explicit size here
        return;
    }

    // If there are parse errors, print errors and usage
    if (nerrors > 0) {
        arg_dstr_t ds = arg_dstr_create();
        arg_print_errors_ds(ds, end, argv[0]);

		const char *error_str = arg_dstr_cstr(ds);
		if (error_str) {
			char *error_copy = strdup(error_str);
			if (error_copy) {
				char *line = strtok(error_copy, "\n");
				while (line != NULL) {
					printf("%s\n", line);
					line = strtok(NULL, "\n");
				}
				free(error_copy);
			}
		} else {
			printf("Failed to generate error string\n");
		}

        arg_dstr_destroy(ds);

        printf("Try '%s --help' for more information.\n", argv[0]);
        arg_freetable(argtable, argtable_size);  // Use the explicit size here
        return;
    }

    // Execute user-provided handler function
    if (handler) {
        handler(argtable);
    }

    // Free allocated memory for argtable
    arg_freetable(argtable, argtable_size);  // Use the explicit size here
}

