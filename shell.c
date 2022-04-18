#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int input(char *command);
int countArgs(char *s);
void printDirectory();
void onChildExit();
void setupEnvironment();
void shell();
void evaluate(char *command);
int execute(char **args, int numArgs, char *command);
void prepareCommand(char *str, char **args, int numArgs);
void executeCommand(char **args, int numArgs, int wait);

int main(int argc, char *argv[])
{
	// erasing the log file each time the shell is launched
	FILE *f = fopen("log.txt", "w");
	fprintf(f, "%s", "");
	fclose(f);

	signal(SIGCHLD, onChildExit);
	setupEnvironment();
	shell();

	return 0;
}

// Function to take input from user
int input(char *command)
{

	scanf("%[^\n]%*c", command); // scan line
	if (strlen(command) == 0)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

// Function to print Current Directory in appropiate formate
void printDirectory()
{
	char *username = getenv("USER");
	printf("\033[31m"
		   "<<<<MYSHELL>>>>");
	printf("\033[0;32m"); // green
	printf("\033[1m");	  // bold
	printf("@%s", username);
	printf("\033[0m"); // reset
	printf(":");
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	printf("\033[0;34m"); // blue
	printf("\033[1m");	  // bold
	printf("~%s", cwd);
	printf("\033[0m"); // reset
	printf("$ ");
}

// Function to be executed after child finshed and sends SIGCHILD signal to parent
void onChildExit()
{
	FILE *f = fopen("log.txt", "a");
	int stat;
	wait(&stat);
	fprintf(f, "%s", "Child Terminat\n");
	fclose(f);
}

// Function to movr the directory to the current directory
void setupEnvironment()
{
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	chdir(cwd);
}

// Main flow of the program
void shell()
{
	char command[100];
	char commandName[100];
	int numArgs;
	char *mainCommand = "";
	int wait = 1; // varible to determine whether to run childs in background or not
	while (strcmp(command, "exit") != 0)
	{
		printDirectory();
		command[0] = '\0'; // clear command
		input(command);
		if (strstr(command, "$") && !strstr(command, "echo")) // evaluate environment variables after $ - but don't do this with "echo" as it specially handled there
		{
			int i;
			for (i = 0; i < 100; i++)
			{
				if (command[i] == '$')
					break;
			}
			char envVariable[5];
			memcpy(envVariable, &command[i + 1], 4);
			envVariable[4] = '\0';
			for (; i < 100; i++)
			{
				command[i] = '\0';
			}
			strcat(command, getenv(envVariable));
		}

		if (strstr(command, "&"))
		{
			wait = 0;
			for (int i = 0; i < 100; i++)
			{
				if (command[i] == '$')
				{
					command[i] = '\0';
					break;
				}
			}
		}

		memcpy(commandName, &command[0], 99);
		numArgs = countArgs(command);
		char *args[numArgs + 1];
		mainCommand = NULL;
		args[0] = "";
		prepareCommand(command, args, numArgs);
		mainCommand = args[0];

		if (strcmp(mainCommand, "cd") == 0 || strcmp(mainCommand, "echo") == 0 || strcmp(mainCommand, "export") == 0 || strcmp(mainCommand, "exit") == 0)
		{
			execute(args, numArgs, commandName);
		}
		else
		{
			executeCommand(args, numArgs, wait);
		}
	}
}

// Function counts number of arguments in command
int countArgs(char *s)
{
	int count = 0;
	for (int i = 0; s[i] != '\0'; i++)
	{
		if (s[i] == ' ' && s[i + 1] != ' ' && s[i + 1] != '\0')
			count++;
	}
	return count + 1;
}

// Function to cut command string into words in an array of strings (preparing vector for execv()) - taking space as splitter
void prepareCommand(char *str, char **args, int numArgs)
{
	char *token = strtok(str, " ");
	int i = 0;
	// loop through the string to extract all other tokens
	while (token != NULL)
	{
		args[i++] = token;
		token = strtok(NULL, " ");
	}
	args[numArgs] = NULL;
}

// Function to evaluate an expression which is passed after "echo" command
void evaluate(char *command)
{
	char val[100]; // variable to store value of each word inside expression
	int i = 0;
	for (i = 1; i < 100; i++)
	{ // remove "echo "" from the command
		if (command[i] == '"' && command[i - 1] != ' ')
			break;
		command[i - 1] = command[i + 5];
	}
	i = 0;
	int l = 0; // indicator for val string
	while (command[i] != '\0')
	{ // loop till the end of the expression
		if (command[i] == ' ' || command[i] == '"')
		{ // if there is a space or " this indecates the end of a word so evaluate it
			val[l] = '\0';
			char envVariable[100]; // variable to store enviroment variable name if user write $
			if (val[0] == '$')
			{ // if environment variable
				strncpy(envVariable, &val[1], 99);
				char *envValue = getenv(envVariable); // environment variable's value
				if (envValue != NULL)
				{ // if it has value print it, else print space
					printf("%s ", envValue);
				}
				else
					printf(" ");
			}
			else
			{
				printf("%s ", val);
			}
			l = 0;
			i++;
		}
		val[l] = command[i];
		l++;
		i++;
	}
	printf("\n");
}

// Function to execute executable commands
int execute(char **args, int numArgs, char *command)
{
	char envVariable[10]; // environment variable name for export
	char envValue[50];	  // environment variable's value for export

	if (strcmp(args[0], "exit") == 0)
	{
		exit(0);
	}
	else if (strcmp(args[0], "cd") == 0)
	{
		if (strlen(command) == 4 && args[1][0] == '~')
		{
			chdir(getenv("HOME"));
		}
		else
		{
			chdir(args[1]);
		}
		return 1;
	}
	else if (strcmp(args[0], "export") == 0)
	{
		// prepare command
		for (int i = 0; i < strlen(command); i++) // remove "export " from command
		{
			command[i] = command[i + 7];
		}

		// prepare variable name
		int j;
		for (j = 0; j < strlen(command); j++) // copy variable name from command -till reach to "="
		{
			if (command[j] == '=')
				break;
			envVariable[j] = command[j];
		}
		envVariable[j] = '\0';
		for (int i = 0; i < strlen(command); i++) // remove variable name from the command
		{
			if (command[j] == '"')
				command[i] = command[i + j + 2];
			else
			{
				command[i] = command[i + j + 1];
			}
		}
		if (command[0] == '"')
		{
			for (int i = 0; i < 99; i++)
				command[i] = command[i + 1];
		}

		// prepare value
		for (j = 0; j < 50; j++) // copy value of the variable from the command
		{
			if (command[j] == '"' || command[j] == '\0')
				break;
			envValue[j] = command[j];
		}
		envValue[j] = '\0';
		// store it as an environment variable - allow to overwrite if it exist so put 1
		setenv(envVariable, envValue, 1);
		return 1;
	}
	else if (strcmp(args[0], "echo") == 0)
	{
		evaluate(command);
		return 1;
	}
	return 0;
}

// Fuction to execute shell built-in commands
void executeCommand(char **args, int numArgs, int wait)
{
	int p = fork();
	if (p == 0)
	{
		execvp(args[0], args);
		printf("Error\n");
		exit(0);
	}
	else
	{
		if (wait == 1)
			waitpid(p, NULL, 0);
	}
}
