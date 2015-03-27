/** 
 * section: 	XPath
 * synopsis: 	Evaluate XPath expression and prints result node set.
 * purpose: 	Shows how to evaluate XPath expression and register 
 *          	known namespaces in XPath context.
 * usage:	xpath1 <xml-file> <xpath-expr> [<known-ns-list>]
 * test:	xpath1 test3.xml '//child2' > xpath1.tmp && diff xpath1.tmp $(srcdir)/xpath1.res
 * author: 	Aleksey Sanin
 * copy: 	see Copyright for the status of this software.
 */

#include "john.h"
#include "alloc-inl.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#define STRING_MAX_SIZE 10000
#define BUFFER_MAX_SIZE 2000000000

#if defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_SAX1_ENABLED)

static int element_id = 0;
static int mutate_element_id = 0;
static struct JohnElement **current_element = NULL;

static struct JohnElement * new_john_element() {
  struct JohnElement *element = (struct JohnElement*)malloc(sizeof(struct JohnElement));

  element->element_id = element_id++;
  element->is_mutate = 0;
  element->mutate_element_id = 0;
  element->orig_value = NULL;
  element->mutated_value = NULL;
  element->mutated_value_size = 0;

  element->length.min_len = 0;
  element->length.max_len = STRING_MAX_SIZE;

  element->type = JOHN_NONE;

  element->children = NULL;
  element->next = NULL;

  element->is_top_dataelement = 0;
  element->element_type = JOHN_E_NONE;

  return element;
}

static int get_string_type(xmlNodePtr node, struct JohnElement *string_element) {
  xmlChar *ret = xmlGetProp(node, "type");
  if (NULL == ret) {
    // default type
    string_element->type = JOHN_STR;
  } else {

    if (7 == strlen(ret) && (0 == strncmp("HEX_STR", ret, 7))) {
      string_element->type = JOHN_HEX_STR;
    } else if (9 == strlen(ret) && (0 == strncmp("HEX_STR_U", ret, 9))) {
      string_element->type = JOHN_HEX_STR_U;
    } else if (9 == strlen(ret) && (0 == strncmp("HEX_STR_L", ret, 9))) {
      string_element->type = JOHN_HEX_STR_L;
    } else if (3 == strlen(ret) && (0 == strncmp("STR", ret, 3))) {
      // default type
      string_element->type = JOHN_STR;
    } else if (3 == strlen(ret) && (0 == strncmp("NUM", ret, 3))) {
      string_element->type = JOHN_NUM;
    } else if (0 == strlen(ret)) {
      // default type
      string_element->type = JOHN_STR;
    } else {
      printf("currently does not support type=%s\n", ret);
      return 1;
    }
  }
  xmlFree(ret);

  return 0;
}

static int get_string_length(xmlNodePtr node, struct JohnElement *string_element) {
  xmlChar *ret = xmlGetProp(node, "length");

  if (NULL == ret) {
    string_element->length.min_len = 0;
    string_element->length.max_len = STRING_MAX_SIZE;
  } else {
    // case 1: length = "[1,2]"
    // case 2: length = "[,2]"
    // case 3: length = "[1,]"
    // case 4: length = "40"
    // TBD: here should be more robust

    int min_len = 0;
    int max_len = STRING_MAX_SIZE;

    char first_num[100] = { 0 };
    char second_num[100] = { 0 };

    int first_begin = 0;
    int first_end = 0;

    int i;
    int first_index = 0;
    int second_index = 0;
    for (i = 0; i < strlen(ret); ++i) {
      if (!first_begin) {
	if ('[' == ret[i]) first_begin = 1;
      } else if (!first_end) {
	if (',' == ret[i]) first_end = 1;
	else if (' ' != ret[i]) first_num[first_index++] = ret[i];
      } else if (first_end) {
	if (']' == ret[i]) break;
	else if (' ' != ret[i]) second_num[second_index++] = ret[i];
      }

      if (99 == first_index) first_end = 1;
      if (99 == second_index) break;
    }

    if (first_begin) {
      // case 1,2,3
      const int j = atoi(first_num);
      if (j > min_len) min_len = j;

      const int k = atoi(second_num);
      if (0 != k && k < max_len) max_len = k;
    } else {
      // case 4: length = "40"
      const int j = atoi(ret);
      min_len = max_len = j;
    }

    string_element->length.min_len = min_len;
    string_element->length.max_len = max_len;

    // printf("ret=%s\n", ret);
    // printf("first_num=%s\n", first_num);
    // printf("second_num=%s\n", second_num);
    // printf("min_len=%d\n", min_len);
    // printf("max_len=%d\n", max_len);
  }
  xmlFree(ret);

  return 0;
}

