/*
 *  process_image.cpp
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

// Object detector
#include <libabr.h>

// Command-line argument processing
#include <argtable2.h>

// OpenCV
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

// STL
#include <iostream>
#include <list>
#include <string>

#include <libgen.h>

using namespace std;


void print_results(const char * file, CvSize sz, Detection * first, Detection * last, bool detections, ostream & out)
{
    out << file << ",";
    out << sz.width << "," << sz.height << ",";
    out << 0 << "," << 0 << ",";
    out << last-first << ",";
    if (detections)
    {
        for (Detection * r = first; r != last; ++r)
            out << r->x << "," << r->y << "," << r->width << "," << r->height << "," << r->response << ",";
    }
    out << flush << endl;
}

int align2(int x)
{
    return (x + 1) & ~1;
}

CvSize align_size_2(CvSize sz)
{
    return cvSize(align2(sz.width), align2(sz.height));
}

int main(int argc, char ** argv)
{
    // Process arguments
    const char * progname = "process_image2";
    arg_file * files = arg_filen(NULL, NULL, "FILE", 0, argc-1, "Input files");
    arg_str * output = arg_str0("o", NULL, "<PREFIX>", "Save output (prefix will be added to the filename)");
    arg_str * engine = arg_str0("e", "engine", "<ENGINE>", "Detection engine to use (itensity, integral, conv, iconv, lbp)");
    arg_file * classifier = arg_file1("c", NULL, "<FILE>", "Classifier to use");
    arg_lit * det = arg_lit0("d", NULL, "Output detections");
    arg_lit * help = arg_lit0("h", "help", "Display this help and exit");
    arg_dbl * thr = arg_dbl0("t", "threshold", "<FLOAT>", "Detection threshold");
    struct arg_end * end = arg_end(20);

    void *argtable[] = { help, classifier, engine, det, thr, output, files, end };

    int nerrors = arg_parse(argc, argv, argtable);
    
    if(help->count > 0)
    {
        fprintf(stderr, "Usage: %s", progname);
        arg_print_syntax(stderr, argtable, "\n\n");
        arg_print_glossary(stderr, argtable, "  %-30s %s\n");
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 0;
    }
    if (nerrors > 0)
    {
        arg_print_errors(stderr, end, progname);
        fprintf(stderr, "Try '%s --help' for more information.\n", progname);
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 1;
    }

    if (files->count == 0)
    {
        fprintf(stderr, "%s: No input files\n", progname);
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 1;
    }

    // Load classifier
    TClassifier * c = load_classifier_XML(classifier->filename[0]);

    if (!c)
    {
        fprintf(stderr, "%s: Cannot load classifier '%s'\n", progname, classifier->filename[0]);
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 1;
    }

    init_classifier(c);

    c->threshold = (thr->count > 0) ? thr->dval[0] : c->threshold; 
    
    // Select engine and preprocessing options
    ScanImageFunc scan = 0;
    int pp_opts = 0;
    int pc_opts = 0;

    if (engine->count > 0)
    {
        if (string(engine->sval[0]) == "intensity")
        {
            scan = scan_image_intensity;
            pp_opts = PP_COPY_IMAGE;
            pc_opts = RECALC_OFFSET;
        }
        if (string(engine->sval[0]) == "integral")
        {
            scan = scan_image_integral;
            pp_opts = PP_INTEGRAL_IMAGE;
            pc_opts = RECALC_OFFSET | OFFSET_INTEGRAL;
        }
        if (string(engine->sval[0]) == "conv")
        {
            scan = scan_image_conv_bunch16;
            pp_opts = PP_CONV_IMAGE;
            pc_opts = RECALC_RANKS;
        }
        if (string(engine->sval[0]) == "iconv")
        {
            scan = scan_image_iconv;
            pp_opts = PP_ICONV_IMAGE;
            pc_opts = RECALC_RANKS;
        }
        if (string(engine->sval[0]) == "lbp")
        {
            scan = scan_image_lbp;
            pp_opts = PP_LBP_IMAGE;
            pc_opts = NONE;
        }
    }

    if (scan == 0)
    {
        fprintf(stderr, "%s: Selected engine of classifier is not supported\n", progname);
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 1;
    }

    init_preprocess();

    Detection results[10000];
    ScanParams sp;

    for (int i = 0; i < files->count; ++i)
    {
        IplImage * src = cvLoadImage(files->filename[i], CV_LOAD_IMAGE_GRAYSCALE);
        
        if (!src)
        {
            fprintf(stderr, "Cannot load image '%s'\n", files->filename[i]);
            continue;
        }

        PreprocessedPyramid * pp = create_pyramid(align_size_2(cvGetSize(src)), cvSize(c->width, c->height), 8, 4);
        
        insert_image(src, pp, pp_opts);
        
        int n = detect_objects(pp, c, &sp, scan, results, results+10000, pc_opts, 1, 0);

        char fn[1024];
        strncpy(fn, files->filename[i], 1024);
        char * name = basename(fn); // basename of image file (to get rid of ../)
        print_results(name, pp->PI[0]->sz, results, results+n, det->count > 0, cout);

        if (output->count > 0) // output will be saved
        {
            // draw detected objects
            for (int k = 0; k < n; ++k)
            {
                Detection r = results[k];
                cvRectangle(src, cvPoint(r.x,r.y), cvPoint(r.x+r.width,r.y+r.height), CV_RGB(0,0,0), 3);
                cvRectangle(src, cvPoint(r.x,r.y), cvPoint(r.x+r.width,r.y+r.height), CV_RGB(255,255,255), 1);
            }
            // save file
            char out_file[1024];
            sprintf(out_file, "%s%s", output->sval[0], name);
            cvSaveImage(out_file, src);
        }

        release_pyramid(&pp);
        cvReleaseImage(&src);
    }

    release_classifier(&c);
}

