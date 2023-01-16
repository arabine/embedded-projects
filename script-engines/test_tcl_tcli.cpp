
#include <stdio.h>
#include <string.h>
#include "tcli.h"
#include "tclie.h"

void output(void * arg, const char * str)
{
    printf("%s", str);
}

static const char test1[] = R"(puts "This is line 1";)";

int echo(void * arg, int argc, const char ** argv)
{
    if(argc > 1)
        printf("%s\r\n", argv[1]);

    return 0;
}

static const tclie_cmd_t cmds[] = {
    // Name, callback, min user level, min args, max args, description (for help)
    {"echo", echo, "Echo input."}
};



void test_tcl_tcli()
{

    tclie_t tclie;
    tclie_init(&tclie, output, NULL);

    tclie_reg_cmds(&tclie, cmds, 1);
//    while (1) {

        for (int i = 0; i < strlen(test1); i++)
        {
            tclie_input_char(&tclie, test1[i]);
        }

//        char c = getchar(); // Read e.g. serial input
//        tclie_input_char(&tclie, c);
//    }
}


