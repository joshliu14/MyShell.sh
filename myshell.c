#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define LOG_FILE_PATH "$HOME/.myshell.log"
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define MAX_CONFIG_LINE_LENGTH 256
#define MAX_KEY_LENGTH 50
#define MAX_VALUE_LENGTH 100

// Struct to hold key-value pairs from the configuration file
struct Config {
    char custom_prompt[MAX_VALUE_LENGTH];
    int input_size; 
    char pre_shell_commands[10][MAX_VALUE_LENGTH]; // Up to 10 pre-shell commands
    int num_pre_shell_commands;
};

/*
 * Structure to hold information about a command
 * argv is an array of strings where each string is a token of the command
 */
struct command {
	char **argv;
};

/*
 * If there is a pipe and is the not the last command you output to the pipe
 */
int spawn_proc (int in, int out, struct command *cmd)
{
	pid_t pid;

	if ((pid = fork ()) == 0)
	{
		if (in != 0)
		{
			dup2 (in, 0);
			close (in);
		}

		if (out != 1)
		{
			dup2 (out, 1);
			close (out);
		}

		if (execvp (cmd->argv [0], (char * const *)cmd->argv) < 0)
		{
			printf("myshell: command not found: %s\n", cmd->argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	return pid;
}

/*
 * Forks the necessary number of pipes and executes the commands
 */
void fork_pipes (int n, struct command *cmd)
{
	int i;
	int in, fd [2];

	in = 0;

	for (i = 0; i < n - 1; ++i)
	{
		pipe (fd);
		spawn_proc (in, fd [1], cmd + i);
		close (fd [1]);
		in = fd [0];
	}

	if (in != 0)
		dup2 (in, 0);

	if (execvp (cmd [i].argv [0], (char * const *)cmd [i].argv) < 0)
	{
		printf("myshell: command not found: %s\n", cmd[i].argv[0]);
		exit(EXIT_FAILURE);
	}
}

/*
 * Trims the string s by removing leading and trailing spaces, tabs and newlines
 */
void trim(char *s) {
	int i = strlen(s)-1;
	while (i > 0) {
		if (s[i] == ' ' || s[i] == '\t' || s[i] == '\n') i--;
		else break;
	}
	s[i+1] = '\0';
	int count = 0;
	while (s[count] == ' ' || s[count] == '\t' || s[count] == '\n') count++;
	if (count != 0) {
		i = 0;
		while (s[i+count] != '\0') {
			s[i] = s[i+count];
			i++;
		}
		s[i] = '\0';
	}
}

/*
 * Parses the string s and fills the array of commands cmd
 * Returns the number of commands
 */
int parseCommand(char *s, struct command *cmd) {
	trim(s);
	char *p = strsep(&s, "|");
	int i = 0;
	while (p != NULL) {
		char **argv = malloc(20*sizeof(char*));
		if (argv == NULL) {
			perror("malloc");
			exit(1);
		}
		trim(p);
		char *q = strsep(&p, " ");
		int j = 0;
		while (q != NULL) {
			argv[j] = q;
			j++;
			q = strsep(&p, " ");
		}
		argv[j] = NULL;
		cmd[i].argv = argv;
		i++;
		p = strsep(&s, "|");
	}
	return i;
}

/* Function to write log entry to the log file
 * @param command: the command to be logged
 */
void write_log(const char *command, const char *status) {
    // Get current time
    time_t now;
    time(&now);
    struct tm *local_time = localtime(&now);

    // Open the log file
    char log_file_path[256];
    snprintf(log_file_path, sizeof(log_file_path), "%s/.myshell.log", getenv("HOME"));
    FILE *log_file = fopen(log_file_path, "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        return;
    }

    // Format log entry
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", local_time);

    // Write log entry
    fprintf(log_file, "[%s] Command executed: %s, Status: %s\n", timestamp, command, status);

    // Close the log file
    fclose(log_file);
}

// Function to parse a single configuration file
void parseConfigFile(const char *filename, struct Config *config) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
       	write_log(filename, "failed to open configuration file");
        return;
    }

    char line[MAX_CONFIG_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL) {
        // Ignore comments (lines starting with #)
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
		trim(line);
        // Tokenize line to extract key and value
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");
        // Remove leading and trailing whitespace from key and value
        if (key != NULL && value != NULL) {
            trim(key);
			trim(value);
            // Check for different options and update config accordingly
            if (strcmp(key, "CustomPrompt") == 0) {
                strncpy(config->custom_prompt, value, sizeof(config->custom_prompt) - 1);
            } else if (strcmp(key, "InputSize") == 0) {
                config->input_size = atoi(value);
            } else if (strncmp(key, "PreShellCommand", 15) == 0) {
                strncpy(config->pre_shell_commands[config->num_pre_shell_commands], value, sizeof(config->pre_shell_commands[0]) - 1);
                config->num_pre_shell_commands++;
            }
            // Add more configuration options handling as needed
        } else {
			write_log(filename, "syntax error in configuration file");
        }
    }
    fclose(file);
}

// Function to parse both system-wide and user-specific configuration files
void parseConfiguration(struct Config *config) {
    // Parse system-wide configuration file
    parseConfigFile("/etc/myshell", config);

    // Parse user-specific configuration file
    char *home_dir = getenv("HOME");
    if (home_dir != NULL) {
        char user_config_file[256];
        snprintf(user_config_file, sizeof(user_config_file), "%s/.myshell_rc", home_dir);
        parseConfigFile(user_config_file, config);
    }
}

/*
 * Function to run pre-shell commands
 * @param config: the configuration struct
 */
void runPreShellCommands(struct Config *config) {
	for (int i = 0; i < config->num_pre_shell_commands; i++) {
		char *input = config->pre_shell_commands[i];
		struct command cmd[20];
		int n = parseCommand(input, cmd);
		if (strcmp(cmd->argv[0], "exit") == 0) {
			exit(0);
		}
		if (strcmp(cmd->argv[0], "cd") == 0) {
			chdir(cmd->argv[1]);
			continue;
		}

		pid_t pid = fork();
		if (pid == 0) {
			fork_pipes(n, cmd);
			exit(EXIT_SUCCESS);
		}
		else if (pid < 0) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else {
			int status;
			waitpid(pid, &status, 0);
			if (status != 0) {
				write_log(input, "failed");
			}
			else {
				write_log(input, "success");
			}
		}
		for (int i = 0; i < n; i++) {
			free(cmd[i].argv);
		}

	}
}

/*
 * Main function
 */
int main ()
{
	struct Config config;

    // Initialize config values
	config.custom_prompt[0] = '\0';
    config.input_size = 0;
    config.num_pre_shell_commands = 0;

    // Parse and apply the configuration files
    parseConfiguration(&config);

    // Print the configuration
	if (strlen(config.custom_prompt) != 0) {
    	printf("Custom Prompt: %s\n", config.custom_prompt);
	}
	if (config.input_size != 0) {
    	printf("Input size set to: %d\n", config.input_size);
	}
	if (config.num_pre_shell_commands > 0) {
		runPreShellCommands(&config);
	}
    
	char input[100];
	struct command cmd[20];
	printf("Welcome to myshell\n");
	while (1) {
		memset(input, 0, sizeof(input));
		if (strlen(config.custom_prompt) != 0) {
			printf("%s ",config.custom_prompt);
		} else {
			printf("> ");
		}
		if (scanf("%[^\n]%*c", input) == 0) {
			getchar();
			continue;
		}
		int n = parseCommand(input, cmd);
		if (strcmp(cmd->argv[0], "exit") == 0) {
			exit(0);
		}
		if (strcmp(cmd->argv[0], "cd") == 0) {
			chdir(cmd->argv[1]);
			continue;
		}

		pid_t pid = fork();
		if (pid == 0) {
			fork_pipes(n, cmd);
			exit(EXIT_SUCCESS);
		}
		else if (pid < 0) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else {
			int status;
			waitpid(pid, &status, 0);
			if (status != 0) {
				write_log(input, "failed");
			}
			else {
				write_log(input, "success");
			}
		}
		for (int i = 0; i < n; i++) {
			free(cmd[i].argv);
		}
	}
}
