/*
 * classifier.cpp
 */

#include "classifier.h"
#include "simplexml.h"

#include <sstream>
#include <iostream>
#include <map>
#include <stack>
#include <vector>
#include <list>
#include <cassert>

using namespace std;


// Set all thresholds to -1E10 which effectively disables WaldBoost evaluation
template<class S>
static inline void disableWB(S *s, const unsigned c)
{
    for(unsigned i = 0; i < c; ++i)
    {
        s[i].theta_b = -1e+010;
    }
}


void disableClassifierWaldBoost(TClassifier * c)
{
    assert(c != NULL);

    switch(c->tp)
    {
        case LRD:
        case LRP:
        case LBP:
                  disableWB((TStage*)(c->stage), c->stageCount);
                  break;
        case HAAR:
                  disableWB((THaarStage*)(c->stage), c->stageCount);
                  break;
        default:
                  cerr << "Classifier type not supported!" << endl;
    };
}


void releaseClassifier(TClassifier ** classifier)
{
    if (classifier && *classifier)
    {
        TClassifier & c = **classifier;
        if (c.alpha) delete [] c.alpha;
        
        switch (c.tp)
        {
        case LRD:
        case LRP:
        case LBP:
            if (c.stage) delete [] (TStage*)c.stage;
            break;
        case HAAR:
            if (c.stage) delete [] (THaarStage*)c.stage;
            break;
        default:
            break;
        };
        
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

static void loadHaarFeature(xmlNodePtr fNode, THaarStage * stage)
{
    if (!fNode) return;

    xmlNodePtr discretizeNode = getNode("TCont2DiscFeature", fNode);
    if (!discretizeNode)
        return;

    getAttr(stage->min, "minValue", discretizeNode);
    getAttr(stage->max, "maxValue", discretizeNode);
    getAttr(stage->bins, "numberOfBins", discretizeNode);

    //cerr << stage->min << "," << stage->max << "," << stage->bins << endl;
    
    xmlNodePtr lrNode = 0;
    if (!lrNode)
    {
        lrNode = getNode("HaarHorizontalDoubleFeature", discretizeNode);
        stage->tp = 0;
    }
    if (!lrNode)
    {
        lrNode = getNode("HaarVerticalDoubleFeature", discretizeNode);
        stage->tp = 1;
    }
    if (!lrNode)
    {
        lrNode = getNode("HaarHorizontalTernalFeature", discretizeNode);
        stage->tp = 2;
    }
    if (!lrNode)
    {
        lrNode = getNode("HaarVerticalTernalFeature", discretizeNode);
        stage->tp = 3;
    }
    if (!lrNode)
    {
        lrNode = getNode("HaarDiagonalFeature", discretizeNode);
        stage->tp = 4;
    }
    if (!lrNode)
    {
        lrNode = getNode("HaarSurroundFeature", discretizeNode);
        stage->tp = 5;
    }
    
    if (!lrNode) return;

    getAttr(stage->x, "positionX", lrNode);
    getAttr(stage->y, "positionY", lrNode);
    getAttr(stage->w, "blockWidth", lrNode);
    getAttr(stage->h, "blockHeight", lrNode);
    
    //cerr << "tp="<< stage->tp << ", x=" << stage->x << ", y=" << stage->y << ", w=" << stage->w << ", h=" << stage->h << endl;
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
static void loadHaarClassifier(xmlNodePtr stages, TClassifier * classifier);


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


TClassifier * loadClassifierXML(const char * filename)
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
    if (tp == string("HAAR"))
    {
        return HAAR;
    }
    
    // No 'type' specified, try to determine by classifier content

    map<string, int> elements;
    // Known feature types
    map<string, ClassifierType> knownTypes;
    knownTypes["LRDFeature"] = LRD;
    knownTypes["LRP"] = LRP;
    knownTypes["LBPFeature"] = LBP;
    knownTypes["HaarHorizontalDoubleFeature"] = HAAR;
    knownTypes["HaarVerticalDoubleFeature"] = HAAR;
    knownTypes["HaarHorizontalTernalFeature"] = HAAR;
    knownTypes["HaarVerticalTernalFeature"] = HAAR;
    knownTypes["HaarDiagonalFeature"] = HAAR;
    knownTypes["HaarSurroundFeature"] = HAAR;

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
    getAttr(classifier->width, "sizeX", classifierRoot);
    getAttr(classifier->height, "sizeY", classifierRoot);
    getAttr(classifier->width, "imageSizeX", classifierRoot);
    getAttr(classifier->height, "imageSizeY", classifierRoot);

    //cerr << classifier->width << "x" << classifier->height << endl;

    classifier->tp = determineClassifierType(classifierRoot);

    switch (classifier->tp)
    {
    case LRD:
    case LRP:
    case LBP:
        loadLRDClassifier(classifierRoot->children, classifier);
        break;
    case HAAR:
        loadHaarClassifier(classifierRoot->children, classifier);
        break;
    case UNKNOWN:
    default:
        cerr << "Unknown classifier type!" << endl;
        delete classifier;
        return 0;
        break;
    };
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
            TStage stage = {0,0,1,1,0,1,0.0f};
            
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
            
            //cout << stage.x << ";" << stage.y << "]" << endl;
            tmpStages.push_back(stage);
        }
    }

    // Alloc the stages and alphas, and copy loaded data
    
    classifier->stageCount = tmpStages.size();
    classifier->alphaCount = 0;
    if (classifier->tp == LRD) classifier->alphaCount = 17;
    if (classifier->tp == LRP) classifier->alphaCount = 100;
    if (classifier->tp == LBP) classifier->alphaCount = 256;
    assert(classifier->alphaCount > 0);
    classifier->threshold = 0.0f;

    classifier->stage = new TStage[tmpStages.size()];
    classifier->alpha = new float[tmpStages.size() * classifier->alphaCount];

    int errors = 0;

    for (unsigned s = 0; s < tmpStages.size(); ++s)
    {
        ((TStage*)(classifier->stage))[s] = tmpStages[s];
        if (tmpPredict[s].size() != classifier->alphaCount)
        {
            cout << tmpPredict[s].size() << endl;
            ++errors;
        }
        float * dstAlpha = classifier->alpha + classifier->alphaCount * s;

        copy(tmpPredict[s].begin(), tmpPredict[s].end(), dstAlpha);
    }

    if (errors)
    {
        cerr << "Cannot load the classifier (" << errors << " errors occured)." << endl;
        releaseClassifier(&classifier);
        return;
    }
}


