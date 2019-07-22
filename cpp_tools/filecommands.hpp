
#include <stdlib.h>
#include <stdio.h>

#include <string>
using std::string;

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <fstream>
using std::ifstream;


inline string onlyFilename(char *input){

    string filenameWithExt = input;
    string filenameOnly = filenameWithExt;

    size_t foundDot = filenameWithExt.find_last_of(".");    
    if (foundDot!=string::npos){ // if dot was found 
        filenameOnly = filenameWithExt.substr(0,foundDot);
    }

    return filenameOnly;
}

inline string onlyFilename(string input){

    string filenameWithExt = input;
    string filenameOnly = filenameWithExt;

    size_t foundDot = filenameWithExt.find_last_of(".");    
    if (foundDot!=string::npos){ // if dot was found 
        filenameOnly = filenameWithExt.substr(0,foundDot);
    }

    return filenameOnly;
}

inline bool fileexists (const std::string& name) {
    ifstream f(name.c_str());
    if (f.good()) {
        f.close();
        return true;
    } else {
        f.close();
        return false;
    }   
}

inline void ensureIsDirectory(string& results_folder){
    if (results_folder.size()!=0){ 
      if (results_folder.at(results_folder.size()-1)!='/') {
        results_folder = results_folder+'/'; 
      }
    }
}



inline void makeDirectory(string dr_str){
    // Make the directory, if it doesn't exist
    ensureIsDirectory(dr_str);

    DIR* dir;
    if((dir=opendir(dr_str.c_str()))==NULL) {
        printf("-> Directory '%s' doesn't exist -- creating it now... ",dr_str.c_str());
        if(mkdir(dr_str.c_str(),(S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))!=0) {
            printf("Error creating directory\n");
            exit(1);
        } else {
            printf("Directory was created\n");
        
        }
    }
}
