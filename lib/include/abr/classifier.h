/*
 *  classifier.h
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
 *  I/O interface for classifiers.
 *
 */

#ifndef _CLASSIFIER_H_
#define _CLASSIFIER_H_

#include "core.h"
#include <ostream>

extern "C" {

/// Strings of known classifier types (indexed ty ClassifierType).
extern const char *const classifier_type_strings[];

/// Load classifier from file.
/// \param filename The file to load
/// \returns Pointer to loaded classifier or NULL when failed.
TClassifier * load_classifier_XML(const char * filename);

/// Release a classifier.
/// As a result, The classifier is released and pointer is set to NULL.
/// \param classifier Pointer to classifier
void release_classifier(TClassifier ** classifier);

/// Export classifier as .h file.
/// \param c The classifier to export
/// \param str Output stream
/// \param name Classifier identifier in the .h
void export_classifier_header(TClassifier * c, std::ostream & str, const char * name);

/// Export classifier as .c file.
/// \param c The classifier to export
/// \param str Output stream
/// \param name Classifier identifier in the .h
/// \param headerName The file name of the header file.
void export_classifier_source(TClassifier * c, std::ostream & str, const char * name, const char * headerName);

}

#endif
