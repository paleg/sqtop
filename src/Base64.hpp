
//*********************************************************************
//* C_Base64 - a simple base64 encoder and decoder.
//*
//*     Copyright (c) 1999, Bob Withers - bwit@pobox.com
//*
//* This code may be freely used for any purpose, either personal
//* or commercial, provided the authors copyright notice remains
//* intact.
//*********************************************************************

#ifndef _Base64_hpp
#define _Base64_hpp

#include <string>

namespace Base64
{
	extern std::string Encode(const std::string &data);
	extern std::string Decode(const std::string &data);
}


#endif // _Base64_hpp
