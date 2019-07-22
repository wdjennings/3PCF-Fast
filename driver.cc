/*************************************************************
  Calculates the 3-point correlation of data boxes
*************************************************************/

#include "corr3.hpp"
#include "globals.hpp"

long int jackknife_N = 1;
double sample_fraction = 0.01;
int global_nthreads = 1;

// Get the number of threads
int omp_thread_count() {
    int n = 0;
    #pragma omp parallel reduction(+:n)
    n += 1;
    return n;
}

//  Main Method
int main( int argc, const char * argv[] ){

    cout << "\n ------------------------------------------------------------------\n";

    // Feedback start time of script
    cout << " Started " << pretty_time() << "\n";

   // Start script timer
    startTimer();

    // Command Line arguments parser (short, long, nargs, optional)
    ArgumentParser parser;
    parser.addArgument("-d", "--directory", 1, true);
    parser.addArgument("-i", "--inputfilename", 1, true);
    parser.addArgument("-o", "--outputfilename", 1, true);

    parser.addArgument("-e", "--estimator", 1, true);
    parser.addArgument("-y", "--normalisation", 1, true);

    parser.addArgument("-b", "--vertsfilename", 1, false);    

    parser.addArgument("-s", "--sample_fraction", 1, true);

    parser.addArgument("-N", "--resolution", 1, true);
    parser.addArgument("-L", "--length", 1, true);

    parser.parse(argc, argv);

    // Set number of threads
    global_nthreads = omp_thread_count();
    cout << "  global_nthreads=" << global_nthreads << "\n";
    omp_set_num_threads(global_nthreads);

    // Set sample fraction
    string sample_fraction_st = parser.retrieve<string>("sample_fraction");
    if (sample_fraction_st.length()>0){
        sample_fraction = atof(sample_fraction_st.c_str());
    }

    // If not sample fraction, quit with error
    if (sample_fraction<=0.0){
        cout << "  invalid sample_fraction " << sample_fraction << "\n";
        cout << "\n ------------------------------------------------------------------\n\n";
        exit(2);
    } else if (sample_fraction>1.0){
        cout << "  reducing sample_fraction from " << sample_fraction;
        sample_fraction = 1.0;
        cout << " to " << sample_fraction << "\n";
    } else {
        cout << "  sample_fraction=" << sample_fraction << "\n";
    }

    // Bin filename from command line args
    string vertsfilename = parser.retrieve<string>("vertsfilename");
    string directory = parser.retrieve<string>("directory");

    // Choose estimator (converts DD, DR etc into corr3)
    string estimatorSt = parser.retrieve<string>("estimator");
    estimatorFunctionType estimator = estimatorPlain;                                                   // Default estimator is Landy-Szalay
    if (estimatorSt=="" || estimatorSt=="plain" || estimatorSt=="Plain"){
        estimatorSt = "estimatorPlain";
        estimator = estimatorPlain;
    } else if (estimatorSt=="LS"){
        estimatorSt = "estimatorLandaySzalay";
        estimator = estimatorPlain;
    } else {
        cout << "  ERROR: unrecognised estimator: '" << estimatorSt << "'\n";
        exit(1);
    }

    // Choose normalisation type
    string normalisationSt = parser.retrieve<string>("normalisation");
    if ( normalisationSt=="" || normalisationSt=="normOne" ){
        normalisationSt = "normOne";
        cout << "  Will normalise to ( T / <T> ) \n";
        
    } else if (normalisationSt=="normOverdensity"){
        cout << "  Will normalise to ( T - <T> ) / <T> \n";
    } else {
        cout << "  ERROR: unrecognised normalisation: '" << normalisationSt << "'\n";
        exit(1);
    }

    // Add bin filename to output filenam
    char prefix[500];
    sprintf(prefix,"%s_%s_%s_sample%.3f_%s_","corr3", estimatorSt.c_str(), descriptive(vertsfilename).c_str(), sample_fraction, normalisationSt.c_str());

    // Get input -> output filenames
    vector< pair<string,string> > *file_pairs = get_loop_filenames(parser, "corr3", prefix);
    if (file_pairs==NULL){
        cout << "  Could not get filenames.\n  Method Terminates.\n";
    }
    if (file_pairs->size()==0){
        cout << "  No files found.\n  Method Terminates.\n";        
    }

    // Extract Nres and L from first filename, if not given
    int Nres = -1; 
    float L = -1.0;
    if (parser.retrieve<std::string>("resolution").length()>0){
        Nres = atoi(parser.retrieve<std::string>("resolution").c_str());
    }
    if (parser.retrieve<std::string>("length").length()>0){
        L = atof(parser.retrieve<std::string>("length").c_str());
    }

    // Find the first filename with valid
    int status = 0;
    for (size_t file_i=0; file_i<file_pairs->size(); file_i++){
        string inputfilename = file_pairs->at(file_i).first;
        status = get_filename_N_L(inputfilename, Nres, L);
        if (status!=_SUCCESS){
            continue;
        } else {
            break;
        }
    }

    // Failed to find N and L from any file
    if (status!=_SUCCESS){ 
        cout << "  Could not determine N and L from filename(s).\n  Method Terminates.\n";        
        exit(1);
    }

    // Store Nres powers and cell size
    int Nres2 = Nres*Nres;
    int Nres3 = Nres*Nres*Nres;
    float cell_size = float(L) / float(Nres);

    // NEW METHOD: load explicit triangle vertices
    cout << "  Loading triangle vertices... ";
    vector< triangle_configs > *selectionFunction = 
    load_triangle_configs(vertsfilename.c_str(), cell_size);
    cout << "done\n";

    // Print summary of bins
    double time_per_file = summary_and_time_per_file(selectionFunction, Nres3, false);

    // Run the statistics for every file
    cout << "\n  Running corr3 for " << file_pairs->size() << " files\n";
    for (size_t file_i=0; file_i<file_pairs->size(); file_i++){
        string inputfilename = file_pairs->at(file_i).first;
        string outputfilename = file_pairs->at(file_i).second;
        cout << "    Running for " << basename(inputfilename) << "\n";
        cout << "      Expected end " << pretty_time(time_per_file) << "\n";

        // Make sure the correct N and L
        int this_Nres = -1; float this_L = -1.0;
        if (get_filename_N_L(inputfilename, this_Nres, this_L)!=_SUCCESS){
            cout << "    Failed to get N and L for " << inputfilename << "\n";
            exit(1);
        } else {
            if ( Nres!=this_Nres || L!=this_L){
                cout << "    Different N/L for " << inputfilename << "\n";
                continue;
            }
        }

        // Prepare float box to load from file
        float box[Nres3];

        // Check file size -- make sure correct for double or float
        std::ifstream::pos_type size = filesize(inputfilename.c_str());
        float element_bytes = float(size)/float(Nres3);

        // Doubles data -- for python generated data
        if (element_bytes==8.0){
            double* boxD = load_double_data(inputfilename, Nres);
            for (long int i=0; i<Nres3; i++) { 
                box[i] = float(boxD[i]);
            }
            delete boxD;

        // Float data -- simfast / 21cmfast?
        } else if (element_bytes==4.0) {
            float *new_box = load_float_data(inputfilename, Nres);
            for (long int i=0; i<Nres3; i++) { 
                box[i] = float(new_box[i]);
            }
            delete new_box;

        // Dont know this type
        } else {
            cout << "  ERROR: unknown data type (not float or double)\n";
            cout << "  Mehod terminates.\n";
            exit(1);            
        }

        // Get data mean
        // cout << "  Taking average... ";
        long double sumdata = 0;
        long double sumdata_sq = 0;
        for (long int i=0; i<Nres3; i++) { 
            sumdata += box[i]; 
            sumdata_sq += box[i] * box[i];
        }
        long double ave = sumdata / double(Nres3);
        cout << "      Data mean is " << ave << "\n";

        // Normalise to ( T /<T> ), random field is 1.0 everywhere
        if ( normalisationSt=="" || normalisationSt=="normOne" ){

            if (ave==0.0){ 
                if (sumdata_sq==0.0){
                    cout << "  WARNING: box is all zeros; will give dummy output"; 
                } else {
                    cout << "  ERROR: box ave = 0, but not all zeros, forbiddged for normOne normalisation";                     
                    continue;
                }
            } else {
                for (long int i=0; i<Nres3; i++) { box[i] = (box[i]/ave); }
            }

        // Normalise to ( T - <T> ) / <T>
        } else if (normalisationSt=="normOverdensity"){

            if (ave==0.0){ 
                if (sumdata_sq==0.0){
                    cout << "  WARNING: box is all zeros; will give dummy output"; 
                } else {
                    cout << "  ERROR: box ave = 0, but not all zeros, forbiddged for normOne normalisation";                     
                    continue;
                }
            } else {
                for (long int i=0; i<Nres3; i++) { box[i] = (box[i]-ave)/ave; }
            }

        } else {
            cout << "  ERROR: unrecognised normalisation: '" << normalisationSt << "'\n";
            exit(1);
        }

        // Run the correlation
        cout << "      Correlating... ";
        vector<statistics_with_jk> *results = run_correlation(box, box, box, selectionFunction, Nres);
        cout << "  Done at " << currentTimeTaken() << '\n';

        // Save to file
        cout << "      Saving... ";
        save(results, estimator, selectionFunction, outputfilename.c_str());
        cout << "  Done at " << currentTimeTaken() << '\n';

    }

    cout << " Finished all files at " << pretty_time() << "\n";
    cout << " ------------------------------------------------------------------\n";

    // Successful run
    return 0;
}