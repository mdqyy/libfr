/*
 *   lrd_engine.cpp
 *   Local Rank Difference evaluation
 *   ----
 *   Department of Computer Graphics and Multimedia
 *   Faculty of Information Technology, Brno University of Technology
 *   ----
 */

#include "lrd_engine.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>


TRect rect(int _x, int _y, int _w, int _h)
{
    TRect r = {_x, _y, _w, _h};
    return r;
}


// Set ptr to alphas for each stage
void initClassifier(TClassifier * classifier)
{
    float * stgAlpha = classifier->alpha;
    unsigned s;
    for (s = 0; s < classifier->stageCount; ++s, stgAlpha += classifier->alphaCount)
    {
        switch (classifier->tp)
        {
        case LBP:
        case LRP:
        case LRD:
            {
                TStage * stage = (TStage*)classifier->stage + s;
                stage->alpha = stgAlpha;
                stage->offset = 0;
                break;
            }
        case HAAR:
            {
                THaarStage * stage = (THaarStage*)classifier->stage + s;
                stage->alpha = stgAlpha;
                stage->offset = 0;
                stage->range = stage->max - stage->min;
                stage->size = stage->w * stage->h;
                break;
            }
        default:
            assert("Unknown classifier");
            break;
        }
    }
}


void setClassifierImageSize(TClassifier * classifier, unsigned widthStep)
{
    unsigned s;
    for (s = 0; s < classifier->stageCount; ++s)
    {
        switch (classifier->tp)
        {
        case LBP:
        case LRP:
        case LRD:
            {
                TStage * stage = (TStage*)classifier->stage + s;
                stage->offset = stage->y * widthStep + stage->x;
                break;
            }
        case HAAR:
            {
                THaarStage * stage = (THaarStage*)classifier->stage + s;
                stage->offset = stage->y * widthStep + stage->x;
                break;
            }
        default:
            assert("Unknown classifier");
            break;
        }

    }
}


/// Sum of regions in 3x3 grid.
/// Sums values in regions and stores the results in a vector.
/// @param data Ptr to top-left corner 
static void sumRegions3x3(unsigned char * data, int w, int h, unsigned widthStep, int * v)
{
    unsigned blockStep = h * widthStep;
    widthStep -= w;

    // Prepare pointer array
    unsigned char * base[9] = {
        data, data+w, data+2*w,
        data+blockStep,data+blockStep+w,data+blockStep+2*w,
        data+2*blockStep,data+2*blockStep+w,data+2*blockStep+2*w,
    };

    int y;
    for (y = 0; y < h; ++y)
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
            base[i] += widthStep;
    }
}


/// Simple version of LRD evaluation.
/// Sums the pixels in the feature grid and calculates ranks of selected pixels.
static float evalLRDStageSimple(TImage * image, unsigned smpOffset, void * s)
{
    TStage * stg = (TStage*)s;
    int values[9] = {0,0,0,0,0,0,0,0,0};
    // Get absolute address of feature in image
    unsigned char * base = (unsigned char*)(image->imageData + smpOffset + stg->offset);
    // Get sums in the feature blocks
    // There is no need to calculate the means because we care only about
    // rank of blocks not actual values 
    sumRegions3x3(base, stg->w, stg->h, image->widthStep, values);

    int countA = 0;
    int countB = 0;
    int valA = values[stg->A];
    int valB = values[stg->B];

    // calculate ranks
    int * data = values;
    while (data < values + 9)
    {
        if (valA > *data) ++countA;
        if (valB > *data) ++countB;
        ++data;
    }

    int lrd = countA - countB + 8;

    // return weak hypothesis response
    return stg->alpha[lrd];
}


static float evalLRPStageSimple(TImage * image, unsigned smpOffset, void * s)
{
    TStage * stg = (TStage*)s;
    int values[9] = {0,0,0,0,0,0,0,0,0};
    // Get absolute address of feature in image
    unsigned char * base = (unsigned char*)(image->imageData + smpOffset + stg->offset);
    // Get sums in the feature blocks
    // There is no need to calculate the means because we care only about
    // rank of blocks not actual values 
    sumRegions3x3(base, stg->w, stg->h, image->widthStep, values);

    int countA = 0;
    int countB = 0;
    int valA = values[stg->A];
    int valB = values[stg->B];

    // calculate ranks
    int * data = values;
    while (data < values + 9)
    {
        if (valA > *data) ++countA;
        if (valB > *data) ++countB;
        ++data;
    }

    int lrp = countA + 10 * countB;

    // return weak hypothesis response
    return stg->alpha[lrp];
}