static int get_string_is_mutate(xmlNodePtr node, struct JohnElement *string_element) {
  xmlChar *ret = xmlGetProp(node, "is_mutate");
  if (NULL == ret) {
    string_element->is_mutate = 1;
  } else {
    if (5 == strlen(ret) && (0 == strncmp("false", ret, 5)))
      string_element->is_mutate = 0;
    else if (4 == strlen(ret) && (0 == strncmp("true", ret, 4)))
      string_element->is_mutate = 1;
    else if(0 != strlen(ret)) { 
      printf("currently does not support is_mutate = '%s'\n", ret);
      return 1;
    }
  }
  xmlFree(ret);

  if (string_element->is_mutate) string_element->mutate_element_id = ++mutate_element_id;

  return 0;
}

static int create_string_element(xmlNodePtr node) {
  struct JohnElement * string_element = new_john_element();
  *current_element = string_element;
  current_element = &(string_element->next);

  string_element->element_type = JOHN_E_STRING;

  // printf("\t= element node \"%s\"\n", node->name);

  if (1 == get_string_type(node, string_element))
    return 1;

  if (1 == get_string_length(node, string_element))
    return 1;

  if (1 == get_string_is_mutate(node, string_element))
    return 1;

  // Get String Content
  xmlChar *p_value = xmlNodeGetContent(node);

  string_element->orig_value = strdup(p_value);

  xmlFree(p_value);

  return 0;
}

static int create_john_element(xmlNodePtr node) {
  if (NULL == node) {
    *current_element = NULL; // Set NULL
    return 0;
  }

  // JohnElement
  struct JohnElement * element = new_john_element();
  *current_element = element; // First called to: john_e->children = element
  current_element = &(element->children);
  element->element_type = JOHN_E_DATAELEMENT;

  xmlNodePtr child_node = node->children;
  while (NULL != child_node) {

    if (xmlNodeIsText(child_node)) goto next;

    if (xmlStrEqual("DataElement", child_node->name)) {
      // current_element points to the element->children
      if (1 == create_john_element(child_node))
	return 1;

    } else if (xmlStrEqual("string", child_node->name)) {
      if (1 == create_string_element(child_node))
	return 1;
    }

  next:
    child_node = child_node->next;
  }

  // Check child node finished 

  // current_element points to the last children of element
  *current_element = NULL; 
  // current_element points to the next of element
  current_element = &(element->next);

  return 0;
}

static int gather_xpath_nodes(xmlNodeSetPtr nodes) {
  xmlNodePtr cur;
  int size;
  int i;
    
  size = (nodes) ? nodes->nodeNr : 0;

  // printf("Result (%d nodes):\n", size);

  for(i = 0; i < size; ++i) {
    assert(nodes->nodeTab[i]);

    cur = nodes->nodeTab[i];   	    

    if (1 == create_john_element(cur))
      return 1;
  }
  return 0;
}


static int gather_dataelements(xmlDocPtr doc) {
  /* Create xpath evaluation context */
  xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
  if(xpathCtx == NULL) {
    fprintf(stderr,"Error: unable to create new XPath context\n");
    xmlFreeDoc(doc); 
    return(-1);
  }
    
  /* Evaluate xpath expression */
  xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression("/AFL/DataElement", xpathCtx);
  if(xpathObj == NULL) {
    fprintf(stderr,"Error: unable to evaluate xpath expression \"/AFL/DataElement\"\n");
    xmlXPathFreeContext(xpathCtx); 
    xmlFreeDoc(doc); 
    return(-1);
  }

  if (1 == gather_xpath_nodes(xpathObj->nodesetval)) return 1;

  /* Cleanup */
  xmlXPathFreeObject(xpathObj);
  xmlXPathFreeContext(xpathCtx); 

  return 0;
}


