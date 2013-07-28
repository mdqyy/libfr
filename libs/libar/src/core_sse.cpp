//
// TODO
// * LRP
// ** Preskladani alf v klasifikatoru 10*countA+countB -> 16 * countA + countB
// ** Debug kodu

// OpenCV for image representation
#include <cv.h>
#include <highgui.h>

#include <cmath>
#include <stdio.h>

// SSE
#include <mmintrin.h>
#include <pmmintrin.h>
#include <emmintrin.h>

#include "core.h"
#include "core_sse.h"
#include "const.h"

using namespace std;


static inline int __attribute__((const,always_inline)) get_mod_position(int x, int y)
{
    // posType - yyxx0000
    return ((y & 0x03) << 6) | ((x & 0x03) << 4);
}

////////////////////////////////////////////////////////////////////////////////
// PREPROCESSED IMAGE PROCESSING
// LBP
////////////////////////////////////////////////////////////////////////////////

static inline float eval_lbp_stage_precalc(PreprocessedImage * PI, TStage * s, int x, int y, int mod_pos, int * feature)
{
    // Select source image
    IplImage * image = &(PI->lbp[(int)s->sz_type]);

    int table_idx = (s->sz_type << 8) | mod_pos | s->pos_type;
    
    // pos
    int abs_x = x+s->x;
    int abs_y = y+s->y;
    int pos_x = abs_x / s->w;
    int pos_y = abs_y / s->h;

    // select block
    //use blockTable from lrdconst -- need mod-pos sztype and postype
    //int mod_x = abs_x & (s->w-1);
    //int mod_y = abs_y & (s->h-1);
    //int block = ((s->w) * mod_y) + mod_x;
    int block = block_table[table_idx];

    unsigned char * data = (unsigned char*)(image->imageData + block * PI->cblock_size[(int)s->sz_type]);
    *feature = *(data + (pos_y * image->widthStep) + pos_x);
   
    return s->alpha[*feature];
}


static int eval_classifier_lbp_precalc(PreprocessedImage * img, TClassifier * c, int x, int y, unsigned begin, unsigned end, int * features, float * hypotheses, float * response, int * stages)
{
    int mod_pos = get_mod_position(x, y);
    end = min(end, c->stage_count);
    for (unsigned i = begin; i < end; ++i)
    {
        TStage * s = c->stage + i;
        hypotheses[i] = eval_lbp_stage_precalc(img, s, x, y, mod_pos, features + i);
        *response += hypotheses[i];

        if (*response < s->theta_b)
        {
            *stages += begin - i + 1;
            return 0;
        }
    }

    *stages += end - begin;
    return 1;
}


int scan_image_lbp(PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last, int * hist)
{
    // Can eval onlu LBP 2x2 classifiers
    if ((c->tp != LBP) || (c->fsz != FSZ_2x2))
    {
        return 0;
    }

    int features[c->stage_count];
    float hypotheses[c->stage_count];
    float response;
    int stages;

    Detection * det = first;

    for (unsigned y = 0; y < PI->sz.height-c->height; ++y)
    {
        for (unsigned x = 0; x < PI->sz.width-c->width; ++x)
        {
            response = 0.0f;
            stages = 0;
            int d = eval_classifier_lbp_precalc(PI, c, x, y, 0, c->stage_count, features, hypotheses, &response, &stages);
            if (hist) hist[stages]++;
            if (d && (response > c->threshold))
            {
                Detection tmp = {x, y, c->width, c->height, response, 0.0f};
                *det = tmp;
                ++det;
                if (det == last)
                {
                    return det - first;
                }
            }
        }
    }

    return det - first;
}

