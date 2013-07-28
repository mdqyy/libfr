/*
 *  classifier.cpp
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

#include "classifier.h"
#include "simplexml.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <stack>
#include <vector>
#include <list>
#include <cassert>

using namespace std;


void release_classifier(TClassifier ** classifier)
{
    if ((classifier && *classifier) && ((**classifier).model == C_DYNAMIC))
    {
        TClassifier & c = **classifier;

        if (c.alpha) delete [] c.alpha;
        if (c.stage) delete [] c.stage;
        if (c.ranks) delete [] c.ranks;
        
        delete *classifier;
        *classifier = 0;
    }
}


static void loadLRDFeature(xmlNodePtr fNode, TStage * stage)
{
    if (!fNode) return;
    xmlNodePtr lrNode = 0;
    if (!lrNode)
    {
        lrNode = getNode("LRDFeature", fNode);
    }
    if (!lrNode)
    {
        lrNode = getNode("LRP", fNode);
    }
    if (!lrNode) return;

    getAttr(stage->x, "positionX", lrNode);
    getAttr(stage->y, "positionY", lrNode);
    getAttr(stage->w, "blockWidth", lrNode);
    getAttr(stage->h, "blockHeight", lrNode);
    int A,B;
    getAttr(A, "blockA", lrNode);
    getAttr(B, "blockB", lrNode);
    stage->A = A;
    stage->B = B;
}


static void loadLBPFeature(xmlNodePtr fNode, TStage * stage)
{
    if (!fNode) return;
    xmlNodePtr lrNode = 0;
    if (!lrNode)
    {
        lrNode = getNode("LBPFeature", fNode);
    }
    if (!lrNode) return;

    getAttr(stage->x, "positionX", lrNode);
    getAttr(stage->y, "positionY", lrNode);
    getAttr(stage->w, "blockWidth", lrNode);
    getAttr(stage->h, "blockHeight", lrNode);
}

static void loadHistogramWeakHypothesis(xmlNodePtr hNode, vector<float> & predictTable)
{
    if (!hNode) return;
            
    istringstream predictStr(getAttr("predictionValues", hNode));

    float p;
    while (predictStr >> p)
    {
        predictTable.push_back(p);
    }
}


static void loadDTWeakHypothesis(xmlNodePtr hNode, vector<float> & predictTable)
{
    if (!hNode) return;
            
    istringstream predictStr(getAttr("predictionValues", hNode));
    istringstream binMapStr(getAttr("binMap", hNode));

    vector<float> predict;
    float p;
    while (predictStr >> p)
    {
        predict.push_back(p);
    }

    int bin;
    while (binMapStr >> bin)
    {
        predictTable.push_back(predict[bin]);
    }

}


static void loadLRDClassifier(xmlNodePtr stages, TClassifier * classifier);


TClassifier * loadClassifier(xmlNodePtr);


// Search the XML element with classifier
static xmlNodePtr findClassifierNode(xmlNodePtr rootPtr)
{
    list<xmlNodePtr> classifiers;
    stack<xmlNodePtr> s;
    s.push(rootPtr);
    while (!s.empty())
    {
        xmlNodePtr current = s.top();
        s.pop();
        
        if (xmlStrcmp(current->name, BAD_CAST "WaldBoostClassifier") == 0)
        {
            classifiers.push_back(current);
        }
        for (xmlNodePtr child = current->children; child; child=child->next)
        {
            s.push(child);
        }
    }
    
    if (classifiers.empty())
    {
        return 0;
    }

    // This should never happen. But who knows...
    if (classifiers.size() > 1)
    {
        cerr << "Warning: The xml contains " << classifiers.size() << " classifiers. Using the first." << endl;
    }

    return classifiers.front();
}


TClassifier * load_classifier_XML(const char * filename)
{
    xmlDocPtr inputXML = xmlParseFile(filename);

    if (!inputXML)
    {
        cerr << "Cannot parse file " << filename << endl;
        return 0;
    }

    xmlNodePtr xmlRoot = xmlDocGetRootElement(inputXML);

    if (!xmlRoot)
    {
        cerr << "Invalid XML" << endl;
        return 0;
    }

    xmlNodePtr classifierNode = findClassifierNode(xmlRoot);

    if (!classifierNode)
    {
        cerr << "Cannot find classifier in " << filename << endl;
        return 0;
    }

    // destroy xml doc? how?

    return loadClassifier(classifierNode);
}
    
static ClassifierType determineClassifierType(xmlNodePtr classifierRoot)
{

    // If 'type' attribute is present, use it
    string tp;
    getAttr(tp, "type", classifierRoot);
    
    if (tp == string("LRD"))
    {
        return LRD;
    }
    if (tp == string("LRP"))
    {
        return LRP;
    }
    if (tp == string("LBP"))
    {
        return LBP;
    }
    // No 'type' specified, try to determine by classifier content

    map<string, int> elements;
    // Known feature types
    map<string, ClassifierType> knownTypes;
    knownTypes["LRDFeature"] = LRD;
    knownTypes["LRP"] = LRP;
    knownTypes["LBPFeature"] = LBP;

    // Enumerate elements used in the classifier
    stack<xmlNodePtr> s;
    s.push(classifierRoot);
    while (!s.empty())
    {
        xmlNodePtr current = s.top();
        s.pop();
        const char * name = (const char*) current->name;
        elements[string(name)]++;
        for (xmlNodePtr child = current->children; child; child=child->next)
        {
            s.push(child);
        }
    }

    ClassifierType type = UNKNOWN;
    bool typeSet = false;
    
    // Go through elements found in the classifier
    for (map<string, int>::iterator i = elements.begin(); i != elements.end(); ++i)
    {
        if (knownTypes.find(i->first) != knownTypes.end())
        {
            // Supported feature type found

            if (!typeSet)
            {
                // There is no type determined yet
                // the type of the classifier then corrensponds to the feature type found
                typeSet = true;
                type = knownTypes[i->first];
            }
            else
            {
                if (knownTypes[i->first] != type)
                {
                    // there are mixed feature types while the
                    // framework supprts only monolithic
                    // classifiers
                    // -> fall back to UNKNOWN type
                    type = UNKNOWN;
                }
            }
        }
    }
   
    return type;
}

// Load the classifier from XML structure
TClassifier * loadClassifier(xmlNodePtr classifierRoot)
{
    TClassifier * classifier = new TClassifier();

    string tp;
    classifier->threshold = 0.0f;
    classifier->model = C_DYNAMIC;
    classifier->fsz = FSZ_2x2;
    getAttr(classifier->width, "sizeX", classifierRoot);
    getAttr(classifier->height, "sizeY", classifierRoot);
    getAttr(classifier->width, "imageSizeX", classifierRoot);
    getAttr(classifier->height, "imageSizeY", classifierRoot);

    //cerr << classifier->width << "x" << classifier->height << endl;

    classifier->tp = determineClassifierType(classifierRoot);

    loadLRDClassifier(classifierRoot->children, classifier);

    /*
    cerr << "Classifier info:" << endl;
    cerr << "Type: " << classifierTypeStrings[classifier->tp] << endl;
    cerr << "Scan window: " << classifier->width << "x" << classifier->height << endl;
    cerr << "Stages: " << classifier->stageCount << endl;
    cerr << "Alphas per stages: " << classifier->alphaCount << endl;
    */
    return classifier;
}


