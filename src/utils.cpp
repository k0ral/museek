/**
 * \file utils.cpp
 * \brief Utility functions implementation.
 */

#include <cmath>
#include <ctime>
#include <sstream>

#include "ANN.h"

#include "constants.h"
#include "utils.h"

using namespace std;

vector<string> tokenize(const string& str, const string& delimiters) {
    vector<string> tokens;
    
    // Skip delimiters at beginning
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);

    // Find first "non-delimiter"
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos) {
        tokens.push_back(str.substr(lastPos, pos - lastPos));   // Found a token, add it to the vector.
        
        lastPos = str.find_first_not_of(delimiters, pos);       // Skip delimiters.  Note the "not_of"
        pos     = str.find_first_of(delimiters, lastPos);       // Find next "non-delimiter"
    }

    return tokens;
}


string findAndReplace(const string source, string from, string to) {
    string result(source);

    size_t j;
    for ( ; (j = result.find(from)) != string::npos; )
        result.replace(j, from.length(), to);

    return result;
}


wstring widen(const string& str ) {
    wostringstream wstm ;
    const ctype<wchar_t>& ctfacet =
    use_facet< ctype<wchar_t> >( wstm.getloc() ) ;
    for( size_t i=0 ; i<str.size() ; ++i )
    wstm << ctfacet.widen( str[i] ) ;
    return wstm.str() ;
}
 

string narrow(const wstring& str) {
    ostringstream stm ;
    const ctype<char>& ctfacet =
    use_facet< ctype<char> >( stm.getloc() ) ;
    for( size_t i=0 ; i<str.size() ; ++i )
    stm << ctfacet.narrow( str[i], 0 ) ;
    return stm.str() ;
}


///
ANNpoint randomPointOnSphere(unsigned short dimensions, double r) {
    srand(time(NULL));

    ANNpoint        result = new ANNcoord[dimensions];
    double          radius(0);
    unsigned short  i(0);

    // Generate n normally distributed coordinates
    while (i < dimensions) {
        result[i]   = sqrt(-2*log(rand()/(double)RAND_MAX))*cos(2*PI*rand()/(double)RAND_MAX);
        radius     += pow(result[i], 2);
        i++;
    }

    // Adjust radius
    radius  = sqrt(radius);
    i       = 0;
    while (i < dimensions) {
        result[i] *= r/radius;
        i++;
    }

    return result;
}


string URLEncode(const string& text) {
    string          escaped("");
    unsigned int    maximum(text.length());
    unsigned int    i(0);

    while (i < maximum) {
        if ( 
            (48 <= text[i] && text[i] <= 57) ||     // 0-9
            (65 <= text[i] && text[i] <= 90) ||     // abc...xyz
            (97 <= text[i] && text[i] <= 122) ||    // ABC...XYZ
            (text[i] =='~' || text[i] == '!' ||
            text[i] == '*' || text[i] == '(' || 
            text[i] == ')' || text[i] == '\'')
        ) {
            escaped.append(&text[i], 1);
        } else {
            escaped.append("%");
            escaped.append(char2hex(text[i]));
        }

        i++;
    }

    return escaped;
}

string char2hex(char dec) {
    char dig1 = (dec&0xF0)>>4;
    char dig2 = (dec&0x0F);
    if ( 0<= dig1 && dig1<= 9) dig1+=48;    // 0,48inascii
    if (10<= dig1 && dig1<=15) dig1+=97-10; // a,97inascii
    if ( 0<= dig2 && dig2<= 9) dig2+=48;
    if (10<= dig2 && dig2<=15) dig2+=97-10;

    string r;
    r.append( &dig1, 1);
    r.append( &dig2, 1);
    return r;
}