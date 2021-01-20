//#include "jsonparser.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

int
count_double_quotes (unsigned char *string, int size)
{
  int quotesCount = 0;
  for (int i = 0; i < size; i++)
      if (string[i] == '"') quotesCount++;

  printf("double quotes count: %d \n", quotesCount);
  return quotesCount;
}

bool
validate_json_key (unsigned char *key, int size)
{
  if (count_double_quotes (key, size) == 2 
	&& key[0] == '"' && key[size - 1] == '"')
	{
	  return true;
	}
    
return false;
}

int index_of(unsigned char * string, int size,  char character){
int i = -1;
for(int i=0; i<size; i++){
 if(){
 }
}
return ;
}

bool
validate_json_value (unsigned char *value, int size)
{
	if(count_double_quotes (value, size) == 2
	  && value[0] == '"' && value[size-1] == '"')
	{
	return true;
	}

	if(count_double_quotes (value, size) > 2)
	{
	for(int i=1; i<size-2; i++){
		if(value[i] == '"' && value[i-1] != '\\'){
		return false;
		}
	}
	}	
	return false;
}

void
parse_hack_chat_json (unsigned char *json, unsigned char *nameMessageString)
{
  char jsonText[100] = "{\"name\":\"Alexandr\", \"age\":\"27\"}";
  char name[20] = "";
  char age[40] = "";

  bool isValidKey = validate_json_key("\"name\"", 6);
  bool isValidValue = validate_json_value("\"age\"", 5);

  printf("is valid key: %s \n", isValidKey ? "true" : "false");
  printf("is valid value: %s \n", isValidValue ? "true" : "false");
}

int main(){
parse_hack_chat_json(NULL, NULL);
return 0;
}
