/* cwatch.c
 * Monitor file system activity using the inotify linux kernel library
 *
 * Copyright (C) 2012, Joe Bew <joebew42@gmail.com>,
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

/* Initialize patterns that will be replaced */
const_bstring COMMAND_PATTERN_ROOT = (const_bstring) "%r";
const_bstring COMMAND_PATTERN_PATH = (const_bstring) "%p";
const_bstring COMMAND_PATTERN_FILE = (const_bstring) "%f";
const_bstring COMMAND_PATTERN_EVENT = (const_bstring) "%e";
const_bstring COMMAND_PATTERN_REGEX = (const_bstring) "%x";
const_bstring COMMAND_PATTERN_COUNT = (const_bstring) "%n";

char *root_path;
bstring command;
bstring format;
bstring tmp_command;
struct bstrList *split_event;
uint32_t event_mask;
regex_t *exclude_regex;
regex_t *user_catch_regex;
regmatch_t p_match[2];

int exec_c;
char exec_cstr[10];

bool_t nosymlink_flag;
bool_t recursive_flag;
bool_t verbose_flag;
bool_t syslog_flag;

int (*execute_command)(char *, char *, char *);
int (*watch_descriptor_from)(int, const char *, uint32_t);
int (*remove_watch_descriptor)(int, int);

