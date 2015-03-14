#include "john.h"

#include <stdio.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "alloc-inl.h"
#include "john_mutator.h"

#define BUFFER_MAX_SIZE 2000000000
#define MAX_ELEMENTS    1000
#define FOLDER_NAME_SIZE  100
#define FILE_NAME_SIZE    200

static struct JohnElement *john_p;                           // The head pointer
static int out_buf_index = 0;                                // Index of out_buf
static char out_buf[BUFFER_MAX_SIZE + 1] = { 0 };            // Mutated content

static int mutate_elements_size = 0;                         // The number of elements which are going to be mutated 
                                                             // in current iteration
static int mutate_id_arr[MAX_ELEMENTS + 1] = { 0 };          // Mutate ids which will be mutated in current iteration
static char folder_name_buf[FOLDER_NAME_SIZE + 1] = { 0 };   // Folder name
static char file_name_buf[FILE_NAME_SIZE + 1] = { 0 };       // File name contians the folder name, such as out/7z_fmt/1

static void john_free(struct JohnElement *je) {
  if (NULL == je)
    return;

  if (NULL != je->next)
    john_free(je->next);

  free(je->orig_value);  
  free(je->mutated_value);  

  if (NULL != je->children)
    john_free(je->children);

  free(je);
}

// Randomly select a number from @min_v to @max_v to return
// Including @min_v and Excluding @max_v
int random_int(const int min_v, const int max_v) {
  int ret = 0;

  if (max_v < min_v)
    ret = 0;
  else if (max_v == min_v)
    ret = min_v;
  else {
    // min_v < max_v
    const int step = max_v - min_v;
    ret = min_v + rand() % step;
  }
	
  return ret;
}

// Randomly select N elements to mutate
static void select_mutate_elements(int total_mutate_element_num) {
  if (total_mutate_element_num > MAX_ELEMENTS)
    total_mutate_element_num = MAX_ELEMENTS;

  mutate_elements_size = random_int(1, total_mutate_element_num + 1);
  // printf("N=%d\n", mutate_elements_size);

  int i, j, t;

  for (i = 0; i < total_mutate_element_num; ++i)
    mutate_id_arr[i] = i + 1;

  for (i = 0; (i < total_mutate_element_num) && (i < mutate_elements_size); ++i) {
    t = random_int(i, total_mutate_element_num);
    j = mutate_id_arr[i];
    mutate_id_arr[i] = mutate_id_arr[t];
    mutate_id_arr[t] = j;
  }
  mutate_id_arr[total_mutate_element_num] = 0;
}

static int is_mutate_cnt_element(const int cnt_element_mutate_id) {
  int i;
  for (i = 0; i < mutate_elements_size; ++i) {
    if (cnt_element_mutate_id == mutate_id_arr[i]) return 1;
  }

  return 0;
}


static void mutate_one_element(struct JohnElement *element) {
  if (JOHN_E_STRING == element->element_type)
    mutate_string_element(element);
}

static void mutate(struct JohnElement *element) {
  if (NULL == element) return;

  // printf("mutate_id=%d\n", element->mutate_element_id);

  // Put the element_value to the out_buf
  if (element->type != JOHN_NONE) {

    char *element_value = element->orig_value;
    int element_size = strlen(element->orig_value);
    if (element->is_mutate && is_mutate_cnt_element(element->mutate_element_id)) {
      // Mutate current element
      mutate_one_element(element);
      element_value = element->mutated_value;
      element_size = element->mutated_value_size;
    }

    const long long index = out_buf_index + element_size;

    if (index >= BUFFER_MAX_SIZE) {
      memcpy(out_buf + out_buf_index, element_value, sizeof(out_buf) - out_buf_index - 1);
      out_buf_index = BUFFER_MAX_SIZE;
    } else {
      memcpy(out_buf + out_buf_index, element_value, element_size);
      out_buf_index = index;
    }
  }

  // Mutate children
  mutate(element->children);

  // If element is top dataelement, xpath = "/AFL/DataElement"
  // Add \n to the out_buf in order to line feed
  if ((1 == element->is_top_dataelement) && (out_buf_index < BUFFER_MAX_SIZE))
    out_buf[out_buf_index++] = '\n';

  // Mutate next elements
  mutate(element->next);
}

