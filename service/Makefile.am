# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307,
# USA.

#--------------------------------------------------------------------------
EXTRA_DIST=down run run-ssl

#--------------------------------------------------------------------------
SUBDIRS=log

#--------------------------------------------------------------------------
install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/service/imap

	@if [ ! -e $(DESTDIR)$(sysconfdir)/service/imap/run ]; then $(INSTALL_DATA) -m 0755 $(srcdir)/run $(DESTDIR)$(sysconfdir)/service/imap/run;  else $(INSTALL_DATA) -m 0755 $(srcdir)/run $(DESTDIR)$(sysconfdir)/service/imap/run.new; echo "- New service/imap/run file saved as $(DESTDIR)$(sysconfdir)/service/imap/run.new" >> $(srcdir)/../install.log; fi;
	$(INSTALL_DATA) -m 0644 $(srcdir)/down $(DESTDIR)$(sysconfdir)/service/imap/down
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/service/imaps
	@if [ ! -e $(DESTDIR)$(sysconfdir)/service/imaps/run ]; then $(INSTALL_DATA) -m 0755 $(srcdir)/run-ssl $(DESTDIR)$(sysconfdir)/service/imaps/run;  else $(INSTALL_DATA) -m 0755 $(srcdir)/run-ssl $(DESTDIR)$(sysconfdir)/service/imaps/run.new; echo "- New service/imaps/run file saved as $(DESTDIR)$(sysconfdir)/service/imaps/run.new" >> $(srcdir)/../install.log; fi;
	$(INSTALL_DATA) -m 0644 $(srcdir)/down $(DESTDIR)$(sysconfdir)/service/imaps/down

#--------------------------------------------------------------------------
uninstall-hook:
	rm $(DESTDIR)$(sysconfdir)/service/imap/run
	rm $(DESTDIR)$(sysconfdir)/service/imaps/run
	rm $(DESTDIR)$(sysconfdir)/service/imap/down
	rm $(DESTDIR)$(sysconfdir)/service/imaps/down
