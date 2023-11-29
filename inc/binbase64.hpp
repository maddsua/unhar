//  Copied from stackoverflow
//	No warranties are given, etc...
// This file is a component of the HAR Media Extractor

#ifndef _BASE64_H_
#define _BASE64_H_

#include <vector>
#include <string>

std::string base64_encode(unsigned char const* buf, unsigned int bufLen);
std::vector <unsigned char> base64_decode(std::string const&);

#endif
