/*
 *  core_simple.cpp
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
 *  'Simple' classifier evaluation engine. This engine uses intensity or
 *  integral image to evaluate classifier. It can be used as reference but it
 *  is _very_ slow. There are no size restrictions on features used.  It
 *  supports monolithic classifiers with LRD, LRP or LBP features.
 *
 */

#include "const.h"
#include "core.h"
#include "core_simple.h"
#include "preprocess.h"

#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace std;

typedef float (*StageEvalFunc)(IplImage *, unsigned, TStage * s, int * feature);
typedef void (*SumFunc)(unsigned char * data, int w, int h, unsigned widthStep, int * v);


/// Sum of regions in 3x3 grid.
/// Sums values in regions and stores the results in a vector.
/// @param data Ptr to top-left corner 
void sum_3x3_regions_intensity(unsigned char * data, int w, int h, unsigned widthStep, int * v)
{
    unsigned blockStep = h * widthStep;
    widthStep -= w;

    // Prepare pointer array
    unsigned char * base[9] = {
        data, data+w, data+2*w,
        data+blockStep,data+blockStep+w,data+blockStep+2*w,
        data+2*blockStep,data+2*blockStep+w,data+2*blockStep+2*w,
    };

    for(int y = 0; y < h; ++y)
    {
        // go through all pixels in row and accumulate
        int x = 0;
        while (x < w)
        {
            int i;
            for (i = 0; i < 9; ++i)
            {
                v[i] += *base[i];
                ++base[i];
            }
            ++x;
        }
        
        // set pointers to next line 
        int i;
        for (i = 0; i < 9; ++i)
	{
	  base[i] += widthStep;
	}
    }
}

/// Sum of regions in 3x3 grid.
/// Sums values in regions and stores the results in a vector.
/// @param data Ptr to top-left corner 
void sum_3x3_regions_integral(unsigned char * data, int w, int h, unsigned widthStep, int * v)
{
    int rowOffset = (h * widthStep) / sizeof(int);
    int * I = (int*)data;
    
    v[0] += *I;

    I += w;

    v[0] -= *I, v[1] += *I;

    I += w;

    v[1] -= *I, v[2] += *I;

    I += w;

    v[2] -= *I;

    ///
    
    I += rowOffset;

    v[2] += *I, v[5] -= *I;

    I -= w;

    v[1] += *I, v[2] -= *I, v[5] += *I, v[4] -= *I;

    I -= w;
    
    v[0] += *I, v[1] -= *I, v[4] += *I, v[3] -= *I;

    I -= w;

    v[0] -= *I, v[3] += *I;

    ///
    
    I += rowOffset;

    v[3] -= *I, v[6] += *I;

    I += w;

    v[3] += *I, v[4] -= *I, v[6] -= *I, v[7] += *I;

    I += w;

    v[4] += *I, v[5] -= *I, v[7] -= *I, v[8] += *I;

    I += w;

    v[5] += *I, v[8] -= *I;

    ///
    
    I += rowOffset;

    v[8] += *I;

    I -= w;

    v[8] -= *I, v[7] += *I;

    I -= w;

    v[7] -= *I, v[6] += *I;

    I -= w;

    v[6] -= *I;

    return;
}



/// Calculate ranks of two elements of vector.
/// \param v Vector of values
/// \param n Number of values if the vector
/// \param a Index of the first value
/// \param b Index of the second value
/// \param rankA Rank of a-th item
/// \param rankB Rank of b-th item
static inline void calc_ranks_2(const int * v, const int n, const int a, const int b, int * rankA, int * rankB)
{
    *rankA = *rankB = 0;
    const int * data = v;
    while (data < v + 9)
    {
        if (v[a] > *data) ++(*rankA);
        if (v[b] > *data) ++(*rankB);
        ++data;
    }
}

/// Simple version of LRD evaluation.
/// \param image Input image
/// \param smpOffset Offset of the sample
/// \param s Stage stage parameters
/// \returns Response of the stage 's'
template <SumFunc sum>
float eval_lrd_stage_simple(IplImage * image, unsigned offset, TStage * stg, int * feature) 
{
    int values[9] = {0,0,0,0,0,0,0,0,0};
    // Get absolute address of feature in image
    unsigned char * base = (unsigned char*)(image->imageData + offset + stg->offset);
    // Get sums in the feature blocks
    // There is no need to calculate the means because we care only about
    // rank of blocks not actual values 
    sum(base, stg->w, stg->h, image->widthStep, values);

    int countA = 0;
    int countB = 0;

    // calculate ranks
    calc_ranks_2(values, 9, stg->A, stg->B, &countA, &countB);

    *feature = countA - countB;
    
    return stg->alpha[*feature + 8];
}

/// Local Rank Pattern evaluation.
/// \param image Input image
/// \param smpOffset Offset of the sample
/// \param r Feature response
/// \param s Stage stage parameters
/// \returns Response of the stage 's'
template <SumFunc sum>
float eval_lrp_stage_simple(IplImage * image, unsigned offset, TStage * stg, int * feature)
{
    int values[9] = {0,0,0,0,0,0,0,0,0};
    // Get absolute address of feature in image
    unsigned char * base = (unsigned char*)(image->imageData + offset + stg->offset);
    // Get sums in the feature blocks
    // There is no need to calculate the means because we care only about
    // rank of blocks not actual values 
    sum(base, stg->w, stg->h, image->widthStep, values);

    int countA = 0;
    int countB = 0;

    calc_ranks_2(values, 9, stg->A, stg->B, &countA, &countB);

    *feature = countA + 10 * countB;

    return stg->alpha[*feature];
}


