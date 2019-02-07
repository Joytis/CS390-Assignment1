// #define _XOPEN_SOURCE 700

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <wordexp.h>
#include <unistd.h>
#include <sys/stat.h>
 
//==========================================
// UTILITIES
//==========================================

#define ARRAY_ELEMS(x) (sizeof(x) / sizeof(x[0]))
#define STREQL(x, y) (strcmp(x, y) == 0)

const char* SH_CARAT = "$ ";
const char* PS1_CARAT = "> ";

// Yeah, Yeah. I know. Globals. This is a small program. 
const char* prompt_carat;

void prompt_user()
{
	printf("%s", prompt_carat);
}

// Simple storage for command line args. 
typedef struct
{
	int argc;
	char* buffer;
	char** argv;
} cmd_args;

// Parse a line into posix cmd args. 
cmd_args split_into_args(char* line)
{
	// Precondition
	assert(line != NULL);

	// Copy the line of text. 
	char* buffer = strdup(line);
	int line_len = strlen(buffer);

	// Split it into tokens. 
	char **argv = NULL;
	// destroy newline characters
	for(int i = 0; i < line_len; i++) {
		if(buffer[i] == '\n') {
			buffer[i] = ' ';
		}
	}

	char* token = strtok (buffer, " ");
	int argc = 0;

	// Tokenize string
	while(token) {
		argc++;
		argv = realloc(argv, sizeof(char*) * argc);

		assert(argv != NULL);
		argv[argc - 1] = token;
		token = strtok(NULL, " \n");
	}

	// Add trailing NULL for free call.
	argv = realloc(argv, sizeof(char*) * (argc + 1));
	argv[argc] = 0;

	// Construct and return value
	cmd_args args;
	args.argc = argc;
	args.buffer = buffer;
	args.argv = argv;

	return args;
}

// Free our command arguments. 
void free_cmd_args(cmd_args* args)
{
	free(args->argv);
	free(args->buffer);
}

bool file_exists(char* filename)
{
	return access(filename, F_OK) != -1;
}

bool directory_exists(char* dirname)
{
	struct stat sb;
	return (stat(dirname, &sb) == 0 && S_ISDIR(sb.st_mode));
}

//==========================================
// PROGRAM COMMANDS
//==========================================

// Echo strings that come after echo. 
void cmd_echo(int argc, char** argv, char* line) {
	if(argc < 2) {
		printf("Please provide a string to echo. or options.\n");
	    return;
	}

	// Iterate over argv until we don't find an option. 
	int argv_index = 1;
	bool keep_searching = true;
	bool print_newline = true;

	// Iterate until we have found some kind of not-option or we're at the end. 
	while(keep_searching && argv_index < argc) {
		const char* arg = argv[argv_index];

		// Search for a non-arg.
		if(STREQL(arg, "-n")) {
			print_newline = false;
			argv_index++;
		}
		else {
			keep_searching = false;
		}

	}

	if(argv_index == argc) {
		printf("Please provide a string to echo. only options entered.\n");
	    return;
	}

	printf("%s", argv[argv_index++]); // Print first value. 

	for(int i = argv_index; i < argc; i++)
		printf(" %s", argv[i]); // Print other values. 

	if(print_newline) 
		printf("\n");
}

// Change the carat to the ps1 carat
void cmd_PS1(int argc, char** argv, char* line) {
	prompt_carat = PS1_CARAT;
}

// Change carat back to the sh carat
void cmd_sh(int argc, char** argv, char* line) {
	prompt_carat = SH_CARAT;
}

// print a file out to the screen. 
void cmd_cat(int argc, char** argv, char* line) {
	// Preconditions
	if(argc != 2) {
		printf("Usage: cat [FILE_NAME].\n");
	    return;
	}

	char *file_name = argv[1];
	if(!file_exists(file_name)) {
		printf("File %s does not exist.\n", file_name);
		return;
	}

	// Attempt to open file/
	FILE *file = fopen(file_name, "r");
	if (file == NULL) {
	    printf("Cannot open %s. Try again later.\n", file_name);
	    return;
	}

	// Read all lines
	char *read_line;
	size_t len = 0;
	ssize_t read;
	while((read = getline(&read_line, &len, file)) != -1) {
		printf("%s", read_line);
	}

	fclose(file);
    if (line != NULL) {
        free(line);
    }
	printf("\n");
}

