/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    storage.cc
 *  
 *  Description:
 *    Implementation of the Binc::Storage format.
 *
 *  Authors:
 *    Andreas Aardal Hanssen <andreas-binc@bincimap.org>
 *
 *  Bugs:
 *
 *  ChangeLog:
 *
 *  --------------------------------------------------------------------
 *  Copyright 2002-2004 Andreas Aardal Hanssen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *  --------------------------------------------------------------------
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "storage.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <map>
#include <string>
#include "../src/convert.h"

using namespace ::std;
using namespace ::Binc;

class Lexer {
public:
  Lexer(FILE *fp);

  unsigned int getCurrentColumn() const { return col; }
  unsigned int getCurrentLine() const { return line; }
  const string &getLastError() const { return lastError; }

  enum Type {
    LeftCurly, RightCurly, Equals, Comma, QMark, Colon, Semicolon,
    Text, EndOfFile, Error
  };

  bool nextToken(string *token, Type *type);

protected:
  enum State {
    Searching, RestOfLineComment, CStyleComment,
    QuotedString, HexString, TextString
  };

private:
  FILE *fp;
  State state;
  unsigned int line;
  unsigned int col;
  string lastError;
};

//------------------------------------------------------------------------
Lexer::Lexer(FILE *f) : fp(f), state(Searching), line(0), col(0)
{
}

//------------------------------------------------------------------------
bool Lexer::nextToken(string *token, Type *type)
{
  int lastc = '\0';

  string buffer;
  bool escaped = false;

  for (;;) {
    int c = fgetc(fp);
    if (c == EOF) {
      if (ferror(fp)) {
	lastError = strerror(errno);
	*type = Error;
	return false;
      }

      if (state != Searching) {
	lastError = "unexpected end of file";
	*type = Error;
	return false;
      }

      *type = EndOfFile;
      return true;
    }

    if (c == '\n') {
      ++line;
      col = 0;
    } else
      ++col;

    switch (state) {
    case Searching:
      // If whitespace, keep searching
      if (c == ' ' || c == '\t' || c == '\r' || c == '\n'
	  || c == '\f' || c == '\v');
      else if (col == 0 && (c == '#' || c == ';'))
	state = RestOfLineComment;
      else if (lastc == '/' && c == '/')
	state = RestOfLineComment;
      else if (lastc == '/' && c == '*')
	state = CStyleComment;
      else if (lastc == '*' && c == '/' && state == CStyleComment) {
	state = Searching;
	c = '\0'; // avoid ambiguity
      } else if (c == '?') {
	*token = "?";
	*type = QMark;
	return true;
      } else if (c == ':') {
	*token = ":";
	*type = Colon;
	return true;
      } else if (c == ';') {
	*token = ";";
	*type = Semicolon;
	return true;
      } else if (c == '{') {
	*token = "{";
	*type = LeftCurly;
	return true;
      } else if (c == '}') {
	*token = "}";
	*type = RightCurly;
	return true;
      } else if (c == ',') {
	*token = ",";
	*type = Comma;
	return true;
      } else if (c == '=') {
	*token = "=";
	*type = Equals;
	return true;
      } else if (c == '\"') {
	state = QuotedString;
      } else if (c == '<') {
	state = HexString;
      } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
		 || (c >= '0' && c <= '9') || c == '.' || c == '_'
		 || c == '-' || c == '^') {
	state = TextString;
	buffer = "";
	buffer += (char) c;
      } else if (c != '/') {
	// Syntax error
	lastError = "unexpected character '";
	lastError += (char) c;
	lastError += "'";
	return false;
      }

      break;
    case RestOfLineComment:
      if (c == '\n')
	state = Searching;
      break;
    case CStyleComment:
      if (c == '/' && lastc == '*')
	state = Searching;
      break;
    case QuotedString:
      if (escaped) {
	buffer += (char) c;
	escaped = false;
      } else if (c == '\\') {
	escaped = true;
      } else if (c == '\"') {
	*token = buffer;
	*type = Text;
	buffer = "";
	state = Searching;
	return true;
      } else {
	buffer += (char) c;
      }

      break;
    case TextString:
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
	  || (c >= '0' && c <= '9') || c == '_' 
	  || c == '.' || c == '-' || c == '^')
	buffer += (char) c;
      else {
	*token = buffer;
	buffer = "";
	*type = Text;
	ungetc(c, fp);
	state = Searching;
	return true;
      }
      break;
    case HexString:
      if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')
	  || (c >= '0' && c <= '9'))
	buffer += (char) c;
      else if (c == '>') {
	*token = fromHex(buffer);
	buffer = "";
	*type = Text;
	state = Searching;
	return true;
      } else {
	lastError = "expected a-f, A-F, 0-9 or '>'";
	return false;
      }
      break;
    };

    lastc = c;
  }
}

