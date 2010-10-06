#ifndef UTILS_H
    #define UTILS_H

    /**
     * \file utils.h
     * \brief Utility functions headers.
     */

    #include <string>
    #include <vector>

    std::vector<std::string>    tokenize(const std::string&, const std::string& delimiters = " ");
    std::string                 findAndReplace(const std::string, std::string, std::string);
    std::wstring                widen(const std::string&);
    std::string                 narrow(const std::wstring&);
    ANNpoint                    randomPointOnSphere(unsigned short, double);
    std::string                 URLEncode(const std::string&);
    std::string                 char2hex(char);
#endif