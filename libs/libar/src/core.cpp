#include "core.h"
#include "const.h"
#include <vector>
#include <cstdio>

using namespace std;

// Shuffle alphas for LRP
int init_classifier(TClassifier * c)
{
    float * stg_alpha = c->alpha;

    for (unsigned s = 0; s < c->stage_count; ++s, stg_alpha += c->alpha_count)
    {
        TStage * stage = c->stage + s;

        if (stage->w > 2 || stage->h > 2)
            return 0;

        // get feature type
        // 000000hw
        stage->alpha = stg_alpha;
        stage->sz_type = ((stage->h-1) << 1) | (stage->w-1);
        stage->pos_type = ((stage->y & 0x03) << 2) | (stage->x & 0x03);
        stage->offset = 0;
    }

    return 1;
}


void prepare_classifier(TClassifier * c, PreprocessedImage * PI, int options)
{
    if (options == NONE)
        return;

    if ((options & RECALC_OFFSET))
    {
        IplImage * img;
        int px_sz;
        if (options & OFFSET_INTEGRAL)
        {
            img = &(PI->integral);
            px_sz = sizeof(int);
        }
        else
        {
            img = &(PI->intensity);
            px_sz = sizeof(char);
        }
        for (unsigned s = 0; s < c->stage_count; ++s)
        {
            TStage * stage = (TStage*)c->stage + s;
            stage->offset = stage->y * img->widthStep + (stage->x * px_sz);
        }
    }

    if (options & RECALC_RANKS)
    {
        assert(c->ranks != 0);

        for (unsigned s = 0; s < c->stage_count; ++s)
        {
            const TStage & stage = c->stage[s];
            int * ranks = c->ranks + 8 * s;

            for (int t = 0; t < 4; ++t)
            {
                int & A = ranks[2 * t + 0];
                int & B = ranks[2 * t + 1];
                A = rank_table[t][int(stage.A)];
                B = rank_table[t][int(stage.B)];
                if (A >= 8) A += PI->irow_size[int(stage.sz_type)] - 8;
                if (B >= 8) B += PI->irow_size[int(stage.sz_type)] - 8;
            }
        } // stages
    } // if
}


int detect_objects(
        PreprocessedPyramid * PP,
        TClassifier * c,
        ScanParams * sp,
        ScanImageFunc scan_image,
        Detection * first, Detection * last,
        int options,
        float scale,
        int * hist)
{
    CvSize base_sz = PP->PI[0]->sz;

    vector<PreprocessedImage*>::iterator PI = PP->PI.begin();

    Detection * det = first;

    while (PI != PP->PI.end())
    {
        prepare_classifier(c, *PI, options);
        
        float scale_x = scale * (float(base_sz.width) / (*PI)->sz.width);
        float scale_y = scale * (float(base_sz.height) / (*PI)->sz.height);

        //fprintf(stderr, "%f,%f\n", scale_x,scale_y);

        int n = scan_image(*PI, c, sp, det, last, hist);
        
        for (Detection * r = det; r < (det + n); ++r)
        {
            r->x *= scale_x;
            r->y *= scale_y;
            r->width *= scale_x;
            r->height *= scale_y;
        }

        det += n;
        
        PI++;
    }

    return det - first;
}

