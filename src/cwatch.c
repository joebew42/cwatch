/* cwatch.c
 * Monitor file system activity using the inotify linux kernel library
 *
 * Copyright (C) 2012, Giuseppe Leone <joebew42@gmail.com>,
 *                     Vincenzo Di Cicco <enzodicicco@gmail.com>
 *
 * This file is part of cwatch
 *
 * cwatch is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * cwatch is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "cwatch.h"

/* Command line long options */
static struct option long_options[] =
{
    /* Options that set index */
    {"command",       required_argument, 0, 'c'}, /* exclude format */
    {"format",        required_argument, 0, 'F'}, /* exclude command */
    {"directory",     required_argument, 0, 'd'},
    {"events",        required_argument, 0, 'e'},
    {"exclude",       required_argument, 0, 'x'},
    {"regex-catch",   required_argument, 0, 'X'}, /* catch a regex */
    {"no-symlink",    no_argument,       0, 'n'},
    {"recursive",     no_argument,       0, 'r'},
    {"verbose",       no_argument,       0, 'v'},
    {"syslog",        no_argument,       0, 'l'},
    {"version",       no_argument,       0, 'V'},
    {"help",          no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

/*
 * The inotify events LUT
 * for a complete reference of all events, look here:
 * http://tomoyo.sourceforge.jp/cgi-bin/lxr/source/include/uapi/linux/inotify.h
 */
static struct event_t events_lut[] =
{
    {"access",        event_handler_undefined},  /* IN_ACCESS */
    {"modify",        event_handler_undefined},  /* IN_MODIFY */
    {"attrib",        event_handler_undefined},  /* IN_ATTRIB */
    {"close_write",   event_handler_undefined},  /* IN_CLOSE_WRITE */
    {"close_nowrite", event_handler_undefined},  /* IN_CLOSE_NOWRITE */
    {"open",          event_handler_undefined},  /* IN_OPEN */
    {"moved_from",    event_handler_moved_from}, /* IN_MOVED_FROM */
    {"moved_to",      event_handler_moved_to},   /* IN_MOVED_TO */
    {"create",        event_handler_create},     /* IN_CREATE */
    {"delete",        event_handler_delete},     /* IN_DELETE */
    {"delete_self",   event_handler_undefined},  /* IN_DELETE_SELF */
    {"move_self",     event_handler_undefined},  /* IN_MOVE_SELF */
    {NULL,            event_handler_undefined},
    {"umount",        event_handler_undefined},  /* IN_UMOUNT */
    {"q_overflow",    event_handler_undefined},  /* IN_Q_OVERFLOW */
    {"ignored",       event_handler_undefined},  /* IN_IGNORED */
    {NULL,            event_handler_undefined},
    {NULL,            event_handler_undefined},
    {NULL,            event_handler_undefined},
    {NULL,            event_handler_undefined},
    {NULL,            event_handler_undefined},
    {NULL,            event_handler_undefined},
    {NULL,            event_handler_undefined},
    {NULL,            event_handler_undefined},
    {"onlydir",       event_handler_undefined},  /* IN_ONLYDIR */
    {"dont_follow",   event_handler_undefined},  /* IN_DONT_FOLLOW */
    {"excl_unlink",   event_handler_undefined},  /* IN_EXCL_UNLINK */
    {NULL,            event_handler_undefined},
    {NULL,            event_handler_undefined},
    {"mask_add",      event_handler_undefined},  /* IN_MASK_ADD */
    {"isdir",         event_handler_undefined},  /* IN_ISDIR */
    {"oneshot",       event_handler_undefined},  /* IN_ONESHOT */

    /* threated as edge cases (see get_inotify_event implementation) */
    {"close",         event_handler_undefined},  /* 32. IN_CLOSE */
    {"move",          event_handler_undefined},  /* 33. IN_MOVE */
    {"all_events",    event_handler_undefined},  /* 34. IN_ALL_EVENTS */

};

void
print_version()
{
    printf("%s %s (%s)\n"
           "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n"
           "This is free software: you are free to change and redistribute it.\n"
           "There is NO WARRANTY, to the extent permitted by law.\n",
           PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_STAGE);
}

void
help(
    int error,
    char *message )
{
    printf("Usage: %s -c COMMAND -d DIRECTORY [-v] [-s] [-options]\n", PROGRAM_NAME);
    printf("   or: %s -F FORMAT  -d DIRECTORY [-v] [-s] [-options]\n", PROGRAM_NAME);
    printf("   or: %s [-V|--version]\n", PROGRAM_NAME);
    printf("   or: %s [-h|--help]\n\n", PROGRAM_NAME);
    printf("  -c --command COMMAND\n");
    printf("     Specify the command to execute each time an event occurs\n");
    printf("     Append & at the end of the command for a non-blocking execution\n");
    printf("     Use of specal special characters is allowed\n");
    printf("     (See the TABLE OF SPECIAL CHARACTERS for a complete reference)\n");
    printf("     NOTE: This option exclude the use of -F option\n\n");
    printf("  -F --format  FORMAT\n");
    printf("     Output in a user-specified format, using printf-like syntax.\n");
    printf("     This usage is useful if you want to emebed %s inside a bash script.\n", PROGRAM_NAME);
    printf("     Use of specal special characters is allowed\n");
    printf("     (See the TABLE OF SPECIAL CHARACTERS for a complete reference)\n");
    printf("     NOTE: This option exclude the use of -c and -v option\n\n");
    printf("  *TABLE OF SPECIAL CHARACTERS*\n\n");
    printf("       %sr : full path of the root DIRECTORY\n", "%");
    printf("       %sp : full path of the file/directory where the event occurs\n", "%");
    printf("       %sf : the name of the file/directory that triggered the event\n", "%");
    printf("       %se : the type of the occured event (the the list below)\n", "%");
    printf("       %sx : the first occurence that match the regex given by -X option\n", "%");
    printf("       %sn : the number of times the command is executed\n\n", "%");
    printf("  -d  --directory DIRECTORY\n");
    printf("      The directory to monitor\n\n");
    printf("  *LIST OF OTHER OPTIONS*\n\n");
    printf("  -e  --events [event,[event,[,..]]]\n");
    printf("      Specify which type of events to monitor. List of events:\n");
    printf("        access           : File was modified\n");
    printf("        modify           : File was modified\n");
    printf("        attrib           : File attributes changed\n");
    printf("        close_write      : File closed, after being opened in writeable mode\n");
    printf("        close_nowrite    : File closed, after being opened in read-only mode\n");
    printf("        close            : File closed, regardless of read/write mode\n");
    printf("        open             : File was opened\n");
    printf("        moved_from       : File was moved out of watched directory.\n");
    printf("        moved_to         : File was moved into watched directory.\n");
    printf("        move             : A file/dir within watched directory was moved\n");
    printf("        create           : A file was created within watched directory\n");
    printf("        delete           : A file was deleted within watched directory\n");
    printf("        delete_self      : The watched file was deleted\n");
    printf("        unmount          : File system on which watched file exists was unmounted\n");
    printf("        q_overflow       : Event queued overflowed\n");
    printf("        ignored          : File was ignored\n");
    printf("        isdir            : Event occurred against dir\n");
    printf("        oneshot          : Only send event once\n");
    printf("        all_events       : All events\n");
    printf("        default          : modify, create, delete, move.\n\n");
    printf("  -n  --no-symlink\n");
    printf("      Do not traverse symbolic link\n\n");
    printf("  -r  --recursive\n");
    printf("      Enable the recursively monitor of the directory\n\n");
    printf("  -x  --exclude <regex>\n");
    printf("      Do not process any events whose filename matches the specified POSIX REGEX\n");
    printf("      POSIX extended regular expression, case sensitive\n\n");
    printf("  -X  --regex-catch <regex>\n");
    printf("      Match the parenthetical <regex> against the filename whose triggered the event,\n");
    printf("      The first matched occurrence will be available as %sx special character\n", "%");
    printf("      Usage note: %s will be triggered only if a match occurs!\n", PROGRAM_NAME);
    printf("      POSIX extended regular expression, case sensitive\n\n");
    printf("  -v  --verbose\n");
    printf("      Verbose mode\n\n");
    printf("  -s  --syslog\n");
    printf("      Verbose mode through syslog\n\n");
    printf("  -h  --help\n");
    printf("      Print this help and exit\n\n");
    printf("  -V  --version\n");
    printf("      Print the version of the program and exit\n\n");

    printf("Reports bugs to: <https://github.com/joebew42/cwatch/issues/>\n");
    printf("%s home page: <https://github.com/joebew42/cwatch/>\n\n", PROGRAM_NAME);

    if (message != NULL)
      printf("%s", message);

    exit(error);
}

void
log_message(
    char *message,
    ... )
{
    /* Init variable argument list */
    va_list la;
    va_start(la, message);

    /* Convert message to bstring for better manage */
    bstring b_message = bfromcstr(message);
    int index = 0;

    bstring b_percent = bfromcstr("%");

    if (syslog_flag || (verbose_flag && (NULL == format))){
        /* Find each special char and replace with the correct arg */
        while ( (index = binstr(b_message, index, b_percent)) != BSTR_ERR){

            /* Find the type of value */
            char type = b_message->data[++index];

            if (type == 's') {

                /* Get the next char* arg and replace it */
                char *arg_char = va_arg (la, char *);
                bstring b_arg_char = bfromcstr(arg_char);

                breplace(b_message, index - 1, 2, b_arg_char, ' ');
                bdestroy(b_arg_char);
            } else if (type == 'd') {

                /* Get the next int arg, cast in cstr, and replace it*/
                int arg_int = va_arg (la, int);

                /* TODO: find the correct length of maxint */
                char *str_int = (char *) malloc (15);

                sprintf(str_int, "%d", arg_int);
                bstring b_str_int = bfromcstr(str_int);

                breplace(b_message, index - 1, 2, b_str_int, ' ');
                bdestroy(b_str_int);
            }
        }

        if (verbose_flag && (NULL == format)) {
            printf("%s\n", b_message->data);
        }

        if (syslog_flag) {
            openlog(PROGRAM_NAME, LOG_PID, LOG_LOCAL1);
            syslog(LOG_INFO, b_message->data);
            closelog();
        }
    }

    /* End the variable arguments list and free memory */
    va_end(la);
    bdestroy(b_percent);
    bdestroy(b_message);
}

char *
resolve_real_path(
    const char *path )
{
    char *resolved = (char*) malloc(MAXPATHLEN + 1);

    realpath(path, resolved);

    if (resolved == NULL)
        return NULL;

    strcat(resolved, "/");

    return resolved;
}

LIST_NODE *
get_node_from_path(
    const char *path )
{
    LIST_NODE *node = list_wd->first;
    while (node) {
        WD_DATA *wd_data = (WD_DATA *) node->data;
        if (strcmp(path, wd_data->path) == 0)
            return node;
        node = node->next;
    }

    return NULL;
}

LIST_NODE *
get_node_from_wd(
    const int wd )
{
    LIST_NODE *node = list_wd->first;
    while (node) {
        WD_DATA *wd_data = (WD_DATA *) node->data;
        if (wd == wd_data->wd)
            return node;
        node = node->next;
    }

    return NULL;
}

WD_DATA *
create_wd_data(
    char *real_path,
    int wd )
{
    WD_DATA *wd_data = (WD_DATA*) malloc(sizeof(WD_DATA));

    if (wd_data == NULL)
        return NULL;

    wd_data->wd = wd;
    wd_data->path = real_path;
    wd_data->links = list_init();

    return wd_data;
}

LIST_NODE *
get_link_node_from_path(
    const char *symlink )
{
    LIST_NODE *node = list_wd->first;
    WD_DATA *wd_data;
    LIST_NODE *link_node;
    LINK_DATA *link_data;

    while (node) {
        wd_data = (WD_DATA*) node->data;

        link_node = wd_data->links->first;
        while (link_node) {
            link_data = (LINK_DATA*) link_node->data;

            if (strcmp(link_data->path, symlink) == 0) {
                return link_node;
            }
            link_node = link_node->next;
        }
        node = node->next;
    }

    return NULL;
}

LINK_DATA *
get_link_data_from_wd_data(
    const char *symlink,
    const WD_DATA *wd_data )
{
    if (NULL == wd_data)
        return NULL;

    LIST_NODE *link_node;
    LINK_DATA *link_data;

    link_node = wd_data->links->first;
    while (link_node) {
        link_data = (LINK_DATA*) link_node->data;
        if (strcmp(link_data->path, symlink) == 0) {
            return link_data;
        }
        link_node = link_node->next;
    }

    return NULL;
}

LINK_DATA *
get_link_data_from_path(
    const char *symlink )
{
    LIST_NODE *node = list_wd->first;
    WD_DATA *wd_data;
    LINK_DATA *link_data;

    while (node) {
        wd_data = (WD_DATA*) node->data;

        link_data = get_link_data_from_wd_data(symlink, wd_data);

        if (link_data != NULL) {
            return link_data;
        }
        node = node->next;
    }

    return NULL;
}

LINK_DATA *
create_link_data(
    char *symlink,
    WD_DATA *wd_data )
{
    LINK_DATA *link_data = (LINK_DATA*) malloc(sizeof(LINK_DATA));

    if (link_data == NULL)
        return NULL;

    link_data->path = symlink;
    link_data->wd_data = wd_data;

    return link_data;
}

bool_t
is_child_of(
    const char *child,
    const char *parent )
{
    if (child == NULL
        || parent == NULL
        || strlen(parent) > strlen(child))
    {
        return FALSE;
    }

    return (strncmp(child, parent, strlen(parent)) == 0) ? TRUE : FALSE;
}

bool_t
is_listed_in(
    char* child_path,
    LIST *parents )
{
    if (parents == NULL || parents->first == NULL)
        return FALSE;

    LIST_NODE *node = parents->first;
    while (node) {
        char* parent_path = (char*) node->data;
        if (is_child_of(child_path, parent_path) == TRUE) {
            return TRUE; /* match! */
        }
        node = node->next;
    }
    return FALSE;
}

bool_t
excluded(
    char *str )
{
    if (NULL == exclude_regex)
        return FALSE;

    if (regexec(exclude_regex, str, 0, NULL, 0) == 0)
        return TRUE;

    return FALSE;
}

bool_t
regex_catch(
    char *str )
{
    if (NULL == user_catch_regex)
        return TRUE;

    if (regexec(user_catch_regex, str, 2, p_match, 0) == 0)
        return TRUE;

    return FALSE;
}

char *
get_regex_catch(
    char *str )
{
    if (p_match[1].rm_so == -1)
        return NULL;

    int length = p_match[1].rm_eo - p_match[1].rm_so;
    char *substr = (char *) malloc(length + 1);

    strncpy(substr, str + p_match[1].rm_so, length);
    substr[length] = '\0';

    return substr;
}

bstring
format_command(
    char *command_format,
    char *event_p_path,
    char *file_name,
    char *event_name )
{
    tmp_command = bfromcstr(command_format);

    bstring b_root_path = bfromcstr(root_path);
    bstring b_event_p_path = bfromcstr(event_p_path);
    bstring b_file_name = bfromcstr(file_name);
    bstring b_event_name = bfromcstr(event_name);

    char *reg_catch = get_regex_catch(file_name);
    bstring b_regcat = bfromcstr(reg_catch);
    free(reg_catch);

    bfindreplace(tmp_command, COMMAND_PATTERN_ROOT,  b_root_path, 0);
    bfindreplace(tmp_command, COMMAND_PATTERN_PATH,  b_event_p_path, 0);
    bfindreplace(tmp_command, COMMAND_PATTERN_FILE,  b_file_name, 0);
    bfindreplace(tmp_command, COMMAND_PATTERN_EVENT, b_event_name, 0);
    bfindreplace(tmp_command, COMMAND_PATTERN_REGEX, b_regcat , 0);

    sprintf(exec_cstr, "%d", exec_c);
    bstring b_exec_cstr = bfromcstr(exec_cstr);
    bfindreplace(tmp_command, COMMAND_PATTERN_COUNT, b_exec_cstr, 0);

    bdestroy(b_root_path);
    bdestroy(b_event_p_path);
    bdestroy(b_file_name);
    bdestroy(b_event_name);
    bdestroy(b_regcat);
    bdestroy(b_exec_cstr);

    return tmp_command;
}

int
parse_command_line(
    int argc,
    char *argv[] )
{
    if (argc == 1) {
        help(EINVAL, NULL);
    }

    /* Handle command line options */
    /* TODO: Refactor the parse command line */
    bstring b_optarg;

    int c;
    while ((c = getopt_long(argc, argv, "svnrVhe:c:F:d:x:X:", long_options, NULL)) != -1) {
        switch (c) {
        case 'c': /* --command */
            if (NULL != format)
                help(EINVAL, "The option -c --command exclude the use of -F --format option.\n");

            if (optarg == NULL
                || strcmp(optarg, "") == 0
                || (command = bfromcstr(optarg)) == NULL)
            {
                help(EINVAL, "The option -c --command requires a COMMAND.\n");
            }

            /* Remove both left/right whitespaces */
            btrimws(command);

            /* The command will be executed in inline mode */
            execute_command = execute_command_inline;

            break;

        case 'F': /* --format */
            if (NULL != command)
                help(EINVAL, "The option -F --format exclude the use of -c --command option.\n");

            format = bfromcstr(optarg);

            /* The command will be executed in embedded mode */
            execute_command = execute_command_embedded;

            break;

        case 'd': /* --directory */
            if (NULL == optarg || strcmp(optarg, "") == 0)
                help(EINVAL, "The option -d --directory requires a DIRECTORY.\n");

            /* Check if the path has the ending slash */
            if (optarg[strlen(optarg)-1] != '/') {
                root_path = (char *) malloc(strlen(optarg) + 2);
                strcpy(root_path, optarg);
                strcat(root_path, "/");
            } else {
                root_path = (char *) malloc(strlen(optarg) + 1);
                strcpy(root_path, optarg);
            }

            /* Check if it is a valid directory */
            DIR *dir = opendir(root_path);
            if (dir == NULL) {
                help(ENOENT, "The -d --directory requires a valid DIRECTORY.\n");
            }
            closedir(dir);

            /* Check if the path is absolute or not */
            /* TODO Dealloc after keyboard Ctrl+C interrupt */
            if (root_path[0] != '/') {
                char *real_path = resolve_real_path(root_path);
                free(root_path);
                root_path = real_path;
            }

            break;

        case 'e': /* --events */
            /* Set inotify events mask */
            b_optarg = bfromcstr(optarg);
            split_event = bsplit(b_optarg, ',');

            /* init the events name */
            B_ACCESS =  bfromcstr("access");
            B_MODIFY = bfromcstr("modify");
            B_ATTRIB = bfromcstr("attrib");
            B_CLOSE_WRITE = bfromcstr("close_write");
            B_CLOSE_NOWRITE = bfromcstr("close_nowrite");
            B_CLOSE = bfromcstr("close");
            B_OPEN = bfromcstr("open");
            B_MOVED_FROM = bfromcstr("moved_from");
            B_MOVED_TO = bfromcstr("moved_to");
            B_MOVE = bfromcstr("move");
            B_CREATE = bfromcstr("create");
            B_DELETE = bfromcstr("delete");
            B_DELETE_SELF = bfromcstr("delete_self");
            B_UNMOUNT = bfromcstr("unmount");
            B_Q_OVERFLOW = bfromcstr("q_overflow");
            B_IGNORED = bfromcstr("ignored");
            B_ISDIR = bfromcstr("isdir");
            B_ONESHOT = bfromcstr("oneshot");
            B_DEFAULT = bfromcstr("default");
            B_ALL_EVENTS = bfromcstr("all_events");

            if (split_event != NULL) {
                int i;
                for (i = 0; i < split_event->qty; ++i) {
                    if (bstrcmp(split_event->entry[i], B_ACCESS) == 0) {
                        event_mask |= IN_ACCESS;
                    } else if (bstrcmp(split_event->entry[i], B_MODIFY) == 0) {
                        event_mask |= IN_MODIFY;
                    } else if (bstrcmp(split_event->entry[i], B_ATTRIB) == 0) {
                        event_mask |= IN_ATTRIB;
                    } else if (bstrcmp(split_event->entry[i], B_CLOSE_WRITE) == 0) {
                        event_mask |= IN_CLOSE_WRITE;
                    } else if (bstrcmp(split_event->entry[i], B_CLOSE_NOWRITE) == 0) {
                        event_mask |= IN_CLOSE_NOWRITE;
                    } else if (bstrcmp(split_event->entry[i], B_CLOSE) == 0) {
                        event_mask |= IN_CLOSE;
                    } else if (bstrcmp(split_event->entry[i], B_OPEN) == 0) {
                        event_mask |= IN_OPEN;
                    } else if (bstrcmp(split_event->entry[i], B_MOVED_FROM) == 0) {
                        event_mask |= IN_MOVED_FROM;
                    } else if (bstrcmp(split_event->entry[i], B_MOVED_TO) == 0) {
                        event_mask |= IN_MOVED_TO;
                    } else if (bstrcmp(split_event->entry[i], B_MOVE) == 0) {
                        event_mask |= IN_MOVE;
                    } else if (bstrcmp(split_event->entry[i], B_CREATE) == 0) {
                        event_mask |= IN_CREATE;
                    } else if (bstrcmp(split_event->entry[i], B_DELETE) == 0) {
                        event_mask |= IN_DELETE;
                    } else if (bstrcmp(split_event->entry[i], B_DELETE_SELF) == 0) {
                        event_mask |= IN_DELETE_SELF;
                    } else if (bstrcmp(split_event->entry[i], B_UNMOUNT) == 0) {
                        event_mask |= IN_UNMOUNT;
                    } else if (bstrcmp(split_event->entry[i], B_Q_OVERFLOW) == 0) {
                        event_mask |= IN_Q_OVERFLOW;
                    } else if (bstrcmp(split_event->entry[i], B_IGNORED) == 0) {
                        event_mask |= IN_IGNORED;
                    } else if (bstrcmp(split_event->entry[i], B_ISDIR) == 0) {
                        event_mask |= IN_ISDIR;
                    } else if (bstrcmp(split_event->entry[i], B_ONESHOT) == 0) {
                        event_mask |= IN_ONESHOT;
                    } else if (bstrcmp(split_event->entry[i], B_ALL_EVENTS) == 0) {
                        event_mask |= IN_ALL_EVENTS;
                    } else if (bstrcmp(split_event->entry[i], B_DEFAULT) == 0) {
                        event_mask |= IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE;
                    } else {
                        help(EINVAL, "Unrecognized event or malformed list of events! Please see the help.\n");
                    }
                }
                bdestroy (b_optarg);
                bstrListDestroy(split_event);
            }
            break;

        case 'x': /* --exclude */
            if (optarg == NULL)
                help(EINVAL, "test X\n");

            exclude_regex = (regex_t *) malloc(sizeof(regex_t));

            if (regcomp(exclude_regex, optarg, REG_EXTENDED | REG_NOSUB) != 0) {
                free(exclude_regex);
                help(EINVAL, "The specified regular expression provided for the -x --exclude option, is not valid.\n");
            }

            break;

        case 'X': /* --regex-catch */
            if (optarg == NULL)
                help(EINVAL, NULL);

            user_catch_regex = (regex_t *) malloc(sizeof(regex_t));

            if (regcomp(user_catch_regex, optarg, REG_EXTENDED) != 0) {
                free(user_catch_regex);
                help(EINVAL, "The specified regular expression provided for the -x --exclude option, is not valid.\n");
            }

            break;

        case 'v': /* --verbose */
            verbose_flag = TRUE;
            break;

        case 'n': /* --no-symlink */
            nosymlink_flag = TRUE;
            break;

        case 'r': /* --recursive */
            recursive_flag = TRUE;
            break;

        case 's': /* --syslog */
            syslog_flag = TRUE;
            break;

        case 'V': /* --version */
            print_version();
            exit(EXIT_SUCCESS);

        case 'h': /* --help */

        default:
            help(EINVAL, NULL);
        }
    }

    if (root_path == NULL || command == format) {
        help(EINVAL, "The options -c --command and -d --directory are required.\n");
    }

    if (event_mask == 0) {
        event_mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE;
    }

    return 0;
}

int
watch_directory_tree(
    char *real_path,
    char *symlink,
    bool_t recursive_FAKE,
    int fd_FAKE,
    LIST *list_wd_FAKE )
{
    /* Add initial path to the watch list */
    LIST_NODE *node = add_to_watch_list(real_path, symlink, -1, -1, NULL);
    if (node == NULL)
        return -1;

    /* Temporary list to perform a BFS directory traversing */
    LIST *list = list_init();
    list_push(list, (void *) real_path);

    DIR *dir_stream;
    struct dirent *dir;

    while (list->first != NULL) {
        /* Directory to watch */
        char *p = (char*) list_pop(list);
        dir_stream = opendir(p);

        if (dir_stream == NULL) {
            printf("UNABLE TO OPEN DIRECTORY:\t\"%s\" -> %d\n", p, errno);
            exit(ENOENT);
        }

        /* Traverse directory */
        while ((dir = readdir(dir_stream))) {

            /* Discard all filename that matches regular expression (-x option) */
            if ((dir->d_type == DT_DIR) && excluded(dir->d_name)) {
                continue;
            }

            if ((dir->d_type == DT_DIR)
                && strcmp(dir->d_name, ".") != 0
                && strcmp(dir->d_name, "..") != 0)
            {
                /* Absolute path to watch */
                char *path_to_watch = (char *) malloc(strlen(p) + strlen(dir->d_name) + 2);
                strcpy(path_to_watch, p);
                strcat(path_to_watch, dir->d_name);
                strcat(path_to_watch, "/");

                /* Continue directory traversing */
                if (recursive_flag == TRUE) {
                    add_to_watch_list(path_to_watch, NULL, -1, -1, NULL);
                    list_push(list, (void*) path_to_watch);
                }
            } else if (dir->d_type == DT_LNK && nosymlink_flag == FALSE) {
                /* Resolve symbolic link */
                char *symlink = (char *) malloc(strlen(p) + strlen(dir->d_name) + 1);
                strcpy(symlink, p);
                strcat(symlink, dir->d_name);

                /* Check if the symbolic link is already watched */
                if (get_link_data_from_path(symlink) != NULL) {
                    continue;
                }

                char *real_path = resolve_real_path(symlink);

                DIR *is_a_dir;
                is_a_dir = opendir(real_path);
                if (real_path != NULL && is_a_dir != NULL) {
                    closedir(is_a_dir);

                    /* Continue directory traversing */
                    if (recursive_flag == TRUE) {
                        add_to_watch_list(real_path, symlink, -1, -1, NULL);
                        list_push(list, (void*) real_path);
                    }
                }
            }
        }
        closedir(dir_stream);
    }

    list_free(list);
    return 0;
}

LIST_NODE *
add_to_watch_list(
    char *real_path,
    char *symlink,
    int fd_FAKE,
    int wd_FAKE,
    LIST *list_wd_FAKE )
{
    LIST_NODE *node = get_node_from_path(real_path);

    /* if the resource is not watched yet, then add it into the watch_list */
    if (NULL == node) {
        int wd = watch_descriptor_from(fd, real_path, event_mask);

        /* INFO Check limit in: /proc/sys/fs/inotify/max_user_watches TODO: Extract from here */
        if (wd == -1) {
            printf("AN ERROR OCCURRED WHILE ADDING PATH %s:\n", real_path);
            printf("Please consider these possibilities:\n");
            printf(" - Max number of watched resources reached! See /proc/sys/fs/inotify/max_user_watches\n");
            printf(" - Resource is no more available!?\n");
            return NULL;
        }

        WD_DATA *wd_data = create_wd_data(real_path, wd);

        if (wd_data != NULL) {
            node = list_push(list_wd, (void*) wd_data);
            log_message("WATCHING: (fd:%d,wd:%d)\t\t\"%s\"", fd, wd_data->wd, real_path);
        }
    }

    /* append symbolic link to watched resource */
    if (node != NULL && symlink != NULL) {
        WD_DATA *wd_data = (WD_DATA*) node->data;
        LINK_DATA *link_data = create_link_data(symlink, wd_data);

        if (link_data != NULL) {
            list_push(wd_data->links, (void *) link_data);

            log_message("ADDED SYMBOLIC LINK:\t\t\"%s\" -> \"%s\"", symlink, real_path);
        }
    }

    return node;
}

void
unwatch(
    char *path,
    bool_t is_link )
{
    /* TODO REFACTOR HERE */

    /* Remove the resource from watched resources */
    if (is_link == FALSE) {
        /* Retrieve the watch descriptor from path */
        LIST_NODE *node = get_node_from_path(path);
        if (node != NULL) {
            WD_DATA *wd_data = (WD_DATA *) node->data;

            log_message("UNWATCHING: (fd:%d,wd:%d)\t\t\"%s\"", fd, wd_data->wd, path);

            inotify_rm_watch(fd, wd_data->wd);

            if (wd_data->links->first != NULL)
                list_free(wd_data->links);

            list_remove(list_wd, node);
        }
    } else {
        /* Search for all other symbolic links to unwatch */
        LIST *list = list_init();
        list_push(list, (void *) path);

        while (list->first != NULL) {
            char *symlink = (char*) list_pop(list);

            LIST_NODE *link_node = get_link_node_from_path(symlink);
            if (link_node == NULL)
                continue;

            LINK_DATA *link_data = (LINK_DATA*) link_node->data;
            char *resolved_path = (char*) link_data->wd_data->path;

            /* TODO: Refactor this section */
            LIST_NODE *node = list_wd->first;
            LIST_NODE *sub_node = NULL;

            WD_DATA *wd_data = NULL;
            LINK_DATA *link_data2 = NULL;
            while (node) {
                wd_data = (WD_DATA*) node->data;
                sub_node = (LIST_NODE*) wd_data->links->first;
                while (sub_node) {
                    link_data2 = (LINK_DATA*) sub_node->data;
                    if (is_child_of(link_data2->path, resolved_path) == TRUE) {
                        /* printf("-> SYMLINK TO REMOVE: %s\n", link_data2->path); */
                        list_push(list, (void*) link_data2->path);
                    }
                    sub_node = sub_node->next;
                }
                node = node->next;
            }

            if (link_node != NULL)
                unwatch_symbolic_link(link_node);
        }

        list_free(list);
    }
}

LIST *
list_of_referenced_path(
    const char *path )
{
    LIST *tmp_references_list = list_init();
    LIST_NODE *node;
    WD_DATA *wd_data;

    node = list_wd->first;
    while (node) {
        wd_data = (WD_DATA*) node->data;

        if (wd_data->links->first != NULL
            && (strncmp(path, wd_data->path, strlen(path)) == 0
                || strncmp(path, wd_data->path, strlen(wd_data->path)) == 0)
            && is_listed_in(wd_data->path, tmp_references_list) == FALSE)
        {
            list_push(tmp_references_list, (void*) wd_data->path);
        }
        node = node->next;
    }

    return tmp_references_list;
}

void
remove_orphan_watched_resources(
    const char *path,
    LIST *references_list )
{
    LIST_NODE *node;
    WD_DATA *wd_data;

    node = list_wd->first;
    while (node) {
        wd_data = (WD_DATA*) node->data;

        if (strcmp(root_path, wd_data->path) != 0
            && wd_data->links->first == NULL
            && is_child_of(wd_data->path, path) == TRUE
            && is_listed_in(wd_data->path, references_list) == FALSE)
        {
            log_message("UNWATCHING: (fd:%d,wd:%d)\t\t\"%s\"", fd, wd_data->wd, wd_data->path);

            inotify_rm_watch(fd, wd_data->wd);
            list_remove(list_wd, node);
        }
        node = node->next;
    }
}

void
unwatch_symbolic_link(
    LIST_NODE *link_node )
{
    LINK_DATA *link_data = (LINK_DATA*) link_node->data;
    char *link_path = (char*) link_data->path;
    WD_DATA *wd_data = (WD_DATA*) link_data->wd_data;

    log_message("UNWATCHING SYMBOLIC LINK: \t\"%s\" -> \"%s\"", link_path, wd_data->path);

    list_remove(wd_data->links, link_node);

    /*
     * if there is no other symbolic links that point to the
     * watched resource and the watched resource is not a child
     * of the the root path then unwatch it and relative orphan
     * directories (no longer reached by any symbolic links within root_path)
     */
    if (wd_data->links->first == NULL
        && is_child_of(wd_data->path, root_path) == FALSE)
    {
        LIST *references_list = list_of_referenced_path(wd_data->path);
        if (NULL != references_list) {
            remove_orphan_watched_resources(wd_data->path, references_list);
        }
        list_free(references_list);
    }
}

int
monitor()
{
    /* Initialize patterns that will be replaced */
    COMMAND_PATTERN_ROOT = bfromcstr("%r");
    COMMAND_PATTERN_PATH = bfromcstr("%p");
    COMMAND_PATTERN_FILE = bfromcstr("%f");
    COMMAND_PATTERN_EVENT = bfromcstr("%e");
    COMMAND_PATTERN_REGEX = bfromcstr("%x");
    COMMAND_PATTERN_COUNT = bfromcstr("%n");

    /* Initialize the exec count */
    exec_c = 0;

    /* Buffer for File Descriptor */
    char buffer[EVENT_BUF_LEN];

    /* inotify_event */
    struct inotify_event *event = NULL;
    struct event_t *triggered_event = NULL;

    /* The real path of touched directory or file */
    char *path = NULL;
    size_t len;
    int i;

    /* Temporary node information */
    LIST_NODE *node = NULL;
    WD_DATA *wd_data = NULL;

    /* Wait for events */
    while ((len = read(fd, buffer, EVENT_BUF_LEN))) {
        if (len < 0) {
            printf("ERROR: UNABLE TO READ INOTIFY QUEUE EVENTS!!!\n");
            exit(EIO);
        }

        /* index of the event into file descriptor */
        i = 0;
        while (i < len) {
            /* inotify_event */
            event = (struct inotify_event*) &buffer[i];

            /* Discard all filename that matches regular expression (-x option) */
            if (excluded(event->name)) {
                /* Next event */
                i += EVENT_SIZE + event->len;
                continue;
            }

            /* Build the full path of the directory or symbolic link */
            node = get_node_from_wd(event->wd);
            if (node != NULL) {
                wd_data = (WD_DATA *) node->data;
                path = (char *)malloc(strlen(wd_data->path) + strlen(event->name) + 2);
                strcpy(path, wd_data->path);
                strcat(path, event->name);
                if (event->mask & IN_ISDIR)
                    strcat(path, "/");
            } else {
                /* Next event */
                i += EVENT_SIZE + event->len;
                continue;
            }

            /* Call the specific event handler */
            if (event->mask & event_mask
                && (triggered_event = get_inotify_event(event->mask & event_mask)) != NULL
                && triggered_event->name != NULL
                && regex_catch(event->name)
                && triggered_event->handler(event, path) == 0)
            {
                ++exec_c;

                if (execute_command(triggered_event->name, event->name, wd_data->path) == -1) {
                    printf("ERROR OCCURED: Unable to execute the specified command!\n");
                    exit(EXIT_FAILURE);
                }
            } else {
                free(path);
            }

            /* Next event */
            i += EVENT_SIZE + event->len;
        }
    }

    return 0;
}

int
execute_command_inline(
    char *event_name,
    char *file_name,
    char *event_p_path )
{
    log_message("EVENT TRIGGERED [%s] IN %s%s\nNUMBER OF EXECUTION [%d]\nPROCESS EXECUTED [command: %s]",
                event_name, event_p_path, file_name, exec_c, command->data);

    /* Command token replacement */
    tmp_command = format_command((char *) command->data, event_p_path, file_name, event_name);

    int exit = 0;
    exit = system((const char*) tmp_command->data);

    if (exit == -1 || exit == 127) {
        log_message("Unable to execute the specified command!");
    }

    bdestroy(tmp_command);

    return 0;
}

int
execute_command_embedded(
    char *event_name,
    char *file_name,
    char *event_p_path )
{
    log_message("EVENT TRIGGERED [%s] IN %s%s", event_name, event_p_path, file_name);

    /* Output the formatted string */
    tmp_command = format_command((char *) format->data, event_p_path, file_name, event_name);

    fprintf(stdout, "%s\n", (char *) tmp_command->data);
    fflush(stdout);

    bdestroy(tmp_command);
    return 0;
}

struct event_t *
get_inotify_event(
    const uint32_t event_mask )
{
    switch (event_mask) {
    case IN_CLOSE:       return &events_lut[32];
    case IN_MOVE:        return &events_lut[33];
    case IN_ALL_EVENTS:  return &events_lut[34];
    default:             return &events_lut[ffs(event_mask)-1];
    }
}

/*
 * EVENT HANDLER IMPLEMENTATION
 */

int
event_handler_undefined(
    struct inotify_event *event,
    char *path )
{
    return 0;
}

int
event_handler_create(
    struct inotify_event *event,
    char *path )
{
    /* Return 0 if recurively monitoring is disabled */
    if (recursive_flag == FALSE)
        return 0;

    /* Check for a directory */
    if (event->mask & IN_ISDIR) {
        watch_directory_tree(path, NULL, FALSE, -1, NULL);
    } else if (nosymlink_flag == FALSE) {
        /* Check for a symbolic link */
        DIR *dir_stream = opendir(path);
        if (dir_stream != NULL) {
            closedir(dir_stream);

            char *real_path = resolve_real_path(path);
            watch_directory_tree(real_path, path, FALSE, -1, NULL);
        }
    }

    return 0;
}

int
event_handler_delete(
    struct inotify_event *event,
    char *path )
{
    /* Check if it is a folder. If yes unwatch it */
    if (event->mask & IN_ISDIR) {
        unwatch(path, FALSE);
    } else if (nosymlink_flag == FALSE) {
        /*
         * XXX Since it is not possible to know if the
         *     inotify event belongs to a file or a symbolic link,
         *     the unwatch function will be called for each file.
         *     That is because the file is deleted from filesystem,
         *     so there is no way to stat it.
         *     This is a big computational issue to be treated.
         */
        unwatch(path, TRUE);
    }

    return 0;
}

int
event_handler_moved_from(
    struct inotify_event *event,
    char *path )
{
    return event_handler_delete(event, path);
}

int
event_handler_moved_to(
    struct inotify_event *event,
    char *path )
{
    if (strncmp(path, root_path, strlen(root_path)) == 0) /* replace with is_child_of */
        return event_handler_create(event, path);

    return 0; /* do nothing */
}

void
signal_callback_handler(
    int signum )
{
    printf("Cleaning...\n");

    bdestroy (command);
    bdestroy (format);
    bdestroy (tmp_command);
    bdestroy (B_ACCESS);
    bdestroy (B_MODIFY);
    bdestroy (B_ATTRIB);
    bdestroy (B_CLOSE_WRITE);
    bdestroy (B_CLOSE_NOWRITE);
    bdestroy (B_CLOSE);
    bdestroy (B_OPEN);
    bdestroy (B_MOVED_FROM);
    bdestroy (B_MOVED_TO);
    bdestroy (B_MOVE);
    bdestroy (B_CREATE);
    bdestroy (B_DELETE);
    bdestroy (B_DELETE_SELF);
    bdestroy (B_UNMOUNT);
    bdestroy (B_Q_OVERFLOW);
    bdestroy (B_IGNORED);
    bdestroy (B_ISDIR);
    bdestroy (B_ONESHOT);
    bdestroy (B_DEFAULT);
    bdestroy (B_ALL_EVENTS);

    bdestroy (COMMAND_PATTERN_ROOT);
    bdestroy (COMMAND_PATTERN_PATH );
    bdestroy (COMMAND_PATTERN_FILE);
    bdestroy (COMMAND_PATTERN_EVENT);
    bdestroy (COMMAND_PATTERN_REGEX);
    bdestroy (COMMAND_PATTERN_COUNT);

    list_free(list_wd);
    exit(signum);
}
