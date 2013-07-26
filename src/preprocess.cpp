/*
 *  preprocess.cpp
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

#include <cv.h>
#include "preprocess.h"
#include "lbp.h"

#include <iostream>

using namespace std;


static float _kernel1[1] = {1};
static float _kernel2[2] = {0.5,0.5};
static float _kernel3[2] = {0.5,0.5};
static float _kernel4[4] = {0.25,0.25,0.25,0.25};

static CvMat kernel[4];

void init_preprocess()
{
    cvInitMatHeader(&(kernel[0]), 1, 1, CV_32FC1, _kernel1, CV_AUTOSTEP);
    cvInitMatHeader(&(kernel[1]), 1, 2, CV_32FC1, _kernel2, CV_AUTOSTEP);
    cvInitMatHeader(&(kernel[2]), 2, 1, CV_32FC1, _kernel3, CV_AUTOSTEP);
    cvInitMatHeader(&(kernel[3]), 2, 2, CV_32FC1, _kernel4, CV_AUTOSTEP);
}


static inline int align2(int x)
{
    return (x + 1) & ~1;
}

void integrate(IplImage * src, IplImage * dst)
{
    assert(src->width == dst->width);
    assert(src->height == dst->height);

    unsigned char * srcbase = (unsigned char*)src->imageData;
	unsigned * dstbase = (unsigned*)dst->imageData;

	// top-left corner
	*dstbase = *srcbase;

	// first row
	for (int x = 1; x < src->width; ++x)
	{
		*(dstbase+x) = *(dstbase+x-1) + *(srcbase+x);
	}

    srcbase += src->widthStep;
    dstbase += dst->widthStep/sizeof(unsigned);
    
    // rest of image
	for (int y = 1; y < src->height; ++y, srcbase += src->widthStep, dstbase += dst->widthStep/sizeof(unsigned))
	{
		unsigned tmp = 0;

		for (int x = 0; srcbase+x < srcbase+src->width; ++x)
		{
			tmp += *(srcbase+x);
			*(dstbase+x) = tmp + *(dstbase+x-(dst->widthStep/sizeof(unsigned)));
		}
	}
}

static void interleaved_convolution(
        PreprocessedImage * PI,
        IplImage * src,
        int i)
{
    IplImage * tmp = &(PI->tmp);
    IplImage * conv = &(PI->conv[i]);
    CvMat * k = &(kernel[i]);

    // src -> tmp
    cvFilter2D(src, tmp, k, cvPoint(0,0));

    int block_size = PI->cblock_size[i];
    /*
    cerr << i <<  ":\n";
    cerr << "conv: " << conv->widthStep << "x" << conv->height << "; blocks_size=" <<  block_size << endl;
    cerr << "iconv: " << iconv->widthStep << "x" << iconv->height << "; blocks_size=" <<  PI->iblock_size[i] << "; row_size=" << PI->irow_size[i] << endl;
    cerr << "k: " << k->cols << "x" << k->rows << endl;
    */
    // Rearrange blocks
    // tmp -> conv
    //
    unsigned char * dst_end = (unsigned char*)conv->imageData + (conv->height * conv->widthStep);
    unsigned char * src_end = (unsigned char*)tmp->imageData + (tmp->height * tmp->widthStep);

    for (int v = 0; v < k->rows; ++v)
        for (int u = 0; u < k->cols; ++u)
        {
            unsigned char * base = (unsigned char*)(tmp->imageData + (v * tmp->widthStep) + u);
            int block_id = v * k->cols + u;
            unsigned char * dst_base = (unsigned char *)(conv->imageData + block_id * block_size);
            int y = 0;
            while (y < tmp->height - k->rows)
            {
                int x = 0;
                int m = 0;
                while (x < tmp->width && m < conv->width)
                {
                    dst_base[m] = base[x] ^ 0x80;
                    assert((dst_base + m < dst_end) && (base+x < src_end));
                    x += k->cols;
                    m++;
                }
                base += k->rows * tmp->widthStep;
                dst_base += conv->widthStep;
                y += k->rows;
            }
        }

    // maybe use block xor here instead of xoring individual pixels
    // * Faster, SSE-enabled
}


