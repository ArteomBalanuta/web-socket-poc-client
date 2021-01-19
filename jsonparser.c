#include "jsonparser.h"

int
count_double_quotes (unsigned char *string, int size)
{
  int quotesCount = 0;
  for (int i = 0; i < size - 1; i++)
    {
      if (string[i] == '"')
	{
	  quotesCount++;
	}
    }
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
    }
  return false;
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
		if(value[i] == '"' && value[i-1] != '\'){
		return false;
		}
	}
	}	
	return true;
}

void
parse_hack_chat_json (unsigned char *json, unsigned char *nameMessageString)
{
  char[6] trip = "";
  char[20] name = "";
  char[40] message = "";

  



}
