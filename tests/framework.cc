
/*-*-mode:c++-*-*/
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <wait.h>
#include <errno.h>
#include "framework.h"
#include "../src/convert.h"
#include "../src/regmatch.h"
#include "../src/tools.h"

using namespace ::std;
using namespace ::Binc;


FrameWork::FrameWork(const std::string &binary)
{
  pipe(readpipe);
  pipe(writepipe);

  cout << "Starting " << binary << "." << endl;

  if ((childspid = fork()) == 0) {
    dup2(writepipe[0], 0);
    dup2(readpipe[1], 1);
    close(writepipe[1]);
    close(readpipe[0]);

    char *cwd = getcwd(0, 0);
    string dest = cwd;
    dest += "/";
    dest += binary;

    execl(dest.c_str(), 0);
    exit(2);
  }
}

bool FrameWork::test(const std::string &request, const std::string &result)
{
  string tmp = request;
  trim(tmp);
  string tmp2 = result;
  trim(tmp2);

  if (request != "") {
    ssize_t res = write(writepipe[1], request.c_str(), request.length());
    if (res != (ssize_t) request.length()) {
      printf("test(\"%s\", \"%s\") failed: %s\n", tmp.c_str(), tmp2.c_str(), strerror(errno));
      return false;
    }
  }
  
  char c;
  string got;

  while (read(readpipe[0], &c, 1) == 1 && c != '\n')
    got += c;
  got += '\n';

  if (got != result) {
    printf("test(\"%s\", \"%s\") failed: got %s\n", tmp.c_str(), tmp2.c_str(), got.c_str());
    return false;
  }

  printf("test(\"%s\", \"%s\") ok.\n", tmp.c_str(), tmp2.c_str());
  return true;
}


FrameWork::~FrameWork(void)
{
  kill(childspid, SIGTERM);
  int result;
  wait(&result);
}

void FrameWork::setConfig(const std::string &section, const string &key, const string &value)
{
  Tools::getInstance().setenv("BINCIMAP_GLOBALCONFIG_" 
			      + toHex(section) + "::" + toHex(key),
			      toHex(value));
}
