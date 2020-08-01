### TODO

- As part of the work we are doing in order to rename List into a Queue, we still have to rename all the occurrences that contains the `node`, with `element`. (e.g. "get_node_from_path" -> "get_element_from_path")
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