int is_classifier_supported_lbp(TClassifier * c)
{
    if ((c->tp == LBP) && (c->fsz == FSZ_2x2))
        return 1;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// PRECONVOLVED IMAGE PROCESSING
// LBP, LRP, LRD
////////////////////////////////////////////////////////////////////////////////


/// Core for 16 LBP evaluation
static inline __attribute__((const,always_inline)) __m128i eval_lbp_16(const __m128i * data)
{
    __m128i code = _mm_setzero_si128();
    __m128i weight = ones.q;
    code = _mm_or_si128(code, _mm_and_si128(_mm_cmpgt_epi8(data[0], data[4]), weight));
    weight = _mm_slli_epi64(weight, 1);                                   
    code = _mm_or_si128(code, _mm_and_si128(_mm_cmpgt_epi8(data[1], data[4]), weight));
    weight = _mm_slli_epi64(weight, 1);                                   
    code = _mm_or_si128(code, _mm_and_si128(_mm_cmpgt_epi8(data[2], data[4]), weight));
    weight = _mm_slli_epi64(weight, 1);                                   
    code = _mm_or_si128(code, _mm_and_si128(_mm_cmpgt_epi8(data[5], data[4]), weight));
    weight = _mm_slli_epi64(weight, 1);                                   
    code = _mm_or_si128(code, _mm_and_si128(_mm_cmpgt_epi8(data[8], data[4]), weight));
    weight = _mm_slli_epi64(weight, 1);                                   
    code = _mm_or_si128(code, _mm_and_si128(_mm_cmpgt_epi8(data[7], data[4]), weight));
    weight = _mm_slli_epi64(weight, 1);                                   
    code = _mm_or_si128(code, _mm_and_si128(_mm_cmpgt_epi8(data[6], data[4]), weight));
    weight = _mm_slli_epi64(weight, 1);                                   
    code = _mm_or_si128(code, _mm_and_si128(_mm_cmpgt_epi8(data[3], data[4]), weight));

    return code;
}

static inline __attribute__((const,always_inline)) __m128i eval_lrd_16(const __m128i * data, const __m128i A, const __m128i B)
{
    __m128i sumA, sumB;
    sumA = _mm_slli_epi64(ones.q, 3); // {8,8,8,...8} just to avoid adding in the end (A-B) + 8 = (A+8) - B
    sumB = _mm_setzero_si128();
    
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[0]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[1]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[2]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[3]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[4]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[5]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[6]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[7]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[8]), ones.q));
    
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[0]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[1]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[2]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[3]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[4]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[5]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[6]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[7]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[8]), ones.q));
    
    return _mm_sub_epi8(sumA, sumB);
}

static inline __attribute__((const,always_inline)) __m128i eval_lrp_16(const __m128i * data, const __m128i A, const __m128i B)
{
    __m128i sumA, sumB;
    sumA = _mm_setzero_si128();
    sumB = _mm_setzero_si128();
    
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[0]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[1]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[2]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[3]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[4]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[5]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[6]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[7]), ones.q));
    sumA = _mm_add_epi8(sumA, _mm_and_si128(_mm_cmpgt_epi8(A, data[8]), ones.q));
    
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[0]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[1]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[2]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[3]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[4]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[5]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[6]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[7]), ones.q));
    sumB = _mm_add_epi8(sumB, _mm_and_si128(_mm_cmpgt_epi8(B, data[8]), ones.q));
    
    sumA = _mm_slli_si128(sumA, 4); // 16 * A + B more simple then 10 * A + B
    
    return _mm_add_epi8(sumA, sumB);
}

