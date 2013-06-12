#include <stdlib.h>
#include <check.h>

#include "../src/cwatch.h"

/* helper functions */

/* end of helper functions */

LIST *list;

void setup(void)
{
    list_wd = list_init();
}

void teardown(void)
{
    list_free(list_wd);
}

START_TEST(creates_a_wd_data)
{
    ck_assert_ptr_eq(NULL, NULL);
}
END_TEST

Suite *cwatch_suite(void)
{
    Suite *s = suite_create("cWatch");

    /* Core test case */
    TCase *tc_core = tcase_create("When dealing with cWatch");
    tcase_add_checked_fixture(tc_core, setup, teardown);

    tcase_add_test(tc_core, creates_a_wd_data);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = cwatch_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free (sr);
    return(number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
