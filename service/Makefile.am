# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307,
# USA.

#--------------------------------------------------------------------------
EXTRA_DIST=run run-ssl

#--------------------------------------------------------------------------
SUBDIRS=log

#--------------------------------------------------------------------------
install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/service/imap

	@if [ ! -e $(DESTDIR)$(localstatedir)/service/imap/run ]; then $(INSTALL_DATA) -m 0755 $(srcdir)/run $(DESTDIR)$(localstatedir)/service/imap/run;  else $(INSTALL_DATA) -m 0755 $(srcdir)/run $(DESTDIR)$(localstatedir)/service/imap/run.new; echo "- New service/imap/run file saved as $(DESTDIR)$(localstatedir)/service/imap/run.new" >> $(srcdir)/../install.log; fi;
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/service/imaps
	@if [ ! -e $(DESTDIR)$(localstatedir)/service/imaps/run ]; then $(INSTALL_DATA) -m 0755 $(srcdir)/run-ssl $(DESTDIR)$(localstatedir)/service/imaps/run;  else $(INSTALL_DATA) -m 0755 $(srcdir)/run-ssl $(DESTDIR)$(localstatedir)/service/imaps/run.new; echo "- New service/imaps/run file saved as $(DESTDIR)$(localstatedir)/service/imaps/run.new" >> $(srcdir)/../install.log; fi;

#--------------------------------------------------------------------------
uninstall-hook:
	rm $(DESTDIR)$(localstatedir)/service/imap/run
	rm $(DESTDIR)$(localstatedir)/service/imaps/run
	rm $(DESTDIR)$(localstatedir)/service/imap/down
	rm $(DESTDIR)$(localstatedir)/service/imaps/down
