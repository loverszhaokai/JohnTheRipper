#include "fuzz_options.h"

#include <stdlib.h>

unsigned int element_size; // Size of all the elements
unsigned int element_id[MAX_PARAM_NUMBER];

extern struct Param *params_arr[MAX_PARAM_NUMBER];

// If element_size = 10 and current_size = 3
// If current combination = [ 1, 2, 3 ]
//
// Return 0 and the next combination = [ 1, 2, 4 ]
// Return 1 if current combination is the last combination
int next_combination(const int current_size)
{
	int i;
	for (i = current_size - 1; i >= 0; i--) {
		if (element_id[i] != element_size + i - current_size + 1) {
			element_id[i]++; // Add one to element_id[i]
			// Change the element_id[i + 1] to element_id[current_size - 1]
			int j;
			for (j = i + 1; j < current_size; j++) {
				element_id[j] = element_id[j - 1] + 1;
			}

			return 0;
		}
	}

	return 1;
}

// Iteration and Combination
// Iteration is the child of combination, since each element may have 
// many values
// Return 0 if current iteration is not the last iteration
// Return 1 otherwise
int next_iteration(const int current_size)
{
	int i, j;
	struct ParamValue **cnt_param_value;

	for (i = current_size - 1; i >= 0; i--) {

		cnt_param_value = &params_arr[element_id[i]]->cnt_param_value;

		if (NULL != *cnt_param_value && NULL != (*cnt_param_value)->next) {
			// Move to the next ParamValue
			*cnt_param_value = (*cnt_param_value)->next;
			
			for (j = i + 1; j < current_size; j++) {
				params_arr[element_id[j]]->cnt_param_value = 
					params_arr[element_id[j]]->param_value;
			}
			return 0;
		}
	}

	return 1;
}