static float evalLBPStageSimple(TImage * image, unsigned smpOffset, void * s)
{
    TStage * stg = (TStage*)s;
    static const int LBPOrder[8] = {0, 1, 2, 5, 8, 7, 6, 3};

    int values[9] = {0,0,0,0,0,0,0,0,0};
    // Get absolute address of feature in image
    unsigned char * base = (unsigned char*)(image->imageData + smpOffset + stg->offset);
    // Get sums in the feature blocks
    // There is no need to calculate the means because we care only about
    // rank of blocks not actual values 
    sumRegions3x3(base, stg->w, stg->h, image->widthStep, values);

    // calculate ranks
    int lbp = 0;
    int i;
    for (i = 0; i < 8; ++i)
    {
        lbp |= (values[LBPOrder[i]] > values[4]) << i;
    }    

    // return weak hypothesis response
    return stg->alpha[lbp];
}

static inline int min(int a, int b)
{
    return (a < b) ? a : b;
}
static inline int max(int a, int b)
{
    return (a > b) ? a : b;
}

///////////
// Haar evaluation on the intensity image

/// Number of samples for different types of Haar features
static const int haarSampling[12]= {
    2, 1, // DH
    1, 2, // DV
    3, 1, // TH
    1, 3, // TV
    2, 2, // DIAG
    3, 3, // SRND
};

/// Scales for Haar normalizations
static const int haarNormScale[6]= {
    1, // DH
    1, // Dv
    2, // TH
    2, // TV
    2, // DIAG
    8, // SRND
};

/// Weights of samples
static const float haarWeights[6][16] = {
    {-1,  1},

    {-1,  1},

    {1, -2, 1},

    {1, -2, 1},

    {1, -1, -1, 1},

    {1, 1, 1, 1, -8, 1, 1, 1, 1},
};

// Normalization factor for Haar features
static float normFactor;

/// Sum values in an area.
/// @param base Top-left corner of the area (image is assumed to be 8bit, single channel)
/// @param w Width of the area
/// @param h Height of the area
/// @param widthStep Distance between two rows in bytes
/// @returns sum of the area
static inline int sumArea(unsigned char * base, int w, int h, int widthStep)
{
    int sum = 0;
    int y;
    for (y = 0; y < h; ++y, base += widthStep)
    {
        unsigned char * px = base;
        while (px < base + w)
        {
            sum += *px;
            ++px; 
        }
    }
    return sum;
}

/// Calculate sum and sum of squares of an area.
/// @param base Top-left corner of the area (image is assumed to be 8bit, single channel)
/// @param w Width of the area
/// @param h Height of the area
/// @param widthStep Distance between two rows in bytes
/// @param sum Sum of the area
/// @param sum2 Sum of squares of the area
static inline void sumArea2(unsigned char * base, int w, int h, int widthStep, unsigned * sum, unsigned * sum2)
{
    *sum = 0;
    *sum2 = 0;
    int y;
    for (y = 0; y < h; ++y, base += widthStep)
    {
        unsigned char * px = base;
        while (px < base + w)
        {
            *sum += *px;
            *sum2 += *px * *px;
            ++px;
        }
    }
}


static float evalHaarStageSimple1(TImage * image, unsigned smpOffset, void * s)
{
    THaarStage * stg = (THaarStage*)s;
    int xs = haarSampling[2*stg->tp];
    int ys = haarSampling[2*stg->tp+1];
    int norm = haarNormScale[stg->tp];
    const float * weights = haarWeights[stg->tp];
    unsigned char * base = (unsigned char*)(image->imageData + smpOffset + stg->offset);

    int x,y;
    float f = 0.0f;
    for (y = 0; y < ys; ++y, base+=stg->h*image->widthStep)
    {
        for (x = 0; x < xs; ++x)
        {
            float w = *weights++;
            unsigned sum = sumArea(base + x * stg->w, stg->w, stg->h, image->widthStep);
            f += ((float)sum) * w;
        }
    }

    // Normalize
    f = f / (norm * stg->size) / normFactor;

    // Discretize
    int bin = (int)((f - stg->min) / stg->range * (stg->bins-1) + 0.5);
    bin = min(stg->bins, max(0, bin));
    
    //fprintf(stderr, "x=%d,y=%d,w=%d,h=%d\n", stg->x, stg->y, stg->w, stg->h);
    //fprintf(stderr, "min=%f,max=%f,bins=%d\n", stg->min, stg->max,stg->bins);
    //fprintf(stderr, "f=%f,bin=%d\n", f, bin);

    // return hypothesis respone
    return stg->alpha[bin];

}

