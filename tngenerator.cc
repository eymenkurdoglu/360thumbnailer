#include "tngenerator.hh"

/* Constructor */
TNGenerator::TNGenerator ( const char* _path_, 
							   double _ts_,
							   double _yaw_,
							   double _pitch_,
							   double _fov_,
							   int _arw_,
							   int _arh_ )
: invideo_path ( _path_ )
, pngFile ( fopen( "thumbnail.png", "wb") )
, pFormatCtx ( nullptr )
, pCodecCtx ( nullptr )
, pVideoStream ( nullptr )
, pCodec ( nullptr )
, pYUVframe ( nullptr )
, pRGBframe ( nullptr )
, pRECframe ( nullptr )
, sws_ctx ( nullptr )
, i_vidstream_ix ( -1 )
, d_timestamp ( _ts_ )
, d_yaw ( _yaw_ )
, d_pitch ( _pitch_ )
, d_fov ( _fov_ )
, i_aspectratio_w ( _arw_ )
, i_aspectratio_h ( _arh_ )
{
	av_register_all();
	
	/* Open the video */
	avformat_open_input( &pFormatCtx, invideo_path.c_str(), nullptr, nullptr );
	avformat_find_stream_info( pFormatCtx, nullptr );
	
	if ( d_timestamp*AV_TIME_BASE >= pFormatCtx->duration )
		complain_and_exit ( "> Timestamp exceeds video duration." );
	
	/* Find the video stream */	
	for ( int i = 0; i < pFormatCtx->nb_streams; i++ ) {
		if ( pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ) {
		  i_vidstream_ix = i;
		  break;
		}
	}
	
	if ( i_vidstream_ix == -1 )
		complain_and_exit ( "> No video stream in file." );
	else
		pVideoStream = pFormatCtx->streams[ i_vidstream_ix ];
	
	/* Init Video Codec and Video Codec Context */
	pCodec = avcodec_find_decoder( pVideoStream->codecpar->codec_id );
	if ( pCodec == nullptr )
		complain_and_exit ( "> Unsupported codec." );
	else {
		pCodecCtx = avcodec_alloc_context3( pCodec );
		avcodec_parameters_to_context( pCodecCtx, pFormatCtx->streams[ i_vidstream_ix ]->codecpar );
	}
	/* Init PNG Codec and PNG Codec Context */
	i_thumbnail_height = i_thumbnail_width * i_aspectratio_h / i_aspectratio_w;
	
	pCodecPng = avcodec_find_encoder( AV_CODEC_ID_PNG );
	if ( pCodecPng == nullptr )
		complain_and_exit ( "> Cannot open PNG codec." );
	else {
		pCodecCtxPng = avcodec_alloc_context3( pCodecPng );
		pCodecCtxPng->pix_fmt = AV_PIX_FMT_RGB24;
		pCodecCtxPng->width = i_thumbnail_width;
		pCodecCtxPng->height = i_thumbnail_height;
		pCodecCtxPng->time_base.num = pCodecCtxPng->time_base.den = 1;
		pCodecCtxPng->compression_level = 10;		
	}
	
	/* Open Video Codec */
	avcodec_open2( pCodecCtx, pCodec, nullptr );
	
	/* Open PNG Codec */
	avcodec_open2( pCodecCtxPng, pCodecPng, nullptr );
	
	i_eqrect_width = pCodecCtx->width;
	i_eqrect_height = pCodecCtx->height;
	i_pix_fmt = pCodecCtx->pix_fmt;
	
	/* Allocate a YUV frame and an RGB frame for equirectangular, and another RGB frame for rectilinear */
	if ( new_frame( &pYUVframe, i_eqrect_width, i_eqrect_height, i_pix_fmt ) < 0 )
		complain_and_exit ( "> YUV eqrect frame alloc. problem" );
	if ( new_frame( &pRGBframe, i_eqrect_width, i_eqrect_height, AV_PIX_FMT_RGB24 ) < 0 )
		complain_and_exit ( "> RGB eqrect frame alloc. problem" );
	if ( new_frame( &pRECframe, i_thumbnail_width, i_thumbnail_height, AV_PIX_FMT_RGB24 ) < 0 )
		complain_and_exit ( "> RGB rectilinear frame alloc. problem" );
	
	/* Init SwsContext struct to perform YUV420 to RGB24 conversion */
	sws_ctx = sws_getContext( i_eqrect_width, 
							  i_eqrect_height,
							  i_pix_fmt,
							  i_eqrect_width,
							  i_eqrect_height,
							  AV_PIX_FMT_RGB24,
							  SWS_BILINEAR, NULL, NULL, NULL );
	if ( sws_ctx == nullptr )
		complain_and_exit ( "> Sws context failed to init." );
		
	/* Init AVPackets to hold compressed video and image */											 
	av_init_packet( &pkt );
	av_init_packet( &pktPng );
	
	/* Convert given timestamp to AVStream.time_base units */
	i_target_pts = d_timestamp * pVideoStream->time_base.den / pVideoStream->time_base.num;
	
	/* Due to B-frames, there can be an offset between DTS and PTS of the I-frame. With this offset, find a rough DTS estimate for timestamp to be used by av_seek_frame. The flags ensure that the most recent I-frame before the provided DTS is found. */
	i_target_dts = i_target_pts + get_offset_dts( pFormatCtx, pCodecCtx, &pkt, i_vidstream_ix );
	av_seek_frame( pFormatCtx, i_vidstream_ix, i_target_dts, AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD );
}

