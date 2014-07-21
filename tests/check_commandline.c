#include <stdlib.h>
#include <check.h>

#include "../src/commandline.h"

void setup(void)
{
}

void teardown(void)
{
}

START_TEST(test_that_foo_pass)
{
    ck_assert_int_eq(foo(), 0);
}
END_TEST

Suite *list_suite(void)
{
    Suite *s = suite_create("Command line parser");

    TCase *tc_core = tcase_create("When parsing the command line");
    tcase_add_checked_fixture(tc_core, setup, teardown);

    tcase_add_test(tc_core, test_that_foo_pass);

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
