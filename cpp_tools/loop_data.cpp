#include "loop_data.hpp"

vector< pair<string,string> >* get_loop_filenames(ArgumentParser parser, string output_folder, string output_prefix){
    /* Parse command line args from ArgumentParser object 
    	Get and return list of input->output filename pairs*/

	// Cast command line args into useful types
    string directory = parser.retrieve<std::string>("directory");
    string inputfilename = parser.retrieve<std::string>("inputfilename");
    string outputfilename = parser.retrieve<std::string>("outputfilename");

    // Determine if in folder or file mode
    bool using_folder = false;
    if (directory.length()>0){
        // cout << "Running in folder mode: " << directory << "\n";
        if (inputfilename.length()>0 or outputfilename.length()>0){
            cout << "  WARNING: -i and -o arguments were set but unused\n";
        }
        using_folder = true;
    } else if (inputfilename.length()>0){
        // cout << "Running in file mode: " << inputfilename << "\n";
        using_folder = false;
    } else {
        cout << parser.usage() << "\n";
        cout << "  ERROR: Use EITHER (-d) OR (-i) args for input filename(s)\n";
        cout << "  Method Terminates.\n";
        exit(1);
    }

    // Find intput->output filenames to run for
    vector< pair<string,string> > *file_pairs = new vector< pair<string,string> >();
    if (using_folder){

        // Get list of all valid .dat files in folder
        DIR* my_dir = opendir(directory.c_str());
        struct dirent * dp = NULL;
        while ((dp = readdir(my_dir)) != NULL){

        	// Convert to string
            string filename_st = dp->d_name;
            // cout << "  Found " << filename_st << "\n";

            // Ignore silently for folder and superfolder and non-dat files
            if (filename_st.compare(".")==0
                or filename_st.compare("..")==0
                or has_suffix(filename_st , ".dat.catalog")
                or !has_suffix(filename_st , ".dat")){
                continue;
            }

            // Create input filename and matching output filename
            string inputfilename = join(directory, filename_st);
            string outputfilename = add_filename_prefix(inputfilename, join(output_folder, output_prefix));

            // Add to list
            file_pairs->push_back(make_pair(inputfilename,outputfilename));
            // cout << "Input to: " << inputfilename << "\n";
            // cout << "Output to: " << outputfilename << "\n";

        }
       (void)closedir(my_dir);

        // Create the output folder (if any input files)
        if (file_pairs->size()>0){

            // mkdir /pk/ in the folder location
            string pk_folder = join(directory,output_folder);
            if (!dir_exists(pk_folder)){
                char command[500];
                sprintf(command, "mkdir %s", pk_folder.c_str());
                system(command);
            // } else {
            //     cout << "  Skipping mkdir (existing " << pk_folder << ")" << "\n";
            }
        } else {
            cout << "  ERROR: No files found in folder " << directory << "\n";
            cout << "  Method terminates.\n";
            exit(1);
        }


    } else {

        // Single file input -> output
        if (outputfilename.length()==0){
            outputfilename = add_filename_prefix(inputfilename, output_prefix);
        }
        file_pairs->push_back(make_pair(inputfilename,outputfilename));
        if (file_pairs->size()==0){
            cout << "  ERROR: No files found matching " << inputfilename << "\n";
            cout << "  Method terminates.\n";
        }
    }

    return file_pairs;
}

float round_float(float d){
  return floor(d + 0.5);
}

int get_filename_N_L(string filename, int &N, float &L){
    /* Set the resolution and box size from a filename.
	    Tries to determine N if not in filename */

    // Try to get N, if not set
    if (N<0){
        string Nst = get_part(filename, "N", '_');
        if (Nst.length()>0){
            N = atoi(Nst.c_str());
        } else {

            // Last resort -- try to determine N from the file size
            cout << "  FAILED to get N from filename.\n Method terminates\n";
            exit(1);

            // cout << "  WARNING: no resoution part (N) in input filename\n";            
            // cout << "  WARNING: manually determining resolution from file size...\n";            
            // std::ifstream mystream (filename.c_str(), std::ifstream::binary);
            // if (mystream){
            //     mystream.seekg (0, mystream.end);
            //     int float_size_bytes = 4;
            //     int filelength = mystream.tellg()/float_size_bytes;
            //     int N_ish = int(round_float(pow(float(filelength), 1.0/3.0)));
            //     if (N_ish*N_ish*N_ish==filelength){
            //         N = N_ish;
            //         cout << "  Resolution is " << N << "\n";
            //     } else {
            //         cout << "  Could not get exact resolution (type wrong?)\n";
            //         return _FAILURE;                  
            //     }
            // } else {
            //     cout << "  Could not determine N from file: " << filename << "\n";
            //     cout << "  Skipping...\n";
            //     return _FAILURE;
            // }
        }
    }

    // Try to get L, if not set
    if (L<0){
        string Lst = get_part(filename, "L", '_');
        if (Lst.length()>0){
            L = atof(Lst.c_str());
        } else {
            cout << "  WARNING: no length part (L) in input filename\n";
        }
    }

    // Successfull if both positive now
    if (N>0 and L>0){
        return _SUCCESS;
    } else {
        return _FAILURE;
    }
}

double* load_double_data(string inputfilename, int N){
    double *box = (double *)malloc(sizeof(double)*(N*N*N));
    FILE* fid = NULL;
    if((fid = fopen(inputfilename.c_str(),"rb"))==NULL) {
        cout << "  Error opening double data file: " << inputfilename << "\n";
        return NULL;
    } else {
        fread(box, sizeof(double), N*N*N, fid );  
        fclose(fid);
    }
    return box;
}

float* load_float_data(string inputfilename, int N){
    float *box = (float *)malloc(sizeof(float)*(N*N*N));
    FILE* fid = NULL;
    if((fid = fopen(inputfilename.c_str(),"rb"))==NULL) {
        cout << "  Error opening float data file: " << inputfilename << "\n";
        return NULL;
    } else {
        fread(box, sizeof(float), N*N*N, fid );  
        fclose(fid);
    }
    return box;
}