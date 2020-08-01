#include <stdlib.h>
#include <check.h>

#include "../src/queue.h"

/* helper functions */
void add_items_to(Queue *queue, const int number_of_items)
{
    if (number_of_items < 1 || NULL == queue)
        return;

    int i;
    for (i = 0; i < number_of_items; ++i)
    {
        int *item = (int *)malloc(sizeof(int));
        *item = i;

        queue_enqueue(queue, (void *)item);
    }
}
/* end of helper functions */

Queue *queue;

void setup(void)
{
    queue = queue_init();
}

void teardown(void)
{
    queue_free(queue);
}

START_TEST(has_a_good_factory)
{
    ck_assert_ptr_ne(queue, NULL);
    ck_assert_ptr_eq(queue->first, NULL);
    ck_assert_ptr_eq(queue->last, NULL);
}
END_TEST

START_TEST(enqueue_one_item)
{
    add_items_to(queue, 1);

    ck_assert_ptr_ne(queue->first, NULL);
    ck_assert_ptr_eq(queue->first, queue->last);
}
END_TEST

START_TEST(enqueue_two_items)
{
    add_items_to(queue, 2);

    ck_assert_ptr_ne(queue->last, NULL);
    ck_assert_ptr_ne(queue->first, queue->last);
}
END_TEST

START_TEST(dequeue_one_item)
{
    add_items_to(queue, 2);

    int expected = 0;
    int *actual = (int *)queue_dequeue(queue);

    ck_assert_int_eq(*actual, expected);
}
END_TEST

START_TEST(remove_one_item)
{
    add_items_to(queue, 1);

    queue_remove(queue, queue->first);

    ck_assert_ptr_eq(queue->first, NULL);
    ck_assert_ptr_eq(queue->last, NULL);
}
END_TEST

START_TEST(size_of_the_queue)
{
    int expected = 10;

    add_items_to(queue, expected);

    int actual = queue_size(queue);

    ck_assert_int_eq(actual, expected);
}
END_TEST

Suite *queue_suite(void)
{
    Suite *s = suite_create("Queue");

    /* Core test case */
    TCase *tc_core = tcase_create("When dealing with a Queue");
    tcase_add_checked_fixture(tc_core, setup, teardown);

    tcase_add_test(tc_core, has_a_good_factory);
    tcase_add_test(tc_core, enqueue_one_item);
    tcase_add_test(tc_core, enqueue_two_items);
    tcase_add_test(tc_core, dequeue_one_item);
    tcase_add_test(tc_core, remove_one_item);
    tcase_add_test(tc_core, size_of_the_queue);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = queue_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