// LRD/LRP/LBP specific stuff

static void loadLRDClassifier(xmlNodePtr stages, TClassifier * classifier)
{
    vector<TStage> tmpStages;
    vector< vector<float> > tmpPredict;
    
    for (xmlNodePtr node = stages; node; node = node->next)
    {
        if (xmlStrcmp(node->name, BAD_CAST "stage") == 0)
        {
            // load stage
            TStage stage = {0,0,1,1,0,1,0.0f,0,0,0};
            
            double theta;
            getAttr(theta, "negT", node);
            if (theta > 5000.0) theta = 5000.0;
            if (theta < -5000.0) theta = -5000.0;
            stage.theta_b = float(theta);

            // load hypothesis part
            xmlNodePtr hypothesisNode = 0;
            if (!hypothesisNode)
            {
                hypothesisNode = getNode("HistogramWeakHypothesis", node);
                if (hypothesisNode)
                {
                    tmpPredict.push_back(vector<float>());
                    loadHistogramWeakHypothesis(hypothesisNode, tmpPredict.back());
                }
                else 
                {
                    hypothesisNode = getNode("DecisionTreeWeakHypothesis", node);
                    if (hypothesisNode)
                    {
                        tmpPredict.push_back(vector<float>());
                        loadDTWeakHypothesis(hypothesisNode, tmpPredict.back());
                    }
                }
            }

            if (!hypothesisNode)
            {
                cerr << "Warning: Incomplete stage encountered" << endl;
            }
            else
            {
                if (classifier->tp == LRD || classifier->tp == LRP)
                    loadLRDFeature(hypothesisNode, &stage);
                if (classifier->tp == LBP)
                    loadLBPFeature(hypothesisNode, &stage);
            }
            if (stage.w > 2 || stage.h > 2)
                classifier->fsz = FSZ_UNRESTRICTED;
            //cout << stage.x << ";" << stage.y << "]" << endl;
            tmpStages.push_back(stage);
        }
    }

    // Alloc the stages and alphas, and copy loaded data
    
    classifier->stage_count = tmpStages.size();
    classifier->alpha_count = 0;
    if (classifier->tp == LRD) classifier->alpha_count = 17;
    if (classifier->tp == LRP) classifier->alpha_count = 100;
    if (classifier->tp == LBP) classifier->alpha_count = 256;
    assert(classifier->alpha_count > 0);
    classifier->threshold = 0.0f;

    classifier->stage = new TStage[tmpStages.size()];
    classifier->alpha = new float[tmpStages.size() * classifier->alpha_count];
    classifier->ranks = new int[8 * tmpStages.size()]; // Alloc always or only in necessary cases (LRD, LRP)?
    fill(classifier->ranks, classifier->ranks + 8*tmpStages.size(), 0);

    int errors = 0;

    for (unsigned s = 0; s < tmpStages.size(); ++s)
    {
        ((TStage*)(classifier->stage))[s] = tmpStages[s];
        if (tmpPredict[s].size() != classifier->alpha_count)
        {
            cout << tmpPredict[s].size() << endl;
            ++errors;
        }
        float * dstAlpha = classifier->alpha + classifier->alpha_count * s;

        copy(tmpPredict[s].begin(), tmpPredict[s].end(), dstAlpha);
    }

    // TODO ALPHA rearrangement for LRP

    if (errors)
    {
        cerr << "Cannot load the classifier (" << errors << " errors occured)." << endl;
        release_classifier(&classifier);
        return;
    }
}


