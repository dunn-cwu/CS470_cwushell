// ==============================
// School: Central Washington University
// Course: CS470 Operating Systems
// Instructor: Dr. Szilárd VAJDA
// Student: Andrew Dunn
// Assignment: Lab 1
// ==============================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

// Preprocessor defines for constants and
// built-in commands

#define HELP_FILE "manual.txt"

#define DEFAULT_PROMPT "cwushell"
#define DELIMITER " "
#define PROMPT_BUFFER_SIZE 64
#define LINE_BUFFER_SIZE  1024
#define MAX_ARGUMENTS  16

#define HELP_CMD "manual"
#define HELP_CODE 0

#define EXIT_CMD "exit"
#define EXIT_CODE 1 

#define PROMPT_CMD "prompt"
#define PROMPT_CODE 2

#define CHANGEDIR_CMD "cd"
#define CHANGEDIR_CODE 3

#define CPUINFO_CMD "cpuinfo"
#define CPUINFO_CODE 4

#define MEMINFO_CMD "meminfo"
#define MEMINFO_CODE 5

int process_cmd(char* s_cmd, int default_ret_val, int* keep_running);
int execute_external(char* s_cmd);
int get_cmd_code(const char* cmd_root);
int print_help_file();

// Global variable which stores the current prompt string
static char g_prompt[PROMPT_BUFFER_SIZE];

// Program entry point and main loop
int main(void)
{
    char* line_buffer;
    size_t line_buffer_size = LINE_BUFFER_SIZE;
    int running = 1;
    int shell_ret_val = 0;

    // Allocate buffer for user input string
    line_buffer = malloc(line_buffer_size * sizeof(char));

    // Copy default prompt string into g_prompt
    strcpy(g_prompt, DEFAULT_PROMPT);

    // Main program loop
    while (running)
    {
        printf("%s> ", g_prompt);

        // Get user input line from stdin
        int chars_read = getline(&line_buffer, &line_buffer_size, stdin);

        if (chars_read == -1)
            printf("Error reading input.\n");
        else
        {
            // Strip new-line char at end of string
            if (line_buffer[chars_read - 1] == '\n')
                line_buffer[chars_read - 1] = '\0';

            // Process input
            shell_ret_val = process_cmd(line_buffer, shell_ret_val, &running);
        }
            
    }
    
    printf("Exiting with code %d.\n", shell_ret_val);

    free(line_buffer);

    return shell_ret_val;
}

// Prints info about the cpu depending on the given switch
int print_cpuinfo(char* s_switch)
{
    int ret_val = 0;

    // If no switch provided, default to -help
    if (s_switch == NULL) s_switch = "-help";

    if (strcmp(s_switch, "-c") == 0)
    {
        // Print cpu clock speed
        ret_val = execute_external("lscpu | grep MHz");
    }
    else if (strcmp(s_switch, "-t") == 0)
    {
        // Print cpu type/model
        execute_external("cat /proc/cpuinfo | grep 'vendor' | uniq");
        ret_val = execute_external("cat /proc/cpuinfo | grep 'model name' | uniq");
    }
    else if (strcmp(s_switch, "-n") == 0)
    {
        // Print number of cpu's/cpu cores
        printf("Number of cpu's/cores: ");
        fflush(stdout);

        ret_val = execute_external("cat /proc/cpuinfo | grep processor | wc -l");
    }
    else if (strcmp(s_switch, "-help") == 0)
    {
        // Print help info
        printf("Usage: cpuinfo [-switch]\n");
        printf("Valid switches:\n");
        printf("  -help: Prints this help message.\n");
        printf("  -c:    Prints your cpu clock speed.\n");
        printf("  -t:    Prints your cpu type.\n");
        printf("  -n:    Prints your cpu core count.\n");
    }
    else
    {
        // Invalid switch provided
        printf("%s is not a valid command switch. Enter 'cpuinfo -help' for usage.\n", s_switch);
    }

    return ret_val;
}

// Prints info about the system RAM depending on the given switch
int print_meminfo(char* s_switch)
{
    int ret_val = 0;

    // If no switch provided, default to -help
    if (s_switch == NULL) s_switch = "-help";

    if (strcmp(s_switch, "-t") == 0)
    {
        // Print amount of total memory
        ret_val = execute_external("vmstat -s | grep 'total memory' | uniq");
    }
    else if (strcmp(s_switch, "-u") == 0)
    {
        // Print amound of used memory
        ret_val = execute_external("vmstat -s | grep 'used memory' | uniq");
    }
    else if (strcmp(s_switch, "-c") == 0)
    {
        // Print the size of the cpu L2 cache
        printf("Size of CPU L2 Cache: ");
        fflush(stdout);

        ret_val = execute_external("cat /sys/devices/system/cpu/cpu0/cache/index2/size");
    }
    else if (strcmp(s_switch, "-help") == 0)
    {
        // Print help info
        printf("Usage: meminfo [-switch]\n");
        printf("Valid switches:\n");
        printf("  -help: Prints this help message.\n");
        printf("  -t:    Prints total available RAM.\n");
        printf("  -u:    Prints currently used RAM.\n");
        printf("  -c:    Prints size of L2 cache.\n");
    }
    else
    {
        // Invalid switch provided
        printf("%s is not a valid command switch. Enter 'meminfo -help' for usage.\n", s_switch);
    }

    return ret_val;
}

