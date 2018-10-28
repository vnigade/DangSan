set terminal png transparent nocrop enhanced size 450,320  
set output 'histograms.8.png'
#set border 3 front lt black linewidth 1.000 dashtype solid
set boxwidth 0.8 absolute
set style fill   solid 1.00 noborder
set grid nopolar
set grid noxtics nomxtics ytics nomytics noztics nomztics \
 nox2tics nomx2tics noy2tics nomy2tics nocbtics nomcbtics
set grid layerdefault   lt 0 linewidth 0.500,  lt 0 linewidth 0.500
set key bmargin center horizontal Left reverse noenhanced autotitle columnhead nobox
set style histogram rowstacked title textcolor lt -1 offset character 2, 0.25, 0
set datafile missing '-'
set style data histograms
set xtics border in scale 0,0 nomirror rotate by -45  autojustify
#set xtics  norangelimit font ",8"
set xtics   ()
set ytics border in scale 0,0 mirror norotate  autojustify
set ytics autofreq  norangelimit font ",8"
set ztics border in scale 0,0 nomirror norotate  autojustify
set cbtics border in scale 0,0 mirror norotate  autojustify
set rtics axis in scale 0,0 nomirror norotate  autojustify
set paxis 1 tics border in scale 0,0 nomirror norotate  autojustify
set paxis 1 tics autofreq  rangelimit
set paxis 2 tics border in scale 0,0 nomirror norotate  autojustify
set paxis 2 tics autofreq  rangelimit
set paxis 3 tics border in scale 0,0 nomirror norotate  autojustify
set paxis 3 tics autofreq  rangelimit
set paxis 4 tics border in scale 0,0 nomirror norotate  autojustify
set paxis 4 tics autofreq  rangelimit
set paxis 5 tics border in scale 0,0 nomirror norotate  autojustify
set paxis 5 tics autofreq  rangelimit
set paxis 6 tics border in scale 0,0 nomirror norotate  autojustify
set paxis 6 tics autofreq  rangelimit
set paxis 7 tics border in scale 0,0 nomirror norotate  autojustify
set paxis 7 tics autofreq  rangelimit
set title "Immigration from different regions\n(give each histogram a separate title)" 
set xlabel "(Same plot using rowstacked rather than clustered histogram)" 
set xlabel  offset character 0, -2, 0 font "" textcolor lt -1 norotate
set ylabel "Immigration by decade" 
set yrange [ 0.00000 : 900000. ] noreverse nowriteback
x = 0.0
i = 23
## Last datafile plotted: "immigration.dat"
plot newhistogram "TRANSE", 'data-sample.dat' using "TRANSE-SG-Total":xtic(1) t col, '' u "TRANSE-SG-Good" t col, '' u "TRANSE-SG-Bad" t col, newhistogram "HOLE", '' u "HOLE-SG-Total":xtic(1) t col, '' u "HOLE-SG-Good" t col, '' u "HOLE-SG-Bad" t col
