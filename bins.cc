// /* Methods to load bin files, and match to grid points */
#include "bins.hpp"
#include "globals.hpp"


/* Method to load vector of triangle configurations
   from stored .verts file
   Use bin_maker scripts to generate valid .verts files

    triangle_set:
        list of primary points
        for each primary point, list of secondary points

    verts file format must be:
        n_bins | 
        bin1_n_primary | 
        bin1_primary_pt1 | bin1_primary_pt1_n_secondary | bin1_primary_pt1_secondary_pt1 | bin1_primary_pt1_secondary_pt2 | ...
        bin1_primary_pt2 | bin1_primary_pt2_n_secondary | bin1_primary_pt2_secondary_pt1 | bin1_primary_pt2_secondary_pt2 | ...
        bin2_n_primary | 
        bin2_primary_pt1 | bin2_primary_pt1_n_secondary | bin2_primary_pt1_secondary_pt1 | bin2_primary_pt1_secondary_pt2 | ...
        bin2_primary_pt2 | bin2_primary_pt2_n_secondary | bin2_primary_pt2_secondary_pt1 | bin2_primary_pt2_secondary_pt2 | ...
        ...
*/

vector<triangle_configs>* load_triangle_configs(const char *vertsfilename, float cell_size){

    // All configurations for all bins
    vector<triangle_configs>* all_configs = new vector <triangle_configs>();

    // Open raw binary file
    FILE* file = fopen(vertsfilename,"rb");
    if (file){

        // Get how many bins from first int
        int nbins = -1;
        if (fread(&nbins, sizeof(int), 1, file) != 1) exit(1);

        // Loop over each bins
        for ( int bin_index=0; bin_index<nbins; bin_index++){

            // Make vector for this bin
            triangle_configs* this_bin_configs = new triangle_configs();

            // Get the Rmin and Rmax values
            float rvalue = -1;
            if (fread(&rvalue, sizeof(float), 1, file) != 1) {
                // Likely that make_bins is still running, or terminated early last time
                // Just skip the remaining bins
                if (feof(file)) { break; } 
                printf("  Failed to load rmin");
                printf(", bin %d of %d\n",1+bin_index,nbins);
                exit(1);
            }
            this_bin_configs->rmin = rvalue;
            rvalue = -1;
            if (fread(&rvalue, sizeof(float), 1, file) != 1) {
                printf("  Failed to load rmax");
                printf(", bin %d of %d\n",1+bin_index,nbins);
                exit(1);
            }
            this_bin_configs->rmax = rvalue;

            // Prepare to get average for each triangle edge
            float R1_sum = 0; int R1_count = 0; // to primary point
            float R2_sum = 0; int R2_count = 0; // to seconary point
            float R3_sum = 0; int R3_count = 0; // beteen primary and secondary points

            // Get the number of primary points (ptB)
            int n_ptsB = -1;
            if (fread(&n_ptsB, sizeof(int), 1, file) != 1){
                printf("  Failed to load n_ptsB");
                printf(", bin %d of %d\n",1+bin_index,nbins);
                exit(1);
            }

            // Load for each primary point
            // printf(" Loading Bin #%d, with %d ptsB\n",bin_index,n_ptsB);
            for ( int ptB_i=0; ptB_i<n_ptsB; ptB_i++){

                // Load and make the ptB
                int ptB_ints[3];
                if (fread(ptB_ints, sizeof(int)*3, 1, file) != 1) { 
                    printf("  Failed to load ptB");
                    printf(", bin %d of %d",1+bin_index,nbins);
                    printf(", ptB (%d of %d)\n",1+ptB_i,n_ptsB);
                    exit(1);
                }
                point ptB(ptB_ints[0], ptB_ints[1], ptB_ints[2]);

                // Store matches in the struct
                struct triangle_set this_set(ptB);
                vector<point> &ptsC = this_set.ptsC;

                // Find out how many secondary points
                int n_ptsC = -1;
                if (fread(&n_ptsC, sizeof(int), 1, file) != 1){
                    printf("  Failed to load n_ptsC");
                    printf(", bin %d of %d",1+bin_index,nbins);
                    printf(", ptB (%d of %d)\n",1+ptB_i,n_ptsB);
                    exit(1);
                }

                // Add to the R1 sum
                R1_sum += mag(ptB) * n_ptsC;
                R1_count += n_ptsC;

                // Load all secondary points
                for ( int ptC_i=0; ptC_i<n_ptsC/3; ptC_i++){

                    int ptC_ints[3];
                    if (fread(ptC_ints, sizeof(int)*3, 1, file) != 1) { 
                        printf("  Failed to load ptC_ints");
                        printf(", bin %d of %d",1+bin_index,nbins);
                        printf(", ptB (%d of %d)",1+ptB_i,n_ptsB);
                        printf(", ptc (%d of %d)\n",1+ptC_i,n_ptsC);
                        exit(1); 
                    }
                    point ptC(ptC_ints[0], ptC_ints[1], ptC_ints[2]);

                    // Add to the list
                    ptsC.push_back(ptC);
                    // cout << " ptsC size " << this_set.ptsC.size() << '\n'; 

                    // Add to the R2 and R3 sums
                    R2_sum += mag(ptC);
                    R2_count += 1;
                    R3_sum += sqrt(distSq(ptB,ptC));
                    R3_count += 1;

                } // end for over ptC_i

                // Add this set to the set for the full bin
                this_bin_configs->add_set(this_set);

            } // end for over ptB_i

            // Take the average of the sums and store in the struct
            this_bin_configs->r1avg = (R1_sum / float(R1_count)) * cell_size;
            this_bin_configs->r2avg = (R2_sum / float(R2_count)) * cell_size;
            this_bin_configs->r3avg = (R3_sum / float(R3_count)) * cell_size;

            // Add this bin to the main set
            all_configs->push_back(*this_bin_configs);

        } // end for over bin_index

    } else {
        printf("File does not exist: '%s'\n", vertsfilename);
        exit(1);
    }

    // Return the loaded matches
    return all_configs;
}


