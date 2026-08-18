#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>

#define MAX_TASKS 100
#define MAX_LEN 126
#define DATA_FILE "todo.txt"


typedef struct {
  char text[MAX_LEN];
  int done;
} Task;

static Task tasks[MAX_TASKS];
static int tasks_count = 0;


static void trim_newline(char *s) {
  size_t n = strlen(s);
  if (n > 0 && s[n - 1] == '\n') {
    s[n - 1] = '\0';
  }
}

static int read_line(char *buf, size_t size) {
  if (!fgets(buf, (int)size, stdin))
    return 0;
  if (strchr(buf, '\n') == NULL) {
    int c;

    while ((c = getchar()) != '\n' && c != EOF);
  } else {
    trim_newline(buf);
  }
  return 1;
}

static void load_tasks(void) {
  char line[MAX_LEN + 16];
  FILE *f = fopen(DATA_FILE, "r");
  if (!f)
    return;
  while (tasks_count < MAX_TASKS && fgets(line, sizeof line, f)) {
    char *sep;
    trim_newline(line);

    sep = strchr(line, '|');
    if (!sep)
      continue;
    *sep = '\0';

    tasks[tasks_count].done = (atoi(line) != 0);
    strncpy(tasks[tasks_count].text, sep + 1, MAX_LEN - 1);
    tasks[tasks_count].text[MAX_LEN - 1] = '\0';

    if (tasks[tasks_count].text[0] != '\0')
      tasks_count++;
  }
  fclose(f);
}

static void save_tasks(void) {
  int i;
  FILE *f = fopen(DATA_FILE, "w");
    if (!f) {
    printf("Warning: could not save to %s\n", DATA_FILE);
    return;
  }
  for (i = 0; i < tasks_count; i++)
    fprintf(f, "%d|%s\n", tasks[i].done, tasks[i].text);
  fclose(f);
}

static void list_tasks(void){
  int i, remaining = 0;
  if (tasks_count == 0) {
    printf("\nYour list is empty.\n");
    return;
  }
  printf("\n--- TO-DO LIST---\n");
  for (i = 0; i < tasks_count; i++) {
    printf("%2d. [%c] %s\n", i + 1,
          tasks[i].done ? 'x' : ' ',
          tasks[i].text);
    if (!tasks[i].done)
      remaining++;
  }
  printf("%d task(s), %d remaining\n", tasks_count, remaining);
}

static void add_task() {
  char buf[MAX_LEN];
  if (tasks_count >= MAX_TASKS) {
    printf("List is full (max %d tasks).\n", MAX_TASKS);
    return;
  }
  printf("New task: ");
  if (!read_line(buf, sizeof buf))
    return;
  if (buf[0] == '\0') {
    printf("Nothing has been entered. Please enter something\n");
    return;
  }
  strcpy(tasks[tasks_count].text, buf);
  tasks[tasks_count].done = 0;
  tasks_count++;
  save_tasks();

  printf("Added: %s\n", buf);
}

static int ask_for_index(const char *prompt) {
  char buf[32];
  int n;

  if (tasks_count == 0) {
    printf("Your list is empty.\n");
    return -1;
  }
  printf("%s", prompt);
  if (!read_line(buf, sizeof buf))
    return -1;

  if (sscanf(buf, "%d", &n) != 1 || n < 1 || n > tasks_count) {
    printf("Invalid task number.\n");
    return -1;
  }
  return n - 1;
}

static void toggle_task(void) {
  int i = ask_for_index("Task number to toggle done/undone: ");
  if (i < 0)
    return;
  tasks[i].done = !tasks[i].done;
  save_tasks();
  printf("Marked \%s\" as %s.\n", tasks[i].text,
         tasks[i].done ? "done" : "not done");
}

static void delete_task(void) {
  char removed[MAX_LEN];
  int i = ask_for_index("Task number to delete: ");
  if (i < 0)
    return;

  strcpy(removed, tasks[i].text);
  memmove(&tasks[i], &tasks[i + 1],
          (size_t)(tasks_count - i - 1) * sizeof(Task));
  tasks_count--;
  save_tasks();
  printf("Deleted: %s\n", removed);
}

static void print_menu(void) {
  printf("\n1. Show list \n"
         "2. Add task \n"
         "3. Toggle task \n"
         "4. Delete task \n"
         "5. Quit\n"
         "Choice: ");
}

int main(void) {
  char buf[32];
  int running = 1;

  load_tasks();
  printf("To-do list (%d  task(s) loaded)\n", tasks_count);

  while (running) {
    print_menu();

    if (!read_line(buf, sizeof  buf))
      break;

    switch (buf[0]) {
        case '1': list_tasks(); break;
        case '2': add_task(); break;
        case '3': toggle_task(); break;
        case '4': delete_task(); break;
        case '5': running = 0; break;
        default: printf("Please enter  1-5 \n"); break;
      }
  }
  save_tasks();
  printf("Saved to %s.\n", DATA_FILE);
  return 0;
}
