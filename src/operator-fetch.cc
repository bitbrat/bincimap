/* -*- Mode: c++; -*- */
/*  --------------------------------------------------------------------
 *  Filename:
 *    operator-fetch.cc
 *  
 *  Description:
 *    Implementation of the FETCH command.
 *
 *  Authors:
 *    Andreas Aardal Hanssen <andreas-binc curly bincimap spot org>
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

#include <string>

#include "depot.h"
#include "io.h"
#include "mailbox.h"
#include "operators.h"
#include "imapparser.h"
#include "recursivedescent.h"
#include "session.h"
#include "convert.h"

using namespace ::std;
using namespace Binc;

namespace {
  void outputFlags(const Message & message) 
  {
    IO &com = IOFactory::getInstance().get(1);

    com << "FLAGS ";
	  
    com << "(";
    int flags = message.getStdFlags();
    vector<string> flagv;
    if (flags & Message::F_SEEN) flagv.push_back("\\Seen");
    if (flags & Message::F_ANSWERED) flagv.push_back("\\Answered");
    if (flags & Message::F_DELETED) flagv.push_back("\\Deleted");
    if (flags & Message::F_DRAFT) flagv.push_back("\\Draft");
    if (flags & Message::F_RECENT) flagv.push_back("\\Recent");
    if (flags & Message::F_FLAGGED) flagv.push_back("\\Flagged");
    for (vector<string>::const_iterator k 
	   = flagv.begin(); k != flagv.end(); ++k) {
      if (k != flagv.begin()) com << " ";
      com << *k;
    }
    com << ")";
  }

}

//----------------------------------------------------------------------
FetchOperator::FetchOperator(void)
{
}

//----------------------------------------------------------------------
FetchOperator::~FetchOperator(void)
{
}

//----------------------------------------------------------------------
const string FetchOperator::getName(void) const
{
  return "FETCH";
}

//----------------------------------------------------------------------
int FetchOperator::getState(void) const
{
  return Session::SELECTED;
}

//------------------------------------------------------------------------
Operator::ProcessResult FetchOperator::process(Depot &depot,
					       Request &request)
{
  IO &com = IOFactory::getInstance().get(1);
  Session &session = Session::getInstance();

  bool updateFlags = false;
  Request req = request;

  Mailbox *mailbox = depot.getSelected();

  // If this is a UID FETCH, check if the UID attribute is fetched. If
  // it is not, then add it to the list of fetch attributes.
  vector<BincImapParserFetchAtt>::const_iterator f_i;
  bool uidfetched = false;
  if (request.getUidMode()) {
    f_i = request.fatt.begin();
    while (f_i != request.fatt.end()) {
      if ((*f_i).type == "UID") {
	uidfetched = true;
	break;
      }
      
      f_i++;
    }

    if (!uidfetched) {
      BincImapParserFetchAtt b;
      b.type = "UID";
      req.fatt.push_back(b);
    }
  }

  // Convert macros ALL, FULL and FAST
  f_i = request.fatt.begin();
  while (f_i != request.fatt.end()) {
    const string &type = (*f_i).type;
    if (type == "ALL") {
      req.fatt.push_back(BincImapParserFetchAtt("FLAGS"));
      req.fatt.push_back(BincImapParserFetchAtt("INTERNALDATE"));
      req.fatt.push_back(BincImapParserFetchAtt("RFC822.SIZE"));
      req.fatt.push_back(BincImapParserFetchAtt("ENVELOPE"));
    } else if (type == "FULL") {
      req.fatt.push_back(BincImapParserFetchAtt("FLAGS"));
      req.fatt.push_back(BincImapParserFetchAtt("INTERNALDATE"));
      req.fatt.push_back(BincImapParserFetchAtt("RFC822.SIZE"));
      req.fatt.push_back(BincImapParserFetchAtt("ENVELOPE"));
      req.fatt.push_back(BincImapParserFetchAtt("BODY"));
    } else if (type == "FAST") {
      req.fatt.push_back(BincImapParserFetchAtt("FLAGS"));
      req.fatt.push_back(BincImapParserFetchAtt("INTERNALDATE"));
      req.fatt.push_back(BincImapParserFetchAtt("RFC822.SIZE"));
    }

    ++f_i;
  }

  int mode;
  if (req.getUidMode())
    mode = Mailbox::UID_MODE;
  else
    mode = Mailbox::SQNR_MODE;

  Mailbox::iterator i
    = mailbox->begin(req.bset, Mailbox::SKIP_EXPUNGED | mode);

  for (; i != mailbox->end(); ++i) {
    Message &message = *i;
    
    com << "* " << i.getSqnr() << " FETCH (";
    bool hasprinted = false;
    f_i = req.fatt.begin();
    while (f_i != req.fatt.end()) {
      BincImapParserFetchAtt fatt = *f_i;

      string prefix = "";
      if (hasprinted)
	prefix = " ";
	
      if (fatt.type == "FLAGS") {
	// FLAGS
	hasprinted = true;
	com << prefix;

	outputFlags(message);
      } else if (fatt.type == "UID") {
	// UID
	hasprinted = true;
	com << prefix << "UID " << message.getUID();
      } else if (fatt.type == "RFC822.SIZE") {
	// RFC822.SIZE
	hasprinted = true;
	com << prefix << "RFC822.SIZE " << message.getSize(true);
      } else if (fatt.type == "ENVELOPE") {
	// ENVELOPE
	hasprinted = true;
	com << prefix << "ENVELOPE ";
	message.printEnvelope();
      } else if (fatt.type == "BODYSTRUCTURE") {
	// BODYSTRUCTURE
	hasprinted = true;
	com << prefix << "BODYSTRUCTURE ";
	message.printBodyStructure(true);
      } else if (fatt.type == "BODY" && !fatt.hassection) {
	// BODY with no section
	hasprinted = true;
	session.addBody();
	com << prefix << "BODY ";
	message.printBodyStructure(false);
      } else if (fatt.type == "INTERNALDATE") {
	// INTERNALDATE
	hasprinted = true;
	com << prefix << "INTERNALDATE ";

	time_t iDate = message.getInternalDate();
	struct tm *_tm = gmtime(&iDate);
	char internal[64];
	string iDateStr;
	if (strftime(internal, sizeof(internal),
		     "%d-%b-%Y %H:%M:%S %Z", _tm) != 0)
	  iDateStr = internal;
	else
	  iDateStr = "NIL";

	com << toImapString(iDateStr);
      } else if (fatt.type == "BODY" || fatt.type == "BODY.PEEK") {
	// BODY & BODY.PEEK
	hasprinted = true;
	session.addBody();

	com << prefix;
	bool peek = (fatt.type == "BODY.PEEK");
	com << fatt.toString();

	bool includeheaders = true;
	bool fullheader = false;
	bool bodyfetch = false;

	if (fatt.section != "" || fatt.sectiontext == ""
	    || fatt.sectiontext == "TEXT") {
	  bodyfetch = true;
	  fullheader = true;
	}

	if (fatt.sectiontext == "HEADER.FIELDS.NOT")
	  includeheaders = false;

	if (fatt.sectiontext == "HEADER"
	    || fatt.sectiontext == "HEADER.FIELDS"
	    || fatt.sectiontext == "HEADER.FIELDS.NOT"
	    || fatt.sectiontext == "MIME") {

	  vector<string> v;
	  
	  if (fatt.sectiontext == "MIME") {
	    v.push_back("content-type");
	    v.push_back("content-transfer-encoding");
	    v.push_back("content-disposition");
	    v.push_back("content-description");
	  } else
	    v = fatt.headerlist;
    
	  string dummy;
	  unsigned int size = fullheader
	    ? message.getHeaderSize(fatt.section, v,
				    true, 
				    fatt.offsetstart, 
				    fatt.offsetlength, fatt.sectiontext == "MIME")
	    : message.getHeaderSize(fatt.section, fatt.headerlist,
				    includeheaders, 
				    fatt.offsetstart,
				    fatt.offsetlength, fatt.sectiontext == "MIME");

	  com << "{" << size << "}\r\n";
	  
	  if (fullheader) {
	    message.printHeader(fatt.section, v,
				true, 
				fatt.offsetstart,
				fatt.offsetlength, fatt.sectiontext == "MIME");
	  } else {
	    message.printHeader(fatt.section, fatt.headerlist,
				includeheaders,
				fatt.offsetstart, 
				fatt.offsetlength, fatt.sectiontext == "MIME");
	  }
	} else {
	  unsigned int size;
	  if ((fatt.sectiontext == "" || fatt.sectiontext == "TEXT")
	      && fatt.section == "")
	    size = message.getDocSize(fatt.offsetstart,
				      fatt.offsetlength,
				      fatt.sectiontext == "TEXT");
	  else
	    size = message.getBodySize(fatt.section, 
				       fatt.offsetstart, 
				       fatt.offsetlength);
	  
	  com << "{" << size << "}\r\n";
	  
	  if ((fatt.sectiontext == "" || fatt.sectiontext == "TEXT")
	      && fatt.section == "")
	    message.printDoc(fatt.offsetstart, 
			     fatt.offsetlength,
			     fatt.sectiontext == "TEXT");
	  else
	    message.printBody(fatt.section, fatt.offsetstart, 
			      fatt.offsetlength);
	}

	// set the \Seen flag if .PEEK is not used.
	if (!peek)
	  if ((message.getStdFlags() & Message::F_SEEN) == 0)
	    message.setStdFlag(Message::F_SEEN);

      } else if (fatt.type == "RFC822") {
	com << prefix;
	hasprinted = true;
	session.addBody();

	com << fatt.toString();

	unsigned int size = message.getDocSize(fatt.offsetstart, 
					       fatt.offsetlength);

	com << " {" << size << "}\r\n";

	message.printDoc(fatt.offsetstart, fatt.offsetlength);
	    
	// set the \Seen flag
	if ((message.getStdFlags() & Message::F_SEEN) == 0)
	  message.setStdFlag(Message::F_SEEN);

      } else if (fatt.type == "RFC822.HEADER") {
	com << prefix;
	hasprinted = true;

	com << fatt.toString();

	vector<string> v;
	string dummy;

	unsigned int size = message.getHeaderSize("", v, true,
						  fatt.offsetstart, 
						  fatt.offsetlength);

	com << " {" << size << "}\r\n";

	message.printHeader("", v, true, fatt.offsetstart, 
			    fatt.offsetlength);
	    
      } else if (fatt.type == "RFC822.TEXT") {
	// RFC822.TEXT
	com << prefix;
	hasprinted = true;
	session.addBody();

	com << fatt.toString();

	bool bodyfetch = false;
	bodyfetch = true;

	unsigned int size;
	if (fatt.sectiontext == "" && fatt.section == "")
	  size = message.getDocSize(fatt.offsetstart, 
				fatt.offsetlength);
	else
	  size = message.getBodySize(fatt.section, fatt.offsetstart, 
				     fatt.offsetlength);
	    
	com << "{" << size << "}\r\n";
	    
	if (fatt.sectiontext == "" && fatt.section == "")
	  message.printDoc(fatt.offsetstart, 
			   fatt.offsetlength);
	else
	  message.printBody(fatt.section, fatt.offsetstart, 
			    fatt.offsetlength);

	// set the \Seen flag
	if ((message.getStdFlags() & Message::F_SEEN) == 0)
	  message.setStdFlag(Message::F_SEEN);
      } else {
	// Unrecognized fetch_att, this is stopped by the parser
	// so we never get here.
      }

      f_i++;
    }

    // FIXME: how are parse error passed back?

    com << ")" << endl;

    if (message.hasFlagsChanged()) {
      updateFlags = true;
      com << "* " << i.getSqnr() << " FETCH (";
      outputFlags(message);
      com << ")" << endl;
      message.setFlagsUnchanged();
    }
  }
  
  if (updateFlags) mailbox->updateFlags();

  return OK;
}

//----------------------------------------------------------------------
Operator::ParseResult FetchOperator::parse(Request &c_in) const
{
  Session &session = Session::getInstance();

  Operator::ParseResult res;
  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after FETCH");
    return res;
  }

  if ((res = expectSet(c_in.getSet())) != ACCEPT) {
    session.setLastError("Expected sequence set after FETCH SPACE");
    return res;
  }

  if ((res = expectSPACE()) != ACCEPT) {
    session.setLastError("Expected SPACE after FETCH SPACE set");
    return res;
  }

  BincImapParserFetchAtt f;

  if ((res = expectThisString("ALL")) == ACCEPT) {
    f.type = "ALL";
    c_in.fatt.push_back(f);
  } else if ((res = expectThisString("FULL")) == ACCEPT) {
    f.type = "FULL";
    c_in.fatt.push_back(f);
  } else if ((res = expectThisString("FAST")) == ACCEPT) {
    f.type = "FAST";
    c_in.fatt.push_back(f);
  } else if ((res = expectFetchAtt(f)) == ACCEPT) {
    c_in.fatt.push_back(f);
  } else if ((res = expectThisString("(")) == ACCEPT) {
    while (1) {
      BincImapParserFetchAtt ftmp;
      if ((res = expectFetchAtt(ftmp)) != ACCEPT) {
	session.setLastError("Expected fetch_att");
	return res;
      }
	
      c_in.fatt.push_back(ftmp);
      
      if ((res = expectSPACE()) == REJECT)
	break;
      else if (res == ERROR)
	return ERROR;
    }

    if ((res = expectThisString(")")) != ACCEPT) {
      session.setLastError("Expected )");
      return res;
    }
  } else {
    session.setLastError("Expected ALL, FULL, FAST, fetch_att or (");
    return res;
  }

  if ((res = expectCRLF()) != ACCEPT) {
    session.setLastError("Expected CRLF");
    return res;
  }

  c_in.setName("FETCH");
  return ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult
FetchOperator::expectSectionText(BincImapParserFetchAtt &f_in) const
{
  Session &session = Session::getInstance();

  Operator::ParseResult res;
  if ((res = expectThisString("HEADER")) == ACCEPT) {
    f_in.sectiontext = "HEADER";
    
    if ((res = expectThisString(".FIELDS")) == ACCEPT) {
      f_in.sectiontext += ".FIELDS";
      
      if ((res = expectThisString(".NOT")) == ACCEPT)
	f_in.sectiontext += ".NOT";
      
      if ((res = expectSPACE()) != ACCEPT) {
	session.setLastError("expected SPACE");
	return res;
      }
      
      if ((res = expectHeaderList(f_in)) != ACCEPT) {
	session.setLastError("Expected header_list");
	return res;
      }
    }
  } else if ((res = expectThisString("TEXT")) == ACCEPT)
    f_in.sectiontext = "TEXT";
  else
    return REJECT;

  return ACCEPT;

}

//----------------------------------------------------------------------
Operator::ParseResult
FetchOperator::expectSection(BincImapParserFetchAtt &f_in) const
{
  Session &session = Session::getInstance();

  Operator::ParseResult res;
  if ((res = expectThisString("[")) != ACCEPT)
    return REJECT;
    
  if ((res = expectSectionText(f_in)) != ACCEPT) {
    unsigned int n;
    if ((res = expectNZNumber(n)) == ACCEPT) {
      
      BincStream nstr;
	
      nstr << n;

      f_in.section = nstr.str();
      
      bool gotadotalready = false;
      while (1) {
	if ((res = expectThisString(".")) != ACCEPT)
	  break;
	
	if ((res = expectNZNumber(n)) != ACCEPT) {
	  gotadotalready = true;
	  break;
	}
	
	f_in.section += ".";	
	
	BincStream nstr;
	
	nstr << n;
	
	f_in.section += nstr.str();	 
      }
      
      if (gotadotalready || (res = expectThisString(".")) == ACCEPT) {
	if ((res = expectThisString("MIME")) == ACCEPT) {
	  f_in.sectiontext = "MIME";
	} else if ((res = expectSectionText(f_in)) != ACCEPT) {
	  session.setLastError("Expected MIME or section_text");
	  return res;
	}
      }
    }
  }

  if ((res = expectThisString("]")) != ACCEPT) {
    session.setLastError("Expected ]");
    return res;
  }

  return ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult
FetchOperator::expectHeaderList(BincImapParserFetchAtt &f_in) const
{
  Session &session = Session::getInstance();

  Operator::ParseResult res;
  if ((res = expectThisString("(")) != ACCEPT)
    return REJECT;

  string header_fld_name;
  while (1) {
    if ((res = expectAstring(header_fld_name)) != ACCEPT) {
      session.setLastError("Expected header_fld_name");
      return res;
    }

    f_in.headerlist.push_back(header_fld_name);

    if ((res = expectSPACE()) == ACCEPT)
      continue;
    else break;
  }

  if ((res = expectThisString(")")) != ACCEPT) {
    session.setLastError("Expected )");
    return res;
  }

  return ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult
FetchOperator::expectOffset(BincImapParserFetchAtt &f_in) const
{
  Session &session = Session::getInstance();
  Operator::ParseResult res;

  if ((res = expectThisString("<")) != ACCEPT)
    return REJECT;

  unsigned int i;
  if ((res = expectNumber(i)) != ACCEPT) {
    session.setLastError("Expected number");
    return res;
  }

  if ((res = expectThisString(".")) != ACCEPT) {
    session.setLastError("Expected .");
    return res;
  }

  unsigned int j;
  if ((res = expectNZNumber(j)) != ACCEPT) {
    session.setLastError("expected nz_number");
    return res;
  }

  if ((res = expectThisString(">")) != ACCEPT) {
    session.setLastError("Expected >");
    return res;
  }

  f_in.offsetstart = i;
  f_in.offsetlength = j;
  return ACCEPT;
}

//----------------------------------------------------------------------
Operator::ParseResult
FetchOperator::expectFetchAtt(BincImapParserFetchAtt &f_in) const
{
  Operator::ParseResult res;

  Session &session = Session::getInstance();

  if ((res = expectThisString("ENVELOPE")) == ACCEPT) f_in.type = "ENVELOPE";
  else if ((res = expectThisString("FLAGS")) == ACCEPT) f_in.type = "FLAGS";
  else if ((res = expectThisString("INTERNALDATE")) == ACCEPT)
    f_in.type = "INTERNALDATE";
  else if ((res = expectThisString("UID")) == ACCEPT) f_in.type = "UID";
  else if ((res = expectThisString("RFC822")) == ACCEPT) {
    f_in.type = "RFC822";
    if ((res = expectThisString(".HEADER")) == ACCEPT) f_in.type += ".HEADER";
    else if ((res = expectThisString(".SIZE")) == ACCEPT) f_in.type += ".SIZE";
    else if ((res = expectThisString(".TEXT")) == ACCEPT) f_in.type += ".TEXT";
    else if ((res = expectThisString(".")) == ACCEPT) {
      session.setLastError("Expected RFC822, RFC822.HEADER,"
			   " RFC822.SIZE or RFC822.TEXT");
      return ERROR;
    }

  } else if ((res = expectThisString("BODY")) == ACCEPT) {
    f_in.type = "BODY";

    if ((res = expectThisString("STRUCTURE")) == ACCEPT) f_in.type += "STRUCTURE";
    else if ((res = expectThisString(".PEEK")) == ACCEPT) f_in.type += ".PEEK";
      
    if ((res = expectSection(f_in)) != ACCEPT)
      f_in.hassection = false;
    else {
      f_in.hassection = true;

      if ((res = expectOffset(f_in)) == ERROR)
	return ERROR;
    } 
  } else
    return REJECT;

  return ACCEPT;
}
