#include <stdlib.h>
#include <check.h>

#include "../src/commandline.h"

void setup(void)
{
}

void teardown(void)
{
}

START_TEST(empty_commandline_return_null)
{
    CMDLINE_OPTS *cmdline_opts = commandline_parse("");

    ck_assert_ptr_eq(cmdline_opts, NULL);
}
END_TEST

START_TEST(commandline_without_options_sets_directory)
{
    char *commandline = "/home/cwatch";
    CMDLINE_OPTS *cmdline_opts = commandline_parse(commandline);

    ck_assert_str_eq(cmdline_opts->directory, commandline);

    free(cmdline_opts);
}
END_TEST

Suite *list_suite(void)
{
    Suite *s = suite_create("Command line parser");

    TCase *tc_core = tcase_create("When parsing the command line");
    tcase_add_checked_fixture(tc_core, setup, teardown);

    tcase_add_test(tc_core, empty_commandline_return_null);
    tcase_add_test(tc_core, commandline_without_options_sets_directory);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = list_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return(number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