// This evaluates classifier on a preprocessed image
// * The image is pre-convolved, NOT interleaved convolution!
// * The evaluation proceeds in bunches of 16 weak classifiers
// Properties:
// * Faster than 16 separate evaluations
// * Better for later stages of classification
// * Not good for first stages as waldboost may stop evaluation earlier 
static int eval_classifier_lbp_bunch16(PreprocessedImage * PI, TClassifier * c, int x, int y, unsigned begin, unsigned end, int * features, float * hypotheses, float * response, int * stages)
{
    end = min(end, c->stage_count);
    int mod_pos = get_mod_position(x, y);
    TStage * s;
    int bunch_begin = begin;
    for (s = c->stage+begin; s < c->stage+end; s += 16, bunch_begin += 16)
    {
        int valid_stages = std::min<unsigned long>(c->stage_count - (s - c->stage), 16u);
        // Load data
        int128 feature_data[9];

        for (int i = 0; i < valid_stages; ++i) // feature idx
        {
            TStage * stg = s + i;
            IplImage * conv = &(PI->conv[(int)stg->sz_type]);
            
            int table_idx = (stg->sz_type << 8) | mod_pos | stg->pos_type;

            int abs_x = x + stg->x;
            int abs_y = y + stg->y;
            int pos_x = abs_x / stg->w;
            int pos_y = abs_y / stg->h;

            //select block and mask
            // May use blockTable[szType]
            //int mod_x = abs_x & (stg->w-1);
            //int mod_y = abs_y & (stg->h-1);
            //int block = ((stg->w) * mod_y) + mod_x;
            int block = block_table[table_idx];

            //cerr << "w=" << stg->w << ", h=" << stg->h << ", block=" << block << endl;
            
            char * base = (char*)(conv->imageData + block * PI->cblock_size[(int)stg->sz_type]) + (pos_y * conv->widthStep) + pos_x;
            feature_data[0].i8[i] = *(base + 0);
            feature_data[1].i8[i] = *(base + 1);
            feature_data[2].i8[i] = *(base + 2);
            base += conv->widthStep;
            feature_data[3].i8[i] = *(base + 0);
            feature_data[4].i8[i] = *(base + 1);
            feature_data[5].i8[i] = *(base + 2);
            base += conv->widthStep;
            feature_data[6].i8[i] = *(base + 0);
            feature_data[7].i8[i] = *(base + 1);
            feature_data[8].i8[i] = *(base + 2);
        }
        // xor after loading?

        // Eval all 16 features using SIMD
        __m128i responses = eval_lbp_16((__m128i*)feature_data);
        _mm_empty();
        
        // Eval weak classifiers
        unsigned char * lbp = (unsigned char*)(&responses);

        TStage * stg = s;
        int stg_idx = bunch_begin;
        while(stg != s + valid_stages)
        {
            features[stg_idx] = *lbp;
            hypotheses[stg_idx] = stg->alpha[*lbp];
            *response += hypotheses[stg_idx];
            //fprintf(stderr, "[%d,%f] ", features[stg_idx], hypotheses[stg_idx]);

            if (*response < stg->theta_b)
            {
                *stages += stg_idx - begin + 1;
                //fprintf(stderr, "\n");
                return 0;
            }
            ++stg, ++lbp, ++stg_idx;
        }
    }

    *stages += end - begin;
    //fprintf(stderr, "\n");
    return 1;
}


static int eval_classifier_lrd_bunch16(PreprocessedImage * PI, TClassifier * c, int x, int y, unsigned begin, unsigned end, int * features, float * hypotheses, float * response, int * stages)
{
    end = min(end, c->stage_count);
    int mod_pos = get_mod_position(x, y);
    TStage * s;
    int bunch_begin = begin;
    for (s = c->stage+begin; s < c->stage+end; s += 16, bunch_begin += 16)
    {
        int valid_stages = std::min<unsigned long>(c->stage_count - (s - c->stage), 16u);
        // Load data
        int128 feature_data[9], A, B;

        for (int i = 0; i < valid_stages; ++i) // feature idx
        {
            TStage * stg = s + i;
            IplImage * conv = &(PI->conv[(int)stg->sz_type]);
            
            int table_idx = (stg->sz_type << 8) | mod_pos | stg->pos_type;
            int abs_x = x + stg->x;
            int abs_y = y + stg->y;
            int pos_x = abs_x / stg->w;
            int pos_y = abs_y / stg->h;
            int block = block_table[table_idx];

            char * base = (char*)(conv->imageData + block * PI->cblock_size[(int)stg->sz_type]) + (pos_y * conv->widthStep) + pos_x;
            feature_data[0].i8[i] = *(base + 0);// ^ 0x80;
            feature_data[1].i8[i] = *(base + 1);// ^ 0x80;
            feature_data[2].i8[i] = *(base + 2);// ^ 0x80;
            base += conv->widthStep;
            feature_data[3].i8[i] = *(base + 0);// ^ 0x80;
            feature_data[4].i8[i] = *(base + 1);// ^ 0x80;
            feature_data[5].i8[i] = *(base + 2);// ^ 0x80;
            base += conv->widthStep;
            feature_data[6].i8[i] = *(base + 0);// ^ 0x80;
            feature_data[7].i8[i] = *(base + 1);// ^ 0x80;
            feature_data[8].i8[i] = *(base + 2);// ^ 0x80;
            A.u8[i] = feature_data[(int)stg->A].u8[i];
            B.u8[i] = feature_data[(int)stg->B].u8[i];
        }

        // Eval all 16 features using SIMD
        __m128i responses = eval_lrd_16((__m128i*)feature_data, A.q, B.q);
        _mm_empty();
        
        // Eval weak classifiers
        unsigned char * lbp = (unsigned char*)(&responses);

        TStage * stg = s;
        int stg_idx = bunch_begin;
        while(stg != s + valid_stages)
        {
            features[stg_idx] = *lbp;
            hypotheses[stg_idx] = stg->alpha[*lbp];
            *response += hypotheses[stg_idx];
            //fprintf(stderr, "[%d,%f] ", features[stg_idx], hypotheses[stg_idx]);

            if (*response < stg->theta_b)
            {
                *stages += stg_idx - begin + 1;
                //fprintf(stderr, "\n");
                return 0;
            }
            ++stg, ++lbp, ++stg_idx;
        }
    }

    *stages += end - begin;
    //fprintf(stderr, "\n");
    return 1;
}

