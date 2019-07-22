""" Create bins for corr3 
    Either make logspaced r-bins
    Or make logspaced k-bins and flip
"""
import numpy as np
from os.path import join, exists, split, splitext
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import pickle
from copy import copy
import struct

from tools.grid.matches import *
from tools.grid.rs_sq_fn import *
from tools.pretty_visuals.time_string import *
import argparse


# Get command line args
parser = argparse.ArgumentParser( description = 'Setting parameters')
parser.add_argument('-dr', help='Data directory', default='../verts/', nargs='?', type=str )
parser.add_argument('-N', help='Resolution', default=100, nargs='?', type=int )
parser.add_argument('-L', help='Box size', default=100.0, nargs='?', type=float )

group = parser.add_mutually_exclusive_group()
group.add_argument('-bin_width', help='Bin width (in physical units)', nargs='?', type=float )
group.add_argument('-bin_width_pix', help='Bin width (in pixels)', nargs='?', type=float )

parser.add_argument('-rmin', help='Minimum radius for bins', default=0.0, nargs='?', type=float )
parser.add_argument('-rmax', help='Maximum radius for bins', default=20.0, nargs='?', type=float )
parser.add_argument('-r3mult', help='Multiply factor for third radius', default=1.0, nargs='?', type=float )
parser.add_argument('-Ntri', help='Maximum number of triangles per bin', default=200, nargs='?', type=int )

parser.add_argument('-all','--all_triangles', help='Also calcualte ALL triangles before subsampling', action='store_true' )
parser.add_argument('-nobar', help='Hide the progress bar', action='store_true' )
parser.add_argument('-v', help='Verbosity', action='count', default=0 )
parser.add_argument('-nosave', help='Skip saving the final verts file', action='store_true' )

args =  parser.parse_args()

# Pixel size
pix = args.L/float(args.N)

# Bin width default
if args.bin_width is None:
    # Use pixel width, if given    
    if args.bin_width_pix is None:
        args.bin_width_pix = 1.0
    args.bin_width = args.bin_width_pix * pix


# Choose verts_fname descriptive part from res and L
descriptive = 'linear_N%d_L%.0f_bw%.2f' % (args.N, args.L, args.bin_width)

# Make range of bins in nyquist range (2pix -> full box)
min_value = max(2.0*pix, args.rmin)
max_value = min(args.L, args.rmax)

# Linspaced bins
max_value_plus_one = max_value+args.bin_width
bin_edges = np.arange(min_value, max_value_plus_one, args.bin_width)

# Turn into 2D array
bins = np.vstack([bin_edges[:-1], bin_edges[1:]]).T
n_bins = len(bins)

# Save the bins to .bin file?
# Rmin/rmax already in *.verts file ,so not needed
# np.savetxt(join(args.dr,descriptive+'.bin'),bins)

# Make sure some bins found
if n_bins==0:
    print('No bins [0.00,%.2f], dr=%.2f' % (args.rmax, args.bin_width))
    sys.exit(1)
if args.v>0:
    print(' Using %d bins' % (n_bins))

# Pixel size
pix = float(args.L)/float(args.N)
bins_in_pixels = bins / pix

# Feedback number of digits for bin_i and r_values
ndigit_bin = int(np.ceil(np.log10(n_bins)))
ndigit_rs = int(np.ceil(np.log10(np.max(bins))))
bin_fmt = '#%%0%dd of ' % (ndigit_bin)
bin_fmt += '%%0%dd' % (ndigit_bin) % n_bins
r_fmt = '%%0%d.2f <= r < %%0%d.2f' % (ndigit_rs+3,ndigit_rs+3)

# Make folder for each set of samples
vert_folder = 'vert/N%d_L%.2f/Ntri%d'%(args.N, args.L, args.Ntri)
vert_folder += '/dR%06.2f' % (args.bin_width)
try:
    os.makedirs(vert_folder)
except FileExistsError:
    pass

# Filename for final verts file
full_rmax = np.max(bins[:,1])
verts_fname = descriptive
verts_fname += '_rmax%06.2f' % (full_rmax)
verts_fname += '_Ntri%d.verts' % (args.Ntri)
verts_fname = join(args.dr,verts_fname)

# Feedback if saving this time
if not args.nosave:
    print('  Verts file %s' % (verts_fname))
    _dir, basename = split(verts_fname)
    basename, ext = splitext(basename)
    fullname = basename + '.txt'
    verts_dr = '/share/splinter/wdj/Code/correlation/corr3/verts/'
    print('  vim command to copy \':wn %s\'\n' % (join(verts_dr,fullname)))