/// Strings with types of classifiers (Indexed by ClassifierType).
const char *const classifierTypeStrings[] = {
    "UNKNOWN",
    "LRD",
    "LRP",
    "LBP",
};

const char *const fsz_string[] = {
    "FSZ_UNRESTRICTED",
    "FSZ_2x2",
};


// EXPORT STUFF

/*
// name
//
// Automatically generated on TIMESTAMP
#ifndef _HASH_
#define _HASH_

#include <abr_engine.h>

extern TClassifier * name;

#endif 
 */

void export_classifier_header(TClassifier * c, ostream & str, const char * name)
{
    if (c->tp == UNKNOWN)
    {
        str << "// UNKNOWN CLASSIFIER\n";
        return;
    }
    time_t t = time(0);
    str << "//\n// Classifier " << name << "\n//\n" << "// Automatically generated on " << t << "\n//\n\n";
    str << "#ifndef _" << name << hex << t << "_\n";
    str << "#define _" << name << hex << t << "_\n\n";
    str << "#include <abr/structures.h>\n\n";
    str << "extern TClassifier " << name << ";\n\n";
    str << "#endif" << dec << endl;
    return;
}

void export_classifier_source(TClassifier * c, ostream & str, const char * name, const char * headerName)
{
    if (c->tp == UNKNOWN)
    {
        str << "// UNKNOWN CLASSIFIER\n";
        return;
    }

    assert(sizeof(classifierTypeStrings) == (size_t)numClassifierTypes*sizeof(*classifierTypeStrings));

    str << "#include \"" << headerName << "\"\n\n";

    str << "#define STAGE_COUNT " << c->stage_count << "\n";
    str << "#define ALPHA_COUNT " << c->alpha_count << "\n\n";

    str << "static int _ranks_" << name << "[";
    if (c->tp == LRD || c->tp == LRP)
    {
        str << "8 * STAGE_COUNT";
    }
    else
    {
        str << "1";
    }
    str << "]; //uninitialized\n\n";

    str << "static float _alphas_" << name << "[STAGE_COUNT * ALPHA_COUNT] = {\n" << flush;
    float * alpha = c->alpha;
    for (unsigned s = 0; s < c->stage_count; ++s, alpha+=c->alpha_count)
    {
        for (unsigned a = 0; a < c->alpha_count; ++a)
        {
            str << showpoint << fixed << setprecision(8) << alpha[a] << "f, " << flush;
        }
        str << "\n";
    }
    str << "};\n\n";

    str << "static TStage _stages_" << name << "[STAGE_COUNT] = {\n";
    for (unsigned s = 0; s < c->stage_count; ++s)
    {
        TStage & stg = ((TStage*)(c->stage))[s];
        str << "{ " <<
            stg.x << ", " << stg.y << ", " <<
            stg.w << ", " << stg.h << ", " <<
            int(stg.A) << ", " << int(stg.B) << ", " <<
            showpoint << fixed << setprecision(8)<< stg.theta_b << ", " <<
            "_alphas_" << name << "+" << s*c->alpha_count << ", " << // alpha
            0 << ", " << 0 << ", " << 0 << ", " << // conv, code
            "},\n"; // szType, posType
    }
    str << "};\n\n" << flush;

    str << "TClassifier " << name << " = {\n";
    //str << "(ClassifierType)" << int(c->tp) << ", STAGE_COUNT, STAGE_COUNT, ALPHA_COUNT, " << 
    str << classifierTypeStrings[c->tp] << ", C_STATIC, ";
    str << fsz_string[c->fsz] << ", ";
    str << "STAGE_COUNT, ALPHA_COUNT, " << 
        c->threshold << ", " << 
        c->width << ", " << c->height << ", " <<
        "(TStage*) _stages_" << name << ", (float*) _alphas_" << name << ", (int*) _ranks_" << name << ",\n";
    str << "};\n\n" << flush;
}


