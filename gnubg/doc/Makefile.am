AM_CPPFLAGS = -DAC_DOCDIR='"$(docdir)"' -DAC_DATADIR='"$(datadir)"' -DAC_PKGDATADIR='"$(datadir)/gnubg"'
gnubg_helpdir = $(docdir)
gnubg_imagedir = $(docdir)/images
gnubg_help_DATA =  gnubg.html allabout.html
gnubg_image_DATA = images/1540d81e.png images/16d7944c.png images/1805dc7d.png \
		   images/20bc52ca.png images/20d56757.png images/234924dc.png images/26e34ea5.png \
		   images/29c6aa57.png images/2d9edbab.png images/2dd86698.png images/2e6307ae.png \
		   images/33d3487.png images/34740886.png images/3658e0e8.png images/38371a4c.png \
		   images/3b2e620f.png images/3cb4011e.png images/4332f3e4.png images/48d8024f.png \
		   images/4aad0142.png images/4b13a0e7.png images/4bd6c15f.png images/4e43baf8.png \
		   images/51394706.png images/5308d35c.png images/53ce0fa6.png images/5412263e.png \
		   images/58c77df2.png images/5ada89e8.png images/60df14d2.png images/66ed48bd.png \
		   images/6a6ae1b7.png images/6e75aa33.png images/723e49fc.png images/74a66417.png \
		   images/78be1dd5.png images/analysepane.png images/analysesettings.png \
		   images/appearence.png images/be5c848.png images/clearboard.png \
		   images/cubebuttons.png images/cubefuleq-ex1.png images/cubeful-ex1.png \
		   images/cubeful-ex2.png images/doubleline.png images/e613071.png \
		   images/evalsetting.png images/ff1f1de.png images/gamerecord.png \
		   images/gtkinterface.png images/hintcubewindow.png images/hintwindow.png \
		   images/initialboard.png images/m1921b4.png images/m19f9a2cc.png \
		   images/m1bd07579.png images/m1e2a8a3a.png images/m20a4701e.png \
		   images/m215edfe5.png images/m23e0471c.png images/m259fcca6.png \
		   images/m2698978a.png images/m2759b1ca.png images/m2a94f4ab.png \
		   images/m2b86c3ea.png images/m2c28ffc2.png images/m2c37a1e7.png \
		   images/m30dc13f6.png images/m3a7e4f1b.png images/m3eb29fd9.png \
		   images/m3fb550fb.png images/m3fd7ee24.png images/m4149eeab.png \
		   images/m46788d89.png images/m4796afa7.png images/m4ed24f10.png \
		   images/m518778bb.png images/m57625db1.png images/m5781f59d.png \
		   images/m5878543.png images/m60d5424b.png images/m634daa5.png \
		   images/m68ad25e2.png images/m6e32590b.png images/m6e43baca.png \
		   images/m6fc19da0.png images/m707a2772.png images/m72075f4e.png \
		   images/m76e2d010.png images/m7bf4f29.png images/m7cee1bfc.png \
		   images/m93f2ca3.png images/mb4b1284.png images/md9b1995.png images/mgcd.png \
		   images/mgtp.png images/movefilter1.png images/movefilter2.png \
		   images/movefilter3.png images/movefilterex.png images/mptp.png \
		   images/newbox.png images/rolldice.png images/rulfig1.png images/rulfig2.png \
		   images/rulfig3.png images/rulfig4.png images/rulfig5.png images/setdice.png \
		   images/setturn.png images/tutor.png images/tutorwarning.png \
		   images/3117171e.png images/m22b92249.png

man6_NOBLD = makeweights.6 bearoffdump.6 makebearoff.6 makehyper.6
notrans_man_MANS = gnubg.6 $(man6_NOBLD)

EXTRA_DIST = allabout.xml gnubgdb.xml gnubgman.xml gnubg/allabout.html \
	     gnubg/allabout.pdf $(man6_NOBLD) gnubg/gnubg.6 gnubg/gnubg.html \
		 gnubg/gnubg.info gnubg/gnubg.pdf gnubg/gnubg.texi $(gnubg_image_DATA)

all: gnubg.html allabout.html gnubg.texi gnubg.info gnubg.pdf allabout.pdf

gnubg.6: gnubgman.xml
	$(DOCBOOK2MAN) $< || \
		cp $(srcdir)/gnubg/gnubg.6 .

gnubg.pdf: gnubgdb.xml $(gnubg_image_DATA)
	$(DBLATEX) $< -o $@ || cp $(srcdir)/gnubg/gnubg.pdf .

allabout.pdf: allabout.xml $(gnubg_image_DATA)
	$(DBLATEX) $< -o $@ || cp $(srcdir)/gnubg/allabout.pdf .

gnubg.html: gnubgdb.xml
	$(XSLTPROC) \
		--stringparam chunker.output.encoding ISO8859-1 \
		--stringparam  use.id.as.filename 1 \
		--stringparam  root.filename  '' \
		--stringparam  html.longdesc 0 \
		--nonet http://docbook.sourceforge.net/release/xsl/current/xhtml/onechunk.xsl \
		$< || \
		cp $(srcdir)/gnubg/gnubg.html .


allabout.html: allabout.xml
	$(XSLTPROC) \
		--stringparam chunker.output.encoding ISO8859-1 \
		--stringparam  use.id.as.filename 1 \
		--stringparam  root.filename  '' \
		--nonet http://docbook.sourceforge.net/release/xsl/current/xhtml/onechunk.xsl \
		$< || \
		cp $(srcdir)/gnubg/allabout.html .

gnubg.texi: gnubgdb.xml
	$(DOCBOOK2TEXI) --encoding=ISO-8859-1 --string-param prefer-textobjects=1 $< || \
	cp $(srcdir)/gnubg/gnubg.texi .

gnubg.info: gnubg.texi
	makeinfo --force $< -o $@ || \
	cp $(srcdir)/gnubg/gnubg.info .

html_node: gnubg.texi
	makeinfo --html $< -o $@

install-data-hook:
	(cd $(DESTDIR)$(mandir)/man6; \
	for file in *.6; do \
		GZIP=$(GZIP_ENV) gzip -f $${file}; \
	done)

uninstall-hook:
	(cd $(DESTDIR)$(mandir)/man6; \
	for file in *.gz; do \
		rm -f $${file}; \
	done)

MOSTLYCLEANFILES=gnubg.html allabout.html gnubg.6 gnubg.info gnubg.texi manpage.links manpage.refs
CLEANFILES=gnubg.texi gnubg.info gnubg.6 gnubg.pdf allabout.pdf
