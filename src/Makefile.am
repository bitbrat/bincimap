# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307,
# USA.

#--------------------------------------------------------------------------
bin_PROGRAMS = bincimapd bincimap-up

#--------------------------------------------------------------------------
bincimapd_SOURCES = address.cc address.h arg-arg.cc arg-arg.h arg-checkers.h arg-error.cc arg-error.h arg-parser.cc arg-parser.h authenticate.cc base64.cc base64.h bincimapd.cc broker.cc broker.h convert.cc convert.h depot.h depot.cc imapparser.cc imapparser.h io.cc io.h mailbox.cc mailbox.h maildir.cc maildir-close.cc maildir-create.cc maildir-delete.cc maildir-expunge.cc maildir.h maildir-readcache.cc maildir-scan.cc maildir-scanfilesnames.cc maildir-select.cc maildir-updateflags.cc maildir-writecache.cc message.h maildirmessage.cc maildirmessage.h bincimapd-module.cc bincimapd-module.h mime.cc mime-getpart.cc mime.h mime-parsefull.cc mime-parseonlyheader.cc mime-printbody.cc mime-printdoc.cc mime-printheader.cc mime-utils.h operators.h operator-authenticate.cc operator-capability.cc operator-noop.cc operator-noop-pending.cc operator-login.cc operator-logout.cc operators.h operator-append.cc operator-examine.cc operator-select.cc operator-create.cc operator-delete.cc operator-list.cc operator-lsub.cc operator-rename.cc operator-status.cc operator-subscribe.cc operator-unsubscribe.cc operators.h operator-check.cc operator-close.cc operator-copy.cc operator-expunge.cc operator-fetch.cc operator-search.cc operator-store.cc pendingupdates.cc pendingupdates.h recursivedescent.cc recursivedescent.h regmatch.cc regmatch.h session.h session.cc session-initialize-bincimapd.cc status.cc status.h storage.cc storage.h tools.cc tools.h

#--------------------------------------------------------------------------
bincimap_up_SOURCES = arg-arg.cc arg-arg.h arg-checkers.h arg-error.cc arg-error.h arg-parser.cc arg-parser.h authenticate.cc authenticate.h base64.cc base64.h bincimap-up.cc broker.cc broker.h convert.cc convert.h greeting.cc imapparser.cc imapparser.h io.cc io.h io-ssl.cc io-ssl.h operators.h operator-authenticate.cc operator-capability.cc operator-noop.cc operator-login.cc operator-logout.cc operator-starttls.cc recursivedescent.cc recursivedescent.h session.h session.cc session-initialize-bincimap-up.cc status.cc status.h storage.cc storage.h tools.cc tools.h

#--------------------------------------------------------------------------
bincimap_up_LDADD = @LIBSSL@ @LIBDL@

#--------------------------------------------------------------------------
bincimapd_LDADD = @LIBDL@

#--------------------------------------------------------------------------
bincimapd_LDFLAGS = @STATIC@ -DBINCIMAPD

#--------------------------------------------------------------------------
bincimap_up_LDFLAGS = @STATIC@
