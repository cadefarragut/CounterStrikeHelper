#include <iostream>
#include <cstdlib>
#include <string>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "include/httplib.h"

using namespace std;
using namespace httplib;

string getenv_string(const char* key){
  const char* env = getenv(key);
  if(!env){
    cout << "Incorrect key name: " << env << endl;
    exit(1);
  }
  return string(env);
}

int main(){
  string web_hook = getenv_string("DISCORD_WEBHOOK_URL");
  string leetify_api = getenv_string("LEETIFY_API_KEY");

  Client cli("https://discord.com");
  auto res = cli.Post(
      web_hook,
      R"({"content":"Test"})",
      "application/json"
      );

  if(res){
    cout << "Status: " << res->status << endl; 
  } else {
    cout << "error sending POST" << endl;
  }

  
  return 0;
}
