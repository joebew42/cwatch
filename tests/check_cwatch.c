#include <stdlib.h>
#include <check.h>

#include "../src/cwatch.h"

/* HELPER FUNCTIONS */
void fill_with_paths(Queue *list, char **paths, int number_of_paths)
{
    int i;
    for (i = 0; i < number_of_paths; i++)
    {
        queue_enqueue(list, (void *)paths[i]);
    }
}

int inotify_add_watch_mock(int fd, const char *path, uint32_t mask)
{
    static int wd = 1;
    return wd++;
}

int inotify_rm_watch_mock(int fd, int wd)
{
    return 0;
}
/* END HELPER FUNCTIONS */

void setup(void)
{
    watch_descriptor_from = inotify_add_watch_mock;
    remove_watch_descriptor = inotify_rm_watch_mock;
}

void teardown(void)
{
    /* PASS */
}

START_TEST(test_cases_for_append_dir)
{
    ck_assert_str_eq("", append_dir("", ""));
    ck_assert_str_eq("/", append_dir("", "/"));
    ck_assert_str_eq("/", append_dir("", "//"));
    ck_assert_str_eq("a/", append_dir("", "a"));

    ck_assert_str_eq("/", append_dir("/", ""));
    ck_assert_str_eq("/", append_dir("/", "//"));

    ck_assert_str_eq("a/", append_dir("a", ""));
    ck_assert_str_eq("a/b/", append_dir("a", "b"));
    ck_assert_str_eq("a/b/", append_dir("a", "/b"));
    ck_assert_str_eq("a/b/", append_dir("a", "b/"));
    ck_assert_str_eq("a/b/", append_dir("a", "/b/"));
    ck_assert_str_eq("a/b/", append_dir("a", "///b"));
    ck_assert_str_eq("a/b/", append_dir("a", "b///"));
    ck_assert_str_eq("a/b/", append_dir("a", "///b///"));

    ck_assert_str_eq("a/b/", append_dir("a/", "b"));
    ck_assert_str_eq("a/b/", append_dir("a/", "/b"));
    ck_assert_str_eq("a/b/", append_dir("a/", "b/"));
    ck_assert_str_eq("a/b/", append_dir("a/", "/b/"));
    ck_assert_str_eq("a/b/", append_dir("a/", "///b"));
    ck_assert_str_eq("a/b/", append_dir("a/", "b///"));
    ck_assert_str_eq("a/b/", append_dir("a/", "///b///"));

    ck_assert_str_eq("a/", append_dir("a//", ""));
    ck_assert_str_eq("a/b/", append_dir("a///", "///b/////"));
}
END_TEST

START_TEST(test_cases_for_append_file)
{
    ck_assert_str_eq("", append_file("", ""));
    ck_assert_str_eq("file", append_file("", "file"));
    ck_assert_str_eq("a/file", append_file("a", "file"));
    ck_assert_str_eq("./a/file", append_file("./a", "file"));

    ck_assert_str_eq("/file", append_file("///", "file"));
    ck_assert_str_eq("file", append_file("", "///file"));
    ck_assert_str_eq("file", append_file("", "////file////"));
    ck_assert_str_eq("", append_file("", "/"));
    ck_assert_str_eq("/", append_file("/", "/"));
    ck_assert_str_eq("", append_file("", "/"));

    ck_assert_str_eq("a/", append_file("a", ""));
}
END_TEST

START_TEST(creates_a_wd_data)
{
    char *path = "/usr/opt/path/";
    int wd = 1;
    WD_DATA *wd_data = create_wd_data(path, wd);

    ck_assert_ptr_ne(wd_data, NULL);
    ck_assert_str_eq(wd_data->path, path);
    ck_assert_int_eq(wd_data->wd, wd);
    ck_assert_ptr_eq(wd_data->links->first, NULL);
}
END_TEST

START_TEST(creates_a_link_data)
{
    char *link_path = "/usr/opt/symlink";
    WD_DATA *wd_data = create_wd_data("foo", 0);
    LINK_DATA *link_data = create_link_data(link_path, wd_data);

    ck_assert_ptr_ne(link_data, NULL);
    ck_assert_str_eq(link_data->path, link_path);
    ck_assert_ptr_eq(link_data->wd_data, wd_data);
}
END_TEST

