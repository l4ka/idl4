all: chacmos-api.ps

chacmos-api.ps: chacmos-api.tex
	latex chacmos-api.tex
	dvips chacmos-api.dvi

clean::
	rm -fv *.dvi *.aux *.log *~ *.ps
