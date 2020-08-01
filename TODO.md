### TODO

- the list is a queue, it should expose enqueue and dequeue methods rather than push and pop.
  - rename `list` in `queue`
- create_wd_data(path, wd);
  - Maybe the order of the arguments does not follow our convention? `wd` should always be the first argument?
  - fix the `WD_DATA` structure: path first and then wd
- refactor unwatch_symlink
- check remove_orphan_watched_resources (especially if is necessary go through all the list or is sufficient only wd_data)
- rename the function symlinks_contained_in ??? (probably yes)
- Extract functions in specific header (regex.h, command.h, etc)
- Extract command_line in a specific header (command_line.h)
  - refactor `excluded` (`exclude_regex` as an argument)
  - refactor `regex_catch` (`user_catch_regex` as an argument)
  - refactor `get_regex_catch` (`p_match` as an argument ???how???)
  - refactor `format_command` (`root_path` and COMMAND_PATTERN.... as an argument)
- Is it possible to use only one list to store all symbolic links?
