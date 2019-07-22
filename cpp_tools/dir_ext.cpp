#include "dir_ext.hpp"

bool dir_exists(string pathname){
    struct stat info;
    if( stat( pathname.c_str(), &info ) != 0 ) {
        // fprintf(stderr," ERROR: cannot access %s\n", pathname.c_str() );
        return false;
    } else if( info.st_mode & S_IFDIR ){
        return true;
    } else {
        return false;
    }
}

std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg(); 
}