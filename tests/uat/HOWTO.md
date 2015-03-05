How to run the tests
====================

All the tests are integrated with the `make` utility so to run all the tests simply run `make check`.

Should be safe to take advantage of the `--jobs` option to speedup the process ;).

How to write a test
===================

A test is a shell script written using the [Sharness](https://github.com/mlafeldt/sharness) library.  
An its core a test is a sequence of sh commands ending with a condition that should be verified to successfully pass.  

This that follow is a real, simple, test case:

    #!/bin/sh

    test_description="cwatch ..."

    . ./libtest/util.sh
    . ./libtest/sharness.sh

    test_expect_success "touch a file on create event" '
            mkdir box &&
            cwatch -d "box" -c "touch expected" -e create &&
            sleep 0.5 &&
            touch box/actual &&
            sleep 1 &&
            kill_cwatch &&
            [ -e expected ]
        '
    test_done

First and foremost it is a runnable script so it needs the execution permission.  
Sharness is written in `sh` so use `#!/bin/sh` for any test.

Each test must have a description that *briefly* describe its target this should be at te beginning of the test inside  
the `test_description` variable.

To use Sharness and to correctly run/kill cwatch it's important to source the `sharness.sh` and `util.sh` library  
inside `libtest/`.  
Note that the order is important so ensure that `sharness.sh` is the *last* script sourced.

Sharness provides some interestings commands to define a test-case, for a full list see [here](https://github.com/mlafeldt/sharness/blob/master/API.md) we briefly describe `test_expect_success`.

The first string is the name of the test case, again it should be really short.  
The second argument contains a list of commands, pay attention when to use the single quote '' or the double quote ""  
since sh will interpret them accordingly.  
To run cwatch you have to use the `cwatch` function provided by `utils.sh` and it will run the proper binary.  
Obviously the commands depends by the test but normally you end with `kill_cwatch` (to properly close cwatch) and a  
conditions using `test`, `[ ]`, `test_cmp` or whatever command that should not fail.

It is mandatory that a test-suite end with the command `test_done`.

How to name and add a test
==========================

We like verbosity so name a test with a complete description of what it wants to test.  
Each test file must end with '.t' extension and follows snake_case convention.  

To integrate your test with `make check`, simply put its name in the `TESTS` variable  
inside `Makefile.am`.

Good practices
==============

1. In some cases is highly suggested to use some `sleep` commands to ensure that
cwatch and the test-suite work correctly, these are:
    - _after cwatch is called_: this ensure that a (big) directory is completly
      initialized (especially when the -r option is provided)

    - _after a trigger action_: this ensure that a command executed has
      completed its life-cycle and the result can be tested

2. It is suggested to separate each command with `&&` to fail the test if a single
step fails.
