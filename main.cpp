#include <iostream>
#include <cstdlib>
#include <string>
#include "include/httplib.h"

using namespace std;

int main(){
  const char* web_hook = getenv("DISCORD_WEBHOOK_URL");
  const char* leetify_api = getenv("LEETIFY_API_KEY");

  cout << "Hello World" << endl;
  
  return 0;
}
