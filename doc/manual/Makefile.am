# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307,
# USA.

#--------------------------------------------------------------------------
EXTRA_DIST = bincimap-manual.dvi bincimap-manual.ps bincimap-manual.tex

#--------------------------------------------------------------------------
ps: dvi
	dvips bincimap-manual.dvi -o bincimap-manual.ps

#--------------------------------------------------------------------------
dvi: bincimap-manual.dvi

#--------------------------------------------------------------------------
bincimap-manual.dvi: bincimap-manual.tex
	latex bincimap-manual.tex
	latex bincimap-manual.tex

#--------------------------------------------------------------------------
install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(prefix)/doc
	$(INSTALL_DATA) $(srcdir)/bincimap-manual.ps $(DESTDIR)$(prefix)/doc
	$(INSTALL_DATA) $(srcdir)/bincimap-manual.dvi $(DESTDIR)$(prefix)/doc

#--------------------------------------------------------------------------
uninstall-hook:
	rm -v $(DESTDIR)$(prefix)/doc/bincimap-manual.ps
	rm -v $(DESTDIR)$(prefix)/doc/bincimap-manual.dvi

