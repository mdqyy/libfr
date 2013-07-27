/*
 *  xml2h.cpp
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
 *  Conversion program that transforms XML classifiers to C source code.
 *
 */

#include <libabr.h>

#include <argtable2.h>

#include <iostream>
#include <fstream>
#include <cstdio>

using namespace std;

int main(int argc, char ** argv)
{
    // Process arguments
    const char * progname = "xml2h";
    arg_file * file = arg_file1("i", "input", "FILE", "The XML file with classifier");
    arg_file * file1 = arg_file1("o", "output", "FILE", "Output file without extension");
    arg_str * name = arg_str1("n", "name", "NAME", "Classifier identifier in the header");
    arg_lit * help = arg_lit0("h", "help", "Display this help and exit");
    struct arg_end * end = arg_end(20);

    void *argtable[] = { file, file1, name, help, end };

    int nerrors = arg_parse(argc, argv, argtable);
    
    if(help->count > 0)
    {
        fprintf(stderr, "Usage: %s", progname);
        arg_print_syntax(stderr, argtable, "\n\n");
        arg_print_glossary(stderr, argtable, "  %-30s %s\n");
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 1;
    }
    if (nerrors > 0)
    {
        arg_print_errors(stderr, end, progname);
        fprintf(stderr, "Try '%s --help' for more information.\n", progname);
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 1;
    }

    // Arguments ok, try load the classifier
    TClassifier * c = load_classifier_XML(file->filename[0]);

    if (!c)
    {
        fprintf(stderr, "Can not load classifier file '%s'\n", file->filename[0]);
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 1;
    }

    // Generate file names
    // BUG: long file names may cause buffer overflow
    char hfile[64];
    char cfile[64];
    sprintf(hfile, "%s.h", file1->filename[0]);
    sprintf(cfile, "%s.c", file1->filename[0]);
    
    // Open files
    // BUG: Files are not checked if they are successfuly opened!
    ofstream header(hfile);
    ofstream source(cfile);

    // Export the data 
    export_classifier_header(c, header, name->sval[0]);
    export_classifier_source(c, source, name->sval[0], hfile);

    return 0;
}

