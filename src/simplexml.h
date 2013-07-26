#ifndef __SimpleXML_h__
#define __SimpleXML_h__ 1

#include <fstream>
#include <stack> 
#include <libxml/tree.h>

extern xmlNodePtr getNode( std::string nodeName, xmlNodePtr);

extern xmlNodePtr getNextNode( std::string nodeName, xmlNodePtr);

extern bool getAttr( int &dest, std::string atrName, xmlNodePtr);
extern bool getAttr( unsigned &dest, std::string atrName, xmlNodePtr);
extern bool getAttr( double &dest, std::string atrName, xmlNodePtr);
extern bool getAttr( float &dest, std::string atrName, xmlNodePtr);
extern bool getAttr( std::string &dest, std::string atrName, xmlNodePtr);
extern bool getAttr( bool &dest, std::string atrName, xmlNodePtr); 
extern std::string getAttr( std::string atrName, xmlNodePtr);
extern xmlAttrPtr getAttrNode( std::string atrName, xmlNodePtr);

extern bool removeAttr( xmlNodePtr, const std::string & attrName);

/** For easy to use xml output*/
class TXMLOutput{
    xmlNodePtr rootNode; //
    xmlNodePtr cur;      // current node
    xmlBufferPtr buffer;
    
    TXMLOutput( const TXMLOutput &){}
    
    TXMLOutput &operator =( const TXMLOutput &);
    
public:
    TXMLOutput( ); 

    // adds new element into the xml tree. This element becomes active 
    void addElement( const std::string &elementName); 
    void addElement( xmlNodePtr element);
    
    void addAttribute( const std::string &name, const std::string &value);
    void addAttribute( const std::string &name, const double value);
    void addAttribute( const std::string &name, const int value);
    void addAttribute( const std::string &name, const unsigned value);
    void addAttribute( const std::string &name, const bool value);
    
    // Parent of current active element becomes active
    void endElement();
    
    // Adds text string into current code after last child
    void addText( const std::string &);

    // returns serialized text of xml tree contained in the object
    // returns pointer to internal buffer - do not try to dealocate it 
    // returned poitner becomes invalid after new call of getText
    char * getText( int indentDepth);


    // returns serialized text of xml tree contained the node
    // returns pointer to internal buffer - do not try to dealocate it 
    // returned poitner becomes invalid after new call of getText
    char * getText( int indentDepth, xmlNodePtr node);
    
    xmlNodePtr getRootNode();
    
    ~TXMLOutput();
};

#endif

