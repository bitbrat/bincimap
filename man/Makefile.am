# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307,
# USA.

#--------------------------------------------------------------------------
EXTRA_DIST=bincimapd.1 bincimap-up.1 bincimap.conf.5

#--------------------------------------------------------------------------
install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) $(srcdir)/bincimapd.1 $(DESTDIR)$(mandir)/man1
	$(INSTALL_DATA) $(srcdir)/bincimap-up.1 $(DESTDIR)$(mandir)/man1
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man5
	$(INSTALL_DATA) $(srcdir)/bincimap.conf.5 $(DESTDIR)$(mandir)/man5

#--------------------------------------------------------------------------
uninstall-hook:
	rm $(DESTDIR)$(mandir)/man1/bincimapd.1
	rm $(DESTDIR)$(mandir)/man1/bincimap-up.1
	rm $(DESTDIR)$(mandir)/man5/bincimap.conf.5
