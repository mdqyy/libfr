/*
 *  core.h
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
 *  Core functions needed for detection.
 *
 */

#ifndef _CORE_H_
#define _CORE_H_

#include "preprocess.h"
#include "structures.h"

// Macros used by prepare_classifier (recalculate internal parameters)
#define NONE                (0x00) ///< Do nothing
#define RECALC_OFFSET       (0x01) ///< Recalculate offsets 
#define OFFSET_INTEGRAL     (0x02) ///< Offsets for integral image
#define RECALC_RANKS        (0x04) ///< Recalculate rank indices

/// General classifier evaluation function.
/// The function evaluates the classifier on the given image on the given position. It evaluates
/// only weak hypotheses between begin and end (including begin and not including end). This
/// mechanism allows for combination of different implementations of classifier evaluation.
/// \param PI Preprocessed image to evaluate the classifier on
/// \param c The classifier
/// \param x Horizontal position of evaluated window
/// \param y Vertical position of evaluated window
/// \param begin The first weak hypothesis to calculate
/// \param end The last weak hypothesis (not calculated!)
/// \param features Array for responses of features (of size c->stage_count)
/// \param hypotheses Array for weak hypotheses responses (of size c->stage_count)
/// \param response Response of the classifier
/// \param stages Number of evaluated stages
/// \returns Classification decision. 0 - sample is rejected, 1 - sample may be positive,
/// and threshold should be tested.
typedef int (*ClassifierEvalFunc)(
        PreprocessedImage * PI, TClassifier * c,
        int x, int y,
        unsigned begin, unsigned end,
        int * features, float * hypotheses, float * response, int * stages);

/// General function for evaluation of a classifier on an image.
/// It scans the given image on all positions in _single_ scale. The options
/// specifying behaviour of the particular implementation can be passed in the
/// 'sp' structure.
/// \param PI Preprocessed image to evaluate the classifier on
/// \param c The classifier
/// \param sp Optinal parameters (only necessary in some cases)
/// \param first Ptr to first free detection item
/// \param last Ptr after last detection item
/// \param hist Histogram of stage execution. When left NULL, the histogram is not accumulated.
/// \returns Number 'n' of detections. Valid range of detections is [first, first+n).
typedef int (*ScanImageFunc)(
        PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last,
        int * hist);

extern "C" {

/// Initialize classifier structure.
/// Necessary to call before the classifier is used.
/// \param classifier The classifier to initialize.
/// \returns 1 if initialized.
int init_classifier(TClassifier * classifier);

/// Prepare the classifier before scanning (or evaluating on) new image.
/// Only necessary when image size changes. The function recalculates internal parameters.
/// \param classifier The classifier to initialize.
/// \param PI Image structure on which the classifier will be evaluated.
/// \param options Which parameters to calculate.
void prepare_classifier(TClassifier * c, PreprocessedImage * PI, int options);

int detect_objects(
        PreprocessedPyramid * PP,
        TClassifier * c,
        ScanParams * sp,
        ScanImageFunc scan_image,
        Detection * first, Detection * last,
        int options,
        float scale,
        int * hist);


} // extern "C"


#endif