/* Destructor */
TNGenerator::~TNGenerator () {
	av_packet_unref( &pkt );
	av_frame_free( &pYUVframe );
	av_frame_free( &pRGBframe );
	av_frame_free( &pRECframe );
	avcodec_close( pCodecCtx );
	avformat_close_input( &pFormatCtx );
    fclose( pngFile );
    av_packet_unref( &pktPng );
    avcodec_close( pCodecCtxPng );	
}

/* This method initializes an AVFrame object, inits some data members and allocates buffers according to the given pixel format */
int TNGenerator::new_frame( AVFrame** f, int w, int h, AVPixelFormat fmt ) {
	
	*f = av_frame_alloc();
	if( (*f) == nullptr ) return -1;
	else {
		(*f)->format = fmt;
		(*f)->width = w;
		(*f)->height = h;
		av_frame_get_buffer( *f, 32 );
	}
return 0;	
}

/* Generic method to display error messages and quit */
void TNGenerator::complain_and_exit ( const char* message ) {
	fprintf( stderr, "%s\n", message );
	exit( EXIT_FAILURE );
}

/* This method checks the DTS and PTS of the AVPacket that holds the compressed I-frame, in order to find the offset between the two. */
long int TNGenerator::get_offset_dts ( AVFormatContext* const pFormatCtx,
									  AVCodecContext* const pCodecCtx,
									  AVPacket* const pPkt,
									  int i_vidstream_ix )
{
	long int dts;
	while ( av_read_frame( pFormatCtx, pPkt ) == 0 ) {
		if( pPkt->stream_index == i_vidstream_ix )
			while ( avcodec_send_packet( pCodecCtx, pPkt ) == 0 ) ;
			dts = pPkt->dts;
			avcodec_flush_buffers( pCodecCtx ); // flush the decoder buffer!
			break;
	}
	// seek back to the beginning
	av_seek_frame( pFormatCtx, i_vidstream_ix, dts, AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD );
	av_packet_unref( pPkt );
	
return dts;
}

/* This method decodes video frames starting from the most recent I-frame up to the desired frame */
void TNGenerator::grab_and_save() {

	// First grab the I-frame and send it to decoder
	do {
		while ( av_read_frame( pFormatCtx, &pkt ) != 0 ) ;
	} while ( pkt.stream_index != i_vidstream_ix );
	while ( avcodec_send_packet( pCodecCtx, &pkt ) == 0 ) ;
	
	// Keep decoding until PTS of next packet exceeds our target
	if ( pkt.pts != i_target_pts ) {
		do {
			avcodec_receive_frame( pCodecCtx, pYUVframe );
			av_packet_unref( &pkt );
			do {
				av_read_frame( pFormatCtx, &pkt );
			} while ( pkt.stream_index != i_vidstream_ix );
			avcodec_send_packet( pCodecCtx, &pkt );
		} while ( pkt.pts > i_target_pts );
	} else {
		avcodec_receive_frame( pCodecCtx, pYUVframe );
	}

	printf("> Frame found. "); // convert from YUV to RGB
	sws_scale ( sws_ctx,
				pYUVframe->data,
				pYUVframe->linesize,
				0,
				pCodecCtx->height,
				pRGBframe->data,
				pRGBframe->linesize );
	printf("Saving...\n");
	save_thumbnail(); // Call the equirectangular to rectilinear projection method and save
}

/* This method performs the rectilinear projection and saves the result as a PNG file */
void TNGenerator::save_thumbnail() {
	
	lg_etg_apperture ( (inter_C8_t*)pRGBframe->data[0],
									i_eqrect_width,
									i_eqrect_height,
									3,
					   (inter_C8_t*)pRECframe->data[0],
									i_thumbnail_width,
									i_thumbnail_height,
									3,
									d_yaw*(LG_PI/180.0),
									d_pitch*(LG_PI/180.0),
									0*(LG_PI/180.0), // can add in roll if we like
									d_fov*(LG_PI/180.0),
									li_bilinearf // fast interpolation
					 );
	save_as_png();
}

/* This method encodes the rectilinear image in PNG and writes it on disk */
void TNGenerator::save_as_png() {
	
	if ( avcodec_send_frame( pCodecCtxPng, pRECframe ) < 0 ) {
		printf("avcodec_send_frame err\n");
		return;
	}
	if ( avcodec_receive_packet( pCodecCtxPng, &pktPng ) < 0 ) {
		printf("avcodec_receive_packet err\n");
		return;
	}
	
    fwrite( pktPng.data, 1, pktPng.size, pngFile );
}