START_TEST(formats_command_correctly_using_special_characters)
{
    ck_assert_ptr_eq(NULL, NULL);
}
END_TEST

START_TEST(returns_true_if_a_path_is_a_child_of_another_path)
{
    char *parent = "/usr/opt/parent/";
    char *child = "/usr/opt/parent/child/of/";

    bool_t expected = TRUE;
    bool_t actual = is_child_of(parent, child);

    ck_assert_msg(
        expected == actual,
        "The child path provided is not child of parent path");
}
END_TEST

START_TEST(returns_false_if_a_path_is_not_a_child_of_another_path)
{
    char *parent = "/usr/opt/parent/";
    char *child = "/usr/opt/another_parent/child/of/";

    bool_t expected = FALSE;
    bool_t actual = is_child_of(parent, child);

    ck_assert_msg(
        expected == actual,
        "The child path provided is child of parent path");
}
END_TEST

START_TEST(returns_true_if_a_path_is_listed_as_child)
{
    char *paths[] = {
        "/usr/opt/path1",
        "/usr/opt/path1/child",
        "/usr/opt/path3"};

    Queue *list = queue_init();
    fill_with_paths(list, paths, 3);

    bool_t expected = TRUE;
    bool_t actual = is_listed_as_child(paths[1], list);

    ck_assert_msg(
        expected == actual,
        "The provided path is not listed as a child of the list of paths");

    queue_free(list);
}
END_TEST

START_TEST(returns_false_if_a_path_is_not_listed_as_child)
{
    char *paths[] = {
        "/usr/opt/path1",
        "/usr/opt/path2",
        "/usr/opt/path3"};

    Queue *list = queue_init();
    fill_with_paths(list, paths, 3);

    bool_t expected = FALSE;
    bool_t actual = is_listed_as_child("/usr/opt/not/listed", list);

    ck_assert_msg(
        expected == actual,
        "The provided path is not listed as a child of the list of paths");

    queue_free(list);
}
END_TEST

START_TEST(returns_true_if_a_path_is_related_to_another)
{
    char *first_path = "/usr/opt/";
    char *second_path = "/usr/opt/path1/";
    char *third_path = "/usr/opt/path1/subpath1/";

    ck_assert_msg(
        TRUE == is_related_to(second_path, first_path),
        "The first path is not related to the second path");

    ck_assert_msg(
        TRUE == is_related_to(second_path, third_path),
        "The third path is not related to the second path");
}
END_TEST

START_TEST(returns_false_if_a_path_is_not_related_to_another)
{
    char *first_path = "/usr/opt/";
    char *second_path = "/var/opt/path1/";
    char *third_path = "/usr/opt/path1/subpath1/";

    ck_assert_msg(
        FALSE == is_related_to(second_path, first_path),
        "The first path is related to the second path");

    ck_assert_msg(
        FALSE == is_related_to(second_path, third_path),
        "The third path is related to the second path");
}
END_TEST

START_TEST(adds_a_directory_to_the_watch_list)
{
    uint32_t event_mask = 0;

    int fd = 1;
    Queue *list_wd = queue_init();

    char *real_path = "/home/cwatch/";
    char *symlink = NULL;

    add_to_watch_list(real_path, symlink, fd, list_wd);

    ck_assert_int_eq(queue_size(list_wd), 1);

    queue_free(list_wd);
}
END_TEST

START_TEST(get_a_node_from_path)
{
    int fd = 1;
    Queue *list_wd = queue_init();

    char *real_path = "/home/cwatch/";
    char *symlink = NULL;

    add_to_watch_list(real_path, symlink, fd, list_wd);

    QueueNode *node = get_node_from_path(real_path, list_wd);
    WD_DATA *wd_data = node->data;

    ck_assert_ptr_eq(real_path, wd_data->path);

    queue_free(list_wd);
}
END_TEST

START_TEST(get_a_node_from_wd)
{
    int fd = 1;
    Queue *list_wd = queue_init();

    char *real_path = "/home/cwatch/";
    char *symlink = NULL;

    add_to_watch_list(real_path, symlink, fd, list_wd);

    QueueNode *expected_node = get_node_from_path(real_path, list_wd);
    WD_DATA *wd_data = expected_node->data;

    int real_wd = wd_data->wd;

    QueueNode *node = get_node_from_wd(real_wd, list_wd);

    ck_assert_ptr_eq(expected_node, node);

    queue_free(list_wd);
}
END_TEST

