/*
 *   lrd_engine.cpp
 *   Local Rank Difference evaluation
 *   ----
 *   Department of Computer Graphics and Multimedia
 *   Faculty of Information Technology, Brno University of Technology
 *   ----
 */


#ifndef __LRD_ENGINE_H__
#define __LRD_ENGINE_H__

#include "image.h"

/// Types of known classifiers.
typedef enum { UNKNOWN=0, LRD, LRP, LBP, HAAR, numClassifierTypes } ClassifierType;

/// Structure for a detection
typedef struct
{
    int x, y, width, height;
    float response;
} TRect;

/// Initialize rectangle
TRect rect(int _x, int _y, int _w, int _h);

/// A single weak hypothesis.
/// One LRD feature and threshold
//   <-w->
// ^ +---+---+---+
// h | 0 | 1 | 2 |
// V +---+---+---+
//   | 3 | 4 | 5 |
//   +---+---+---+
//   | 6 | 7 | 8 |
//   +---+---+---+
typedef struct
{
    // Feature and hypothesis parameters
    unsigned x, y; // Position in sample [px]
    unsigned w, h; // Size of one feature block [px]
    unsigned A, B; // Indexes of Rank blocks
    float theta_b; // wald_boost threshold

    // Following values are calculated at runtime
    int conv; // Not used!
    int code; // Not used!
    float * alpha; // Ptr to table with alphas - assigned at runtime
    unsigned offset; // Feature offset relative to a sample
} TStage;


typedef struct
{
    unsigned x, y;
    unsigned w, h;
    unsigned tp;
    float min, max;
    unsigned bins;
    float theta_b;
    
    // Following values are calculated at runtime
    float * alpha;
    float range; // max-min - just to avoid subtraction during evaluation
    float size; // w*h - to avoid multiplication in evaluation
    unsigned offset;
} THaarStage;

/// Classifier structure.
/// Holds informations about a classifier - parameters,
/// weak hypotheses and alphas.
typedef struct
{
    ClassifierType tp; ///< Type of the classifier
    unsigned stageCount; ///< Number of stages
    unsigned alphaCount; ///< Alphas per stage
    float threshold; ///< Final classification threshold
    unsigned width; ///< Width of scan window
    unsigned height; ///< Height of scan window
    void * stage; ///< List of stages
    float * alpha; ///< List of alphas
} TClassifier;

#ifdef _cplusplus
extern "C" {
#endif

/// Initialize a classifier.
/// Must be called before the classifier is used!!
/// @param classifier The classifier to process
void initClassifier(TClassifier * classifier);

/// Recalculates feature offset for specified image size.
/// The function must be called whenever size of input image is changed!
/// This allows passing feature offsets directly to evaluation instead of
/// coordinates - performance gain.
/// @param classifier The classifier to process
/// @param widthStep Address offset between subsequent rows (IplImgae::widthStep)
void setClassifierImageSize(TClassifier * classifier, unsigned widthStep);

/// Scan entire image with a classifier.
/// @param image Input intensity image (8 bit, single chanel)
/// @param classifier Classifier to evaluate
/// @param results Preallocated vector for results
/// @param end The end of preallocated vector
/// @returns Number of detections
unsigned scanImage(TImage * image, TClassifier * classifier, TRect * results, TRect * end);

#ifdef _cplusplus
}
#endif

#endif
