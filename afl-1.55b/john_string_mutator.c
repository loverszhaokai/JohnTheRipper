#include "john.h"

#include <stdio.h>

typedef void (*RAND_TWO_BYTE_fun) ();

static unsigned char two_hex_byte[5]; // JOHN_HEX_STR_*
static unsigned char two_byte[2];     // JOHN_STR

static void rand_two_byte() {
  unsigned short r = (unsigned short)random_int(0, 65536);
  two_byte[0] = r / 256;
  two_byte[1] = r % 256;
}

static void rand_two_hex_byte() {
  unsigned short r = (unsigned short)random_int(0, 65536);
  if (0 == r % 2)
    sprintf(two_hex_byte, "%04x", r);
  else 
    sprintf(two_hex_byte, "%04X", r);
}

static void rand_two_hex_byte_u() {
  unsigned short r = (unsigned short)random_int(0, 65536);
  sprintf(two_hex_byte, "%04X", r);
}

static void rand_two_hex_byte_l() {
  unsigned short r = (unsigned short)random_int(0, 65536);
  sprintf(two_hex_byte, "%04x", r);
}

static void rand_hex_str_impl(struct JohnElement *element, const int min_len, 
			      const int max_len, 
			      const RAND_TWO_BYTE_fun rand_two_hex_byte_fun) {
  // printf("rand_hex_str_impl(%d, %d)\n", min_len, max_len);

  const int len = random_int(min_len, max_len + 1);

  // printf("len=%d\n", len);

  char *hex_str = (char*)malloc((1 + len )* sizeof(char));
	
  int i;
  for (i = 0; i < len / 4; ++i) {
    rand_two_hex_byte_fun();
    hex_str[4 * i] = two_hex_byte[0];
    hex_str[4 * i + 1] = two_hex_byte[1];
    hex_str[4 * i + 2] = two_hex_byte[2];
    hex_str[4 * i + 3] = two_hex_byte[3];
  }

  if (0 != len % 4) {
    rand_two_hex_byte_fun();
    for (i = 0; i < len % 4; ++i) {
      hex_str[len - len % 4 + i] = two_hex_byte[i];
    }
  }

  hex_str[len] = '\0';

  // printf("str=%s||\n\n", hex_str);

  element->mutated_value = hex_str;
  element->mutated_value_size = len;
}

static void rand_hex_str(struct JohnElement *element, const int min_len, 
			 const int max_len) {
  // printf("\n\trand_hex_str(%d, %d)\n", min_len, max_len);
  rand_hex_str_impl(element, min_len, max_len, rand_two_hex_byte);
}

static void rand_hex_str_u(struct JohnElement *element, const int min_len, 
			   const int max_len) {
  // printf("\n\trand_hex_str_u(%d, %d)\n", min_len, max_len);
  rand_hex_str_impl(element, min_len, max_len, rand_two_hex_byte_u);
}

static void rand_hex_str_l(struct JohnElement *element, const int min_len, 
			   const int max_len) {
  // printf("\n\trand_hex_str_l(%d, %d)\n", min_len, max_len);
  rand_hex_str_impl(element, min_len, max_len, rand_two_hex_byte_l);
}

static void rand_str(struct JohnElement *element, const int min_len, 
		     const int max_len) {
  // printf("rand_str(%d, %d)\n", min_len, max_len);

  const int len = random_int(min_len, max_len + 1);

  // printf("len=%d\n", len);
	
  char *str = (char*)malloc((1 + len )* sizeof(char));

  int i;
  for (i = 0; i < len / 2; ++i) {
    rand_two_byte();
    str[2 * i] = two_byte[0];
    str[2 * i + 1] = two_byte[1];
  }
  if (0 != len % 2) {
    rand_two_byte();
    str[len - 1] = two_byte[0];
  }

  str[len] = '\0';

  // printf("str=%s||\n\n", str);
  element->mutated_value = str;
  element->mutated_value_size = len;
}

static void rand_num_str(struct JohnElement *element, const int min_len, 
			 const int max_len) {
  // printf("rand_str(%d, %d)\n", min_len, max_len);

  const int len = random_int(min_len, max_len + 1);

  // printf("len=%d\n", len);

  char *num_str = (char*)malloc((1 + len )* sizeof(char));
	
  int i, t;
  char num_buf[5];
  for (i = 0; i < len / 4; ++i) {

    t = random_int(0 ,10000); // From 0 to 9999
    sprintf(num_buf, "%04d", t);

    num_str[4 * i] = num_buf[0];
    num_str[4 * i + 1] = num_buf[1];
    num_str[4 * i + 2] = num_buf[2];
    num_str[4 * i + 3] = num_buf[3];
  }

  if (0 != len % 4) {

    t = random_int(0 ,10000); // From 0 to 9999
    sprintf(num_buf, "%04d", t);

    for (i = 0; i < len % 4; i++) {
      num_str[len - len % 4 + i] = num_buf[i];
    }
  }

  num_str[len] = '\0';

  element->mutated_value = num_str;
  element->mutated_value_size = len;
}

int mutate_string_element(struct JohnElement *element) {
  free(element->mutated_value);
  element->mutated_value = NULL;

  if (JOHN_HEX_STR == element->type)
    rand_hex_str(element, element->length.min_len, element->length.max_len);
  else if (JOHN_HEX_STR_U == element->type)
    rand_hex_str_u(element, element->length.min_len, element->length.max_len);
  else if (JOHN_HEX_STR_L == element->type)
    rand_hex_str_l(element, element->length.min_len, element->length.max_len);
  else if (JOHN_STR == element->type)
    rand_str(element, element->length.min_len, element->length.max_len);
  else if (JOHN_NUM == element->type)
    rand_num_str(element, element->length.min_len, element->length.max_len);
  else 
    rand_str(element, element->length.min_len, element->length.max_len);

  return 0;
}