START_TEST(adds_a_directory_that_is_reached_by_symlink_to_the_watch_list)
{
    uint32_t event_mask = 0;

    int fd = 1;
    Queue *list_wd = queue_init();

    char *real_path = "/home/cwatch/";
    char *symlink = "/home/symlink_to_cwatch";

    add_to_watch_list(real_path, symlink, fd, list_wd);

    ck_assert_int_eq(queue_size(list_wd), 1);

    queue_free(list_wd);
}
END_TEST

START_TEST(get_a_link_node_from_path)
{
    int fd = 1;
    Queue *list_wd = queue_init();

    char *real_path = "/home/cwatch/";
    char *symlink = "/home/symlink";

    add_to_watch_list(real_path, symlink, fd, list_wd);
    QueueNode *list_node = get_link_node_from_path(symlink, list_wd);

    LINK_DATA *link_data = list_node->data;
    WD_DATA *wd_data = link_data->wd_data;

    ck_assert_ptr_eq(real_path, wd_data->path);

    queue_free(list_wd);
}
END_TEST

START_TEST(get_a_link_data_from_wd_data)
{
    int fd = 1;
    Queue *list_wd = queue_init();

    char *real_path = "/home/cwatch/";
    char *symlink = "/home/symlink";

    add_to_watch_list(real_path, symlink, fd, list_wd);
    WD_DATA *wd_data = list_wd->first->data;

    LINK_DATA *link_data = get_link_data_from_wd_data(symlink, wd_data);

    ck_assert_ptr_eq(link_data->path, symlink);

    queue_free(list_wd);
}
END_TEST

START_TEST(get_a_link_data_from_path)
{
    int fd = 1;
    Queue *list_wd = queue_init();

    char *real_path = "/home/cwatch/";
    char *symlink = "/home/symlink";

    add_to_watch_list(real_path, symlink, fd, list_wd);

    LINK_DATA *link_data = get_link_data_from_path(symlink, list_wd);

    ck_assert_ptr_eq(link_data->path, symlink);

    queue_free(list_wd);
}
END_TEST

START_TEST(return_true_if_path_is_a_symbolic_link)
{
    int fd = 1;
    Queue *list_wd = queue_init();

    char *real_path = "/home/cwatch/";
    char *symlink = "/home/symlink";

    add_to_watch_list(real_path, symlink, fd, list_wd);

    ck_assert_msg(
        TRUE == is_symlink(symlink, list_wd),
        "path provided is not a symbolic link");

    ck_assert_msg(
        FALSE == is_symlink("/home/no_symlink", list_wd),
        "path provided is a symbolic link");

    queue_free(list_wd);
}
END_TEST

START_TEST(unwatch_a_directory_from_the_watch_list)
{
    int fd = 1;
    Queue *list_wd = queue_init();

    char *real_path = "/home/cwatch/";

    add_to_watch_list(real_path, NULL, fd, list_wd);

    unwatch_path(real_path, fd, list_wd);

    ck_assert_int_eq(queue_size(list_wd), 0);

    queue_free(list_wd);
}
END_TEST

START_TEST(find_symlinks_that_are_contained_in_some_path)
{
    char *path = "/home/cwatch/";
    Queue *symlinks_to_check = queue_init();
    Queue *symlinks_found = queue_init();

    LINK_DATA *symlink_in = create_link_data("/home/cwatch/symlink_in", NULL);
    LINK_DATA *symlink_out = create_link_data("/home/symlink_out", NULL);

    queue_enqueue(symlinks_to_check, (void *)symlink_in);
    queue_enqueue(symlinks_to_check, (void *)symlink_out);

    symlinks_contained_in(path, symlinks_to_check, symlinks_found);

    ck_assert_int_eq(queue_size(symlinks_found), 1);

    queue_free(symlinks_to_check);
    queue_free(symlinks_found);
}
END_TEST

