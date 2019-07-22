/************************************************************
  Implementation for 3-point correlation
*************************************************************/

#include "corr3.hpp"                
#include "globals.hpp"
#include <iomanip>

// Periodic condition for integer
int wrap_int(int value, int Nres){
    if (value<0){
        return wrap_int(value + Nres, Nres);
    } else if (value >= Nres){
        return wrap_int(value - Nres, Nres);
    } else {
        return value;
    }
}

// Structure for corr3 statistics
// DDD, DDR, DRR, RRR
struct statistics{ 
    double DDD, DDR, DRR, RRR; 

    statistics() : DDD(0.0), DDR(0.0), DRR(0.0), RRR(0.0) {};

    statistics(double _DDD, double _DDR, double _DRR, double _RRR) : 
                DDD(_DDD), DDR(_DDR), DRR(_DRR), RRR(_RRR) { };

};

// corr3 statistics with jackknifed values
struct statistics_with_jk{
    statistics stats;
    vector<statistics> stats_JK;
    statistics_with_jk() : stats_JK(jackknife_N) {}
};


// statistics struct addition assignment operator
statistics& operator+=(statistics& sa, statistics& sb){ 
    sa.DDD += sb.DDD; 
    sa.DDR += sb.DDR; 
    sa.DRR += sb.DRR; 
    sa.RRR += sb.RRR; 
    return sa; 
}

// statistics struct addition operator
statistics operator+(statistics& sa, statistics& sb){ 
    statistics newStats(
           sa.DDD+sb.DDD, 
            sa.DDR+sb.DDR, 
            sa.DRR+sb.DRR, 
            sa.RRR+sb.RRR 
        ); 
    return newStats; 
}

// statistics struct subtractions operator
statistics operator-(statistics& sa, statistics& sb){ 
    statistics newStats
        ( sa.DDD-sb.DDD, 
            sa.DDR-sb.DDR, 
            sa.DRR-sb.DRR, 
            sa.RRR-sb.RRR 
        ); 
    return newStats; 
}


// Estimator Functions: {DDD, DDR, DRR, RRR} -> correlation value
// Simple estimator (DDD/RRR)
double estimatorPlain(statistics& s){ 
    return (s.DDD/s.RRR)-1.0; 
}
// Landay-Szalay estimator
double estimatorLS(statistics& s) { 
    return ((s.DDD-(3*s.DDR)+(3*s.DRR))/s.RRR) - 1.0; 
}