//------------------------------------------------------------------------
Storage::Storage(const string &f, Mode m) 
  : fp(0), fd(0), fileName(f), state(Searching), mode(m),
    lastError("unknown error"), atEndOfFile(false), firstSection(true)
{
  if (m == ReadOnly) {
    if ((fp = fopen(f.c_str(), m == ReadOnly ? "r" : "w")) == 0) {
      lastError = "when opening \"" + f + "\": ";
      lastError += strerror(errno);
    }
  } else {
    // create a safe file name
    string tpl = fileName + "XXXXXX";
    char *ftemplate = new char[tpl.length() + 1];
    if (ftemplate == 0) {
      lastError = "out of memory, couldn't create safe filename";
      return;
    }

    if (ftemplate) {
      strcpy(ftemplate, tpl.c_str());
      while ((fd = mkstemp(ftemplate)) == 0 && errno == EEXIST) {
      }

      if (fd == 0) {
	lastError = "when opening \"" + f + "\": ";
	lastError += strerror(errno);
      }
    }

    fileName = ftemplate;
  }
}

//------------------------------------------------------------------------
Storage::~Storage(void)
{
  if (fp)
    fclose(fp);
  if (fd)
    close(fd);
}

//------------------------------------------------------------------------
bool Storage::get(string *section, string *key, string *value)
{
  if (mode == WriteOnly) {
    lastError = "unable to read from \"" + fileName + "\" when"
      " in WriteOnly mode";
    return false;
  }

  if (!fp)
    return false;

  string keytmp;
  string valuetmp;
  
  Lexer lex(fp);
  Lexer::Type type;
  string token;
  while (lex.nextToken(&token, &type) && type != Lexer::Error) {
    if (type == Lexer::EndOfFile) {
      if (state != Searching) {
	lastError = "unexpected end of file";
	return false;
      }

      atEndOfFile = true;
      return false;
    }

    // convert from alias to token
    if (type == Lexer::Text && aliases.find(token) != aliases.end())
	token = aliases[token];

    switch (state) {
    case Searching:
      if (type == Lexer::QMark) {
	state = AliasKey;
      } else if (type != Lexer::Text) {
	lastError = "expected text";
	lastError += " at line " + toString(lex.getCurrentLine());
	lastError += ", col " + toString(lex.getCurrentColumn());
  	return false;
      } else {
	curSection = token;
	state = Section;
      }
      break;
    case AliasKey: 
      if (type != Lexer::Text) {
	lastError = "expected text";
	lastError += " at line " + toString(lex.getCurrentLine());
	lastError += ", col " + toString(lex.getCurrentColumn());
  	return false;
      }
      
      keytmp = token;
      state = AliasColon;
      break;
    case AliasColon:
      if (type != Lexer::Colon) {
	lastError = "expected colon";
	lastError += " at line " + toString(lex.getCurrentLine());
	lastError += ", col " + toString(lex.getCurrentColumn());
  	return false;
      }
      state = AliasValue;
      break;
    case AliasValue:
      if (type != Lexer::Text) {
	lastError = "expected text";
	lastError += " at line " + toString(lex.getCurrentLine());
	lastError += ", col " + toString(lex.getCurrentColumn());
  	return false;
      }
      
      valuetmp = token;
      state = AliasEnd;
      break;
    case AliasEnd:
      if (type != Lexer::Semicolon) {
	lastError = "expected text";
	lastError += " at line " + toString(lex.getCurrentLine());
	lastError += ", col " + toString(lex.getCurrentColumn());
  	return false;
      }

      aliases[keytmp] = valuetmp;
      state = Searching;
      break;
    case Section:
      if (type != Lexer::LeftCurly) {
	lastError = "expected '{'";
	lastError += " at line " + toString(lex.getCurrentLine());
	lastError += ", col " + toString(lex.getCurrentColumn());
	return false;
      }

      state = Key;
      keytmp = "";
      break;
    case Key:
      if (type == Lexer::Text) {
	if (keytmp != "")
	  keytmp += " ";
	keytmp += token;
      } else if (type == Lexer::Equals) {
	state = Value;
      } else {
	if (type == Lexer::RightCurly) {
	  if (keytmp != "") {
	    lastError = "unexpected '}'";
	    lastError += " at line " + toString(lex.getCurrentLine());
	    lastError += ", col " + toString(lex.getCurrentColumn());
	    return false;
	  } else
	    state = Searching;
	} else {
	  lastError = "expected text or '='";
	  lastError += " at line " + toString(lex.getCurrentLine());
	  lastError += ", col " + toString(lex.getCurrentColumn());
	  return false;
	}
      }
      break;
    case Value:
      if (type == Lexer::Text) {
	if (valuetmp != "")
	  valuetmp += " ";
	valuetmp += token;
      } else if (type == Lexer::Comma || type == Lexer::RightCurly) {
	*section = curSection;
	*key = keytmp;
	*value = valuetmp;
	keytmp = "";
	valuetmp = "";

	if (type == Lexer::RightCurly)
	  state = Searching;
	else
	  state = Key;

	return true;
      }
    };
  }

  lastError = lex.getLastError();
  lastError += " at line " + toString(lex.getCurrentLine() + 1);
  lastError += ", col " + toString(lex.getCurrentColumn() + 1);
  return false;
}