/* Command line long options */
static struct option long_options[] =
    {
        /* Options that set index */
        {"command", required_argument, 0, 'c'}, /* exclude format */
        {"format", required_argument, 0, 'F'},  /* exclude command */
        {"directory", required_argument, 0, 'd'},
        {"events", required_argument, 0, 'e'},
        {"exclude", required_argument, 0, 'x'},
        {"regex-catch", required_argument, 0, 'X'}, /* catch a regex */
        {"no-symlink", no_argument, 0, 'n'},
        {"recursive", no_argument, 0, 'r'},
        {"verbose", no_argument, 0, 'v'},
        {"syslog", no_argument, 0, 'l'},
        {"version", no_argument, 0, 'V'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

/*
 * The inotify events LUT
 * for a complete reference of all events, look here:
 * http://tomoyo.sourceforge.jp/cgi-bin/lxr/source/include/uapi/linux/inotify.h
 */
static struct event_t events_lut[] =
    {
        {"access", IN_ACCESS, event_handler_undefined},
        {"modify", IN_MODIFY, event_handler_undefined},
        {"attrib", IN_ATTRIB, event_handler_undefined},
        {"close_write", IN_CLOSE_WRITE, event_handler_undefined},
        {"close_nowrite", IN_CLOSE_NOWRITE, event_handler_undefined},
        {"open", IN_OPEN, event_handler_undefined},
        {"moved_from", IN_MOVED_FROM, event_handler_moved_from},
        {"moved_to", IN_MOVED_TO, event_handler_moved_to},
        {"create", IN_CREATE, event_handler_create},
        {"delete", IN_DELETE, event_handler_delete},
        {"delete_self", IN_DELETE_SELF, event_handler_undefined},
        {"move_self", IN_MOVE_SELF, event_handler_undefined},
        {NULL, 0, event_handler_undefined},
        {"unmount", IN_UNMOUNT, event_handler_undefined},
        {"q_overflow", IN_Q_OVERFLOW, event_handler_undefined},
        {"ignored", IN_IGNORED, event_handler_undefined},
        {NULL, 0, event_handler_undefined},
        {NULL, 0, event_handler_undefined},
        {NULL, 0, event_handler_undefined},
        {NULL, 0, event_handler_undefined},
        {NULL, 0, event_handler_undefined},
        {NULL, 0, event_handler_undefined},
        {NULL, 0, event_handler_undefined},
        {NULL, 0, event_handler_undefined},
        {"onlydir", IN_ONLYDIR, event_handler_undefined},
        {"dont_follow", IN_DONT_FOLLOW, event_handler_undefined},
        {"excl_unlink", IN_EXCL_UNLINK, event_handler_undefined},
        {NULL, 0, event_handler_undefined},
        {NULL, 0, event_handler_undefined},
        {"mask_add", IN_MASK_ADD, event_handler_undefined},
        {"isdir", IN_ISDIR, event_handler_undefined},
        {"oneshot", IN_ONESHOT, event_handler_undefined},

        /* handled as edge cases (see get_inotify_event implementation) */
        {"close", IN_CLOSE, event_handler_undefined},
        {"move", IN_MOVE, event_handler_undefined},
        {"all_events", IN_ALL_EVENTS, event_handler_undefined},
        {"default", IN_MODIFY | IN_DELETE | IN_CREATE | IN_MOVE, event_handler_undefined},
};

void print_version()
{
    printf("%s %s (%s)\n"
           "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n"
           "This is free software: you are free to change and redistribute it.\n"
           "There is NO WARRANTY, to the extent permitted by law.\n",
           PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_STAGE);
}

void help(int error, char *message)
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
    printf("        access           : File was accessed\n");
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

void log_message(char *message, ...)
{
    /* Init variable argument list */
    va_list la;
    va_start(la, message);

    /* Convert message to bstring for better manage */
    bstring b_message = bfromcstr(message);
    int index = 0;

    bstring b_percent = bfromcstr("%");

    if (syslog_flag || (verbose_flag && (NULL == format)))
    {
        /* Find each special char and replace with the correct arg */
        while ((index = binstr(b_message, index, b_percent)) != BSTR_ERR)
        {

            /* Find the type of value */
            char type = b_message->data[++index];

            if (type == 's')
            {

                /* Get the next char* arg and replace it */
                char *arg_char = va_arg(la, char *);
                bstring b_arg_char = bfromcstr(arg_char);

                breplace(b_message, index - 1, 2, b_arg_char, ' ');
                bdestroy(b_arg_char);
            }
            else if (type == 'd')
            {

                /* Get the next int arg, cast in cstr, and replace it*/
                int arg_int = va_arg(la, int);

                /* TODO: find the correct length of maxint */
                char *str_int = (char *)malloc(15);

                sprintf(str_int, "%d", arg_int);
                bstring b_str_int = bfromcstr(str_int);

                breplace(b_message, index - 1, 2, b_str_int, ' ');
                bdestroy(b_str_int);
            }
        }

        if (verbose_flag && (NULL == format))
        {
            printf("%s\n", b_message->data);
            fflush(stdout);
        }

        if (syslog_flag)
        {
            openlog(PROGRAM_NAME, LOG_PID, LOG_LOCAL1);

            char *cstr_message = bstr2cstr(b_message, '\0');
            syslog(LOG_INFO, cstr_message);
            bcstrfree(cstr_message);

            closelog();
        }
    }

    /* End the variable arguments list and free memory */
    va_end(la);
    bdestroy(b_percent);
    bdestroy(b_message);
}

char *
resolve_real_path(const char *path)
{
    char *resolved = (char *)malloc(MAXPATHLEN + 1);

    realpath(path, resolved);

    if (resolved == NULL)
        return NULL;

    strcat(resolved, "/");

    return resolved;
}

inline bool_t
is_dir(const char *path)
{
    struct stat st_path;

    if (!stat(path, &st_path) && S_ISDIR(st_path.st_mode))
        return TRUE;

    return FALSE;
}

char *
append_dir(const char *path, const char *dir)
{
    char *clean_path, *ret;
    size_t lret, lpath, ldir;

    lpath = strlen(path);
    ldir = strlen(dir);

    if (!lpath)
    {
        if (!ldir)
            return strdup("");
        else
        {
            /* handle such as append_dir(dir, path) */
            path = dir;
            lpath = ldir;
            dir = "";
            ldir = 0;
        }
    }

    for (; lpath && path[lpath - 1] == '/'; --lpath)
        ;
    for (; ldir && dir[ldir - 1] == '/'; --ldir)
        ;
    for (; ldir && dir[0] == '/'; --ldir, dir++)
        ;

    /* if dir is not empty count the final slash */
    lret = lpath + 1 + ldir + !!ldir;
    ret = (char *)malloc(lret + 1);
    clean_path = strndup(path, lpath);

    if (!ret || !clean_path)
        return NULL;

    snprintf(ret, lret + 1, "%s/%s/", clean_path, dir);

    free(clean_path);
    return ret;
}

char *
append_file(const char *path, const char *file)
{
    char *ret;
    int lret;

    for (; *file == '/'; file++)
        ;

    /* handle with append_dir and remove
     * the trailing slash
     */
    ret = append_dir(path, file);
    if (!ret)
        return NULL;

    if (*file)
    {
        lret = strlen(ret);
        ret[lret - 1] = '\0';
    }

    return ret;
}

QueueElement *
get_node_from_path(const char *path, Queue *queue_wd)
{
    QueueElement *element = queue_wd->first;
    while (element)
    {
        WD_DATA *wd_data = (WD_DATA *)element->data;
        if (strcmp(path, wd_data->path) == 0)
            return element;
        element = element->next;
    }

    return NULL;
}

QueueElement *
get_node_from_wd(const int wd, Queue *queue_wd)
{
    QueueElement *element = queue_wd->first;
    while (element)
    {
        WD_DATA *wd_data = (WD_DATA *)element->data;
        if (wd == wd_data->wd)
            return element;
        element = element->next;
    }

    return NULL;
}

WD_DATA *
create_wd_data(char *real_path, int wd)
{
    WD_DATA *wd_data = (WD_DATA *)malloc(sizeof(WD_DATA));

    if (wd_data == NULL)
        return NULL;

    wd_data->wd = wd;
    wd_data->path = real_path;
    wd_data->links = queue_init();

    return wd_data;
}

QueueElement *
get_link_node_from_path(const char *symlink, Queue *queue_wd)
{
    QueueElement *element = queue_wd->first;
    WD_DATA *wd_data;
    QueueElement *link_node;
    LINK_DATA *link_data;

    while (element)
    {
        wd_data = (WD_DATA *)element->data;

        link_node = wd_data->links->first;
        while (link_node)
        {
            link_data = (LINK_DATA *)link_node->data;

            if (strcmp(link_data->path, symlink) == 0)
            {
                return link_node;
            }
            link_node = link_node->next;
        }
        element = element->next;
    }

    return NULL;
}

bool_t
is_symlink(char *path, Queue *queue_wd)
{
    return (NULL != get_link_node_from_path(path, queue_wd));
}

LINK_DATA *
get_link_data_from_wd_data(const char *symlink, const WD_DATA *wd_data)
{
    if (NULL == wd_data)
        return NULL;

    QueueElement *link_node;
    LINK_DATA *link_data;

    link_node = wd_data->links->first;
    while (link_node)
    {
        link_data = (LINK_DATA *)link_node->data;
        if (strcmp(link_data->path, symlink) == 0)
        {
            return link_data;
        }
        link_node = link_node->next;
    }

    return NULL;
}

LINK_DATA *
get_link_data_from_path(const char *symlink, Queue *queue_wd)
{
    QueueElement *element = queue_wd->first;
    WD_DATA *wd_data;
    LINK_DATA *link_data;

    while (element)
    {
        wd_data = (WD_DATA *)element->data;

        link_data = get_link_data_from_wd_data(symlink, wd_data);

        if (link_data != NULL)
        {
            return link_data;
        }
        element = element->next;
    }

    return NULL;
}

LINK_DATA *
create_link_data(char *symlink, WD_DATA *wd_data)
{
    LINK_DATA *link_data = (LINK_DATA *)malloc(sizeof(LINK_DATA));

    if (link_data == NULL)
        return NULL;

    link_data->path = symlink;
    link_data->wd_data = wd_data;

    return link_data;
}

bool_t
is_listed_as_child(char *string, Queue *queue)
{
    if (queue == NULL || queue->first == NULL)
        return FALSE;

    QueueElement *element = queue->first;
    while (element)
    {
        if (is_child_of((char *)element->data, string))
        {
            return TRUE;
        }
        element = element->next;
    }
    return FALSE;
}

bool_t
is_child_of(const char *parent, const char *child)
{
    if (child == NULL || parent == NULL || strlen(parent) > strlen(child))
    {
        return FALSE;
    }

    return (strncmp(child, parent, strlen(parent)) == 0) ? TRUE : FALSE;
}

bool_t
excluded(char *str)
{
    if (NULL == exclude_regex)
        return FALSE;

    if (regexec(exclude_regex, str, 0, NULL, 0) == 0)
        return TRUE;

    return FALSE;
}

bool_t
regex_catch(char *str)
{
    if (NULL == user_catch_regex)
        return TRUE;

    if (regexec(user_catch_regex, str, 2, p_match, 0) == 0)
        return TRUE;

    return FALSE;
}

char *
get_regex_catch(char *str)
{
    if (p_match[1].rm_so == -1)
        return NULL;

    int length = p_match[1].rm_eo - p_match[1].rm_so;
    char *substr = (char *)malloc(length + 1);

    strncpy(substr, str + p_match[1].rm_so, length);
    substr[length] = '\0';

    return substr;
}

bstring
format_command(char *command_format, char *event_p_path, char *file_name, char *event_name)
{
    tmp_command = bfromcstr(command_format);

    bstring b_root_path = bfromcstr(root_path);
    bstring b_event_p_path = bfromcstr(event_p_path);
    bstring b_file_name = bfromcstr(file_name);
    bstring b_event_name = bfromcstr(event_name);

    char *reg_catch = get_regex_catch(file_name);
    bstring b_regcat = bfromcstr(reg_catch);
    free(reg_catch);

    bfindreplace(tmp_command, COMMAND_PATTERN_ROOT, b_root_path, 0);
    bfindreplace(tmp_command, COMMAND_PATTERN_PATH, b_event_p_path, 0);
    bfindreplace(tmp_command, COMMAND_PATTERN_FILE, b_file_name, 0);
    bfindreplace(tmp_command, COMMAND_PATTERN_EVENT, b_event_name, 0);
    bfindreplace(tmp_command, COMMAND_PATTERN_REGEX, b_regcat, 0);

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

int parse_command_line(int argc, char *argv[])
{
    if (argc == 1)
    {
        help(EINVAL, NULL);
    }

    /* Handle command line options */
    /* TODO: Refactor the parse command line */
    bstring b_optarg;

    int c;
    while ((c = getopt_long(argc, argv, "svnrVhe:c:F:d:x:X:", long_options, NULL)) != -1)
    {
        switch (c)
        {
        case 'c': /* --command */
            if (NULL != format)
                help(EINVAL, "The option -c --command exclude the use of -F --format option.\n");

            if (optarg == NULL || strcmp(optarg, "") == 0 || (command = bfromcstr(optarg)) == NULL)
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

            /* Ensure that the path has the trailing slash */
            root_path = append_dir(optarg, "/");

            /* Check if it is a valid directory */
            if (!is_dir(root_path))
                help(ENOENT, "The -d --directory requires a valid DIRECTORY.\n");

            /* Check if the path is absolute or not */
            /* TODO Dealloc after keyboard Ctrl+C interrupt */
            if (root_path[0] != '/')
            {
                char *real_path = resolve_real_path(root_path);
                free(root_path);
                root_path = real_path;
            }

            break;

        case 'e': /* --events */
            /* Set inotify events mask */
            b_optarg = bfromcstr(optarg);
            split_event = bsplit(b_optarg, ',');

            if (split_event != NULL)
            {
                int len_events_lut = ARRAY_SIZE(events_lut);

                int i;
                for (i = 0; i < split_event->qty; ++i)
                {
                    int j;
                    for (j = 0; j < len_events_lut; ++j)
                    {
                        bstring event_name = bfromcstr(events_lut[j].name);
                        if (bstrcmp(split_event->entry[i], event_name) == 0)
                        {
                            event_mask |= events_lut[j].mask;
                            break;
                        }
                        bdestroy(event_name);
                    }
                    if (j == len_events_lut)
                        help(EINVAL, "Unrecognized event or malformed list of events! Please see the help.\n");
                }
                bdestroy(b_optarg);
                bstrListDestroy(split_event);
            }
            break;

        case 'x': /* --exclude */
            if (optarg == NULL)
                help(EINVAL, "test X\n");

            exclude_regex = (regex_t *)malloc(sizeof(regex_t));

            if (regcomp(exclude_regex, optarg, REG_EXTENDED | REG_NOSUB) != 0)
            {
                free(exclude_regex);
                help(EINVAL, "The specified regular expression provided for the -x --exclude option, is not valid.\n");
            }

            break;

        case 'X': /* --regex-catch */
            if (optarg == NULL)
                help(EINVAL, NULL);

            user_catch_regex = (regex_t *)malloc(sizeof(regex_t));

            if (regcomp(user_catch_regex, optarg, REG_EXTENDED) != 0)
            {
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

    if (root_path == NULL || command == format)
    {
        help(EINVAL, "The options -c --command and -d --directory are required.\n");
    }

    if (event_mask == 0)
    {
        event_mask = IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVE;
    }

    return 0;
}

int watch_directory_tree(char *real_path, char *symlink, bool_t recursive, int fd, Queue *queue_wd)
{
    /* Add initial path to the watch list */
    QueueElement *element = add_to_watch_list(real_path, symlink, fd, queue_wd);
    if (element == NULL)
        return -1;

    if (recursive == FALSE)
        return 0;

    /* Temporary queue to perform a BFS directory traversing */
    Queue *queue = queue_init();
    queue_enqueue(queue, (void *)real_path);

    DIR *dir_stream;
    struct dirent *dir;

    while (queue->first != NULL)
    {
        char *directory_to_watch = (char *)queue_dequeue(queue);
        dir_stream = opendir(directory_to_watch);

        if (dir_stream == NULL)
        {
            printf("UNABLE TO OPEN DIRECTORY:\t\"%s\" -> %d\n", directory_to_watch, errno);
            exit(ENOENT);
        }

        while ((dir = readdir(dir_stream)))
        {
            /* Discard all file names that matches regular expression (-x option) */
            if ((dir->d_type == DT_DIR) && excluded(dir->d_name))
            {
                continue;
            }

            if ((dir->d_type == DT_DIR) && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
            {
                /* Absolute path to watch */
                char *path_to_watch = append_dir(directory_to_watch, dir->d_name);

                /* Continue directory traversing */
                add_to_watch_list(path_to_watch, NULL, fd, queue_wd);
                queue_enqueue(queue, (void *)path_to_watch);
            }
            else if (dir->d_type == DT_LNK && nosymlink_flag == FALSE)
            {
                /* Resolve symbolic link */
                char *symlink = append_file(directory_to_watch, dir->d_name);
                char *real_path = resolve_real_path(symlink);

                if (is_dir(real_path))
                {
                    /* Check if the symbolic link is already watched */
                    if (get_link_data_from_path(symlink, queue_wd) != NULL)
                    {
                        continue;
                    }

                    /* Continue directory traversing */
                    add_to_watch_list(real_path, symlink, fd, queue_wd);
                    queue_enqueue(queue, (void *)real_path);
                }
            }
        }
        closedir(dir_stream);
    }

    queue_free(queue);
    return 0;
}

QueueElement *
add_to_watch_list(char *real_path, char *symlink, int fd, Queue *queue_wd)
{
    QueueElement *element = get_node_from_path(real_path, queue_wd);

    /* if the resource is not watched yet, then add it into the watch_list */
    if (NULL == element)
    {
        int wd = watch_descriptor_from(fd, real_path, event_mask);

        /* INFO Check limit in: /proc/sys/fs/inotify/max_user_watches TODO: Extract from here */
        if (wd == -1)
        {
            printf("AN ERROR OCCURRED WHILE ADDING PATH %s:\n", real_path);
            printf("Please consider these possibilities:\n");
            printf(" - Max number of watched resources reached! See /proc/sys/fs/inotify/max_user_watches\n");
            printf(" - Resource is no more available!?\n");
            return NULL;
        }

        WD_DATA *wd_data = create_wd_data(real_path, wd);

        if (wd_data != NULL)
        {
            element = queue_enqueue(queue_wd, (void *)wd_data);
            log_message("WATCHING: (fd:%d,wd:%d)\t\t\"%s\"", fd, wd_data->wd, real_path);
        }
    }

    /* append symbolic link to watched resources */
    if (element != NULL && symlink != NULL)
    {
        WD_DATA *wd_data = (WD_DATA *)element->data;
        LINK_DATA *link_data = create_link_data(symlink, wd_data);

        if (link_data != NULL)
        {
            queue_enqueue(wd_data->links, (void *)link_data);
            log_message("ADDED SYMBOLIC LINK:\t\t\"%s\" -> \"%s\"", symlink, real_path);
        }
    }

    return element;
}

void unwatch_path(char *absolute_path, int fd, Queue *queue_wd)
{
    QueueElement *element = get_node_from_path(absolute_path, queue_wd);
    if (NULL == element)
        return;

    WD_DATA *wd_data = (WD_DATA *)element->data;

    log_message("UNWATCHING: (fd:%d,wd:%d)\t\t\"%s\"", fd, wd_data->wd, absolute_path);

    remove_watch_descriptor(fd, wd_data->wd);

    if (wd_data->links->first != NULL)
        queue_free(wd_data->links);

    queue_remove(queue_wd, element);
}

void all_symlinks_contained_in(char *path, Queue *queue_wd, Queue *symlinks_found)
{
    QueueElement *element = queue_wd->first;

    WD_DATA *wd_data = NULL;
    while (element)
    {
        wd_data = (WD_DATA *)element->data;

        symlinks_contained_in(path, wd_data->links, symlinks_found);

        element = element->next;
    }
}

void symlinks_contained_in(char *path, Queue *symlinks_to_check, Queue *symlinks_found)
{
    QueueElement *link_node = symlinks_to_check->first;
    while (link_node)
    {
        LINK_DATA *link_data = (LINK_DATA *)link_node->data;
        if (is_child_of(path, link_data->path) == TRUE)
        {
            queue_enqueue(symlinks_found, (void *)link_data->path);
        }
        link_node = link_node->next;
    }
}

void remove_unreachable_resources(WD_DATA *wd_data, int fd, Queue *queue_wd)
{
    // TODO EXTRACT THIS CONTROL IN is_orphan
    // wd_data->links->first == NULL && !is_child_of(root_path, wd_data->path)
    if (wd_data->links->first != NULL || is_child_of(root_path, wd_data->path) == TRUE)
        return;

    Queue *referenced_paths = common_referenced_paths_for(wd_data->path, queue_wd);
    if (NULL != referenced_paths)
    {
        remove_orphan_watched_resources(wd_data->path, referenced_paths, fd, queue_wd);
    }
    queue_free(referenced_paths);
}

Queue *
common_referenced_paths_for(const char *path, Queue *queue_wd)
{
    Queue *referenced_paths = queue_init();
    QueueElement *element;
    WD_DATA *wd_data;

    element = queue_wd->first;
    while (element)
    {
        wd_data = (WD_DATA *)element->data;

        if (wd_data->links->first != NULL && is_related_to(path, wd_data->path) && !is_listed_as_child(wd_data->path, referenced_paths))
        {
            queue_enqueue(referenced_paths, (void *)wd_data->path);
        }
        element = element->next;
    }

    return referenced_paths;
}

bool_t
is_related_to(const char *path, const char *path_to_check)
{
    if (strncmp(path, path_to_check, strlen(path)) == 0 || strncmp(path, path_to_check, strlen(path_to_check)) == 0)
    {
        return TRUE;
    }
    return FALSE;
}

void remove_orphan_watched_resources(const char *path, Queue *references_list, int fd, Queue *queue_wd)
{
    QueueElement *element;
    WD_DATA *wd_data;

    element = queue_wd->first;
    while (element)
    {
        wd_data = (WD_DATA *)element->data;

        if (strcmp(root_path, wd_data->path) != 0 && wd_data->links->first == NULL && is_child_of(path, wd_data->path) == TRUE && !is_listed_as_child(wd_data->path, references_list))
        {
            log_message("UNWATCHING: (fd:%d,wd:%d)\t\t\"%s\"", fd, wd_data->wd, wd_data->path);

            remove_watch_descriptor(fd, wd_data->wd);
            queue_remove(queue_wd, element);
        }
        element = element->next;
    }
}

void unwatch_symlink(char *path_of_symlink, int fd, Queue *queue_wd)
{
    Queue *symlinks_to_remove = queue_init();
    queue_enqueue(symlinks_to_remove, (void *)path_of_symlink);

    while (symlinks_to_remove->first != NULL)
    {
        char *symlink = (char *)queue_dequeue(symlinks_to_remove);

        QueueElement *link_node = get_link_node_from_path(symlink, queue_wd);

        LINK_DATA *link_data = (LINK_DATA *)link_node->data;
        WD_DATA *wd_data = (WD_DATA *)link_data->wd_data;

        char *resolved_path = (char *)wd_data->path;
        char *link_path = (char *)link_data->path;

        log_message("UNWATCHING SYMBOLIC LINK: \t\"%s\" -> \"%s\"", link_path, wd_data->path);
        queue_remove(wd_data->links, link_node);

        all_symlinks_contained_in(resolved_path, queue_wd, symlinks_to_remove);

        remove_unreachable_resources(wd_data, fd, queue_wd);
    }

    queue_free(symlinks_to_remove);
}

int monitor(int fd, Queue *queue_wd)
{
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

    /* Temporary element information */
    QueueElement *element = NULL;
    WD_DATA *wd_data = NULL;

    /* Wait for events */
    while ((len = read(fd, buffer, EVENT_BUF_LEN)))
    {
        if (len < 0)
        {
            printf("ERROR: UNABLE TO READ INOTIFY QUEUE EVENTS!!!\n");
            exit(EIO);
        }

        /* index of the event into file descriptor */
        i = 0;
        while (i < len)
        {
            /* inotify_event */
            event = (struct inotify_event *)&buffer[i];

            /* Discard all filename that matches regular expression (-x option) */
            if (excluded(event->name))
            {
                /* Next event */
                i += EVENT_SIZE + event->len;
                continue;
            }

            /* Build the full path of the directory or symbolic link */
            element = get_node_from_wd(event->wd, queue_wd);
            if (element != NULL)
            {
                wd_data = (WD_DATA *)element->data;
                path = append_file(wd_data->path, event->name);
                if (event->mask & IN_ISDIR)
                {
                    free(path);
                    path = append_dir(wd_data->path, event->name);
                }
            }
            else
            {
                /* Next event */
                i += EVENT_SIZE + event->len;
                continue;
            }

            /* Call the specific event handler */
            if (event->mask & event_mask && (triggered_event = get_inotify_event(event->mask & event_mask)) != NULL && triggered_event->name != NULL && regex_catch(event->name) && triggered_event->handler(event, path, fd, queue_wd) == 0)
            {
                ++exec_c;

                if (execute_command(triggered_event->name, event->name, wd_data->path) == -1)
                {
                    printf("ERROR OCCURED: Unable to execute the specified command!\n");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                free(path);
            }

            /* Next event */
            i += EVENT_SIZE + event->len;
        }
    }

    return 0;
}

int execute_command_inline(char *event_name, char *file_name, char *event_p_path)
{
    log_message("EVENT TRIGGERED [%s] IN %s%s\nNUMBER OF EXECUTION [%d]\nPROCESS EXECUTED [command: %s]",
                event_name, event_p_path, file_name, exec_c, command->data);

    /* Command token replacement */
    tmp_command = format_command((char *)command->data, event_p_path, file_name, event_name);

    int exit = 0;
    exit = system((const char *)tmp_command->data);

    if (exit == -1 || exit == 127)
    {
        log_message("Unable to execute the specified command!");
    }

    bdestroy(tmp_command);

    return 0;
}

int execute_command_embedded(char *event_name, char *file_name, char *event_p_path)
{
    log_message("EVENT TRIGGERED [%s] IN %s%s", event_name, event_p_path, file_name);

    /* Output the formatted string */
    tmp_command = format_command((char *)format->data, event_p_path, file_name, event_name);

    fprintf(stdout, "%s\n", (char *)tmp_command->data);
    fflush(stdout);

    bdestroy(tmp_command);
    return 0;
}

struct event_t *
get_inotify_event(const uint32_t event_mask)
{
    /* NOTE: The following events are combinations
     * of other base-event so the first bit set can't uniquely
     * identify these events.
     */
    switch (event_mask)
    {
    case IN_CLOSE:
        return &events_lut[32];
    case IN_MOVE:
        return &events_lut[33];
    case IN_ALL_EVENTS:
        return &events_lut[34];
    default:
        return &events_lut[ffs(event_mask) - 1];
    }
}

/*
 * EVENT HANDLER IMPLEMENTATION
 */

int event_handler_undefined(struct inotify_event *event, char *path, int fd, Queue *queue_wd)
{
    return 0;
}

int event_handler_create(struct inotify_event *event, char *path, int fd, Queue *queue_wd)
{
    /* Return 0 if recurively monitoring is disabled */
    if (recursive_flag == FALSE)
        return 0;

    /* Check for a directory */
    if (event->mask & IN_ISDIR)
    {
        watch_directory_tree(path, NULL, FALSE, fd, queue_wd);
    }
    else if (nosymlink_flag == FALSE)
    {
        /* Check for a symbolic link */
        if (is_dir(path))
        {
            char *real_path = resolve_real_path(path);
            watch_directory_tree(real_path, path, TRUE, fd, queue_wd);
        }
    }

    return 0;
}

int event_handler_delete(struct inotify_event *event, char *path, int fd, Queue *queue_wd)
{
    /* Check if it is a folder. If yes unwatch it */
    if (event->mask & IN_ISDIR)
    {
        unwatch_path(path, fd, queue_wd);
    }
    else if (nosymlink_flag == FALSE)
    {
        /*
         * XXX Since it is not possible to know if the
         *     inotify event belongs to a file or a symbolic link,
         *     the unwatch function will be called for each file.
         *     That is because the file is deleted from filesystem,
         *     so there is no way to stat it.
         *     This is a big computational issue to be treated.
         */
        if (is_symlink(path, queue_wd))
            unwatch_symlink(path, fd, queue_wd);
    }

    return 0;
}

int event_handler_moved_from(struct inotify_event *event, char *path, int fd, Queue *queue_wd)
{
    return event_handler_delete(event, path, fd, queue_wd);
}

int event_handler_moved_to(struct inotify_event *event, char *path, int fd, Queue *queue_wd)
{
    if (strncmp(path, root_path, strlen(root_path)) == 0) /* TODO: replace with is_child_of */
        return event_handler_create(event, path, fd, queue_wd);

    return 0; /* do nothing */
}

void signal_callback_handler(int signum)
{
    printf("Cleaning...\n");

    /* TODO how to free??? queue_free(queue_wd); */
    exit(signum);
}