static void loadHaarClassifier(xmlNodePtr stages, TClassifier * classifier)
{
    vector<THaarStage> tmpStages;
    vector< vector<float> > tmpPredict;
    
    for (xmlNodePtr node = stages; node; node = node->next)
    {
        if (xmlStrcmp(node->name, BAD_CAST "stage") == 0)
        {
            // load stage
            THaarStage stage = {0,0,1,1,0,0,1,1,0.0f,0,1,0,0};
            
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
                if (classifier->tp == HAAR)
                    loadHaarFeature(hypothesisNode, &stage);
            }
            
            //cout << stage.x << ";" << stage.y << "]" << endl;
            tmpStages.push_back(stage);
        }
    }

    // Alloc the stages and alphas, and copy loaded data
    
    classifier->stageCount = tmpStages.size();
    classifier->alphaCount = tmpPredict[0].size();
    classifier->threshold = 0.0f;

    classifier->stage = new THaarStage[tmpStages.size()];
    classifier->alpha = new float[tmpStages.size() * classifier->alphaCount];

    int errors = 0;

    for (unsigned s = 0; s < tmpStages.size(); ++s)
    {
        ((THaarStage*)(classifier->stage))[s] = tmpStages[s];
        if (tmpPredict[s].size() != classifier->alphaCount)
        {
            cout << tmpPredict[s].size() << endl;
            ++errors;
        }
        float * dstAlpha = classifier->alpha + classifier->alphaCount * s;

        copy(tmpPredict[s].begin(), tmpPredict[s].end(), dstAlpha);
    }

    if (errors)
    {
        cerr << "Cannot load the classifier (" << errors << " errors occured)." << endl;
        releaseClassifier(&classifier);
        return;
    }
}



/// Strings with types of classifiers (Indexed by ClassifierType).
const char *const classifierTypeStrings[] = {
    "UNKNOWN",
    "LRD",
    "LRP",
    "LBP",
    "HAAR"
};

// EXPORT STUFF

void exportClassifierHeader(TClassifier * c, ostream & str, const char * name)
{
    assert(sizeof(classifierTypeStrings) == (size_t)numClassifierTypes*sizeof(*classifierTypeStrings));

    str << "#ifndef _"<< name << "_\n#define _" << name << "_\n\n";

    str << "#include <classifier.h>\n\n";

    str << "#define STAGE_COUNT " << c->stageCount << "\n";
    str << "#define ALPHA_COUNT " << c->alphaCount << "\n\n";

    switch (c->tp)
    {
    case LRD:
    case LRP:
    case LBP:
        {
            str << "TStage _stages_" << name << "[STAGE_COUNT] = {\n";
            for (unsigned s = 0; s < c->stageCount; ++s)
            {
                TStage & stg = ((TStage*)(c->stage))[s];
                str << "{ " <<
                    stg.x << ", " << stg.y << ", " <<
                    stg.w << ", " << stg.h << ", " <<
                    int(stg.A) << ", " << int(stg.B) << ", " <<
                    showpoint << stg.theta_b << "f, " <<
                    0 << ", " << // alpha
                    0 << ", " << 0 << "},\n"; // szType, posType
            }
            str << "};\n\n" << flush;
        }
        break;
    default:
        str << "// Unknown classifier type.\n\n";
        break;
    }

    str << "float _alphas_" << name << "[STAGE_COUNT * ALPHA_COUNT] = {\n" << flush;
    float * alpha = c->alpha;
    for (unsigned s = 0; s < c->stageCount; ++s, alpha+=c->alphaCount)
    {
        for (unsigned a = 0; a < c->alphaCount; ++a)
        {
            str << alpha[a] << "f, " << flush;
        }
        str << "\n";
    }
    str << "};\n\n";

    str << "TClassifier " << name << " = {\n";
    //str << "(ClassifierType)" << int(c->tp) << ", STAGE_COUNT, STAGE_COUNT, ALPHA_COUNT, " << 
    str << classifierTypeStrings[c->tp] << ", STAGE_COUNT, ALPHA_COUNT, " << 
        c->threshold << ", " << 
        c->width << ", " << c->height << ", " <<
        "(TStage*) _stages_" << name << ", (float*) _alphas_" << name << ", \n";
    str << "};\n\n" << flush;
    
    str << "#endif\n" << endl;
}