static void mutate_all(const int total_mutate_element_num) {
  out_buf_index = 0;

  select_mutate_elements(total_mutate_element_num);

  /*
  printf("\n\tmutate ids: ");

  int i;
  for (i = 0; i < mutate_elements_size; ++i) {
    printf("%d  ", mutate_id_arr[i]);
  }

  printf("\n");
  */

  mutate(john_p);

  // Set the end of out_buf = '\0';
  out_buf[out_buf_index] = '\0';
}


// file_name="7z_fmt.xml"
void save_to_file(const int iteration_id, const char* const buffer, const int buffer_size) {
  snprintf(file_name_buf, FILE_NAME_SIZE, "%s/%d", folder_name_buf, iteration_id);
  // printf("file_name_buf=%s\n\n", file_name_buf);

  FILE *file = fopen(file_name_buf, "w");
  fwrite(buffer, sizeof(char), buffer_size, file);
  fclose(file);
}


// file_name="7z_fmt.xml"
// file_name_buf="7z_fmt"
// folder=out_dir + "7z_fmt"
int create_folder(const char* const out_dir, const char* const file_name) {
  char *p = (char*)out_dir;
  int i = 0;
  while (i < FOLDER_NAME_SIZE && '\0' != *p) {
    folder_name_buf[i++] = *(p++);
  }

  // If the out_dir ends without '\', add to folder_name_buf
  if (i > 0 && '/' != folder_name_buf[i - 1]) folder_name_buf[i++] = '/';

  p = (char*)file_name;
  while (i < FOLDER_NAME_SIZE && '\0' != *p && '.' != *p) {
    folder_name_buf[i++] = *(p++);
  }

  folder_name_buf[i] = '\0';
	
  printf("folder_name_buf=%s\n", folder_name_buf);

  if (0 != access(folder_name_buf, R_OK | W_OK)) {

    if(0 != mkdir(folder_name_buf, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
      printf("Failed to create folder:%s\n", folder_name_buf);
      return 1;
    }
  }

  return 0;
}

/*************************
 * John the ripper fuzz  *
 *************************/

int john_fuzz(char **argv, const u8 * const in_buf, const s32 len,
	      const char * const out_dir, const char * const file_name,
	      const u8 is_save_cases) {
  printf("\n\t====john_fuzz()====\n\n");

  srand (time(NULL));

  // printf("out_dir=%s\n", out_dir);
  // printf("file_name=%s\n", file_name);
  // printf("is_save_cases=%d\n", is_save_cases);

  if (is_save_cases && (0 != create_folder(out_dir, file_name)))
    return 1;

  // printf("\nin_buf=%s\n", in_buf);

  int total_mutate_element_num; // The total number of mutate elements
  int case_number;              // Default is 10000

  if (0 != john_parse(in_buf, len, &john_p, &total_mutate_element_num, &case_number)) {
    printf("\n\tjohn_parse() failed\n\n");
    return 1;
  }

  // printf("total_mutate_element_num=%d\n", total_mutate_element_num);

  // First iteration, Do not mutate content 
  common_fuzz_stuff(argv, out_buf, out_buf_index);

  int iteration_id = 1;

  while (iteration_id < case_number + 1) {

    printf("\n\t%s--[%d]\n", file_name, iteration_id);

    // FUZZ until user cancel

    mutate_all(total_mutate_element_num);

    // printf("out_buf=%s\n", out_buf);

    common_fuzz_stuff(argv, out_buf, out_buf_index);

    if (is_save_cases)
      save_to_file(iteration_id, out_buf, out_buf_index);

    iteration_id++;
  }
  
  john_free(john_p);

  return 0;
}