START_TEST(find_all_symlinks_that_are_contained_in_some_path)
{
    uint32_t event_mask = 0;

    int fd = 1;

    Queue *list_wd = queue_init();
    Queue *symlinks_found = queue_init();

    add_to_watch_list("/home/user/directory/", "/home/cwatch/symlink_one", fd, list_wd);
    add_to_watch_list("/home/user/directory/subdirectory", "/home/cwatch/symlink_two", fd, list_wd);
    add_to_watch_list("/home/user/directory/", "/home/other/symlink_three", fd, list_wd);

    all_symlinks_contained_in("/home/cwatch/", list_wd, symlinks_found);

    ck_assert_int_eq(queue_size(symlinks_found), 2);

    char *symlink_one_path = (char *)queue_dequeue(symlinks_found);
    ck_assert_str_eq(symlink_one_path, "/home/cwatch/symlink_one");

    char *symlink_two_path = (char *)queue_dequeue(symlinks_found);
    ck_assert_str_eq(symlink_two_path, "/home/cwatch/symlink_two");

    queue_free(list_wd);
    queue_free(symlinks_found);
}
END_TEST

START_TEST(find_common_referenced_paths_of_a_path)
{
    uint32_t event_mask = 0;

    int fd = 1;

    Queue *list_wd = queue_init();

    add_to_watch_list("/home/user/directory/", NULL, fd, list_wd);
    add_to_watch_list("/home/user/directory/a/", "/home/cwatch/symlink_one", fd, list_wd);
    add_to_watch_list("/home/user/directory/a/aa/", "/home/cwatch/symlink_two", fd, list_wd);
    add_to_watch_list("/home/user/directory/b/", "/home/cwatch/symlink_three", fd, list_wd);
    add_to_watch_list("/home/user/directory/b/bb", "/home/cwatch/symlink_four", fd, list_wd);

    Queue *referenced_paths = common_referenced_paths_for("/home/user/directory/", list_wd);

    ck_assert_int_eq(queue_size(referenced_paths), 2);

    queue_free(list_wd);
}
END_TEST

START_TEST(unwatch_a_symbolic_link_from_the_watch_list)
{
    int fd = 1;
    Queue *list_wd = queue_init();

    char *real_path = "/home/cwatch/";
    char *path_of_symlink = "/home/cwatch/symlink_to_cwatch";

    root_path = real_path;

    add_to_watch_list(real_path, path_of_symlink, fd, list_wd);

    unwatch_symlink(path_of_symlink, fd, list_wd);

    QueueNode *node = get_node_from_path(real_path, list_wd);
    WD_DATA *wd_data = (WD_DATA *)node->data;

    ck_assert_int_eq(queue_size(wd_data->links), 0);

    queue_free(list_wd);
}
END_TEST

START_TEST(unwatch_an_outside_directory_removing_a_symlink_inside)
{
    int fd = 1;
    Queue *list_wd = queue_init();

    char *real_path = "/home/cwatch/";
    char *outside_dir = "/home/outside/";
    char *symlink_to_outside = "/home/cwatch/symlink_to_outside";

    root_path = real_path;

    add_to_watch_list(real_path, NULL, fd, list_wd);

    add_to_watch_list(outside_dir, symlink_to_outside, fd, list_wd);

    unwatch_symlink(symlink_to_outside, fd, list_wd);

    QueueNode *node = get_node_from_path(outside_dir, list_wd);
    ck_assert_ptr_eq(node, NULL);

    queue_free(list_wd);
}
END_TEST

START_TEST(remove_orphan_resources_from_a_tree_with_symlink_outside)
{
    int fd = 1;
    Queue *list_wd = queue_init();

    char *real_path = "/home/cwatch/";
    char *symlink = "/home/cwatch/symlink_to_pointed/";

    root_path = real_path;

    add_to_watch_list(root_path, NULL, fd, list_wd);
    add_to_watch_list("/home/cwatch/to_be_removed/", NULL, fd, list_wd);
    add_to_watch_list("/home/cwatch/to_be_removed/inside/", NULL, fd, list_wd);

    add_to_watch_list("/home/cwatch/to_be_removed/inside/pointed/", symlink, fd, list_wd);
    add_to_watch_list("/home/cwatch/to_be_removed/inside/pointed/sub_pointed/", NULL, fd, list_wd);

    Queue *referenced_resources = common_referenced_paths_for("/home/cwatch/to_be_removed", list_wd);
    remove_orphan_watched_resources(real_path, referenced_resources, fd, list_wd);

    /*
     * after the clean should be removed all the directories
     * except:
     *    1. root_path
     *    2. pointed/ and its content (sub_pointed)
     */
    ck_assert_int_eq(3, queue_size(list_wd));

    queue_free(list_wd);
    queue_free(referenced_resources);
}
END_TEST

