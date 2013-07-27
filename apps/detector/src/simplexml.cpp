#include "simplexml.h"

#include <iostream>
#include <sstream>
#include <cstdio>

using namespace std;

template <class _T>
bool strConvert(const char * str, _T & value, bool check = true, std::ios_base & (*f)(std::ios_base&) = std::dec)
{
	if (!str)
	{
		value = _T();
		return false;
	}
	
	std::istringstream iss(str);
	char c;

	//!(!(iss >> f >> value) || (check && iss.get(c)));
	return (iss >> value) && !(check && iss.get(c));
}


bool removeAttr( xmlNodePtr node, const string &attrName){
    if( node != NULL){
        for( xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
            if( attrName == (char *) attr->name){     
                return (xmlRemoveProp( attr) == 0);
            }
        }
    }
    return false;
}

xmlNodePtr getNode( string nodeName, xmlNodePtr root){
    if( root != NULL) {
        for( xmlNodePtr node = root->children; node; node = node->next) {
            if( xmlStrcmp( node->name, BAD_CAST nodeName.data()) == 0){        
                return node;
            }
        }
    }
    return NULL;    
}

xmlNodePtr getNextNode( std::string nodeName, xmlNodePtr node){
    if( node != NULL) {
        for( ; node; node = node->next) {
            if( xmlStrcmp( node->name, BAD_CAST nodeName.data()) == 0){        
                return node;
            }
        }
    }
    return NULL;    
}


bool getAttr( int &dest, std::string atrName, xmlNodePtr node){
    if( node != NULL) {
        for( xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
            if( xmlStrcmp( attr->name, BAD_CAST atrName.data()) == 0){        
                if( attr->children != NULL){			
					return strConvert((const char*)attr->children->content, dest);
                } else {
                    return false;   
                }
            }
        }
    }
    return false;
}

bool getAttr( unsigned &dest, std::string atrName, xmlNodePtr node){
    if( node != NULL) {
        for( xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
            if( xmlStrcmp( attr->name, BAD_CAST atrName.data()) == 0){        
                if( attr->children != NULL){
					return strConvert((const char*)attr->children->content, dest);
                } else {
                    return false;   
                }
            }
        }
    }
    return false;
}
 
bool getAttr( float &dest, std::string atrName, xmlNodePtr node){
    if( node != NULL) {
        for( xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
            if( xmlStrcmp( attr->name, BAD_CAST atrName.data()) == 0){        
                if( attr->children != NULL){
                    return strConvert((const char*)attr->children->content, dest);
                } else {
                    return false;   
                }
            }
        } 
    }
    return false; 
}

bool getAttr( double &dest, std::string atrName, xmlNodePtr node){
    if( node != NULL) {
        for( xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
            if( xmlStrcmp( attr->name, BAD_CAST atrName.data()) == 0){        
                if( attr->children != NULL){

                    double result;
                    if( !strConvert((const char*)attr->children->content, result)) {
                        return false;
                    } else {
                        dest = result;
                        return true;
                    }
                } else {
                    return false;   
                }
            }
        }
    }
    return false;
}
    
bool getAttr( string &dest, std::string atrName, xmlNodePtr node){
    if( node != NULL) {
        for( xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
            if( xmlStrcmp( attr->name, BAD_CAST atrName.data()) == 0){        
                if( attr->children != NULL){
                    dest = (const char*)attr->children->content;
                    return true;
                } 
            }
        }
    }
    return false;
}

bool getAttr( bool &dest, std::string atrName, xmlNodePtr node){
    if( node != NULL) {
        for( xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
            if( xmlStrcmp( attr->name, BAD_CAST atrName.data()) == 0){        
                if( attr->children != NULL){
                    if( xmlStrcmp( attr->children->content, BAD_CAST "true") == 0){
                        dest = true;
                    } else if( xmlStrcmp( attr->children->content, BAD_CAST "false") == 0){
                        dest = false;
                    } else {
                        return false;
                    }
                    return true;
                } 
            }
        }
    }
    return false;
}

string getAttr( std::string atrName, xmlNodePtr node){
    if( node != NULL) {
        for( xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
            if( xmlStrcmp( attr->name, BAD_CAST atrName.data()) == 0){     
                if( attr->children != NULL){
                    return (const char*)attr->children->content;
                }
            }
        }
    }
    return "";
}

xmlAttrPtr getAttrNode(std::string atrName, xmlNodePtr node)
{
    if (node != NULL)
    {
        xmlAttrPtr attr;
        for ( attr = node->properties; attr; attr = attr->next)
        {
            if (xmlStrcmp(attr->name, BAD_CAST atrName.data()) == 0)
                return attr;
        }
    }
    return NULL;
}


TXMLOutput::TXMLOutput( ): rootNode( NULL), cur( NULL), buffer( xmlBufferCreate()){
    
}

TXMLOutput::~TXMLOutput( ){
    xmlBufferFree( buffer);
    if( rootNode){
        xmlFreeNode( rootNode);   
    }
}

void TXMLOutput::addElement( const string &elementName){
    if( rootNode == NULL){
        rootNode = cur = xmlNewNode( NULL, (const xmlChar *) elementName.data());
    } else {
        cur = xmlNewChild( cur, NULL, (const xmlChar *) elementName.data(), NULL);   
    }
}

void TXMLOutput::addElement( xmlNodePtr node)
{
    if( rootNode == NULL){
        rootNode = cur = xmlCopyNode( node, 1);
    } else {
        xmlAddChild( cur, xmlCopyNode( node, 1));
    }
}



void TXMLOutput::addAttribute( const string &name, const string &value){
    if( cur){
        xmlNewProp( cur, (const xmlChar *) name.data(), (const xmlChar *) value.data());        
    } 
}

void TXMLOutput::addAttribute( const string &name, const int value){
    if( cur){
        stringstream s;
        s << value;
        xmlNewProp( cur, (const xmlChar *) name.data(), (const xmlChar *) s.str().data());        
    }
}

void TXMLOutput::addAttribute( const string &name, const unsigned value){
    if( cur){
        stringstream s;
        s << value;
        xmlNewProp( cur, (const xmlChar *) name.data(), (const xmlChar *) s.str().data());        
    }
}

void TXMLOutput::addAttribute( const string &name, const double value){
    if( cur){
        stringstream s;
        s << value;
        xmlNewProp( cur, (const xmlChar *) name.data(), (const xmlChar *) s.str().data());        
    }
}

void TXMLOutput::addAttribute( const std::string &name, const bool value)
{
    if( cur){
        if( value){
            xmlNewProp( cur, (const xmlChar *) name.data(), (const xmlChar *) "true");        
        } else {
            xmlNewProp( cur, (const xmlChar *) name.data(), (const xmlChar *) "false");        
        }
    }
}


void TXMLOutput::endElement(){
    if( cur != rootNode){
        cur = cur->parent;
    }
}

void TXMLOutput::addText( const std::string &text){
    xmlNodeAddContent( cur, (const xmlChar *)text.data());
}

char * TXMLOutput::getText( int indentDepth){
    xmlBufferEmpty( buffer);
    
    xmlNodeDump( buffer, NULL, rootNode, indentDepth, 1);
    return (char *) buffer->content;
}
    
char * TXMLOutput::getText( int indentDepth, xmlNodePtr node){
    static char* empty = (char *)"";
    if( node != NULL){
        xmlBufferEmpty( buffer);
        
        xmlNodeDump( buffer, NULL, node, indentDepth, 1);
        return (char *) buffer->content;
    } else {
        return empty;
    }
}
    
xmlNodePtr TXMLOutput::getRootNode(){
    return rootNode;   
}
