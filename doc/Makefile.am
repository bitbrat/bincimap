# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307,
# USA.

#--------------------------------------------------------------------------
SUBDIRS = manual

#--------------------------------------------------------------------------
EXTRA_DIST = README rfc1733.txt rfc2086.txt rfc2095.txt rfc2192.txt rfc2221.txt rfc2595.txt rfc3028.txt	rfc1731.txt rfc2060.txt rfc2087.txt rfc2177.txt	rfc2193.txt rfc2342.txt rfc2683.txt rfc3348.txt rfc1732.txt rfc2061.txt rfc2088.txt rfc2180.txt rfc2195.txt rfc2359.txt rfc2971.txt bincimap-faq.html bincimap.css bincimap-tech.html bincimap-imapdir.html bincimap-goals.html

#--------------------------------------------------------------------------
install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(datadir)/doc/bincimap
	$(INSTALL_DATA) -m 0644 $(srcdir)/bincimap-faq.html $(DESTDIR)$(datadir)/doc/bincimap/bincimap-faq.html
	$(INSTALL_DATA) -m 0644 $(srcdir)/bincimap.css $(DESTDIR)$(datadir)/doc/bincimap/bincimap.css
	$(INSTALL_DATA) -m 0644 $(srcdir)/bincimap-tech.html $(DESTDIR)$(datadir)/doc/bincimap/bincimap-tech.html
	$(INSTALL_DATA) -m 0644 $(srcdir)/bincimap-imapdir.html $(DESTDIR)$(datadir)/doc/bincimap/bincimap-imapdir.html
	$(INSTALL_DATA) -m 0644 $(srcdir)/bincimap-goals.html $(DESTDIR)$(datadir)/doc/bincimap/bincimap-goals.html
	$(INSTALL_DATA) -m 0644 $(srcdir)/../README $(DESTDIR)$(datadir)/doc/bincimap/README

#--------------------------------------------------------------------------
uninstall-hook:
	rm -f $(DESTDIR)$(datadir)/doc/bincimap/bincimap-faq.html
	rm -f $(DESTDIR)$(datadir)/doc/bincimap/bincimap.css.html
	rm -f $(DESTDIR)$(datadir)/doc/bincimap/bincimap-tech.html
	rm -f $(DESTDIR)$(datadir)/doc/bincimap/bincimap-imapdir.html
	rm -f $(DESTDIR)$(datadir)/doc/bincimap/bincimap-goals.html
	rm -f $(DESTDIR)$(datadir)/doc/bincimap/README
	-rmdir $(DESTDIR)$(datadir)/doc/bincimap
