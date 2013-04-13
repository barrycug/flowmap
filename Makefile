all: 9.pdf 13.pdf

%.pdf: %.ps
	ps2pdf -dDEVICEHEIGHTPOINTS=306 $<

voronoi/voronoi:
	cd voronoi; make

dijkstra: dijkstra.c
	cc -g -Wall -O3 -o dijkstra dijkstra.c -lm

13.ps: voronoi/voronoi dijkstra science-flowmap-subd09.csv land.ps make-triangulation draw-route
	( cat land.ps; cat science-flowmap-subd13.csv | ./make-triangulation | ./dijkstra | ./draw-route ) > 13.ps

9.ps: voronoi/voronoi dijkstra science-flowmap-subd09.csv land.ps make-triangulation draw-route
	( cat land.ps; cat science-flowmap-subd09.csv | ./make-triangulation | ./dijkstra | ./draw-route ) > 9.ps