static void rearrange_convolution(
        PreprocessedImage * PI,
        int i)
{
    IplImage * conv = &(PI->conv[i]);
    IplImage * iconv = &(PI->iconv[i]);
    // Rearrange 2x2
    // conv -> iconv
    char * row1 = conv->imageData;
    char * row2 = conv->imageData + conv->widthStep;
    char * dst = iconv->imageData;

    while (row2 < conv->imageData + (conv->height - 1) * conv->widthStep)
    {
        for (int x = 0; x < conv->width-1; x+=2)
        {
            dst[2*x+0] = row1[x+0];// ^ 0x80;
            dst[2*x+1] = row1[x+1];// ^ 0x80;
            dst[2*x+2] = row2[x+0];// ^ 0x80;
            dst[2*x+3] = row2[x+1];// ^ 0x80;
        }
        row1 += 2*conv->widthStep;
        row2 += 2*conv->widthStep;
        dst += iconv->widthStep;
    }
}

static int align_2(int x)
{
    return (x + 1) & ~1;
}

PreprocessedImage * create_preprocessed_image(CvSize src_sz)
{
    PreprocessedImage * PI = new PreprocessedImage();

    if (!PI)
        return 0;

    src_sz.width = align_2(src_sz.width);
    src_sz.height = align_2(src_sz.height);

    PI->sz = src_sz;
    
    //cerr << PI->sz.width << "x" << PI->sz.height << endl;
    
    cvInitImageHeader(&(PI->intensity), src_sz, IPL_DEPTH_8U, 1, 0, 4);
    cvCreateData(&(PI->intensity));
    cvInitImageHeader(&(PI->integral), src_sz, IPL_DEPTH_32S, 1, 0, 4);
    cvCreateData(&(PI->integral));
    cvInitImageHeader(&(PI->tmp), src_sz, IPL_DEPTH_8U, 1, 0, 4);
    cvCreateData(&(PI->tmp));
    
    for (int i = 0; i < 4; ++i)
    {
        CvSize conv_sz, iconv_sz;

        PI->block_count[i] = kernel[i].width * kernel[i].height;

        conv_sz.width = align2(ceil((float)(src_sz.width) / kernel[i].width));
        conv_sz.height = align2(ceil((float)(src_sz.height) / kernel[i].height));
        
        iconv_sz.width = 2 * conv_sz.width;
        iconv_sz.height = conv_sz.height / 2;

        cvInitImageHeader(&(PI->conv[i]), cvSize(conv_sz.width, conv_sz.height * PI->block_count[i]), IPL_DEPTH_8U, 1, 0, 4);
        cvCreateData(&(PI->conv[i]));
        cvZero(&(PI->conv[i]));
        cvInitImageHeader(&(PI->lbp[i]), cvSize(conv_sz.width, conv_sz.height * PI->block_count[i]), IPL_DEPTH_8U, 1, 0, 4);
        cvCreateData(&(PI->lbp[i]));
        cvZero(&(PI->lbp[i]));
        cvInitImageHeader(&(PI->iconv[i]), cvSize(iconv_sz.width, iconv_sz.height * PI->block_count[i]), IPL_DEPTH_8U, 1, 0, 4);
        cvCreateData(&(PI->iconv[i]));
        cvZero(&(PI->iconv[i]));

        PI->irow_size[i] = PI->iconv[i].widthStep;
        PI->iblock_size[i] = iconv_sz.height * PI->iconv[i].widthStep;
        PI->cblock_size[i] = conv_sz.height * PI->conv[i].widthStep;
    }

    // Prepare addressing
    //cerr << "xtbl:";
    PI->xtbl = new int[src_sz.width * 4];
    for (int i = 0; i < src_sz.width; ++i)
    {
        PI->xtbl[4 * i + 0] = (i & 0xFFFFFFFE) << 1;
        PI->xtbl[4 * i + 1] = (i & 0xFFFFFFFC);
        PI->xtbl[4 * i + 2] = (i & 0xFFFFFFFE) << 1;
        PI->xtbl[4 * i + 3] = (i & 0xFFFFFFFC);
        //cerr << "[" << PI->xtbl[4*i+0] << ",";
        //cerr << PI->xtbl[4*i+1] << ",";
        //cerr << PI->xtbl[4*i+2] << ",";
        //cerr << PI->xtbl[4*i+3] << "],";
    }

    PI->ytbl = new int[src_sz.height * 4];
    //cerr << endl << "ytbl:";
    for (int i = 0; i < src_sz.height; ++i)
    {
        PI->ytbl[4 * i + 0] = (i >> 1) * PI->irow_size[0];
        PI->ytbl[4 * i + 1] = (i >> 1) * PI->irow_size[1];
        PI->ytbl[4 * i + 2] = (i >> 2) * PI->irow_size[2];
        PI->ytbl[4 * i + 3] = (i >> 2) * PI->irow_size[3];
        //cerr << "[" << PI->ytbl[4*i+0] << ",";
        //cerr << PI->ytbl[4*i+1] << ",";
        //cerr << PI->ytbl[4*i+2] << ",";
        //cerr << PI->ytbl[4*i+3] << "],";
    }
    //cerr << endl;

    return PI;
}