// Measured running speeds on different architectures
// Get likely node from the number of threads
// Can update these easily
double runSpeed(){
           if (global_nthreads==24){ return 7.50E+7; // donatello
    } else if (global_nthreads==16){ return 3.00E+7; // cores16
    } else if (global_nthreads==12){ return 9.00E+6; // cores12
    } else {                         return 5.00E+6; // Macbook
    }
}

double summary_and_time_per_file(vector<triangle_configs>* selectionFunction, int Nres3, bool verbose){

    // Keep track of TOTAL number of configs to run
    size_t total_configs = 0;

    // Print summary of each bin 
    for (int bin_index=0; bin_index<(int)selectionFunction->size(); bin_index++){

        // Add all matches for this bin
        triangle_configs& this_bin = selectionFunction->at(bin_index);
        size_t matches_in_bin = this_bin.size();
        total_configs += matches_in_bin;

        // Extract r values for bin (average and requested range )
        // And number of matches in bin (struct sums all matches)
        float r1_avg = this_bin.r1avg;
        float r2_avg = this_bin.r2avg;
        float r3_avg = this_bin.r3avg;

        if (verbose){
            // Print the details of this bin
            printf("    Bin %d:\t",bin_index);
            printf("r_mean = {%.2f, %.2f, %.2f}", r1_avg, r2_avg, r3_avg);
            printf(" (%.1f < r < %.1f)\t", this_bin.rmin, this_bin.rmax);
            printf("%ld triangles\n",matches_in_bin);
        }
    }

    // Print how many calculations
    if (verbose) {
        printf("\n");    
    }
    printf("   %ld triangles per sampled lattice point\n",total_configs);

    // How many actual calculations
    double n_calculations = sample_fraction * total_configs * Nres3;
    double time_per_file = n_calculations/runSpeed();
    time_per_file /= sqrt(float(global_nthreads));

    printf("   Expected time : %.0f seconds for each file\n",time_per_file);
    if (time_per_file>60) {     printf("                = %.2f mins\n",time_per_file/60.0); } 
    if (time_per_file>3600) {     printf("                = %.2f hours\n",time_per_file/3600.0); } 
    if (time_per_file>3600*12.0) {     printf("                = %.2f days\n",time_per_file/(3600*24.0)); } 

    return time_per_file;
}