static int eval_classifier_lrp_bunch16(PreprocessedImage * PI, TClassifier * c, int x, int y, unsigned begin, unsigned end, int * features, float * hypotheses, float * response, int * stages)
{
    end = min(end, c->stage_count);
    int mod_pos = get_mod_position(x, y);
    TStage * s;
    int bunch_begin = begin;
    for (s = c->stage+begin; s < c->stage+end; s += 16, bunch_begin += 16)
    {
        int valid_stages = std::min<unsigned long>(c->stage_count - (s - c->stage), 16u);
        // Load data
        int128 feature_data[9], A, B;

        for (int i = 0; i < valid_stages; ++i) // feature idx
        {
            TStage * stg = s + i;
            IplImage * conv = &(PI->conv[(int)stg->sz_type]);
            
            int table_idx = (stg->sz_type << 8) | mod_pos | stg->pos_type;
            int abs_x = x + stg->x;
            int abs_y = y + stg->y;
            int pos_x = abs_x / stg->w;
            int pos_y = abs_y / stg->h;
            int block = block_table[table_idx];

            char * base = (char*)(conv->imageData + block * PI->cblock_size[(int)stg->sz_type]) + (pos_y * conv->widthStep) + pos_x;
            feature_data[0].i8[i] = *(base + 0);// ^ 0x80;
            feature_data[1].i8[i] = *(base + 1);// ^ 0x80;
            feature_data[2].i8[i] = *(base + 2);// ^ 0x80;
            base += conv->widthStep;
            feature_data[3].i8[i] = *(base + 0);// ^ 0x80;
            feature_data[4].i8[i] = *(base + 1);// ^ 0x80;
            feature_data[5].i8[i] = *(base + 2);// ^ 0x80;
            base += conv->widthStep;
            feature_data[6].i8[i] = *(base + 0);// ^ 0x80;
            feature_data[7].i8[i] = *(base + 1);// ^ 0x80;
            feature_data[8].i8[i] = *(base + 2);// ^ 0x80;
            A.u8[i] = feature_data[(int)stg->A].u8[i];
            B.u8[i] = feature_data[(int)stg->B].u8[i];
        }
        // xor after loading?
        // xor during PP?
        for (int i = 0; i < 9; ++i) // will be unrolled?
             feature_data[i].q = _mm_xor_si128(feature_data[i].q, sign_bit.q);

        // Eval all 16 features using SIMD
        __m128i responses = eval_lrp_16((__m128i*)feature_data, A.q, B.q);
        _mm_empty();
        
        // Eval weak classifiers
        unsigned char * lbp = (unsigned char*)(&responses);

        TStage * stg = s;
        int stg_idx = bunch_begin;
        while(stg != s + valid_stages)
        {
            features[stg_idx] = *lbp;
            hypotheses[stg_idx] = stg->alpha[*lbp];
            *response += hypotheses[stg_idx];
            //fprintf(stderr, "[%d,%f] ", features[stg_idx], hypotheses[stg_idx]);

            if (*response < stg->theta_b)
            {
                *stages += stg_idx - begin + 1;
                //fprintf(stderr, "\n");
                return 0;
            }
            ++stg, ++lbp, ++stg_idx;
        }
    }

    *stages += end - begin;
    //fprintf(stderr, "\n");
    return 1;
}

