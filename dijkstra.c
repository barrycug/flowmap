#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <time.h>

void *cmalloc(size_t s) {
	void *p = malloc(s);
	if (p == NULL) {
		fprintf(stderr, "memory allocation failed for %d\n", s);
		exit(1);
	}
	return p;
}

struct point {
	double lat;
	double lon;

	int *neighbors;
	double *dists;
	int neighborcount;

	double d;
	int strength;

	struct point *prev;
	struct point *next;
} *points = NULL;

int npoint = 0;
int nalloc = 0;

unsigned short *global_next = NULL;

#define GLOBAL_NEXT(a,b) global_next[(a) * npoint + (b)]

double ptdist(double lat1, double lon1, double lat2, double lon2) {
	double latd = lat2 - lat1;
	double lond = fabs(lon2 - lon1);

	if (fabs(lon2 + 360 - lon1) < lond) {
#if 0
		fprintf(stderr, "%lf,%lf to %lf,%lf better than %lf   %lf\n",
			lat1, lon1, lat2, lon2 + 360, lat2, lond);
#endif
		lond = fabs(lon2 + 360 - lon1);
	}
	if (fabs(lon1 + 360 - lon2) < lond) {
#if 0
		fprintf(stderr, "%lf,%lf to %lf,%lf better than %lf  %lf\n",
			lat2, lon2, lat1, lon1 + 360, lat1, lond);
#endif
		lond = fabs(lon1 + 360 - lon2);
	}

	lond = lond * cos((lat1 + lat2) / 2 * M_PI / 180);

	double ret = sqrt(latd * latd + lond * lond);
	return ret * ret;
}

int findclosest(double lat, double lon, double *ret) {
	double bestd = INT_MAX;
	int best = -1;

	int i;
	for (i = 0; i < npoint; i++) {
		double d = ptdist(lat, lon, points[i].lat, points[i].lon);

		if (ret != NULL) {
			ret[i] = d;
		}

		if (d < bestd) {
			bestd = d;
			best = i;
		}
	}

	return best;
}

double dist(int one, int two) {
        double ret = ptdist(points[one].lat, points[one].lon, points[two].lat, points[two].lon);
        return ret / sqrt(points[one].strength) / sqrt(points[two].strength);
}

void point(double lat, double lon, int strength) {
	if (npoint + 2 >= nalloc) {
		nalloc = npoint + 1000;
		points = realloc(points, nalloc * sizeof(struct point));
	}

	points[npoint].lat = lat;
	points[npoint].lon = lon;
	points[npoint].strength = strength;
	points[npoint].neighborcount = 0;
	points[npoint].neighbors = NULL;
	points[npoint].dists = NULL;
	npoint++;
}

void addneighbor(int i, int j, double dist) {
	int x;

	for (x = 0; x < points[i].neighborcount; x++) {
		if (points[i].neighbors[x] == j) {
			return;
		}
	}

	points[i].neighbors = realloc(points[i].neighbors, (points[i].neighborcount + 1) * sizeof(int));
	points[i].dists = realloc(points[i].dists, (points[i].neighborcount + 1) * sizeof(double));

	points[i].neighbors[points[i].neighborcount] = j;
	points[i].dists[points[i].neighborcount] = dist;

	points[i].neighborcount++;
}

void printPath(int start, int dest) {
	int i = start;

	while (i != USHRT_MAX) {
		printf("%.6f,%.6f ", points[i].lat, points[i].lon);
		i = GLOBAL_NEXT(i, dest);

		if (i == dest) {
			return;
		}
	}

	printf("\nERROR found ourselves at %d\n", i);
}

