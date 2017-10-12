# 360thumbnailer

**Build instructions**
External libraries used in this project are 
* FFmpeg (`libav*`)
* `libgnomonic` (https://github.com/FoxelSA/libgnomonic).

I used the `libav*` libraries to demux the MP4 file, to decode the H.264 bytestream and to encode the final PNG image, while the `libgnomonic` library (along with its sibling library `libinter` for interpolation) allowed me to transform the equirectangular projection to the rectilinear projection. Just `make && sudo make install` the external libraries (libgnomonic comes with libinter, which can be found in the `lib` folder in the git). The included header files for libgnomonic were then installed in `/usr/include/gnomonic/0.4.1/` and `/usr/include/inter/0.3/`, while FFmpeg headers are located at `/usr/local/include`. The binaries for libgnomonic are `/usr/lib/libgnomonic.a.0.4.1` and `/usr/lib/libinter.a.0.3`. Finally, libgnomonic also requires `libgomp`. 

The code can then be run by `./thum [input file] [timestamp] [yaw] [pitch] [fov] [aspect ratio width] [aspect ratio height]`, where the input arguments are not optional and are as defined in the problem definition. 
Examples:
- ./thum a.mp4 0 180 -20 110 16 10
- ./thum b.mp4 40.12 180 -20 90 4 3

[Here](https://github.com/eymenkurdoglu/eymenkurdoglu.github.io/blob/master/thumbnail.png) is a sample output.

**General Flow of the Code**
The input MP4 file is opened and demuxed using ffmpeg. Then, assuming the timestamp value provided is valid (between 0 and the duration of the video in seconds), we seek to the nearest I-frame (keyframe) that comes before the timestamp. Then, we need to decode the frames up to the frame that is less than or equal to the given timestamp. The coding structure can be anything, such as IPPP or hierarchical-B or any other, so we need to keep track of the PTS values in the AVPacket structs, rather than the DTS. Here, timestamp==0 corresponds to the very first frame. 

Once we acquire the frame and decode it, the raw pixels are usually in YUV420 format, which is a planar format, meaning that the chroma values come right after the luminance values in the memory. Since libgnomonic only works with RGB pixels, we convert YUV to RGB, which means each pixel is now 3-byte wide. 

The gnomonic projection (known mostly as rectilinear in computer graphics, or flat projection) works by placing an NxM pixel grid on the sphere co-located at the origin (according to the yaw and pitch values provided) through a rotation operation (initially the grid is assumed to be sitting at a known, arbitrary vector). Then, for each pixel on this grid, a line ray is drawn from the center of the pixel to the origin, and the color value is found by an 'interpolation'. To make sure the projection is correct, we can take a look at how the straight lines look; if they are indeed straight, this is a rectilinear projection. I randomly chose 960 pixels as the width of the projection, and the corresponding height is found through the given aspect ratio. This only affects the size of the image, not how it looks, as the latter depends on the Field of View and the angles. Finally, we can also change the roll angle if we like, as if we are tilting our head. 

After this phase all that remains is to take the RGB image and encode it into a PNG image and write it to disk.
