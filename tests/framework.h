#ifndef FRAMEWORK_H_INCLUDED
#define FRAMEWORK_H_INCLUDED

/*-*-mode:c++-*-*/
#include <string>

class FrameWork
{
 public:
  FrameWork(const std::string &binary);
  ~FrameWork(void);

  bool test(const std::string &request, const std::string &result);

  static void setConfig(const std::string &section, const std::string &key, const std::string &value);


 private:
  int childspid;
  int readpipe[2];
  int writepipe[2];
};

#endif
