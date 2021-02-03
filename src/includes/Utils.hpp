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

        static bool readFile(const std::string & filename, std::vector<char> & buffer) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                std::cerr << "Failed To Open File" << std::endl;
                return false;
            }

            size_t fileSize = (size_t) file.tellg();
            buffer.resize(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();

            return true;
         }

};



#endif
