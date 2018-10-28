sed 's/,/=/1' gcc_8K_0Lookup.stats | sort -t$'=' -k2n | sed 's/=/,/2'
