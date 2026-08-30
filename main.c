/*
 * todo.c - A simple terminal to-do list.
 *
 * Build: gcc -Wall -Wextra -o todo todo.c
 * Run:   ./todo
 *
 * Tasks are saved to $XDG_DATA_HOME/todo/todo.txt, falling back to
 * ~/.local/share/todo/todo.txt. The directory is created on first run,
 * so you get the same list no matter where you invoke the program from.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_TASKS 100
#define MAX_LEN   128
#define PATH_BUF  512
#define DIR_BUF   400

typedef struct {
    char text[MAX_LEN];
    int  done;
} Task;

static Task tasks[MAX_TASKS];
static int  task_count = 0;
static char data_path[PATH_BUF];


static void trim_newline(char *s)
{
    size_t n = strlen(s);
    if (n > 0 && s[n - 1] == '\n')
        s[n - 1] = '\0';
}

static int read_line(char *buf, size_t size)
{
    if (!fgets(buf, (int)size, stdin))
        return 0;

    if (strchr(buf, '\n') == NULL) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    } else {
        trim_newline(buf);
    }
    return 1;
}

static int make_dirs(const char *path)
{
    char tmp[DIR_BUF];
    char *p;
    size_t len;

    strncpy(tmp, path, sizeof tmp - 1);
    tmp[sizeof tmp - 1] = '\0';

    len = strlen(tmp);
    if (len > 0 && tmp[len - 1] == '/')
        tmp[len - 1] = '\0';

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
                return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
        return -1;

    return 0;
}

static void init_data_path(void)
{
    char dir[DIR_BUF];
    const char *xdg  = getenv("XDG_DATA_HOME");
    const char *home = getenv("HOME");

    if (xdg && xdg[0])
        snprintf(dir, sizeof dir, "%s/todo", xdg);
    else if (home && home[0])
        snprintf(dir, sizeof dir, "%s/.local/share/todo", home);
    else {
        strcpy(data_path, "todo.txt");
        return;
    }

    if (make_dirs(dir) != 0) {
        printf("Warning: could not create %s (%s).\n"
               "Falling back to the current directory.\n",
               dir, strerror(errno));
        strcpy(data_path, "todo.txt");
        return;
    }

    snprintf(data_path, sizeof data_path, "%s/todo.txt", dir);
}


static void load_tasks(void)
{
    char line[MAX_LEN + 16];
    FILE *f = fopen(data_path, "r");
    if (!f)
        return;

    while (task_count < MAX_TASKS && fgets(line, sizeof line, f)) {
        char *sep;
        trim_newline(line);

        sep = strchr(line, '|');
        if (!sep)
            continue;
        *sep = '\0';

        tasks[task_count].done = (atoi(line) != 0);
        strncpy(tasks[task_count].text, sep + 1, MAX_LEN - 1);
        tasks[task_count].text[MAX_LEN - 1] = '\0';

        if (tasks[task_count].text[0] != '\0')
            task_count++;
    }
    fclose(f);
}

static void save_tasks(void)
{
    int i;
    FILE *f = fopen(data_path, "w");
    if (!f) {
        printf("Warning: could not save to %s (%s)\n",
               data_path, strerror(errno));
        return;
    }
    for (i = 0; i < task_count; i++)
        fprintf(f, "%d|%s\n", tasks[i].done, tasks[i].text);
    fclose(f);
}


static void list_tasks(void)
{
    int i, remaining = 0;

    if (task_count == 0) {
        printf("\nYour list is empty.\n");
        return;
    }

    printf("\n--- TO-DO LIST ---\n");
    for (i = 0; i < task_count; i++) {
        printf("%2d. [%c] %s\n", i + 1,
               tasks[i].done ? 'x' : ' ',
               tasks[i].text);
        if (!tasks[i].done)
            remaining++;
    }
    printf("------------------\n");
    printf("%d task(s), %d remaining\n", task_count, remaining);
}

static void add_task(void)
{
    char buf[MAX_LEN];

    if (task_count >= MAX_TASKS) {
        printf("List is full (max %d tasks).\n", MAX_TASKS);
        return;
    }

    printf("New task: ");
    if (!read_line(buf, sizeof buf))
        return;

    if (buf[0] == '\0') {
        printf("Nothing entered.\n");
        return;
    }

    strcpy(tasks[task_count].text, buf);
    tasks[task_count].done = 0;
    task_count++;
    save_tasks();

    printf("Added: %s\n", buf);
}

static int ask_for_index(const char *prompt)
{
    char buf[32];
    int n;

    if (task_count == 0) {
        printf("Your list is empty.\n");
        return -1;
    }

    printf("%s", prompt);
    if (!read_line(buf, sizeof buf))
        return -1;

    if (sscanf(buf, "%d", &n) != 1 || n < 1 || n > task_count) {
        printf("Invalid task number.\n");
        return -1;
    }
    return n - 1;
}

static void toggle_task(void)
{
    int i = ask_for_index("Task number to toggle done/undone: ");
    if (i < 0)
        return;

    tasks[i].done = !tasks[i].done;
    save_tasks();
    printf("Marked \"%s\" as %s.\n", tasks[i].text,
           tasks[i].done ? "done" : "not done");
}

static void delete_task(void)
{
    char removed[MAX_LEN];
    int i = ask_for_index("Task number to delete: ");
    if (i < 0)
        return;

    strcpy(removed, tasks[i].text);
    memmove(&tasks[i], &tasks[i + 1],
            (size_t)(task_count - i - 1) * sizeof(Task));
    task_count--;
    save_tasks();

    printf("Deleted: %s\n", removed);
}


static void print_menu(void)
{
    printf("\n1) Show list\n"
           "2) Add task\n"
           "3) Toggle done\n"
           "4) Delete task\n"
           "5) Quit\n"
           "Choice: ");
}

int main(void)
{
    char buf[32];
    int running = 1;

    init_data_path();
    load_tasks();
    printf("To-do list (%d task(s) loaded from %s)\n",
           task_count, data_path);

    while (running) {
        print_menu();

        if (!read_line(buf, sizeof buf))   /* Ctrl-D */
            break;

        switch (buf[0]) {
        case '1': list_tasks();   break;
        case '2': add_task();     break;
        case '3': toggle_task();  break;
        case '4': delete_task();  break;
        case '5': running = 0;    break;
        default:  printf("Please enter 1-5.\n"); break;
        }
    }

    save_tasks();
    printf("Saved to %s. Bye!\n", data_path);
    return 0;
}