void release_preprocessed_image(PreprocessedImage ** PI)
{
    if (PI && *PI)
    {
        PreprocessedImage * p = *PI;
        delete [] p->xtbl;
        delete [] p->ytbl;
        cvReleaseData(&(p->tmp));
        cvReleaseData(&(p->intensity));
        cvReleaseData(&(p->integral));
        for (int i = 0; i < 4; ++i)
        {
            cvReleaseData(&(p->conv[i]));
            cvReleaseData(&(p->iconv[i]));
            cvReleaseData(&(p->lbp[i]));
        }
        delete *PI;
        *PI = 0;
    }
}

void preprocess_image(IplImage * img, PreprocessedImage * PI, int options)
{
    if (options & PP_COPY)
    {
        if (img->width == PI->sz.width && img->height == PI->sz.height)
            cvCopy(img, &(PI->intensity));
        else
            cvResize(img, &(PI->intensity), CV_INTER_LINEAR);
    }

    if (options & PP_INTEGRAL)
    {
        assert(options && PP_COPY);
        integrate(img, &(PI->integral));
    }

    if (options & PP_CONV)
    {
        assert(options && PP_COPY);
        for (int i = 0; i < 4; ++i)
        {
            interleaved_convolution(PI, &(PI->intensity), i);
        }
    }

    if (options & PP_ICONV)
    {
        assert(options && PP_CONV);
        for (int i = 0; i < 4; ++i)
        {
            rearrange_convolution(PI, i);
        }
    }

    if (options & PP_LBP)
    {
        assert(options && PP_CONV);
        for (int i = 0; i < 4; ++i)
        {
            calc_LBP11_sse(&(PI->conv[i]), &(PI->lbp[i]));
        }
    }
}


PreprocessedPyramid * create_pyramid(CvSize base_sz, CvSize min_sz, int octaves, int levels_per_octave)
{
    PreprocessedPyramid * PP = new PreprocessedPyramid();

    PP->octaves = octaves;
    PP->levels_per_octave = levels_per_octave;

    float scale = pow(2.0f, 1.0f/levels_per_octave);

    for (int octave = 0; octave < octaves; ++octave)
    {
        CvSize sz = base_sz;
        sz.width /= octave + 1;
        sz.height /= octave + 1;

        for (int i = 0; i < levels_per_octave; ++i)
        {
            PP->PI.push_back(create_preprocessed_image(sz));
            sz.width /= scale;
            sz.height /= scale;
            if ((sz.width <= min_sz.width) || (sz.height <= min_sz.height))
                return PP;
        }
    }

    return PP;
}

void release_pyramid(PreprocessedPyramid ** PP)
{
    if (PP && *PP)
    {
        PreprocessedPyramid * pp = *PP;
        for (unsigned i = 0; i < pp->PI.size(); ++i)
            release_preprocessed_image(&(pp->PI[i]));
        delete *PP;
        *PP = 0;
    }
}

void insert_image(IplImage * img, PreprocessedPyramid * PP, int options)
{
    options |= PP_COPY;
    preprocess_image(img, PP->PI[0], options);
    
    int max_level = min(unsigned(PP->octaves * PP->levels_per_octave), PP->PI.size());

    for (int octave_base = 0; octave_base < max_level; octave_base += PP->levels_per_octave)
    {
        IplImage * src = &(PP->PI[octave_base]->intensity);
        for (int i = 1; i < PP->levels_per_octave && i+octave_base < max_level; ++i)
        {
            preprocess_image(src, PP->PI[octave_base+i], options);
        }
        if (octave_base+PP->levels_per_octave < max_level)
            preprocess_image(src, PP->PI[octave_base+PP->levels_per_octave], options);
    }
}

