// bulk of the includes are located here.


#pragma once
#include <iostream>
#include <fstream>

//From boost we need lzma, filesys, and serialization
// response_rules parser needs tokenizer
//in C++11 Boost::threads is now in std
#include <filesystem>
#include <boost/tokenizer.hpp>
#include <boost/crc.hpp>

#include <boost/iostreams/filter/lzma.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/algorithm/string.hpp>




#include <bitsery/bitsery.h>
#include <bitsery/brief_syntax.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/string.h>


#include "split_serialization.h"


#include <map>

#include "helper.hpp"

using namespace Helper;





// see src/game/server/AI_responserules.cpp from source-sdk for reference implementation of response_rules parser

