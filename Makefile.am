# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307,
# USA.

#--------------------------------------------------------------------------
SUBDIRS = conf contrib doc man service src

#--------------------------------------------------------------------------
EXTRA_DIST = bincimap.spec config.h COPYING.OpenSSL config.h.in README.SSL

#--------------------------------------------------------------------------
cert:
	@um=$(umask); umask 77 >/dev/null 2>&1; KEY=`mktemp /tmp/openssl.XXXXXX`; CERT=`mktemp /tmp/openssl.XXXXXX` ; openssl req -newkey rsa:1024 -keyout $$KEY -nodes -days 365 -out $$CERT ; cat $$KEY > bincimap.pem; echo "" >> bincimap.pem ; cat $$CERT >> bincimap.pem; rm $$CERT $$KEY; umask $(um) >/dev/null 2>&1
	@echo
	@echo "Created 1024 bit private key and a certificate"
	@echo "request in bincimap.pem. For more info, please"
	@echo "read README.SSL."

#--------------------------------------------------------------------------
testcert:
	@um=$(umask); umask 77 >/dev/null 2>&1; PEM1=`mktemp /tmp/openssl.XXXXXX`; PEM2=`mktemp /tmp/openssl.XXXXXX` ; openssl req -newkey rsa:1024 -keyout $$PEM1 -nodes -x509 -days 365 -out $$PEM2 ; cat $$PEM1 >  $(DESTDIR)$(sysconfdir)/bincimap.pem; echo "" >> $(DESTDIR)$(sysconfdir)/bincimap.pem ; cat $$PEM2 >> $(DESTDIR)$(sysconfdir)/bincimap.pem; $(RM) $$PEM1 $$PEM2; umask $(um) >/dev/null 2>&1
	@echo
	@echo "Created 1024 bit bincimap.pem"
	@echo "private key with a 1 year test certificate."
	@echo "This certificate is only secure in closed networks."
	@echo "For more info, please read README.SSL."

#--------------------------------------------------------------------------
cacert:
	@openssl req -out ca.pem -new -x509
	@um=$(umask); umask 77 >/dev/null 2>&1; KEY=`mktemp /tmp/openssl.XXXXXX`; CERT=`mktemp /tmp/openssl.XXXXXX` ; openssl req -newkey rsa:1024 -keyout $$KEY -CA ca.pem -nodes -x509 -days 365 -out $$CERT ; cat $$KEY > bincimap.pem; echo "" >> bincimap.pem; cat $$CERT >> bincimap.pem; rm $$KEY $$CERT; umask $(um) >/dev/null 2>&1
	@echo
	@echo "Created CA certificate in ca.pem."
	@echo "Created 1024 bit private key and certificate in bincimap.pem (lasts 365 days)."
	@echo "For more info, please read README.SSL."

#--------------------------------------------------------------------------
install-exec-hook:
	@echo >> $(srcdir)/install.log
	@echo "Installation of Binc IMAP v@VERSION@ is complete. Happy IMAPing!" >> $(srcdir)/install.log
	@echo
	@cat $(srcdir)/install.log
	@echo
