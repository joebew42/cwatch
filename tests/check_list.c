#include <stdlib.h>
#include <check.h>
#include "../src/list.h"

/* helper functions */
void add_items_to(LIST *list, const int number_of_items)
{
    if (number_of_items < 1 || NULL == list)
        return;

    int i;
    for (i = 0; i < number_of_items; ++i) {
        int *item = (int *) malloc(sizeof(int));
        *item = i;

        list_push(list, (void *) item);
    }
}
/* end of helper functions */

LIST *list;

void setup(void)
{
    list = list_init();
}

void teardown(void)
{
    list_free(list);
}

START_TEST(has_a_good_factory)
{
    ck_assert_ptr_ne(list, NULL);
    ck_assert_ptr_eq(list->first, NULL);
    ck_assert_ptr_eq(list->last, NULL);
}
END_TEST

START_TEST(push_one_item)
{
    add_items_to(list, 1);

    ck_assert_ptr_ne(list->first, NULL);
    ck_assert_ptr_eq(list->first, list->last);
}
END_TEST

START_TEST(push_two_items)
{
    add_items_to(list, 2);

    ck_assert_ptr_ne(list->last, NULL);
    ck_assert_ptr_ne(list->first, list->last);
}
END_TEST

START_TEST(pop_one_item)
{
    add_items_to(list, 2);

    int expected = 0;
    int *actual = (int *) list_pop(list);

    ck_assert_int_eq(*actual, expected);
}
END_TEST

START_TEST(remove_one_item)
{
    add_items_to(list, 1);

    list_remove(list, list->first);

    ck_assert_ptr_eq(list->first, NULL);
    ck_assert_ptr_eq(list->last, NULL);
}
END_TEST

START_TEST(count_number_of_items)
{
    int expected = 10;
    int actual = 0;

    add_items_to(list, expected);

    LIST_NODE *node = list->first;
    while (node != NULL) {
        node = node->next;
        actual++;
    }

    ck_assert_int_eq(actual, expected);
}
END_TEST

Suite *list_suite(void)
{
    Suite *s = suite_create("List");

    /* Core test case */
    TCase *tc_core = tcase_create ("Core");
    tcase_add_checked_fixture(tc_core, setup, teardown);

    tcase_add_test(tc_core, has_a_good_factory);
    tcase_add_test(tc_core, push_one_item);
    tcase_add_test(tc_core, push_two_items);
    tcase_add_test(tc_core, pop_one_item);
    tcase_add_test(tc_core, remove_one_item);
    tcase_add_test(tc_core, count_number_of_items);

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
    srunner_free (sr);
    return(number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
