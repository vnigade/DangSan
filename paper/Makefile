OUT = paper.pdf
TEX = *.tex 
BIB = refs.bib
SRC = $(TEX) $(BIB) $(IMG)

all: $(OUT)

$(OUT): $(SRC)

clean:
	rm -f $(OUT:.pdf=).{aux,bbl,blg,log,out,pdf}

.SUFFIXES: .tex .pdf

.tex.pdf:
	pdflatex --shell-escape $(<:.tex=)
	bibtex $(<:.tex=)
	pdflatex $(<:.tex=)
	pdflatex $(<:.tex=)
