// ------------------------------------------- IMPORTS -------------------------------------------
#include "bitmap.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

// When Processes are not enough, might suffer page faults etc

// ---------------------------------- Create our Image Structure ----------------------------------
typedef struct {
	struct bitmap * bm;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	int max;
	int start;
	int end;
} img_struct;


// --------------------------------- Number of Thread Declaration ---------------------------------
int numThreads;


// ------------------------------------- Function Declaration -------------------------------------
int iteration_to_color( int i, int max );
int iterations_at_point( double x, double y, int max );
void *compute_image(void *args);


// --------------------------------- Help Function Implementation ---------------------------------
void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates. (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=500)\n");
	printf("-H <pixels> Height of the image in pixels. (default=500)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-n <threads> Number of threads you want to have\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}


// --------------------------------- Main Function Implementation ---------------------------------
int main( int argc, char *argv[] )
{
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.

	const char *outfile = "mandel.bmp";
	double xcenter = 0;
	double ycenter = 0;
	double scale = 4;
	int    image_width = 500;
	int    image_height = 500;
	int    max = 1000;
	int numThreads = 1;
	int i;
	int time_duration;
	struct timeval begin;
	struct timeval end;

	// For each command line argument given,
	// override the appropriate configuration value.
	// NOTES: Add in the -n argument to take in the number of threads the user wants
	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:n:h"))!=-1) {
		switch(c) {
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				scale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'o':
				outfile = optarg;
				break;
			case 'n':
				numThreads = atoi(optarg);
				break;
			case 'h':
				show_help();
				exit(1);
				break;
		}
	}

	// Display the configuration of the image.
	printf("mandel: x=%lf y=%lf scale=%lf max=%d outfile=%s numThreads=%d\n",xcenter,ycenter,scale,max,outfile, numThreads);

	// Begin Time of Day Count
	gettimeofday(&begin, NULL);

	// The height of pixels each thread will have 
	int height = image_height / numThreads;

	// Allocate Memory to our Structures and Threads 
	pthread_t *threads = malloc(numThreads * sizeof(pthread_t));
	img_struct *man_struct = malloc(numThreads * sizeof(img_struct));
	
	// Create a bitmap of the appropriate size.
	struct bitmap *bm = bitmap_create(image_width,image_height);

	// Fill it with a dark blue, for debugging
	bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

	// Compute the Mandelbrot image
	// compute_image(bm,xcenter-scale,xcenter+scale,ycenter-scale,ycenter+scale,max);

	// Replace the above function with a For-Loop
	for(i = 0 ; i < numThreads ; ++i)
	{
		// Begin populating the structure
		man_struct[i].bm = bm;
		man_struct[i].xmin = xcenter - scale;
		man_struct[i].xmax = xcenter + scale;
		man_struct[i].ymin = ycenter - scale;
		man_struct[i].ymax = ycenter + scale;
		man_struct[i].max = max;

		// Specify the start and end of each file for it to be computed later
		if(i == 0)
		{
			man_struct[i].start = 0;
			man_struct[i].end = height;
		}
		else
		{
			man_struct[i].start = man_struct[i - 1].end;
			man_struct[i].end = man_struct[i - 1].end + height;
		}

		// call pthread_create()
		// pass in address of the structure and thread. Call compute images
		pthread_create(&threads[i], NULL, compute_image, (void*) &man_struct[i]);
	}

	// call pthread_join
	// thread in pthread_Create()
	for(i = 0 ; i < numThreads ; ++i)
	{
		pthread_join(threads[i], NULL);
	}

	// Save the image in the stated file.
	if(!bitmap_save(bm,outfile)) {
		fprintf(stderr,"mandel: couldn't write to %s: %s\n",outfile,strerror(errno));
		return 1;
	}

	gettimeofday(&end, NULL);
	time_duration = ((end.tv_sec - begin.tv_sec) * 1000000 + (end.tv_usec - begin.tv_usec));	
	printf("Duration: %d\n", time_duration );
	
	return 0;
}


// ------------------------------------ Compute Image Function ------------------------------------
/* ********************************************************************************************* *
 * Compute an entire Mandelbrot image, writing each point to the given bitmap.					 *
 * Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"				 *	
 * ********************************************************************************************* */
void *compute_image(void *args)
{
	// struct computeImageStruct *mystruct = (struct mystruct *) args;
	img_struct *man_struct = (img_struct *) args; 

	// For every variable here that is in our struct, we will point to it using "man_struct"

	int i,j;

	int width = bitmap_width(man_struct -> bm);
	int height = bitmap_height(man_struct -> bm);

	// For every pixel in the image...
	// start = height divided by numThreads * threadID (assuming threadId starts at 0)
	// end = height divided by numThreads * threadID + by height/numThread
	// j = start
	// j < end

	// Grab our start and end from the structure
	int start = man_struct -> start;
	int end = man_struct -> end;

	for(j = start ; j < end ; j++) 
	{
		for(i = 0 ; i < width ; i++) 
		{
			// Determine the point in x,y space for that pixel.
			double x = man_struct -> xmin + i * (man_struct -> xmax - man_struct -> xmin)/width;
			double y = man_struct -> ymin + j * (man_struct -> ymax - man_struct -> ymin)/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x, y, man_struct -> max);

			// Set the pixel in the bitmap.
			bitmap_set(man_struct -> bm, i, j, iters);
		}
	}

	return 0;
}


// ----------------------------------- Point Iteration Function -----------------------------------
/* ********************************************************************************************* *
 * Return the number of iterations at point x, y												 *		
 * in the Mandelbrot space, up to a maximum of max.												 *
 * ********************************************************************************************* */
int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iteration_to_color(iter,max);
}


// ----------------------------------- Color Iteration Function -----------------------------------
/* ********************************************************************************************* *
 * Convert a iteration number to an RGBA color.												     *
 * Here, we just scale to gray with a maximum of imax.											 *
 * Modify this function to make more interesting colors.										 *
 * ********************************************************************************************* */
int iteration_to_color( int i, int max )
{
	int gray = 255*i/max;
	return MAKE_RGBA(gray,gray,gray,0);
}
