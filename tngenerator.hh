#ifndef TNGENERATOR_HH
#define TNGENERATOR_HH

#include <string>
#include <stdio.h>
#include <cstdint>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <gnomonic-all.h>
#include <inter-all.h>
}

class TNGenerator {

public:
	TNGenerator( const char*, double, double, double, double, int, int );
   ~TNGenerator();
    int new_frame ( AVFrame**, int, int, AVPixelFormat );
    long int get_offset_dts ( AVFormatContext* const, AVCodecContext* const, AVPacket* const, int );
	void complain_and_exit ( const char* );
	void grab_and_save ( void );
	void save_thumbnail( void );
	void save_as_png( void );
	
private:
	std::string invideo_path;
	FILE* pngFile; 
	
	AVFormatContext* pFormatCtx;
	AVCodecContext* pCodecCtx;
	AVCodecContext* pCodecCtxPng;
	AVStream* pVideoStream;
	AVCodec* pCodec;
	AVCodec* pCodecPng;
	AVPacket pkt;
	AVPacket pktPng;
	AVFrame* pYUVframe;
	AVFrame* pRGBframe;
	AVFrame* pRECframe;
	struct SwsContext* sws_ctx;
	
	int i_vidstream_ix;
	long int i_target_pts;
	long int i_target_dts;
	int i_eqrect_width;
	int i_eqrect_height;
	AVPixelFormat i_pix_fmt;
	const int i_thumbnail_width = 960;
	int i_thumbnail_height;
	
	double d_timestamp;
	double d_yaw;
	double d_pitch;
	double d_fov;
	int i_aspectratio_w;
	int i_aspectratio_h;	
	
};

#endif
