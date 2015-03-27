#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "fuzz_options.h"

// FUZZ OPTIONS

struct Param *params_arr[MAX_PARAM_NUMBER]; // Array of pointers to struct Param*
static struct ParamValue **tail_param_value; // Tail of ParameterValue List
static int param_id = 1;
static int param_value_id = 1;
char *params_passed_to_target[MAX_PARAM_NUMBER]; // The parameter list passed to the target
char single_param[MAX_SINGLE_PARAM_SIZE]; // Used to record temporary parameter

FILE *fuzz_result;

extern unsigned int element_size;
extern unsigned int element_id[MAX_PARAM_NUMBER];

static void usage()
{
	printf("Usage: ./fuzz_options  /path/to/app  /path/to/parameter_config_file\n");
}

static int add_parameter_value(char *param_value)
{
	// printf("\t\t\tadd_parameter_value('%s')\n", param_value);

	struct ParamValue *new_param_value = (struct ParamValue*)malloc(sizeof(struct ParamValue));
	new_param_value->next = NULL;
	new_param_value->id = param_value_id++;
	new_param_value->value = strdup(param_value);

	(*tail_param_value) = new_param_value;
	tail_param_value = &new_param_value->next;

	return 0;
}

// @param_name: [***]
static int add_parameter(char *param_name)
{
	// printf("\t\tadd_parameter(%s)\n", param_name);
	param_value_id = 1;
	param_name++;

	char *pend = strchr(param_name, ']');
	if (NULL == pend) {
		printf("failed to find ']' in parameter name:%s\n", param_name);
		return -1;
	}

	*pend = '\0';
	char *copy_param_name = strdup(param_name);
	struct Param *new_param = (struct Param*)malloc(sizeof(struct Param));
	new_param->id = param_id;
	new_param->param_name = copy_param_name;
	new_param->param_value = NULL;
	new_param->cnt_param_value = NULL;

	params_arr[param_id] = new_param;

	param_id++;
	tail_param_value = &new_param->param_value;

	return 0;
}

static int process_line(char *line)
{
	// printf("\tprocess_line(%s)\n", line);

	if (NULL == line)
		return 0;
	if (0 == strlen(line))
		return 0;

	else if ('[' == line[0])
		add_parameter(line);
	else 
		add_parameter_value(line);

	return 0;
}

static char* trim(char *str)
{
	while (' ' == *str || '\t' == *str) str++;
	char *pend = str + strlen(str) - 1;
	while (str <= pend && (' ' == *pend || '\t' == *pend || '\n' == *pend)) pend--;
	*(pend + 1) = '\0';

	return str;
}

static int read_parameter_config(char *file_name)
{
	char line[MAX_LINE_SIZE];
	FILE *pfile = fopen(file_name, "r");

	if (NULL == pfile) {
		printf("failed to open file:'%s'\n", file_name);
		return -1;
	}

	while (fgets(line, sizeof(line), pfile)) {
		trim(line);
		if (0 != process_line(line)) {
			printf("failed to process line: %s", line);
			return -1;
		}
	}

	fclose(pfile);
	return 0;
}

static int get_cnt_parameter(const int current_size)
{
	int i;
	struct Param *cnt_param;
	struct ParamValue *cnt_param_value;

	for (i = 0; i < current_size; ++i) {
		cnt_param = params_arr[element_id[i]];
		cnt_param_value = cnt_param->cnt_param_value;

		// printf("param_name=%s ", cnt_param->param_name);

		if (strlen(cnt_param->param_name) > sizeof(single_param)) {
			printf("the length of param_name[%s] is longer than %lu\n", 
			       cnt_param->param_name, sizeof(single_param));
			return -1;
		}

		char *p = strncpy(single_param, cnt_param->param_name, 
				  strlen(cnt_param->param_name));
		p += strlen(cnt_param->param_name);
		*p = '\0';

		if (NULL != cnt_param_value) {
			if (strlen(cnt_param->param_name) + strlen(cnt_param_value->value) 
			    > sizeof(single_param)) {
				printf("the length of param_name[%s] and param_value[%s] is "
				       "longer than %lu\n", cnt_param->param_name,
				       cnt_param_value->value, sizeof(single_param));
				return -1;
			}
			strcpy(p, cnt_param_value->value);
		}

		// printf("single_param='%s'\n", single_param);

		free(params_passed_to_target[i + 2]);
		params_passed_to_target[i + 2] = strdup(single_param);
	}

	params_passed_to_target[i + 2] = NULL;

	return 0;
}


