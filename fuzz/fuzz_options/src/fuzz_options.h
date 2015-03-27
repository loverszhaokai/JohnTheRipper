#ifndef _FUZZ_OPTIONS_H_
#define _FUZZ_OPTIONS_H_

#define MAX_LINE_SIZE  1000000
#define MAX_PARAM_NUMBER 10000 // The max number of parameters
#define MAX_SINGLE_PARAM_SIZE 10000 // The max size of sinlge paramter

#define DEBUG 0

// Describe the value of a parameter
struct ParamValue
{
	struct ParamValue *next;

	int id; // From 1
	char *value;
};

// Describe a parameter
struct Param
{
	int id; // From 1
	char *param_name;
	struct ParamValue *param_value;

	struct ParamValue *cnt_param_value; // Points to the current ParamValue
};



#endif // _FUZZ_OPTIONS_H_
