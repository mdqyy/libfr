/*
 *   classifier.h
 *   Classifier management
 *   ----
 *   Department of Computer Graphics and Multimedia
 *   Faculty of Information Technology, Brno University of Technology
 *   ----
 */

#ifndef _CLASSIFIER_H_
#define _CLASSIFIER_H_

#include "lrd_engine.h"
#include <ostream>


/// Strings of known classifier types (indexed ty ClassifierType).
extern const char *const classifierTypeStrings[];

/// Disable WaldBoost Evaluation.
/// @param c The classifier
void disableClassifierWaldBoost(TClassifier * c);

/// Load classifier from file.
/// @param filename The file to load
/// @returns Pointer to loaded classifier or NULL when fails
TClassifier * loadClassifierXML(const char * filename);

/// Release a classifier.
/// @param classifier Pointer to classifier
void releaseClassifier(TClassifier ** classifier);

/// Export classifier as .h file.
/// @param c The classifier to export
/// @param str Output stream
/// @param name Classifier identifier in the .h
void exportClassifierHeader(TClassifier * c, std::ostream & str, const char * name);


#endif
