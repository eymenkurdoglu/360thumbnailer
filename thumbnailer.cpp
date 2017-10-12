#include <stdio.h>
#include <cstdint>
#include "tngenerator.hh"

/* A few points:
 * 1. if timestamp exceeds the video duration, program will terminate.
 * 2. -180 <= yaw (degrees) <= 180 
 * 3. -90 <= pitch (degrees) <= 90 
 * 4. for rectilinear (flat) projection 0 <= fov <= 180, in practice should be < 110 
 * 5. aspect ratio width and height values are positive integer pairs such as [16 9] or [4 3] 
 * 6. the output PNG image is named "thumbnail.png" and is located in the same folder as the code */
int main ( int argc, char* argv[] ) {
	
	if ( argc != 8 ) {
		printf( "> Usage: %s [input file] [timestamp] [yaw] [pitch] [fov] [aspect ratio width] [aspect ratio height]\n", argv[0] );
		exit( EXIT_FAILURE );
	}
	
	TNGenerator thumbnail( argv[1],
				   atof( argv[2] ),
				   atof( argv[3] ),
				   atof( argv[4] ),
				   atof( argv[5] ),
				   atoi( argv[6] ),
				   atoi( argv[7] )
				 );
	
	printf("> Seeking frame...\n");
	thumbnail.grab_and_save();
	printf("> Done.\n");
return 0;
}