/// Classic LBP evaluation.
/// Evaluation of 8 bit LBP code. Expects 9 samples in the input.
/// \param v Vector of samples
/// \returns 8bit LBP code
static inline int calc_lbp(const int * v)
{
    int code = 0;
    for (int i = 0; i < 8; ++i)
    {
        code |= (v[lbp_bit_order[i]] > v[4]) << i;
    }
    return code;
}

/// Local Binary Pattern evaluation.
/// \param image Input image
/// \param smpOffset Offset of the sample
/// \param s Stage stage parameters
/// \returns Response of the stage 's'

template<SumFunc sum>
float eval_lbp_stage_simple(IplImage * image, unsigned offset, TStage * stg, int * feature)
{
    int values[9] = {0,0,0,0,0,0,0,0,0};

    // Get absolute address of feature in image
    unsigned char * base = (unsigned char*)(image->imageData + offset + stg->offset);
    // Get sums in the feature blocks
    // There is no need to calculate the means because we care only about
    // rank of blocks not actual values 
    sum(base, stg->w, stg->h, image->widthStep, values);

    
    //for (int i = 0; i < 9; ++i)
    //{
    //    fprintf(stderr,"%d,", values[i]);
    //}
    //fprintf(stderr,"\n");

    *feature = calc_lbp(values);
    
    return stg->alpha[*feature];
}

enum ImType
{
    IMG_INTENSITY, IMG_INTEGRAL
};

template <StageEvalFunc eval, ImType tp>
int eval_classifier_simple(
        PreprocessedImage * PI, TClassifier * c,
        int x, int y,
        unsigned begin, unsigned end,
        int * features, float * hypotheses, float * response, int * stages)
{
    end = min(end, c->stage_count);
    IplImage * img;
    int px_sz;
    
    if (tp == IMG_INTENSITY)
    {
        img = &(PI->intensity);
        px_sz = sizeof(unsigned char);
    }
    if (tp == IMG_INTEGRAL)
    {
        img = &(PI->integral);
        px_sz = sizeof(unsigned);
        x-=1, y-=1;
    }

    //fprintf(stderr, "px_sz=%d\n", px_sz);

    // Better pass teh offset 
    int offset = y * img->widthStep + (x * px_sz);
    for (unsigned s = begin; s < end; ++s)
    {
        TStage * stg = c->stage + s;
        hypotheses[s] = eval(img, offset, stg, features+s);
        //fprintf(stderr, "feature=%d\n", features[s]);
        *response += hypotheses[s];
        if (*response < stg->theta_b)
        {
            *stages = s - begin + 1;
            return 0;
        }
    }
    *stages = end - begin;
    return 1;
}

static ClassifierEvalFunc get_eval_func(TClassifier * c, ImType tp)
{
    ClassifierEvalFunc eval = 0;
    if (tp == IMG_INTENSITY)
    { 
        switch (c->tp)
        {
        case LBP:
            eval = eval_classifier_simple< eval_lbp_stage_simple<sum_3x3_regions_intensity>, IMG_INTENSITY>;
            break;
        case LRP:
            eval = eval_classifier_simple< eval_lrp_stage_simple<sum_3x3_regions_intensity>, IMG_INTENSITY>;
            break;
        case LRD:
            eval = eval_classifier_simple< eval_lrd_stage_simple<sum_3x3_regions_intensity>, IMG_INTENSITY>;
            break;
        default:
            break;
        };
    }
    else
    {    
        switch (c->tp)
        {
        case LBP:
            eval = eval_classifier_simple< eval_lbp_stage_simple<sum_3x3_regions_integral>, IMG_INTEGRAL>;
            break;
        case LRP:
            eval = eval_classifier_simple< eval_lrp_stage_simple<sum_3x3_regions_integral>, IMG_INTEGRAL>;
            break;
        case LRD:
            eval = eval_classifier_simple< eval_lrd_stage_simple<sum_3x3_regions_integral>, IMG_INTEGRAL>;
            break;
        default:
            break;
        };
    }
    return eval;
}

static int scan_image_simple(
        PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last,
        int * hist, ImType tp)
{
    // This is actually not used but its needed for evaluation functions

    float hypotheses[c->stage_count];
    int features[c->stage_count];
    float response = 0.0f;
    int stages;
    
    ClassifierEvalFunc eval = get_eval_func(c, tp);

    Detection * det = first;
  
    for (unsigned y = 1; y < PI->sz.height-c->height-1; y+=1)
    {
        for (unsigned x = 1; x < PI->sz.width-c->width-1; x+=1)
        {
            response = 0.0f;
            int d = eval(PI, c, x, y, 0, c->stage_count, features, hypotheses, &response, &stages);
            if (hist) hist[stages-1]++;

            if (d && (response > c->threshold))
            {
                Detection tmp = {x, y, c->width, c->height, response, 0.0f};
                *det = tmp;
                det++;
                if (det == last)
                {
                    return det - first;
                }
            }
        }
    }
    return det - first;
}

int scan_image_intensity(PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last, int * hist)
{
    return scan_image_simple(PI, c, sp, first, last, hist, IMG_INTENSITY);
}

int scan_image_integral(PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last, int * hist)
{
    return scan_image_simple(PI, c, sp, first, last, hist, IMG_INTEGRAL);
}

int is_classifier_supported_intensity(const TClassifier * c)
{
    if (c->tp == LBP || c->tp == LRP || c->tp == LRD)
        return 1;
    return 0;
}

int is_classifier_supported_integral(const TClassifier * c)
{
    if (c->tp == LBP || c->tp == LRP || c->tp == LRD)
        return 1;
    return 0;
}