// histogram of stage counts?
int scan_image_conv_bunch16(PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last, int * hist)
{
    if (c->fsz != FSZ_2x2)
    {
        return 0;
    }

    ClassifierEvalFunc eval = 0;

    switch (c->tp)
    {
    case LRD:
        eval = eval_classifier_lrd_bunch16;
        break;
    case LRP:
        eval = eval_classifier_lrp_bunch16;
        break;
    case LBP:
        eval = eval_classifier_lbp_bunch16;
        break;
    default:
        break;
    }
    
    if (!eval) return 0;
     

    int features[c->stage_count];
    float hypotheses[c->stage_count];
    float response;
    int stages;

    Detection * det = first;

    if (det >= last) return 0;
 
    for (unsigned y = 0; y < PI->sz.height-c->height; ++y)
    {
        for (unsigned x = 0; x < PI->sz.width-c->width; ++x)
        {
            response = 0.0f;
            stages = 0;
            int d = eval(PI, c, x, y, 0, c->stage_count, features, hypotheses, &response, &stages);
            if (hist) hist[stages]++;
            if (d && (response > c->threshold))
            {
                Detection tmp = {x, y, c->width, c->height, response, 0.0f};
                *det = tmp;
                ++det;
                if (det == last)
                {
                    return det - first;
                }
            }
        }
    }

    return det - first;
}

