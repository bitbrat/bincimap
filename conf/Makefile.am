# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307,
# USA.

#--------------------------------------------------------------------------
EXTRA_DIST=bincimap.conf xinetd-bincimap xinetd-bincimaps bincimap.conf.in toimapdir tomaildir++ checkpassword.pl

#--------------------------------------------------------------------------
install-exec-hook:
	@rm -f $(srcdir)/../install.log
	@$(mkinstalldirs) -m 755 $(DESTDIR)$(sysconfdir)
	@if [ ! -e $(DESTDIR)$(sysconfdir)/bincimap.conf ]; then $(INSTALL_DATA) $(srcdir)/bincimap.conf $(DESTDIR)$(sysconfdir);  else $(INSTALL_DATA) $(srcdir)/bincimap.conf $(DESTDIR)$(sysconfdir)/bincimap.conf.new; echo "- New conf file saved as $(DESTDIR)$(sysconfdir)/bincimap.conf.new" >> $(srcdir)/../install.log; fi;
	@$(mkinstalldirs) -m 755 $(DESTDIR)$(sysconfdir)/xinetd

	@if [ ! -e $(DESTDIR)$(sysconfdir)/xinetd/imap ]; then $(INSTALL_DATA) $(srcdir)/xinetd-bincimap $(DESTDIR)$(sysconfdir)/xinetd/imap;  else $(INSTALL_DATA) $(srcdir)/xinetd-bincimap $(DESTDIR)$(sysconfdir)/xinetd/imap.new; echo "- New xinetd/imap file saved as $(DESTDIR)$(sysconfdir)/xinetd/imap.new" >> $(srcdir)/../install.log; fi;
	@if [ ! -e $(DESTDIR)$(sysconfdir)/xinetd/imaps ]; then $(INSTALL_DATA) $(srcdir)/xinetd-bincimaps $(DESTDIR)$(sysconfdir)/xinetd/imaps;  else $(INSTALL_DATA) $(srcdir)/xinetd-bincimaps $(DESTDIR)$(sysconfdir)/xinetd/imaps.new; echo "- New xinetd/imaps file saved as $(DESTDIR)$(sysconfdir)/xinetd/imaps.new" >> $(srcdir)/../install.log; fi;

	@$(mkinstalldirs) -m 755 $(DESTDIR)$(sysconfdir)/scripts
	@if [ ! -e $(DESTDIR)$(sysconfdir)/scripts/checkpassword.pl ]; then $(INSTALL_DATA) -m 0755 $(srcdir)/checkpassword.pl $(DESTDIR)$(sysconfdir)/scripts;  else $(INSTALL_DATA) -m 0755 $(srcdir)/checkpassword.pl $(DESTDIR)$(sysconfdir)/scripts/checkpassword.pl.new; echo "- New file saved as $(DESTDIR)$(sysconfdir)/scripts/checkpassword.pl.new" >> $(srcdir)/../install.log; fi;
	@if [ ! -e $(DESTDIR)$(sysconfdir)/scripts/tomaildir++ ]; then $(INSTALL_DATA) -m 0755 $(srcdir)/tomaildir++ $(DESTDIR)$(sysconfdir)/scripts;  else $(INSTALL_DATA) -m 0755 $(srcdir)/tomaildir++ $(DESTDIR)$(sysconfdir)/scripts/tomaildir++.new; echo "- New file saved as $(DESTDIR)$(sysconfdir)/scripts/tomaildir++.new" >> $(srcdir)/../install.log; fi;
	@if [ ! -e $(DESTDIR)$(sysconfdir)/scripts/toimapdir ]; then $(INSTALL_DATA) -m 0755 $(srcdir)/toimapdir $(DESTDIR)$(sysconfdir)/scripts;  else $(INSTALL_DATA) -m 0755 $(srcdir)/toimapdir $(DESTDIR)$(sysconfdir)/scripts/toimapdir.new; echo "- New file saved as $(DESTDIR)$(sysconfdir)/scripts/toimapdir.new" >> $(srcdir)/../install.log; fi;
	@if [ ! -e $(DESTDIR)$(sysconfdir)/scripts/tomaildir++ ]; then $(INSTALL_DATA) -m 0755 $(srcdir)/tomaildir++ $(DESTDIR)$(sysconfdir)/scripts;  else $(INSTALL_DATA) -m 0755 $(srcdir)/tomaildir++ $(DESTDIR)$(sysconfdir)/scripts/tomaildir++.new; echo "- New file saved as $(DESTDIR)$(sysconfdir)/scripts/tomaildir++.new" >> $(srcdir)/../install.log; fi;

#--------------------------------------------------------------------------
uninstall-hook:
	rm $(DESTDIR)$(sysconfdir)/bincimap.conf
	rm $(DESTDIR)$(sysconfdir)/xinetd/imap
	rm $(DESTDIR)$(sysconfdir)/xinetd/imaps