void dijkstra(int s, int dest) {
	int i;
	int visited[npoint];
	int prev[npoint];
	int remaining = 0;

	struct point head, tail;
	head.prev = NULL;
	head.next = &tail;
	head.d = -1;

	tail.prev = &head;
	tail.next = NULL;
	tail.d = INT_MAX;

	for (i = 0; i < npoint; ++i) {
		prev[i] = -1;

                if (0 && GLOBAL_NEXT(s, i) != USHRT_MAX) {
			//fprintf(stderr, "already know %d to %d follows %d\n", s, i, GLOBAL_NEXT(s, i));
			visited[i] = 1;
                } else {
			if (i == s) {
				points[i].d = 0;

				struct point *second = head.next;
				points[i].next = second;
				second->prev = &points[i];
				head.next = &points[i];
				points[i].prev = &head;
			} else {
				points[i].d = INT_MAX;

				struct point *penult = tail.prev;
				penult->next = &points[i];
				points[i].prev = penult;
				points[i].next = &tail;
				tail.prev = &points[i];
			}

			visited[i] = 0;
			remaining++;
		}
	}

	while (remaining > 0) {
		int u = head.next - points;

		if (points[u].d == INT_MAX) {
			fprintf(stderr, "how did we get infinity for %d\n", u);
			break;
		}

		visited[u] = 1;
		remaining--;

		int i;
		for (i = u; i != s; i = prev[i]) {
			if (GLOBAL_NEXT(prev[i], u) == USHRT_MAX) {
				GLOBAL_NEXT(prev[i], u) = i;
			}
			if (GLOBAL_NEXT(i, s) == USHRT_MAX) {
				GLOBAL_NEXT(i, s) = prev[i];
			}
		}

		points[u].prev->next = points[u].next;
		points[u].next->prev = points[u].prev;

		{
			int a, v;
			for (a = 0; a < points[u].neighborcount; a++) {
				v = points[u].neighbors[a];

				if (!visited[v]) {
					double alt = points[u].d + points[u].dists[a];
					if (alt < points[v].d) {
						points[v].d = alt;
						prev[v] = u;

						while (points[v].d < points[v].prev->d) {
							struct point *prev = points[v].prev;
							struct point *next = points[v].next;
							struct point *prevprev = points[v].prev->prev;

							next->prev = prev;
							prev->next = next;

							points[v].next = prev;
							prev->prev = &points[v];

							prevprev->next = &points[v];
							points[v].prev = prevprev;
						}
					}
				}
			}
		}
	}
}


void route(double lat1, double lon1, double lat2, double lon2, int strength) {
	int one = findclosest(lat1, lon1, NULL);
	int two = findclosest(lat2, lon2, NULL);

	if (one == two) {
		printf("%d ", strength);
		printf("%.6f,%.6f\n", lat1, lon1);
		return;
	}

	int next = GLOBAL_NEXT(one, two);

	if (next == USHRT_MAX) {
		dijkstra(one, two);

		next = GLOBAL_NEXT(one, two);
		if (next == USHRT_MAX) {
			fprintf(stderr, "don't know how to get from %d to %d!\n", one, two);
		}
	} else {
		// fprintf(stderr, "know how to get from %d to %d: %d\n", one, two, next);
	}

	printf("%d ", strength);
	printf("%.6f,%.6f ", lat1, lon1);
	printPath(one, two);
	printf("%.6f,%.6f\n", lat2, lon2);
	fflush(stdout);
}

int main() {
        char s[2000];
	int sorted = 0;
	int did = 0;

        while (fgets(s, 2000, stdin)) {
		int strength;
                double lat1, lon1;
                double lat2, lon2;

                if (sscanf(s, "route %lf,%lf %lf,%lf %d", &lat1, &lon1, &lat2, &lon2, &strength) == 5) {
			if (!sorted) {
				if (global_next != NULL) {
					free(global_next);
				}

				global_next = cmalloc(npoint * npoint * sizeof(unsigned short));

				int i;
				for (i = 0; i < npoint * npoint; i++) {
					global_next[i] = USHRT_MAX;
				}

				sorted = 1;
			}

                        route(lat1, lon1, lat2, lon2, strength);
			did++;
                } else if (sscanf(s, "point %lf,%lf %d", &lat1, &lon1, &strength) == 3) {
			point(lat1, lon1, strength);
			sorted = 0;
                } else if (sscanf(s, "link %lf,%lf %lf,%lf", &lat1, &lon1, &lat2, &lon2) == 4) {
			int one = findclosest(lat1, lon1, NULL);
			int two = findclosest(lat2, lon2, NULL);

			double d = dist(one, two);
			addneighbor(one, two, d);
			addneighbor(two, one, d);
                } else {
                        fprintf(stderr, "huh? %s", s);
                }
        }

        return 0;
}

