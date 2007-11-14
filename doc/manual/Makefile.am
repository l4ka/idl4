## Process this file with automake to produce Makefile.in

LATEX=		@LATEX@
BIBTEX=		@BIBTEX@
DVIPS=		@DVIPS@
DVIPDF=		@DVIPDF@
RM_F=		rm -f

TEXFILES=	appendix.tex commands.tex idl.tex manual.bib manual.tex \
		quickref.tex working.tex
EXTRA_DIST=	$(TEXFILES)
SUFFIXES=	.tex .aux .bbl .toc .pdf .dvi .ps
CLEANFILES=	tex-stamp version.tex

if HAS_DVIPDF
manual_PDF=	manual.pdf
endif

if HAS_DVIPS
manual_PS=	manual.ps
endif

if HAS_LATEX

all-local:	tex-stamp version.tex $(manual_PS) $(manual_PDF)

.tex.dvi:
	${LATEX} $<

.dvi.pdf:
	${DVIPDF} $<

.dvi.ps:
	${DVIPS} $< -o

.tex.toc:
	${LATEX} $<
	${RM_F} *.dvi

.aux.bbl:
	${BIBTEX} $*
	${LATEX} $*
	${RM_F} $*.dvi

.tex.aux:
	${LATEX} $<
	${RM_F} $*.dvi

manual.dvi:	manual.toc manual.bbl
manual.ps: 	manual.dvi
manual.pdf: 	manual.dvi

endif

version.tex:
	echo \\newcommand{\\Version}{${VERSION}} > $@

tex-stamp: $(TEXFILES)
	@for F in $(TEXFILES); do \
	  if test ! -f $${F} -a ! -L $${F}; then \
	    ${LN_S} $(srcdir)/$${F} $${F}; \
	  fi \
	done
	@echo timestamp > tex-stamp 2>/dev/null

clean-local:
	rm -f *.dvi *.aux *.log *~ *.bbl *.blg *.toc *.ps *.pdf
	for F in $(TEXFILES); do \
	  test ! -L $${F} || ${RM_F} $${F}; \
	done