// Processes the given command string and sets
// keep_running to 0 on 'exit' command
int process_cmd(char* s_cmd, int default_ret_val, int* keep_running)
{
    // Duplicate command string and set up local vars
    char* cmd_split = strdup(s_cmd);
    char* cmd_root;
    char* cmd_arg;
    char* cmd_args[MAX_ARGUMENTS];
    int args_count = 0;
    int ret_val = default_ret_val;

    // Split command string and get root command
    cmd_root = strtok(cmd_split, DELIMITER);
    if (cmd_root == NULL)
    {
        printf("Error: Command is empty.\n");
        return 0;
    }

    // Initialize arguments string array to NULL
    for (int i = 0; i < MAX_ARGUMENTS; i++)
        cmd_args[i] = NULL;

    int arg_counter = 0;

    // Extract arguments from command string.
    // There is a limit to the number of command arguments
    // for built-in commands.
    while (arg_counter < MAX_ARGUMENTS)
    {
        cmd_arg = strtok(NULL, DELIMITER);
        if (cmd_arg == NULL)
            break; // No tokens left
        else
        {
            // Set argument string ptr to token
            cmd_args[arg_counter] = cmd_arg;
            arg_counter++;
        }
    }

    // Convert the root command string to a command code integer
    int cmd_code = get_cmd_code(cmd_root);

    // Process the command code
    switch (cmd_code)
    {
        case HELP_CODE:
            ret_val = print_help_file();
            break;
        case EXIT_CODE:
            (*keep_running) = 0;
            if (arg_counter > 0)
                ret_val = atoi(cmd_args[0]);
            break;
        case PROMPT_CODE:
            if (arg_counter == 0)
            {
                printf("Error: Missing new prompt string argument.\n");
                printf("Enter command 'manual' for help.\n");
            }
            else
                strcpy(g_prompt, cmd_args[0]);
            break;
        case CHANGEDIR_CODE:
            if (arg_counter == 0)
            {
                printf("Error: Missing path string argument.\n");
                printf("Enter command 'manual' for help.\n");
            }
            else
            {
                ret_val = chdir(cmd_args[0]);
                if (ret_val != 0)
                  printf("Error: Change directory failed.\n");  
            }
            break;
        case CPUINFO_CODE:
            ret_val = print_cpuinfo(cmd_args[0]);
            break;
        case MEMINFO_CODE:
            ret_val = print_meminfo(cmd_args[0]);
            break;
        default:
            // Command is not built-in, so attempt to execute it in the standard unix shell
            ret_val = execute_external(s_cmd);
            if (ret_val != 0)
                printf("External command terminated with exit code %d.\n", ret_val);
            break;
    }

    free(cmd_split);
    return ret_val;
}

// Executes a given command string in the standard unix shell
int execute_external(char* s_cmd)
{
    // Prepare args array for sh
    char* cmd_args[4];
    
    cmd_args[0] = "sh";
    cmd_args[1] = "-c";
    cmd_args[2] = s_cmd;
    cmd_args[3] = NULL; // Args must be null terminated

    int status;
    int return_code = EXIT_FAILURE;

    // Spawn child process
    pid_t pid = fork();

    if (pid == -1)
    {
        printf("Error: Unable to fork process.\n");
    }
    else if (pid==0)
    {
        // We are in the child process. Execute shell command.
        execvp("sh", cmd_args);
        exit(127); // Command not found return code. This line is not reached on success.
    }

    // Wait for child process to exit
    if (waitpid(pid, &status, 0) == -1)
    {
        printf("Error: Waiting for child process failed.\n");
        return return_code;
    }

    // If child process exited normally, get exit code
    if (WIFEXITED(status)) {
        return_code = WEXITSTATUS(status);
    }

    // If exit code was 127, the given shell command was not found.
    if (return_code == 127)
    {
        printf("Need help? Try the 'manual' command.\n");
    }

    return return_code;
}

// Coverts the given command string to a command code integer
// Returns -1 if command is not found.
int get_cmd_code(const char* cmd_root)
{
    if (strcmp(cmd_root, HELP_CMD) == 0)
        return HELP_CODE;
    else if (strcmp(cmd_root, EXIT_CMD) == 0)
        return EXIT_CODE;
    else if (strcmp(cmd_root, PROMPT_CMD) == 0)
        return PROMPT_CODE;
    else if (strcmp(cmd_root, CHANGEDIR_CMD) == 0)
        return CHANGEDIR_CODE;
    else if (strcmp(cmd_root, CPUINFO_CMD) == 0)
        return CPUINFO_CODE;
    else if (strcmp(cmd_root, MEMINFO_CMD) == 0)
        return MEMINFO_CODE;

    return -1;
}

// Opens the help file and prints the contents to the terminal
int print_help_file()
{
    FILE* f_ptr = fopen(HELP_FILE, "r");
    if (f_ptr == NULL)
    {
        printf("Error: Unable to open help file: " HELP_FILE);
        return 1;
    }

    // Print help file char by char until end of file.
    char c = fgetc(f_ptr);
    while (c != EOF)
    {
        printf("%c", c);
        c = fgetc(f_ptr);
    }

    fclose(f_ptr);
    return 0;
}