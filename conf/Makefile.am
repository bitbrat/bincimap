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

	@if [ ! -e $(DESTDIR)$(sysconfdir)/xinetd/bincimap ]; then $(INSTALL_DATA) $(srcdir)/xinetd-bincimap $(DESTDIR)$(sysconfdir)/xinetd/bincimap;  else $(INSTALL_DATA) $(srcdir)/xinetd-bincimap $(DESTDIR)$(sysconfdir)/xinetd/bincimap.new; echo "- New xinetd/bincimap file saved as $(DESTDIR)$(sysconfdir)/xinetd/bincimap.new" >> $(srcdir)/../install.log; fi;
	@if [ ! -e $(DESTDIR)$(sysconfdir)/xinetd/bincimaps ]; then $(INSTALL_DATA) $(srcdir)/xinetd-bincimaps $(DESTDIR)$(sysconfdir)/xinetd/bincimaps;  else $(INSTALL_DATA) $(srcdir)/xinetd-bincimaps $(DESTDIR)$(sysconfdir)/xinetd/bincimaps.new; echo "- New xinetd/bincimaps file saved as $(DESTDIR)$(sysconfdir)/xinetd/bincimaps.new" >> $(srcdir)/../install.log; fi;

	@$(mkinstalldirs) -m 755 $(DESTDIR)$(libexecdir)/
	@if [ ! -e $(DESTDIR)$(libexecdir)/checkpassword.pl ]; then $(INSTALL_DATA) -m 0755 $(srcdir)/checkpassword.pl $(DESTDIR)$(libexecdir)/checkpassword.pl; else $(INSTALL_DATA) -m 0755 $(srcdir)/checkpassword.pl $(DESTDIR)$(libexecdir)/checkpassword.pl.new; echo "- New file saved as $(DESTDIR)$(libexecdir)/checkpassword.pl.new" >> $(srcdir)/../install.log; fi;
	@if [ ! -e $(DESTDIR)$(libexecdir)/toimapdir ]; then $(INSTALL_DATA) -m 0755 $(srcdir)/toimapdir $(DESTDIR)$(libexecdir)/toimapdir; else $(INSTALL_DATA) -m 0755 $(srcdir)/toimapdir $(DESTDIR)$(libexecdir)/toimapdir.new; echo "- New file saved as $(DESTDIR)$(libexecdir)/toimapdir.new" >> $(srcdir)/../install.log; fi;
	@if [ ! -e $(DESTDIR)$(libexecdir)/tomaildir++ ]; then $(INSTALL_DATA) -m 0755 $(srcdir)/tomaildir++ $(DESTDIR)$(libexecdir)/tomaildir++; else $(INSTALL_DATA) -m 0755 $(srcdir)/tomaildir++ $(DESTDIR)$(libexecdir)/tomaildir++.new; echo "- New file saved as $(DESTDIR)$(libexecdir)/tomaildir++.new" >> $(srcdir)/../install.log; fi;

#--------------------------------------------------------------------------
uninstall-hook:
	rm $(DESTDIR)$(sysconfdir)/bincimap.conf
	rm $(DESTDIR)$(sysconfdir)/xinetd/bincimap
	rm $(DESTDIR)$(sysconfdir)/xinetd/bincimaps
	-rmdir $(DESTDIR)$(sysconfdir)/xinetd
