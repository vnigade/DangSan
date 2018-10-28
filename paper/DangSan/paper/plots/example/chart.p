set title "astar benchmark"
#set xdata time
#set timefmt "%Y-%m-%d"
#set datafile separator " "
#set terminal png size 350,262 enhanced truecolor font 'Verdana,9'
#set terminal svg size 350,262 dynamic enhanced fname "Verdana" fsize 9 
set terminal svg size 350,262 noenhanced fname "Verdana" fsize 9
#set terminal pdf size 350,262 enhanced font 'Verdana,9'
#set term postscript eps size 350, 262 color blacktext "Helvetica" 24
set output "articles_imported.svg"
set ylabel "Number of Pointers"
set xlabel "1000 Objects with object size label"
set xrange ["1":"1000"]
set yrange ["0":"20000"]
set pointsize 0.8
#set format x "%d/%m"
set border 11
#astar
set xtics ("16" 1, "16" 100, "16" 200, "16" 300, "16" 400, "16" 500, "16" 600, "16" 700, "16" 800, "16" 900, "8192" 1000)
#GCC
#set xtics ("16" 1, "16" 100, "16" 200, "512" 300, "512" 400, "512" 500, "8192" 600, "8192" 700, "8192" 800, "8192" 900, "8192" 1000)
#Perlbench
#set xtics ("8" 1, "128" 100, "512" 200, "512" 300, "512" 400, "512" 500, "512" 600, "512" 700, "512" 800, "512" 900, "8192" 1000)
#Xalancbmk
#set xtics ("16" 1, "64" 100, "64" 200, "64" 300, "64" 400, "64" 500, "64" 600, "64" 700, "64" 800, "64" 900, "1024" 1000)
#unset xtics
#set tics front
set key inside top right spacing 1
plot \
  "tmp.csv" using 1:($3+$4+$5) title 'Valid' with filledcurves x1, \
  "tmp.csv" using 1:($3+$4) title 'Duplicate' with filledcurves x1, \
  "tmp.csv" using 1:($3) title 'Unique' with filledcurves x1
