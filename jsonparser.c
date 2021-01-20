//#include "jsonparser.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


struct Pair {
 unsigned char * key;
 unsigned char * value;
};

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

// Returns -1 if char is not found
int index_of(unsigned char * string, int size, unsigned char character, bool lastChar){
if (lastChar) {
for(int i=size; i != 0; i--){
 if(string[i] == character) {
	 return i;
 }
}
}
for(int i=0; i < size; i++){
 if(string[i] == character) {
	 return i;
 }
}
return -1;
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

int* 
getDoubleQuotesPositions (unsigned char *json, int size) 
{
int qCount = 0;
for (int i=0; i<size; i++) {
if(json[i] == '"' && json[i-1] != '\\'){
qCount++;
}
}
int *qPositions = calloc(qCount, sizeof(int));
printf("quotes counted: %d \n", qCount);
int qPosition = 0;
for (int i=0; i<size; i++) {
if(json[i]== '"' && json[i-1] != '\\'){
 qPositions[qPosition] = i;
 qPosition++;
 }
}

return qPositions;
}

unsigned char* substring(unsigned char *string, int size, int start, int end){
if (end < start) return NULL;
	
unsigned char *substring = calloc(end-start, sizeof(unsigned char));
int substringIndex = 0;
for(int i=0; i<size; i++){
 if(i > start && i < end){
   substring[substringIndex] = string[i];
   substringIndex++;
 }
}
return substring;
}

void
parse_hack_chat_json (unsigned char *json, unsigned char *nameMessageString)
{
  unsigned char jsonText[100] = "{\"name\":\"Alex\\\"andr\", \"age\":\"27\", \"email\":\"alex@email.net\"}\n";
  int jsonSize = sizeof(jsonText);

  bool isValidKey = validate_json_key("\"name\"", 6);
  bool isValidValue = validate_json_value("\"age\"", 5);
  
  int index = index_of("abcdefgh", 8, 'a', false);
  int indexSecond = index_of("12345567890", 10, '5', true);
  
  printf("is valid key: %s \n", isValidKey ? "true" : "false");
  printf("is valid value: %s \n", isValidValue ? "true" : "false");

  int *qPositions = getDoubleQuotesPositions(jsonText, jsonSize);
  int qPositionsSize = 12; 
  int pairsCount = qPositionsSize / 4;
  
  struct Pair pair;
  struct Pair * pairs = calloc(pairsCount, sizeof(pair));
  
  for(int i=0, y=0; i < pairsCount; i++) {
   pair.key = substring(jsonText, jsonSize, qPositions[y], qPositions[y+1]);
   pair.value = substring(jsonText, jsonSize, qPositions[y+2], qPositions[y+3]);
   pairs[i] = pair;
   y += 4;
  }
    
  for(int i=0; i < pairsCount; i++){
   printf("key: %s \n",  pairs[i].key);
   printf("value: %s \n",  pairs[i].value);
   free(pairs[i].key);
   free(pairs[i].value);
  }

   free(pairs); 
   free(qPositions);
}

int main(){
parse_hack_chat_json(NULL, NULL);
return 0;
}