// Main correlation method
vector<statistics_with_jk>* 
run_correlation(const float* box1, const float* box2, const float* box3, 
                vector< triangle_configs > *selectionFunction, 
                int Nres){

    // How many bins are there?
    int n_bins = selectionFunction->size();

    // Powers of Nres
    int Nres2 = Nres*Nres;
    int Nres3 = Nres*Nres*Nres;

    // Seed random number from time
    srand (time(NULL));
    // cout << " Test random number is " << (rand()%Nres3) << '\n';

    // Subsampling integer 
    int sample_fraction_int = int(sample_fraction * Nres3);

    // Make the results, vector of the statistics for each radial bin
    vector<statistics_with_jk> *results = new vector<statistics_with_jk>(n_bins);   

    // Get spread of data -- if no spread, return zeros results
    long double min = *std::min_element(box1,box1+Nres3);
    long double max = *std::max_element(box1,box1+Nres3);
    long double range = max - min;
    cout << "      Data min is " << min << "\n";
    cout << "      Data max is " << max << "\n";
    cout << "      Data range is " << range << "\n";
    if (range==0.0){
        cout << "      Zero spread in data, returning zeros results\n";
        return results;
    }

    // Make sure we have defined the number of threads
    omp_set_num_threads(global_nthreads);    

    // Split the data vector into jackknife_N parts
    // Traces z first, then y, then x, 
    // So not split into regular pieces
    // (e.g. jackknife_N=8 does NOT split into octants)
    signed long int jk_length = (Nres*Nres*Nres) / jackknife_N;

    // Start threading section
    // printf("\n    Starting at %s..",currentTimeTaken().c_str());
    // printf("\n    with %d threads..",global_nthreads);

    #pragma omp parallel
    {

        // Each thread gets its own private statistics and statistics_JK array
        // JK vector goes in .at(JK_index).at(bin_i) order, so that jk index can be got outside of the bins
        vector< statistics > results_pvt(n_bins);
        vector< vector< statistics > >  results_jk_pvt(jackknife_N, vector<statistics>(n_bins));

        // REJECTION sample
        // Run over ALL data indices, and reject or accept each
        // Probability for accept depends on the sample_fraction 
        #pragma omp for
        for ( signed long int i=0; i<Nres3; i++ ){
            if (sample_fraction!=1.0){                
                if ( (rand()%Nres3) >= sample_fraction_int ){ continue; }
            }

        // RANDOM ints
        // Run over the requested number of samples, generating random index each time
        // Tests indicate this is slower -- but seems like should be faster??!
        // #pragma omp for
        // for ( int sample_i=0; sample_i<sample_fraction_int; sample_i++ ){
        //     signed long int i = rand()%Nres3;

            // Get first data point value
            const float data1 = box1[i];

            // Get jackknife section and which bin
            const int jk_index1 = (int)floor(i / jk_length);
            vector< statistics >& statistics_for_bins_jk = results_jk_pvt.at(jk_index1);

            // Get location of data point
            const int x = (int)(i/Nres2);
            const int y = (int)( (i % Nres2) / Nres);
            const int z = (i % Nres);

            // Loop over all triangle bins from this data point
            for (int bin_i = 0; bin_i < n_bins; bin_i++){

                // printf("\n **** Bin %d ***** \n", bin_i);

                // Get the statistics (DDD etc) for this bin
                statistics& statistics_for_bin       = results_pvt.at(bin_i);
                statistics& statistics_for_bin_JK1   = statistics_for_bins_jk.at(bin_i);

                // The number of selection function elements used by the first point
                int radial_bin_single_matchsUsedByPixel1 = 0;
                double DDD_fromPixel1 = 0, DDR_fromPixel1 = 0; 
  
                // Get the triangle vertices list for primary points
                triangle_configs &triangles_in_bin = selectionFunction->at(bin_i);

                // Loop over all primary points for triangles (ptB)
                for (int ptB_i=0; ptB_i<(int)triangles_in_bin.sets.size(); ptB_i++){
                    
                    // printf("\n    ---- PointB %d ----- \n", ptB_i);

                    // Get the location of the primary point
                    point ptB = triangles_in_bin.at(ptB_i).ptB;
                    const int x2 = wrap_int(x + ptB.x, Nres);
                    const int y2 = wrap_int(y + ptB.y, Nres);
                    const int z2 = wrap_int(z + ptB.z, Nres);
                    const signed long int i2 = (x2*Nres2) + (y2*Nres) + z2;

                    // Get the data value and which jackknife bin
                    const float data2 = box2[i2];
                    const int jk_index2 = (int)floor(i2 / jk_length);

                    // Store data1*data2 for later
                    const double mult12 = data1 * data2;

                    // The number of selection function elements used by the second point
                    int radial_bin_single_matchsUsedByPixels12 = 0;
                    double DDD_fromPixel2 = 0;
                    
                    // Get the selection function element for this node
                    vector<point>& ptsC_for_radial_bin = triangles_in_bin.at(ptB_i).ptsC;

                    // Loop over the secondary points (ptC)
                    for (int ptC_it=0; ptC_it<(int)ptsC_for_radial_bin.size(); ptC_it++){ 

                        // Get the location of the secondary point
                        point ptC = ptsC_for_radial_bin.at(ptC_it);
                        int x3 = wrap_int(x + ptC.x, Nres);
                        int y3 = wrap_int(y + ptC.y, Nres);
                        int z3 = wrap_int(z + ptC.z, Nres);
                        const signed long int i3 = (x3*Nres2) + (y3*Nres) + z3;

                        // Get the data array index and value
                        const float data3 = box3[i3];
                        const int jk_index3 = (int)floor(i3 / jk_length);

                        // Store data1 * data2 * data3
                        double mult123 = mult12*data3;

                        // Add to the DDD that pixel2 contributed to
                        DDD_fromPixel2 += mult123;

                        // The number of selection function elements used by the second point increases
                        radial_bin_single_matchsUsedByPixels12++;

                        // Third JK -- add to it if it is not the same as any other jk index
                        if (jk_index3!=jk_index2 and jk_index3!=jk_index1){
                            statistics& statistics_for_bin_JK3 = results_jk_pvt.at(jk_index3).at(bin_i);
                            statistics_for_bin_JK3.DDD += mult123;
                            statistics_for_bin_JK3.DDR += mult12;
                            statistics_for_bin_JK3.DRR += data1;
                            statistics_for_bin_JK3.RRR += 1.0;
                        }

                    } // endfor ptC_it (secondary point)

                    // All the DDD that Pixel2 contributed to, was also contribued by pixel1    
                    DDD_fromPixel1 += DDD_fromPixel2;

                    // DDR depends on mult12 and radial_bin_single_matchsUsedByPixels12
                    DDR_fromPixel1 += radial_bin_single_matchsUsedByPixels12 * mult12;

                    // Second JK
                    if ( jk_index2 != jk_index1 ){
                        statistics& statistics_for_bin_JK2 = results_jk_pvt.at(jk_index2).at(bin_i);
                        statistics_for_bin_JK2.DDD += DDD_fromPixel2;
                        statistics_for_bin_JK2.DDR += radial_bin_single_matchsUsedByPixels12 * mult12;
                        statistics_for_bin_JK2.DRR += radial_bin_single_matchsUsedByPixels12 * data1;
                        statistics_for_bin_JK2.RRR += radial_bin_single_matchsUsedByPixels12 * 1.0;
                    }

                    // The number of selection function elements used by the first point increases
                    radial_bin_single_matchsUsedByPixel1 += radial_bin_single_matchsUsedByPixels12;

                } // endfor second point


                // Get the final sums
                // DDD has been summed accumulatively
                // DDR has been summed accumulatively
                double DRR_inPixel1 = radial_bin_single_matchsUsedByPixel1 * data1;
                double RRR_inPixel1 = radial_bin_single_matchsUsedByPixel1 * 1.0;

                // Add the partial sums to the whole sums
                statistics_for_bin.DDD += DDD_fromPixel1;
                statistics_for_bin.DDR += DDR_fromPixel1;
                statistics_for_bin.DRR += DRR_inPixel1;
                statistics_for_bin.RRR += RRR_inPixel1;

                // Also add the partial sums to the first pixel's jackknife array
                // CANT just use statistics_for_bin.DDD etc, because these will generally already contain sums from other pixels
                statistics_for_bin_JK1.DDD += DDD_fromPixel1;
                statistics_for_bin_JK1.DRR += DRR_inPixel1;
                statistics_for_bin_JK1.DDR += DDR_fromPixel1;
                statistics_for_bin_JK1.RRR += RRR_inPixel1;
                
            } // endfor bin_i
        } // end omp for (over positions)


        // Sum the private arrays for each thread in critical section 
        // Can't be run in two threads at once to stop clashes
        #pragma omp critical
        {
            for (int bin_i=0; bin_i<n_bins; bin_i++ ){
                results->at(bin_i).stats += results_pvt.at(bin_i);                

                // DD_JK, DR_JK, RR_JK values start at DD, DR, RR values
                for (int jk_i=0; jk_i<jackknife_N; jk_i++){
                    results->at(bin_i).stats_JK.at(jk_i) += results_jk_pvt.at(jk_i).at(bin_i);
                }
            } // endfor bin_i
        } // end omp critical
    } //end omp parllel

    // All the stats_JK are subtracted from the total stats values
    for (int bin_i=0; bin_i<n_bins; bin_i++ ){
        for (int jk_index=0; jk_index<jackknife_N; jk_index++){
            results->at(bin_i).stats_JK.at(jk_index) = results->at(bin_i).stats - results->at(bin_i).stats_JK.at(jk_index);
        }
    } // endfor over bins, for storing jackknifed results

    // Return the 
    return results;
}


