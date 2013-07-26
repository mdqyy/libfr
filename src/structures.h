/*
 *  structures.h
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
 *  Basic structures used by the library.
 *
 */

#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

/// A detection structure.
/// Holds the response and the position in an image.
typedef struct 
{
    int x, y, width, height;
    float response;
    float angle;
} Detection;

/// A single weak hypothesis.
/// 3x3 grid feature, WaldBoost threshold and ptr to lookup table.
typedef struct
{
	// Feature and hypothesis parameters
    int x, y, w, h;
    char A, B; // feature parameters
    float theta_b; // wald_boost threshold
    float * alpha; // Ptr to table with alphas
    
    char sz_type;  // 2 * (h-1) + (w-1)
    char pos_type; // (4 * (y % 4) + (x % 4))
    unsigned offset;
} TStage;

typedef enum
{
    UNKNOWN, LRD, LRP, LBP, numClassifierTypes
} ClassifierType;

typedef enum
{
    C_STATIC, C_DYNAMIC
} DynamicModel;

typedef enum
{
    FSZ_UNRESTRICTED, FSZ_2x2
} FeatureSize;

typedef enum
{
    NS_NONE, // No suppression
    NS_2x2,
    NS_4x4,
} NS_Type;

/// Classifier structure.
/// Holds informations about a classifier - parameters,
/// weak hypotheses, alphas and convolutions.
typedef struct
{
    ClassifierType tp;  ///< Identifies what features are used
    DynamicModel model; ///< Whether the structure can be released
    FeatureSize fsz;    ///< Type of features
//  NS_Type ns; ///< Neighborhood syppression type
    
    unsigned stage_count; ///< Number of stages
    unsigned alpha_count; ///< Number of alphas per stage
    float threshold; ///< Final classification threshold
    unsigned width, height; ///< Size of scanning window
    
    // Dynamic parameters
    TStage * stage; ///< List of stages
    float * alpha; ///< List of alphas
    int * ranks; ///< Precalculated ranks
} TClassifier;


typedef struct
{
    int step_x, step_y;
    int division_a;
    // And more comes here
} ScanParams;

#endif
