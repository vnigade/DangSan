OUT = main.pdf
TEX = *.tex 
BIB = refs.bib
SRC = $(TEX) $(BIB) $(IMG)

all: $(OUT)

$(OUT): $(SRC)

clean:
	rm -f $(OUT:.pdf=).aux
	rm -f $(OUT:.pdf=).bbl
	rm -f $(OUT:.pdf=).blg
	rm -f *.log
	rm -f $(OUT:.pdf=).out
	rm -f $(OUT:.pdf=).snm
	rm -f $(OUT:.pdf=).toc
	rm -f $(OUT:.pdf=).nav
	rm -f *.pdf

.SUFFIXES: .tex .pdf

.tex.pdf:
	pdflatex --shell-escape $(<:.tex=)
	bibtex $(<:.tex=)
	pdflatex $(<:.tex=)
	pdflatex $(<:.tex=)