// Pad a double value to 22 chars
// char* pad(double value, int length=22){
//     char *s = new char(length);
//     sprintf(s, "%.16f", value);
//     return s;
// }

void save(vector<statistics_with_jk> *results, estimatorFunctionType estimator, vector< triangle_configs > *selectionFunction, const char *outputfilename){

    // Save the correlation results into the specified folder
    ofstream save_file_id(outputfilename);
    if (save_file_id){ 

        // Add header row
        save_file_id.fill(' ');
        save_file_id << setw(22) << left << "# R1_avg";
        save_file_id << '\t' << setw(23) << left << "R2_avg";
        save_file_id << '\t' << setw(23) << left << "R3_avg";
        save_file_id << '\t' << setw(23) << left << "corr3";
        save_file_id << '\t' << setw(23) << left << "Error";
        save_file_id << '\t' << setw(23) << left << "DDD";
        save_file_id << '\t' << setw(23) << left << "DDR";
        save_file_id << '\t' << setw(23) << left << "DRR";
        save_file_id << '\t' << setw(23) << left << "RRR";
        save_file_id << "\n";

        // Add row for each bin
        for (int bin_i=0; bin_i<(int)selectionFunction->size(); bin_i++){ 

            // Get bin for ranges
            triangle_configs& bin = selectionFunction->at(bin_i);

            // Extract this set of stats & jk values
            statistics_with_jk& this_radial_bin_stats = results->at(bin_i);

            // Get the estimator value from the DDD, DDR, DRR, RRR values
            double correlation = (*estimator)( this_radial_bin_stats.stats );

            // Error is given by ~ the variance of the individual estimators for each subset
            double JK_difference_sq_sum = 0.0;
            int jk_counter = 0;
            for (int jk_i=0; jk_i<jackknife_N; jk_i++){
                if (this_radial_bin_stats.stats.RRR!=0){
                    statistics thisJK_stat = this_radial_bin_stats.stats_JK.at(jk_i);
                    double estimator_JK = (*estimator)( thisJK_stat );
                    double JK_difference_sq = pow((estimator_JK - correlation),2.0);
                    JK_difference_sq_sum += JK_difference_sq;
                    jk_counter++;
                }
            }

            // Get the error from these sums
            double jk_counter_double = double(jk_counter);
            double error_sq = ((jk_counter_double - 1.0) / jk_counter_double) * JK_difference_sq_sum;
            double error = pow(error_sq,0.5);

            // Save the resulting corr value to file
            save_file_id.precision(16);
            save_file_id.setf(ios_base::scientific);
            save_file_id << setw(23) << bin.r1avg;
            save_file_id << '\t' << setw(23) << bin.r2avg;
            save_file_id << '\t' << setw(23) << bin.r3avg;
            save_file_id << '\t' << setw(23) << correlation;
            save_file_id << '\t' << setw(23) << error;
            save_file_id << '\t' << setw(23) << this_radial_bin_stats.stats.DDD;
            save_file_id << '\t' << setw(23) << this_radial_bin_stats.stats.DDR;
            save_file_id << '\t' << setw(23) << this_radial_bin_stats.stats.DRR;
            save_file_id << '\t' << setw(23) << this_radial_bin_stats.stats.RRR;
            save_file_id << '\n';
        }                  
        save_file_id.close();
    } else {
       printf("Could not open save file '%s'\n",outputfilename);                
    }     
}