static int run_target()
{
	int status;
	int ret;

	pid_t pid = fork();

	if (0 == pid) {

		printf("\t\t\t");
		int i = 0;
		while (NULL != params_passed_to_target[i]) {
			printf("  %s", params_passed_to_target[i]);
			i++;
		}
		printf("\n\n");

		const int ret = execv(params_passed_to_target[0], params_passed_to_target);

		if (-1 == ret) {
			perror("failed to execv()");
			return -1;
		}

	} else if (0 < pid) {

		ret = waitpid(pid, &status, 0);
		if (0 >= ret) {
			perror("failed to waitpid");
			return -1;
		}

		if (WIFSIGNALED(status)) {
			printf("\n============There is a crashn============\n\n");

			int i = 0;
			while (NULL != params_passed_to_target[i]) {
				fputs(params_passed_to_target[i], fuzz_result);
				fputs("  ", fuzz_result);
				i++;
			}
			fputs("\n", fuzz_result);
			fflush(fuzz_result);
		}
	} else {
		perror("fork failed:");
		return -1;
	}
	return 0;
}

static int run_iterations(const int current_size)
{
	// printf(">>run_iteration()\n");

	int i, j;
	struct Param *cnt_param;
	struct ParamValue *cnt_param_value;

	// Init the cnt_param_value
	for (i = 0; i < current_size; ++i) {
		params_arr[element_id[i]]->cnt_param_value = 
			params_arr[element_id[i]]->param_value;
	}

	j = 1;
	while (1) {
		printf("\t\titeration[%d]: \n", j++);

		get_cnt_parameter(current_size);

		// Pass parameter to target and run the target
		if (0 != run_target()) {
			printf("failed to run_target()\n");
			return -1;
		}
		
		if (1 == next_iteration(current_size)) {
			break;
		}
	}

	// printf("<<run_iteration()\n");
	return 0;
}

// Return 0 if success
// Return 1 if failed
static int run()
{
	fuzz_result = fopen("crashes" , "wr");

	int i, j, k;

	for (i = 0; i <= element_size; ++i) {

		printf("\ncombination[%d]\n", i);

		// Init the element_id in current iteration
		for (j = 0; j < i; ++j) {
			element_id[j] = j + 1;
		}
		
		while (1) {
			printf("\n[");
			for (k = 0; k < i; ++k) {
				printf("\t%d", element_id[k]);
			}
			printf("]\n");

			run_iterations(i);

			if (1 == next_combination(i)) {
				break;
			}
		}
	}

	fclose(fuzz_result);
	return 0;
}


#if DEBUG

static void print_parameter_value(struct ParamValue *param_value)
{
	printf("\tparameter_value:\n");
	struct ParamValue *cnt_param_value = param_value;
	while (NULL != cnt_param_value) {

		printf("\t\tid=%d\n", cnt_param_value->id);
		printf("\t\t\tvalue=%s\n", cnt_param_value->value);

		cnt_param_value = cnt_param_value->next;
	}
}

static void print_parameter_config()
{
	printf("\nThis is parameters\n\n");

	int i;

	for (i = 1; i <= element_size; ++i) {

		printf("id=%d\n", params_arr[i]->id);
		printf("\tparam_name=%s\n", params_arr[i]->param_name);

		print_parameter_value(params_arr[i]->param_value);
	}
}

static void print_iterations(const int current_size)
{

	int i, j;

	// Init the cnt_param_value
	for (i = 0; i < current_size; ++i) {
		params_arr[element_id[i]]->cnt_param_value = 
			params_arr[element_id[i]]->param_value;
	}

	j = 1;
	while (1) {
		printf("\t\titeration[%d]: ", j++);

		for (i = 0; i < current_size; ++i) {
			if (NULL != params_arr[element_id[i]]->cnt_param_value)
				printf("%s\t", params_arr[element_id[i]]->cnt_param_value->value);
			else
 				printf("\tNULL");
		}
		printf("\n");

		if (1 == next_iteration(current_size)) {
			break;
		}
	}
}

static void print_combinations()
{
	printf("print all the combinatins()\n");

	int i, j, k;

	for (i = 0; i <= element_size; ++i) {

		printf("\ncombination[%d]\n\n", i);

		// Init the element_id in current iteration
		for (j = 0; j < i; ++j) {
			element_id[j] = j + 1;
		}
		
		while (1) {
			
			for (k = 0; k < i; ++k) {
				printf("\t%d", element_id[k]);
			}
			printf("\n");

			print_iterations(i);

			if (1 == next_combination(i)) {
				break;
			}

		}
	}
}

#endif



int main(int argc, char **argv)
{
	printf("\n\t\tFuzz Options\n\n");

	if (3 != argc) {
		usage();
		return -1;
	}
	params_passed_to_target[0] = argv[1];
	params_passed_to_target[1] = "--max-run-time=1";


	if (0 != read_parameter_config(argv[2])) {
		printf("failed to read_parameter_config(''%s)\n", argv[2]);
		return -1;
	}

	if (MAX_PARAM_NUMBER < param_id) {
		printf("the number of parameters:%d is too large\n", param_id - 1);
		return -1;
	}

	element_size = param_id - 1;

#if DEBUG

	print_parameter_config();

	print_combinations();

#endif

	if (0 != run()) {
		printf("failed to run()\n");
		return -1;
	}

	return 0;
}