# Loop over bins and store all matches
n_all_bins = 0
results = []
for bin_i in range(n_bins):

    # String for this bin
    bin_st = 'Bin %s' % bin_fmt % (1+bin_i)

    # Get bin edges (in real and pixel coords)
    rmin, rmax = bins[bin_i,:]
    rmin_pix, rmax_pix = bins_in_pixels[bin_i,:]

    # Make parts for filename
    r_range_st = 'R%06.2f-%06.2f' % (rmin, rmax)

    # Filename to store sampled triangles 
    sub_vert_fname = join(vert_folder,r_range_st+'.vert')

    # Filename to store ALL triangles (if calcualted)
    all_vert_fname = 'allTri_'+r_range_st
    all_vert_fname = join(vert_folder,all_vert_fname+'.vert')

    # If previous sample exists, use that one
    if exists(sub_vert_fname):
        where_st = 'Existing file'
        with open(sub_vert_fname,"rb") as vert_file:
            triangle_vertices, n_total = pickle.load(vert_file)

    # If previous full set exists, use that and sample down
    elif exists(all_vert_fname):       
        where_st = 'Resampled all'

        with open(all_vert_fname,"rb") as vert_file:
            triangle_vertices, n_total = pickle.load(vert_file)
        triangle_vertices = subsample_vertices(triangle_vertices, 
                                               args.Ntri, n_total)

    # Otherwise, get the triangles now
    else:
        where_st = 'Found matches'

        # Make the rs_grid for this bin
        # Need up to (2 * rpix_max) + 1 pixels for np.where
        mgrid_size = 2*int(np.ceil(rmax_pix))
        mgrid_size += 1 if mgrid_size%2==0 else 0
        mgrid_rs_pix = rs_mgrid(mgrid_size)

        # Match triangles for this rpix range
        values = triangles_for_range(rmin_pix, rmax_pix, 
                                     mgrid_rs_pix, args.Ntri,
                                     not args.all_triangles,
                                     bar=not args.nobar,
                                     hide_when_finished=True, 
                                     prefix='  Matching %s' % bin_st,
                                     finished='  Matched %s' % (bin_st))
        triangle_vertices, n_total, all_triangles = values


    # Feedback start
    if args.v>0:
        print('  %s Bin %s:'%(where_st,bin_fmt)%(1+bin_i), end='\t')
        print(r_fmt % (rmin, rmax), end='\t')

    # Check if any triangle_vertices match this bin
    count_each_ptB = [len(i[1]) for i in triangle_vertices]        
    triangle_count = np.sum(count_each_ptB)    
    n_all_bins += triangle_count

    # Save this match to the verts file
    n_ptsB = len(triangle_vertices)
    if args.v>0:
        if triangle_count==0:
            print('[no triangles]')
        else:
            print('[%d triangles from %d]' % (triangle_count, n_total))

    # Store 'all_triangles' file if calculated
    if args.all_triangles and not exists(all_vert_fname):
        with open(all_vert_fname,"wb") as vert_file:
            pickle.dump([all_triangles, n_total], vert_file)

    # Also store sampled file, for next time
    if not exists(sub_vert_fname):
        with open(sub_vert_fname,"wb") as vert_file:
            pickle.dump([triangle_vertices, n_total], vert_file)

    # Store triangles for this bin
    results.append(triangle_vertices)

# Rough idea of the time this will take
total_calculations = ((float(args.N)**3.0) * float(n_all_bins))
time_taken = total_calculations / (5.00E+6)
print(' Total of %d triangles' % (n_all_bins))
for fraction in [1.0, 0.5, 0.1, 0.01, 0.001]:
    time_st = time_string(seconds=fraction*time_taken)
    print('   sample_fraction %.3f will take: %s' % (fraction,time_st))

# Quit now, if requested not saving the verts file
if args.nosave:
    sys.exit(0)

# After running, gather togethr into a single binary file 
# Which c++ can load
with open(verts_fname,'wb') as thefile:

    # Store values into file using struct.pack
    def store_values(values, type=int):
        fmt = { int : "{}i", float : "{}f"}[type]
        thefile.write(struct.pack(fmt.format(len(values)), *values))

    # Store number of bins
    store_values([len(results)])

    # Loop over bins
    for bin_i,triangle_vertices in enumerate(results):
        if args.v>1:
            print('  Saving Bin %s'%(bin_fmt)%(1+bin_i))

        # Store bin max and min values
        rmin, rmax = bins[bin_i,:]
        store_values([rmin, rmax], type=float)

        # Store number of points
        n_ptsB = len(triangle_vertices)
        store_values([n_ptsB])

        # Store each primary / n_secondary / secondary1 / sec2/ sec3 / ...
        # Skips if none were found
        # Format for each bin is
        # rmin | rmax | n_primary | ...
        # primary1 | primary1_n_secondary | primary1_secondary1 | primary1_secondary2 | ...
        # primary2 | primary2_n_secondary | primary2_secondary1 | primary2_secondary2 | ...
        # primary3 | primary3_n_secondary | primary3_secondary1 | primary3_secondary2 | ...
        for row in triangle_vertices:
            matches = row[1].flatten()
            ptB_array = np.array([*row[0], len(matches), *matches])
            store_values(ptB_array)

    # Filename feedback
    print('  Saved verts to \'%s\'' % verts_fname)