int is_classifier_supported_conv_bunch16(TClassifier * c)
{
    if ((c->tp == LBP || c->tp == LRP || c->tp == LRD) && c->fsz == FSZ_2x2)
        return 1;
    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// Evaluation on interlieved convolution
// LRD, LRP, LBP
//

inline float eval_lrd_stage_iconv(
        PreprocessedImage * PI,
        int x, int y, int mod_pos,
        const TStage * stg, const int * ranks,
        int * feature)
{
    // index to tables - depends on feature size, sample position and feature position relative to sample
    int table_idx = (stg->sz_type << 8) | mod_pos | stg->pos_type;
    
    // Get the block of the convolution (depends on feature modulo shift)
    int block = block_table[table_idx];
    
    // Get the mask type
    int mask_type = mask_table[table_idx];
    
    // Get the convolution image;
    IplImage * conv = &(PI->iconv[int(stg->sz_type)]);
    char * conv_data = conv->imageData;
    
    // Get address of the feature in image
    int data_offset = PI->xtbl[4 * (x + stg->x) + stg->sz_type] + PI->ytbl[4 * (y+stg->y) + stg->sz_type];
    
    signed char * data0 = (signed char*)conv_data + PI->iblock_size[int(stg->sz_type)] * block + data_offset;
    signed char * data1 = data0 + conv->widthStep;
    
    // Get the A nd B rank index according to the shift type

    int A_offset = ranks[2 * mask_type + 0];
    int B_offset = ranks[2 * mask_type + 1];

    register __m128i data = _mm_set_epi64(*(__m64*)(data1), *(__m64*)(data0));
    register __m128i zero = _mm_setzero_si128();

	union {
        __m128i q;
        signed short s16[8];
    } diff = { _mm_sub_epi16(
        _mm_sad_epu8( // countA
            _mm_and_si128(
                _mm_cmpgt_epi8(_mm_set1_epi8(*(data0+A_offset)), data),
                masks[mask_type].q),
            zero),
        _mm_sad_epu8( // countB
            _mm_and_si128(
                _mm_cmpgt_epi8(_mm_set1_epi8(*(data0+B_offset)), data),
                masks[mask_type].q),
            zero)
        )
    }; 

	*feature = (diff.s16[4] + diff.s16[0]) + 8;
    _mm_empty();

    return stg->alpha[*feature];
}


inline float eval_lrp_stage_iconv(
        PreprocessedImage * PI,
        int x, int y, int mod_pos,
        const TStage * stg, const int * ranks,
        int * feature)
{
    // index to tables - depends on feature size, sample position and feature position relative to sample
    int table_idx = (stg->sz_type << 8) | mod_pos | stg->pos_type;
    
    // Get the block of the convolution (depends on feature modulo shift)
    int block = block_table[table_idx];
    
    // Get the mask type
    int mask_type = mask_table[table_idx];
    
    // Get the convolution image;
    IplImage * conv = &(PI->iconv[int(stg->sz_type)]);
    char * conv_data = conv->imageData;
    
    // Get address of the feature in image
    int data_offset = PI->xtbl[4 * (x + stg->x) + stg->sz_type] + PI->ytbl[4 * (y+stg->y) + stg->sz_type];
    
    signed char * data0 = (signed char*)conv_data + PI->iblock_size[int(stg->sz_type)] * block + data_offset;
    signed char * data1 = data0 + conv->widthStep;
    
    // Get the A nd B rank index according to the shift type

    int A_offset = ranks[2 * mask_type + 0];
    int B_offset = ranks[2 * mask_type + 1];

    register __m128i data = _mm_set_epi64(*(__m64*)(data1), *(__m64*)(data0));
    register __m128i zero = _mm_setzero_si128();

	union {
        __m128i q;
        signed short s16[8];
    } countB = {
        _mm_sad_epu8( // countA
            _mm_and_si128(
                _mm_cmpgt_epi8(_mm_set1_epi8(*(data0+A_offset)), data),
                masks[mask_type].q),
            zero)};

	union {
        __m128i q;
        signed short s16[8];
    } countA = {
        _mm_sad_epu8( // countB
            _mm_and_si128(
                _mm_cmpgt_epi8(_mm_set1_epi8(*(data0+B_offset)), data),
                masks[mask_type].q),
            zero)}; 

	*feature = 10 * (countB.s16[4] + countB.s16[0]) + (countA.s16[4] + countA.s16[0]);
    _mm_empty();

    return stg->alpha[*feature];
}

float eval_lbp_stage_iconv(
        PreprocessedImage * PI,
        int x, int y, int mod_pos,
        const TStage * stg, const int * ranks,
        int * feature)
{
    // index to tables - depends on feature size, sample position and feature position relative to sample
    int table_idx = (stg->sz_type << 8) | mod_pos | stg->pos_type;
    
    // Get the block of the convolution (depends on feature modulo shift)
    int block_id = block_table[table_idx];
    
    // Get the mask type
    int mask_type = mask_table[table_idx];
    
    // Get the convolution image;
    IplImage * conv = &(PI->iconv[int(stg->sz_type)]);
    char * conv_data = conv->imageData;

    // Get address of the feature in image
    int dataOffset = PI->xtbl[4 * (x + stg->x) + stg->sz_type] + PI->ytbl[4 * (y+stg->y) + stg->sz_type];
    
    signed char * data0 = (signed char*)conv_data + PI->iblock_size[int(stg->sz_type)] * block_id + dataOffset;
    signed char * data1 = data0 + conv->widthStep;

    // get the center pixel
    static const int center_offset[4] = {3, 6, 1, 4};
    signed char * center = (mask_type < 2) ? data0 : data1;
    center += center_offset[mask_type];
    
    register __m128i data = _mm_set_epi64(*(__m64*)(data1), *(__m64*)(data0));
    register __m128i zero = _mm_setzero_si128();
    // LBP evaluation
    // Comparison (pixel values to central pixel) result is used
    // to mask LBP weights. The masked vector is then summed up
    // producing the LBP value.
    union {
        __m128i q;
        signed short ss[8];
    } result = {
        _mm_sad_epu8(
            _mm_and_si128(
                lbp_weights[mask_type].q,
                _mm_cmpgt_epi8(data, _mm_set1_epi8(*center))),
            zero)
    };

    *feature = result.ss[4] + result.ss[0];
    _mm_empty();
    
    return stg->alpha[*feature];
}

typedef float (*StageEvalFuncIconv)(PreprocessedImage * PI,
        int x, int y, int mod_pos,
        const TStage * stg, const int * ranks,
        int * feature);


template<StageEvalFuncIconv eval>
int eval_classifier_iconv(
        PreprocessedImage * PI,
        TClassifier * c,
        int x, int y,
        unsigned begin, unsigned end,
        int * features, float * hypotheses, float * response, int * stages)
{
    int mod_pos = get_mod_position(x, y);
    end = min(end, c->stage_count);
    const int * rank = c->ranks + (8 * begin);
    for (unsigned s = begin; s < end; ++s, rank += 8)
    {
	    const TStage * stg = c->stage + s;
        hypotheses[s] = eval(PI, x, y, mod_pos, stg, rank, features + s);
        *response += hypotheses[s];
        //fprintf(stderr, "[%d,%f] ", features[s], hypotheses[s]);
        if (*response < stg->theta_b)
        {
            *stages += s - begin + 1;
            //fprintf(stderr, "\n");
            return 0;
        }
    }
    
    *stages += end - begin;
    //fprintf(stderr, "\n");
    return 1;
}


int scan_image_iconv(PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last, int * hist)
{
    if (c->fsz != FSZ_2x2)
    {
        return 0;
    }

    ClassifierEvalFunc eval = 0;

    switch (c->tp)
    {
    case LRD:
        eval = eval_classifier_iconv<eval_lrd_stage_iconv>;
        break;
    case LRP:
        eval = eval_classifier_iconv<eval_lrp_stage_iconv>;
        break;
    case LBP:
        eval = eval_classifier_iconv<eval_lbp_stage_iconv>;
        break;
    default:
        break;
    };

    if (!eval)
    {
        return 0;
    }

    int features[c->stage_count];
    float hypotheses[c->stage_count];
    float response;
    int stages;

    Detection * det = first;

#if 0 // For testing purposes
    {
    response = 0.0f;
    int d = eval(PI, c, 60, 15, 0, c->stage_count, features, hypotheses, &response, &stages);
    fprintf(stderr, "response: %f, decision: %d, stages evaluated: %d\n", response, d, stages);
    return 0;
    }
#endif

    for (unsigned y = 1; y < PI->sz.height-c->height-1; ++y)
    {
        for (unsigned x = 1; x < PI->sz.width-c->width-1; ++x)
        {
            response = 0.0f;
            stages = 0;
            int d = eval(PI, c, x, y, 0, c->stage_count, features, hypotheses, &response, &stages);
            if (hist) hist[stages-1]++;
            if (d && (response > c->threshold))
            {
                Detection tmp = { x, y, c->width, c->height, response, 0.0f };
                *det = tmp;
                ++det;
                if (det == last)
                {
                    return det - first;
                }
            }
        } // x
    } // y
    return det - first;
}

int is_classifier_supported_iconv(TClassifier * c)
{
    if ((c->tp == LBP || c->tp == LRP || c->tp == LRD) && c->fsz == FSZ_2x2)
        return 1;
    return 0;
}


//// COMBINED SCANNERS

int scan_image_iconv_conv(PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last, int * hist)
{
    if (c->fsz != FSZ_2x2)
    {
        return 0;
    }

    ClassifierEvalFunc eval_A = 0;
    ClassifierEvalFunc eval_B = 0;

    switch (c->tp)
    {
    case LRD:
        eval_A = eval_classifier_iconv<eval_lrd_stage_iconv>;
        eval_B = eval_classifier_lrd_bunch16;
        break;
    case LRP:
        eval_A = eval_classifier_iconv<eval_lrp_stage_iconv>;
        eval_B = eval_classifier_lrp_bunch16;
        break;
    case LBP:
        eval_A = eval_classifier_iconv<eval_lbp_stage_iconv>;
        eval_B = eval_classifier_lbp_bunch16;
        break;
    default:
        break;
    };

    if (!eval_A || !eval_B)
    {
        return 0;
    }

    int features[c->stage_count];
    float hypotheses[c->stage_count];
    float response;
    int stages;
    
    Detection * det = first;

#if 0 // For testing purposes
    {
    response = 0.0f;
    int d = eval(PI, c, 60, 15, 0, c->stage_count, features, hypotheses, &response, &stages);
    fprintf(stderr, "response: %f, decision: %d, stages evaluated: %d\n", response, d, stages);
    return 0;
    }
#endif

    for (unsigned y = 1; y < PI->sz.height-c->height-1; ++y)
    {
        for (unsigned x = 1; x < PI->sz.width-c->width-1; ++x)
        {
            response = 0.0f;
            stages = 0;
            int d0 = eval_A(PI, c, x, y, 0, sp->division_a, features, hypotheses, &response, &stages);
            if (d0)
            {
                int d1 = eval_A(PI, c, x, y, sp->division_a, c->stage_count, features, hypotheses, &response, &stages);
                if (hist) hist[stages-1]++;
                if (d1 && (response > c->threshold))
                {
                    Detection tmp = { x, y, c->width, c->height, response, 0.0f };
                    *det = tmp;
                    ++det;
                    if (det == last)
                    {
                        return det - first;
                    }
                }
            }
            else
            {
                if (hist) hist[stages-1]++;
            }
        } // x
    } // y
    return det - first;
}