namespace {
  //----------------------------------------------------------------------
  inline bool islabelchar(char c)
  {
    switch (c) {
    case 0x2D: case 0x2E:
    case 0x30: case 0x31: case 0x32: case 0x33: case 0x34:
    case 0x35: case 0x36: case 0x37: case 0x38: case 0x39: 
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44:
    case 0x45: case 0x46: case 0x47: case 0x48: case 0x49:
    case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E:
    case 0x4F: case 0x50: case 0x51: case 0x52: case 0x53:
    case 0x54: case 0x55: case 0x56: case 0x57: case 0x58:
    case 0x59: case 0x5A:
      //    case 0x5B:
      //    case 0x5C:
      //    case 0x5D:
    case 0x5E: case 0x5F:
      //    case 0x60:
    case 0x61: case 0x62: case 0x63: case 0x64: case 0x65:
    case 0x66: case 0x67: case 0x68: case 0x69: case 0x6A:
    case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74:
    case 0x75: case 0x76: case 0x77: case 0x78: case 0x79:
    case 0x7A:
      return true;
    default: return false;
    }
  }

  //----------------------------------------------------------------------
  inline const string &encode(const string &s)
  {
    static string output;
    output = "";

    for (string::const_iterator i = s.begin(); i != s.end(); ++i)
      if ((unsigned char)*i < 32 || (unsigned char)*i > 127) {
	output += "<";
	output += toHex(s);
	output += ">";
	return output;
      }

    for (string::const_iterator i = s.begin(); i != s.end(); ++i)
      if (!islabelchar(*i)) {
	output = "\"";
	char c;
	for (string::const_iterator i = s.begin(); i != s.end(); ++i)
	  switch ((c = *i)) {
	  case '\"': output += "\\\""; break;
	  case '\\': output += "\\\\"; break;
	  default: output += c; break;
	  }
	output += "\"";
	return output;
      }

    return output = s;
  }

  //----------------------------------------------------------------------
  inline bool writeData(int fd, const string &out)
  {
    static string buffer;

    buffer += out;
    if (buffer.size() >= 4096 || out == "") {
      int written = 0;
      while (1) {
	int retval = write(fd, buffer.c_str() + written,
			   buffer.length() - written);
	if (retval == (int) buffer.length())
	  break;
	
	if (retval == -1) {
	  if (errno == EINTR) continue;
	  return false;
	}
	
	written += retval;
      }

      buffer = "";
    }

    return true;
  }
}

//------------------------------------------------------------------------
bool Storage::put(const string &section, const string &key,
		  const string &value)
{
  if (!fd)
    return false;

  if (mode == ReadOnly) {
    lastError = "unable to write to \"" + fileName
      + "\" when in ReadOnly mode";
    return false;
  }

  string data;
  if (curSection != section) {
    if (firstSection)
      firstSection = false;
    else
      data = "\n}\n";

    data += section + " {";
    curSection = section;
  } else
    data = ",";

  data += "\n\t";
  data += encode(key);
  data += " = ";
  data += encode(value);

  if (!writeData(fd, data)) {
    lastError = "when writing to \"" + fileName + "\": ";
    lastError += strerror(errno);
    return false;
  }  
  
  return true;
}

//------------------------------------------------------------------------
bool Storage::commit(void)
{
  if (!writeData(fd, "\n}\n")) {
    lastError = "when writing to \"" + fileName + "\":";
    lastError += strerror(errno);
    unlink(fileName.c_str());
  }

  if (!writeData(fd, "")) {
    lastError = "when writing to \"" + fileName + "\":";
    lastError += strerror(errno);
    unlink(fileName.c_str());
  }

  if (fsync(fd) != 0) {
    lastError = "when syncing \"" + fileName + "\":";
    lastError += strerror(errno);
    return false;
  }

  if (rename(fileName.c_str(), 
	     fileName.substr(0, fileName.length() - 6).c_str()) != 0) {
    lastError = "when renaming \"" + fileName + "\" to \""
      + fileName.substr(0, fileName.length() - 6) + "\": ";
    lastError += strerror(errno);
    unlink(fileName.c_str());
    return false;
  }

  if (close(fd) != 0) {
    lastError = "when committing \"" + fileName + "\":";
    lastError += strerror(errno);
    return false;
  }

  fd = 0;

  return true;
}
