#ifndef SRC_INCLUDES_UTILS_H_
#define SRC_INCLUDES_UTILS_H_

#include "shared.h"

class Utils final {
    public:
        static std::string convertCcharToString(const char * cString) {
            std::stringstream cStringstream;

            cStringstream << cString;

            return cStringstream.str();
        }
};



#endif
