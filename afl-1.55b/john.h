#ifndef _AFL_JOHN_H_
#define _AFL_JOHN_H_

#include "types.h"


enum JohnElementType {

JOHN_E_NONE,             // NONE, init value
JOHN_E_DATAELEMENT,      // DataElement
JOHN_E_STRING,           // string

};

enum JohnType {

JOHN_NONE,              // DataElement field's type is NONE

JOHN_HEX_STR,           // Hex string contains upper case and lower case
JOHN_HEX_STR_U,         // Hex string upper case only
JOHN_HEX_STR_L,         // Hex string lower case only

JOHN_STR,               // String

JOHN_NUM,               // String contains only numbers 0-9

};


// Describle the length of element
struct JohnLength {

int min_len;                      // The min length of current element
int max_len;                      // The max length of current element

};


// Describe the element
struct JohnElement {

  int element_id;                    // Identifier of current element
  int is_mutate;                     // Is current element should be mutated
  int mutate_element_id;

  char *orig_value;                  // Original value of the input file, NEVER changed once init
  char *mutated_value;               // Mutated value of the input file, Changed each iteration
  int mutated_value_size;            // The size of mutated_value, strlen() is not the right size of mutated_value
	                             // for rand_str() may generate a string with '\0' inside

  struct JohnLength length;
  enum JohnType type;                

  struct JohnElement *children;      // Children elements
  struct JohnElement *next;          // Next neighbor element

  int is_top_dataelement;            // Is top DataElement, xpath="/AFL/DataElement"
  enum JohnElementType element_type; // Such as, DataElement, string
};

// Parse the xml file to struct JohnElement
int john_parse(const u8 * const file_content, const int file_content_size,
	       struct JohnElement **john_e, int *total_mutate_element_num,
	       int *cn);


int random_int(const int min_v, const int max_v);

/*************************
 * John the ripper fuzz  *
 *************************/
int john_fuzz(char **argv, const u8 * const in_buf, const s32 len,
	      const char * const out_dir, const char * const file_name,
	      const u8 is_save_cases);



// =======afl-fuzz.c========

u8 common_fuzz_stuff(char** argv, u8* out_buf, u32 len);

// =======afl-fuzz.c========



#endif // _AFL_JOHN_H_