// Copy a file from one location to another. 
void cmd_cp(int argc, char** argv, char* line) {
	if(argc != 3) {
		printf("Usage: cp [FROM_FILE] [TO_FILE].\n");
	    return;
	}

	char *from_file = argv[1];
	char *to_file = argv[2];

	if(!file_exists(from_file)) {
		printf("File %s does not exist.\n", from_file);
		return;
	}
	if(file_exists(to_file)) {
		printf("File %s already exists.\n", to_file);
		return;
	}

	int ifd = open(from_file, O_RDONLY);
	if(ifd == -1) {
		printf("Could not open %s. Please try again later.\n", from_file);
		return;
	}

	int ofd = open(to_file, O_WRONLY | O_CREAT, 0644);
	if(ofd == -1) {
		printf("Could not open %s for writing. Please try again later.\n", to_file);
		return;
	}

	// 1k copy buffer. Arbitrary length. 
	char buffer[1024];
	ssize_t read_in;
	ssize_t write_out;
	while((read_in = read(ifd, &buffer, 1024)) > 0) { // read in
		write_out = write(ofd, &buffer, read_in); // write to file

		// Check for write discrepency. 
		if(write_out != read_in) {
			printf("A write error occurred!\n");
			return;
		}
	}

	close(ifd);
	close(ofd);
}

// Delete a file. 
void cmd_rm(int argc, char** argv, char* line) {
	if(argc != 2) {
		printf("Usage: rm [FILE_TO_REMOVE].\n");
	    return;
	}

	char *filename = argv[1];

	if(!file_exists(filename)) {
		printf("File %s does not exist.\n", filename);
		return;
	}

	if(remove(filename) == -1) {
		printf("Error deleting file: %s.\n", filename);
	}
}

// Makes a directory. 
void cmd_mkdir(int argc, char** argv, char* line) {
	if(argc != 2) {
		printf("Usage: mkdir [DIR_TO_CREATE].\n");
	    return;
	}

	char *dirname = argv[1];
	if(directory_exists(dirname)) {
		printf("Directory %s already exists.\n", dirname);
		return;
	}

	// Give permissions to everyone, because we are kind programmer gods. 
	if(mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
		printf("Error creating directory %s.", dirname);
	}
}

// Deletes a directory. 
void cmd_rmdir(int argc, char** argv, char* line) {
	if(argc != 2) {
		printf("Usage: rmdir [DIR_TO_REMOVE].\n");
	    return;
	}

	char *dirname = argv[1];
	if(!directory_exists(dirname)) {
		printf("Directory %s does not exist.\n", dirname);
		return;
	}

	// Give permissions to everyone, because we are kind programmer gods. 
	if(rmdir(dirname) == -1) {
		printf("Error removing directory %s.", dirname);
	}
}

//==========================================
// MAIN BOOTSTRAP
//==========================================

void run_shell(int argc, char** argv, char* line) {
	if(argc < 1) return;

	// Check arguments
	char* command = argv[0];
	if(STREQL(command, "echo")) { 
		cmd_echo(argc, argv, line); 
	}
	else if(STREQL(command, "PS1")) { 
		cmd_PS1(argc, argv, line); 
	}
	else if(STREQL(command, "sh")) { 
		cmd_sh(argc, argv, line); 
	}
	else if(STREQL(command, "cat")) { 
		cmd_cat(argc, argv, line); 
	}
	else if(STREQL(command, "cp")) { 
		cmd_cp(argc, argv, line); 
	}
	else if(STREQL(command, "rm")) { 
		cmd_rm(argc, argv, line); 
	}
	else if(STREQL(command, "mkdir")) { 
		cmd_mkdir(argc, argv, line); 
	}
	else if(STREQL(command, "rmdir")) { 
		cmd_rmdir(argc, argv, line); 
	}
	else if(STREQL(command, "exit")) { 
		exit(EXIT_SUCCESS);
	}
	else {
		printf("Unrecognized Command\n");
	}

}

int main(int argc, char** argv) {
	prompt_carat = SH_CARAT; // Start as SH carat

    char *line = NULL;
	ssize_t read;
    size_t len = 0;
	// Prompt
    prompt_user();

    while ((read = getline(&line, &len, stdin)) != -1) {

        // Parse line as a set of arguments.
        cmd_args args = split_into_args(line);

        run_shell(args.argc, args.argv, line);

		// Free resources. 
		free_cmd_args(&args);

		// Prompt
		prompt_user();
    }

    free(line);
	exit(EXIT_SUCCESS);
}