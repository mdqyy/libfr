/*
 *  benchmark.cpp
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

#include <libabr.h>
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#include <libgen.h>
#include <iostream>
#include <argtable2.h>

using namespace std;

CvHaarClassifierCascade* load_object_detector( const char* cascade_path )
{
    return (CvHaarClassifierCascade*)cvLoad( cascade_path );
}

int64 opencv_detect_objects(IplImage * image, CvHaarClassifierCascade * cascade, int t)
{
    CvMemStorage * storage = cvCreateMemStorage(0);
    CvSeq * faces;

    int64 t0 = cvGetTickCount();
    for (int i = 0; i < t; ++i)
        faces = cvHaarDetectObjects( image, cascade, storage, 1.18, 1, 0);
    // clear seq? mem leak?
    int64 t1 = cvGetTickCount();

    cvReleaseMemStorage( &storage );

    return t1 - t0;
}

int align2(int x)
{
    return (x + 3) & ~3;
}

CvSize align_size_2(CvSize sz)
{
    return cvSize(align2(sz.width), align2(sz.height));
}

int64 libabr_detect_objects(IplImage * image, TClassifier * c, ScanImageFunc scan, int pp_opts, int pc_opts, int t)
{
    PreprocessedPyramid * pp = create_pyramid(align_size_2(cvGetSize(image)), cvSize(c->width+2,c->height+2), 8, 4);
    ScanParams sp;

    Detection results[10000];
    
    int64 t0 = cvGetTickCount();
    for (int i = 0; i < t; ++i)
    {
        insert_image(image, pp, pp_opts);
        (void)detect_objects(pp, c, &sp, scan, results, results+10000, pc_opts, 1, 0);
    }
    int64 t1 = cvGetTickCount();

    release_pyramid(&pp);

    return t1 - t0;
}

void print_results(const char * name, CvSize sz, int64 * c, double f, ostream & out)
{
    out << name << ",";
    out << sz.width << "," << sz.height << ",";
    for (int i = 0; i < 5; ++i)
        out << double(c[i]) / f << ",";
    out << endl << flush;
}

int main( int argc, char** argv )
{
    // Process arguments
    const char * progname = "benchmark";
    arg_file * files = arg_filen(NULL, NULL, "FILE", 0, argc-1, "Input files");
    arg_file * classifier1 = arg_file1(NULL, "abr-classifier", "<FILE>", "libabr classifier to use");
    arg_file * classifier2 = arg_file1(NULL, "cv-classifier", "<FILE>", "OpnCV classifier to use");
    arg_int * repeat = arg_int0("t", NULL, "<INT>", "Repeat preprocessing and detection specified number of times");
    arg_lit * help = arg_lit0("h", "help", "Display this help and exit");
    struct arg_end * end = arg_end(20);

    void *argtable[] = { help, repeat, classifier1, classifier2, files, end };

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
    TClassifier * c1 = load_classifier_XML(classifier1->filename[0]);
    CvHaarClassifierCascade * c2 = load_object_detector(classifier2->filename[0]);

    if (!c1 || !c2)
    {
        fprintf(stderr, "%s: Cannot load classifiers\n", progname);
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 1;
    }

    init_classifier(c1);
    int repeat_times = (repeat->count > 0) ? repeat->ival[0] : 1;
    repeat_times = min(max(repeat_times,0),100);

    init_preprocess();

    int64 counters[files->count][5];

    for (int i = 0; i < files->count; ++i)
    {
        IplImage * src = cvLoadImage(files->filename[i], CV_LOAD_IMAGE_GRAYSCALE);
        
        if (!src)
        {
            fprintf(stderr, "Cannot load image '%s'\n", files->filename[i]);
            continue;
        }

        fill(counters[i], counters[i]+5, 0);
        counters[i][0] += opencv_detect_objects(src, c2, repeat_times);
        counters[i][1] += libabr_detect_objects(src, c1, scan_image_intensity, PP_COPY_IMAGE, RECALC_OFFSET, repeat_times);
        /*
        counters[i][2] += libabr_detect_objects(src, c1, scan_image_integral, PP_INTEGRAL_IMAGE, RECALC_OFFSET | OFFSET_INTEGRAL, repeat_times);
        */
        counters[i][3] += libabr_detect_objects(src, c1, scan_image_iconv, PP_ICONV_IMAGE, RECALC_RANKS, repeat_times);
        // counters[i][4] += libabr_detect_objects(src, c1, scan_image_conv_bunch16, PP_CONV_IMAGE, RECALC_RANKS, repeat_times);

        char fn[1024];
        strncpy(fn, files->filename[i], 1024);
        char * name = basename(fn); // basename of image file (to get rid of ../) 
        print_results(name, cvGetSize(src), counters[i], repeat_times * cvGetTickFrequency(), cerr); 

        cvReleaseImage(&src);
    }

    release_classifier(&c1);
    cvReleaseHaarClassifierCascade( &c2 );

    return 0;
}

