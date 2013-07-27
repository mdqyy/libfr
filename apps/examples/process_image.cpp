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
 *  Single-scale object detection. This application serves as time measurement
 *  tool for comparison of different detection engines. It outputs CSV file
 *  with information about the detection process. It needs libabr.so (the
 *  library with detection engines) to be installed in the system.
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


void print_results(const char * file, CvSize sz, Detection * first, Detection * last, double ptime, double stime, bool detections, ostream & out)
{
    out << file << ",";
    out << sz.width << "," << sz.height << ",";
    out << ptime << "," << stime << ",";
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
    const char * progname = "process_image";
    arg_file * files = arg_filen(NULL, NULL, "FILE", 0, argc-1, "Input files");
    arg_str * output = arg_str0("o", NULL, "<PREFIX>", "Save output (prefix will be added to the filename)");
    arg_str * engine = arg_str0("e", "engine", "<ENGINE>", "Detection engine to use (itensity, integral, conv, iconv, lbp)");
    arg_file * classifier = arg_file1("c", NULL, "<FILE>", "Classifier to use");
    arg_int * repeat = arg_int0("t", NULL, "<INT>", "Repeat preprocessing and detection specified number of times");
    arg_int * div_point = arg_int0("u", NULL, "<INT>", "Division point for iconv-conv engine");
    arg_lit * det = arg_lit0("d", NULL, "Output detections");
    arg_lit * help = arg_lit0("h", "help", "Display this help and exit");
    struct arg_end * end = arg_end(20);

    void *argtable[] = { help, repeat, classifier, engine, det, div_point, output, files, end };

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
        if (string(engine->sval[0]) == "iconv-conv")
        {
            scan = scan_image_iconv_conv;
            pp_opts = PP_ICONV_IMAGE; // implies PP_CONV and PP_ICONV
            pc_opts = RECALC_RANKS;
        }
    }

    if (scan == 0)
    {
        fprintf(stderr, "%s: Selected engine of classifier is not supported\n", progname);
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 1;
    }

    int repeat_times = (repeat->count > 0) ? repeat->ival[0] : 1;
    repeat_times = min(max(repeat_times,0),100);

    init_preprocess();

    Detection results[10000];
    ScanParams sp;
    sp.division_a = (div_point->count > 0) ? div_point->ival[0] : 16;

    for (int i = 0; i < files->count; ++i)
    {
        IplImage * src = cvLoadImage(files->filename[i], CV_LOAD_IMAGE_GRAYSCALE);
        
        if (!src)
        {
            fprintf(stderr, "Cannot load image '%s'\n", files->filename[i]);
            continue;
        }

        PreprocessedImage * pp = create_preprocessed_image(align_size_2(cvGetSize(src)));
        
        // Measure pure preprocessing time
        uint64 t0 = cvGetTickCount();

        for (int t = 0; t < repeat_times; ++t)
            preprocess_image(src, pp, pp_opts);
        
        uint64 t1 = cvGetTickCount();

        prepare_classifier(c, pp, pc_opts);
        
        // Measure pure detection time
        uint64 t2 = cvGetTickCount();

        int n = 0;
        for (int t = 0; t < repeat_times; ++t)
        {
            n = scan(pp, c, &sp, results, results+10000, 0);
        }
        
        uint64 t3 = cvGetTickCount();
        
        double preprocess_time = double(t1 - t0) / (repeat_times * cvGetTickFrequency());
        double scan_time = double(t3 - t2) / (repeat_times * cvGetTickFrequency());

        char fn[1024];
        strncpy(fn, files->filename[i], 1024);
        char * name = basename(fn); // basename of image file (to get rid of ../)
        print_results(name, pp->sz, results, results+n, preprocess_time, scan_time, det->count > 0, cout);

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

        release_preprocessed_image(&pp);
        cvReleaseImage(&src);
    }

    release_classifier(&c);
}

