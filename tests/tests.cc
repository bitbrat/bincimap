#include "framework.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(void)
{
  setenv("BINCIMAP_LOGIN", "LOGIN+1", 1);

  FrameWork::setConfig("Mailbox", "path", "Maildir");
  FrameWork::setConfig("Mailbox", "auto create inbox", "yes");
  FrameWork::setConfig("Mailbox", "umask", "077");

  FrameWork f("../src/bincimapd");
  
  f.test("", "1 OK LOGIN completed\r\n");
  f.test("1 NOOP\r\n", "1 OK NOOP completed\r\n");
  f.test("1 CREATE INBOX/TestMailbox\r\n", "1 OK CREATE completed\r\n");
  f.test("1 DELETE INBOX/TestMailbox\r\n", "1 OK DELETE completed\r\n");
  f.test("X LOGOUT\r\n", "X OK LOGOUT completed\r\n");

  return 0;
}