static int gather_afl(xmlDocPtr doc, int *case_number) {
  /* Create xpath evaluation context */
  xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
  if(xpathCtx == NULL) {
    fprintf(stderr,"Error: unable to create new XPath context\n");
    xmlFreeDoc(doc); 
    return(-1);
  }
    
  /* Evaluate xpath expression */
  xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression("/AFL", xpathCtx);
  if(xpathObj == NULL) {
    fprintf(stderr,"Error: unable to evaluate xpath expression \"/AFL\"\n");
    xmlXPathFreeContext(xpathCtx); 
    xmlFreeDoc(doc); 
    return(-1);
  }

  *case_number = 1000;

  const xmlNodePtr afl_node = xpathObj->nodesetval->nodeTab[0];
	
  xmlChar *ret = xmlGetProp(afl_node, "case_number");
  if (NULL != ret) {
    *case_number = atoi(ret);
  }
  xmlFree(ret);

  /* Cleanup */
  xmlXPathFreeObject(xpathObj);
  xmlXPathFreeContext(xpathCtx); 

  return 0;
}


static int execute_xpath_expression(const char* file_content, const int file_content_size, 
				    int *case_number) {
  xmlDocPtr doc;

  assert(file_content);

  /* Load XML document */
  doc = xmlParseMemory(file_content, file_content_size);
  if (doc == NULL) {
    fprintf(stderr, "Error: unable to parse file \"%s\"\n", file_content);
    return(-1);
  }

  if (0 != gather_afl(doc, case_number)) return 1;

  if (0 != gather_dataelements(doc)) return 1;

  xmlFreeDoc(doc); 
    
  return(0);
}


static void print_john_elements(struct JohnElement* element) {
  printf("\n\n");
	
  if (NULL == element) return;

  printf("address           =%p\n", element);
  printf("element_id        =%d\n", element->element_id);
  printf("is_mutate         =%d\n", element->is_mutate);
  printf("mutate_element_id =%d\n", element->mutate_element_id);
  printf("orig_value        =%s\n", element->orig_value);
  printf("mutated_value     =%s\n", element->mutated_value);
  printf("mutated_value_size=%d\n", element->mutated_value_size);
  printf("min_len           =%d\n", element->length.min_len);
  printf("max_len           =%d\n", element->length.max_len);
  printf("type              =%d\n", element->type);
  printf("is_top_dataelement=%d\n", element->is_top_dataelement);
  printf("element_type      =%d\n", element->element_type);

  print_john_elements(element->children);
  print_john_elements(element->next);
}

static void init_top_element(struct JohnElement *john_e) {
  struct JohnElement *top_dataelement = john_e->children;
  while (NULL != top_dataelement) {
    top_dataelement->is_top_dataelement = 1;
    top_dataelement = top_dataelement->next;
  }
}

int john_parse(const u8 * const file_content, const int file_content_size,
	       struct JohnElement **john_e, int *total_mutate_element_num,
	       int *case_number) {
  element_id = 0;
  mutate_element_id = 0;
  current_element = NULL;

  *john_e = new_john_element();
  current_element = &((*john_e)->children);

  /* Init libxml */     
  xmlInitParser();
  LIBXML_TEST_VERSION

    /* Do the main job */
    if(execute_xpath_expression(file_content, file_content_size, case_number) != 0) {
      printf("%s format is invalic\n", file_content);
      return(-1);
    }

  *total_mutate_element_num = mutate_element_id;

  init_top_element(*john_e);

  /* Shutdown libxml */
  xmlCleanupParser();
    
  /*
   * this is to debug memory for regression tests
   */
  xmlMemoryDump();

  // printf("\n\t+++++print_john_element()+++++\n\n");
  // print_john_elements(*john_e);
  // sleep(5);

  return 0;
}


#else
int john_parse(void) {
  fprintf(stderr, "XPath support not compiled in\n");
  exit(1);
}
#endif