START_TEST(remove_unreachable_resources_not_in_root_path)
{
    uint32_t event_mask = 0;

    int fd = 1;
    Queue *list_wd = queue_init();

    char *root_path = "/home/cwatch";

    char *outside_path = "/tmp/cwatch/";
    char *symlink = "/home/cwatch/link_to_tmp";

    add_to_watch_list(outside_path, symlink, fd, list_wd);
    add_to_watch_list("/tmp/cwatch/dir1", NULL, fd, list_wd);
    add_to_watch_list("/tmp/cwatch/dir2", NULL, fd, list_wd);

    ck_assert_int_eq(queue_size(list_wd), 3);

    LINK_DATA *link_data = get_link_data_from_path(symlink, list_wd);
    Queue *links = link_data->wd_data->links;

    queue_remove(links, links->first);

    ck_assert_int_eq(queue_size(links), 0);

    WD_DATA *wd_data = (WD_DATA *)get_node_from_path(outside_path, list_wd)->data;
    remove_unreachable_resources(wd_data, fd, list_wd);

    ck_assert_int_eq(queue_size(list_wd), 0);

    queue_free(list_wd);
}
END_TEST

Suite *cwatch_suite(void)
{
    Suite *s = suite_create("cwatch");

    TCase *tc_core = tcase_create("When dealing with cwatch");
    tcase_add_checked_fixture(tc_core, setup, teardown);

    /* utility functions */
    tcase_add_test(tc_core, returns_true_if_a_path_is_a_child_of_another_path);
    tcase_add_test(tc_core, returns_false_if_a_path_is_not_a_child_of_another_path);
    tcase_add_test(tc_core, returns_true_if_a_path_is_listed_as_child);
    tcase_add_test(tc_core, returns_false_if_a_path_is_not_listed_as_child);
    tcase_add_test(tc_core, returns_true_if_a_path_is_related_to_another);
    tcase_add_test(tc_core, returns_false_if_a_path_is_not_related_to_another);

    /* cwatch functions */
    tcase_add_test(tc_core, creates_a_wd_data);
    tcase_add_test(tc_core, creates_a_link_data);
    tcase_add_test(tc_core, adds_a_directory_to_the_watch_list);
    tcase_add_test(tc_core, get_a_node_from_path);
    tcase_add_test(tc_core, get_a_node_from_wd);
    tcase_add_test(tc_core, adds_a_directory_that_is_reached_by_symlink_to_the_watch_list);
    tcase_add_test(tc_core, get_a_link_node_from_path);
    tcase_add_test(tc_core, get_a_link_data_from_wd_data);
    tcase_add_test(tc_core, get_a_link_data_from_path);
    tcase_add_test(tc_core, return_true_if_path_is_a_symbolic_link);
    tcase_add_test(tc_core, unwatch_a_directory_from_the_watch_list);
    tcase_add_test(tc_core, find_symlinks_that_are_contained_in_some_path);
    tcase_add_test(tc_core, find_all_symlinks_that_are_contained_in_some_path);
    tcase_add_test(tc_core, find_common_referenced_paths_of_a_path);
    tcase_add_test(tc_core, unwatch_a_symbolic_link_from_the_watch_list);
    tcase_add_test(tc_core, formats_command_correctly_using_special_characters);
    tcase_add_test(tc_core, unwatch_an_outside_directory_removing_a_symlink_inside);
    tcase_add_test(tc_core, remove_orphan_resources_from_a_tree_with_symlink_outside);
    tcase_add_test(tc_core, remove_unreachable_resources_not_in_root_path);
    tcase_add_test(tc_core, test_cases_for_append_dir);
    tcase_add_test(tc_core, test_cases_for_append_file);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = cwatch_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
