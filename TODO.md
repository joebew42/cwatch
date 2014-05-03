### TODO

1. code coverage on remove_unreachable_resources
2. refactoring  unwatch_symlink
3. check remove_orphan_watched_resources (especially if is necessary go through all the list or is sufficient only wd_data)
4. Uniforming the indentation of all code with 2 spaces.
5. ~~Test case for this scenario:~~
   ~~A directory is not in the root_path, but it can be reached by a symbolic link that is in the root_path.~~
   ~~Acceptance Test:~~
   ~~I try to delete this symbolic link and the pointed directory should be removed from list_wd~~
6. rename the function symlinks_contained_in ??? (probably yes)
7. probably during refactor phase we expect to find undercoverd functions

8. Extract utility functions in specific header (regex.h, command.h, etc) and find a easy way to glue them trough a extension system.
9. Extract command_line in a specific header (command_line.h)
10. create_wd_data(path, wd); -> fix the WD_DATA structure: path first and then wd

### TO REFACTOR

excluded (exclude_regex as an argument)
regex_catch (user_catch_regex as an argument)
get_regex_catch (p_match as an argument ???how???)

format_command (root_path and COMMAND_PATTERN.... as an argument)

### WHAT ELSE

* integration test / mock of all system call functions: monitor, event_handler, etc
* glob patterns

### FINAL ADJUSTMENTS

* the list is a QUEUE, it should expose enqueue and dequeue methods rather than push and pop.
* list_push(LIST, element) -> list_push(element, LIST)
* assure that all functions follow a form of foo(INPUT VARIABLE, OUTPUT VARIABLE)
* probably we can use only a list to store all symbolic links
