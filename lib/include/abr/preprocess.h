/*
 *  preprocess.h
 *  $Id$
 *
 *  Author
 *  Roman Juranek <ijuranek@fit.vutbr.cz>
 *
 *  Graph@FIT
 *  Department of Computer Graphics and Multimedia
 *  Faculty of Information Technology
 *  Brno University of Technology
 *
 *  Description
 *
 */

#ifndef _PREPROCESS_H_
#define _PREPROCESS_H_

#include <opencv/cxcore.h>

// Elementary operations available in preprocessing
#define PP_COPY     0x01    ///< Copy or resize image
#define PP_INTEGRAL 0x02    ///< Integral image
#define PP_CONV     0x04    ///< Convolution images (with inverted sign bit)
#define PP_ICONV    0x08    ///< Rearranged (interleaved) convolution images
#define PP_LBP      0x10    ///< Precalculated LBP operator images

// Operations with added dependencies; e.g. integral image need a copy of image to be made
// and thus PP_INTEGRAL_IMAGE invokes PP_COPY and PP_INTEGRAL operations.
#define PP_COPY_IMAGE     (PP_COPY)
#define PP_INTEGRAL_IMAGE (PP_COPY | PP_INTEGRAL)
#define PP_CONV_IMAGE     (PP_COPY | PP_CONV)
#define PP_ICONV_IMAGE    (PP_COPY | PP_CONV | PP_ICONV)
#define PP_LBP_IMAGE      (PP_COPY | PP_CONV | PP_LBP)
#define PP_ALL            (PP_COPY | PP_INTEGRAL | PP_CONV | PP_ICONV | PP_LBP )

/// Structure holding various versions of input image.
struct PreprocessedImage
{
    CvSize sz;          ///< Size of a source image
    
    int block_count[4]; ///< Count of blocks in convolution images
    int iblock_size[4]; ///< Size of blocks in 'iconv' images
    int irow_size[4];   ///< Size of row in 'iconv' images
    int cblock_size[4]; ///< Size of blocks in 'conv' images

    int * xtbl;         ///< Column addressing table in 'iconv'
    int * ytbl;         ///< Row addressing table in 'iconv'

    IplImage tmp;       ///< Temporary image for convolution
    IplImage intensity; ///< Intensity image (copied or scaled source image). This is source for all preprocessing.
    IplImage integral;  ///< Integral image.
    IplImage conv[4];   ///< Block-rearranged convolution images
    IplImage iconv[4];  ///< 2x2 Local-rearranged convolution images
    IplImage lbp[4];    ///< Pre-calculated LBP operator images
};

struct PreprocessedPyramid
{
    int octaves;
    int levels_per_octave;
    std::vector<PreprocessedImage*> PI;
};

extern "C" {

void init_preprocess();

PreprocessedImage * create_preprocessed_image(CvSize src_sz);

void preprocess_image(IplImage * img, PreprocessedImage * PI, int options);

void release_preprocessed_image(PreprocessedImage ** PI);

PreprocessedPyramid * create_pyramid(CvSize base_sz, CvSize min_sz, int octaves, int levels_per_octave);

void release_pyramid(PreprocessedPyramid ** PP);

void insert_image(IplImage * img, PreprocessedPyramid * PP, int options);

}

#endif