/////////////
typedef float (*StageEvalFunc)(TImage*, unsigned, void*);
typedef int (*ClassifierEvalFunc)(TImage*, unsigned, TClassifier*, float*, StageEvalFunc);


static int evalLRDClassifier(TImage * image, unsigned smpOffset, TClassifier * classifier, float * response, StageEvalFunc eval)
{
    *response = 0.0;
    // go through all stages and accumulate response
    TStage * stage;
    for (stage = (TStage*)classifier->stage; stage < (TStage*)classifier->stage + classifier->stageCount; ++stage)
    {
        *response += eval(image, smpOffset, stage);
        //fprintf(stderr, "%f\n", *response);
        // Test the waldboost threshold
        if (*response < stage->theta_b)
        {
            return 0;
        }
    }

    // All stages have been evaluated - test final stage threshold
    return (*response > classifier->threshold) ? 1 : 0;
}


static int evalHaarClassifier(TImage * image, unsigned smpOffset, TClassifier * classifier, float * response, StageEvalFunc eval)
{
    // get norm factor - standard deviation
    unsigned sum = 0;
    unsigned sum2 = 0;
    unsigned size = classifier->width*classifier->height;
    sumArea2((unsigned char*)image->imageData + smpOffset, classifier->width, classifier->height, image->widthStep, &sum, &sum2);
    normFactor = sqrt((float)(sum2) / size - pow(((float)sum) / size, 2));

    *response = 0.0;
    // go through all stages and accumulate response
    THaarStage * stage;
    for (stage = (THaarStage*)classifier->stage; stage < (THaarStage*)classifier->stage + classifier->stageCount; ++stage)
    {
        *response += eval(image, smpOffset, stage);
        // Test the waldboost threshold
        if (*response < stage->theta_b)
        {
            //fprintf(stderr, "\n\n");
            return 0;
        }
    }
    // All stages have been evaluated - test final stage threshold
    return (*response > classifier->threshold) ? 1 : 0;
}


unsigned scanImage(TImage * image, TClassifier * classifier, TRect * results, TRect * end)
{
    StageEvalFunc eval = 0;
    ClassifierEvalFunc ceval = 0;

    switch (classifier->tp)
    {
        case LRD:
            eval = evalLRDStageSimple;
            ceval = evalLRDClassifier;
            break;
        case LRP:
            eval = evalLRPStageSimple;
            ceval = evalLRDClassifier;
            break;
        case LBP:
            eval = evalLBPStageSimple;
            ceval = evalLRDClassifier;
            break;
            // LR?
        case HAAR:
            eval = evalHaarStageSimple1;
            ceval = evalHaarClassifier;
        default:
            break;
    };

    assert(eval != 0 && "Unsupported classifier type");
    assert(ceval != 0 && "Unsupported classifier type");

    //fprintf(stderr, "%d,%d\n", classifier->width, classifier->height);

    // Set pointer to position for first detection
    TRect * iter = results;

    unsigned rowOffset = 0;
    unsigned y;
    for (y = 0; y < image->height-classifier->height; y+=1, rowOffset+=image->widthStep)
    {
        unsigned x;
        for (x = 0; x < image->width-classifier->width; x+=1)
        {
            // Evaluate classifier
            float response;
            int positive = ceval(image, rowOffset+x, classifier, &response, eval);
            if (positive) // write position of the detection
            {
                *iter = rect(x, y, classifier->width, classifier->height);
                iter->response = response;
                ++iter;
                if (iter == end)
                    return iter - results;
            }
        }
    }

    return iter - results;
}

