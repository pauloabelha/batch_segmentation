/*
* So far tested on Ubuntu 14.04 only
* Based on: http://doc.cgal.org/latest/Surface_mesh_segmentation/index.html
* 	`...an implementation of the algorithm relying on the Shape Diameter Function [1] (SDF)'
* 	L. Shapira, A. Shamir, and D. Cohen-Or. Consistent mesh partitioning and skeletonisation using the shape diameter function. The Visual Computer, 24(4):249–259, 2008
* 
* Dependencies: If you want to convert from non .OFF files, then this program will require Meshlab Server to be installed
* 
* Segments every mesh in an input folder and writes the segmented .OFF files to an output folder
* It is possible to use this program to run a nested loop of using different number of clusters and smoothing parameters
* The program can also use MeshlabServer to convert input meshes to .OFF before segmenting them
* There are two possible usages:
* 	Segmenting once for each mesh with a given number of clusters (k) and smoothing (lambda) parameters;
* 	Segmenting many times using different values for k and lambda in a nested loop
* 
* Options:
* 	-i: Followed by the input directory
* 	-o: Followed by the output directory
* 	-e: Followed by the input mesh file extension (if not .OFF)
* 	-k: Followed by the k value (Integer) - integer number of clusters to use (2 <= k <= 9)
* 	-l: Followed by the lambda value (Real) - float smoothing parameter to use (0.01 <= lambda <= 1)
* 	-z: Followed by K_START:K_STEP:K_END,LAMBDA_START:LAMBDA_STEP:LAMBDA_END values for runnning many segmentations in a nested loop
* 	-f: (TO BE IMPLEMENTED) Followed by the minimum number of points of a segment to considered it to be fused with others if too small
* 	Options -i and -o are the only mandatory ones
* 	If options -k or -l are not present, then the recommended default value of k=5 and l=0.25 are used
* 	If option -z is present, options -k and -l are ignored
* 
* Example usages:
* 	segment_meshes -i data -o data_segmented
* 		This will segment every .OFF file in 'data' once, using the recommended values for k and lambda, and output the segmented .OFF files in 'data_segmented'
* 	segment_meshes -i data -o data_segmented -k 5 -l 0.25 -e ply
* 		This will convert every .PLY file in 'data', convert them to .OFF and segment each one once, using k=8 and lambda=0.7, and output the segmented .OFF files in 'data_segmented'
* 	segment_meshes -i data -o data_segmented -z 3:2:7,0.2:0.1:0.8
* 		This will segment every .PLY file in 'data' (without converting them - i.e. assuming they are .OFF files already), using k ranging from 2 to 8 in steps of 2, and so on for lambda
* 
* By Paulo Abelha (github.com/pauloabelha)

*/
 
#include <dirent.h>
#include <errno.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/boost/graph/graph_traits_Polyhedron_3.h>
#include <CGAL/Polyhedron_items_with_id_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/mesh_segmentation.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Polyhedron_3.h>	
#include <CGAL/property_map.h>
#include <iostream>
#include <fstream>
#include <string> 
#include <bitset>
#include <ctime>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Polyhedron_3<K, CGAL::Polyhedron_items_with_id_3>  Polyhedron;
typedef K::Point_3                              		Point_3;
typedef Polyhedron::Vertex_iterator                   	Vertex_iterator;
typedef Polyhedron::Facet_iterator                   	Facet_iterator;
typedef Polyhedron::Halfedge_around_facet_circulator 	Halfedge_facet_circulator;
// Property map associating a facet with an integer as id to an
// element in a vector stored internally
template<class ValueType>
struct Facet_with_id_pmap
    : public boost::put_get_helper<ValueType&,
             Facet_with_id_pmap<ValueType> >
{
    typedef Polyhedron::Facet_const_handle key_type;
    typedef ValueType value_type;
    typedef value_type& reference;
    typedef boost::lvalue_property_map_tag category;

    Facet_with_id_pmap(
      std::vector<ValueType>& internal_vector
    ) : internal_vector(internal_vector) { }

    reference operator[](key_type key) const
    { return internal_vector[key->id()]; }
private:
    std::vector<ValueType>& internal_vector;
};

// Global variables that are all set by function parse_args
// Option --verbose
//Shows information as the program runs (-v)
bool VERBOSE = 0;
// Option -z ks,ke,ls,le
// Loops for number of clusters (ks to ke) and smoothing values (ls to le) 
bool LOOP = 0;
// cluster args
int K_START = 5;
int K_STEP = 1;
int K_END = 5;
// lambda (smoothing) args
float LAMBDA_START = 0.25;
float LAMBDA_STEP = 1;
float LAMBDA_END = 0.25;
// Option -i 
// Input file or folder
std::string INPUT_DIR = "";
// Option -o 
// Output file or folder
std::string OUTPUT_DIR = "";
// Option -e EXT
// Extension of the input file(s); if differente than .off
// Meshlab is used to convert all files befor erunning the segmentation
std::string INPUT_FILE_EXT = "";

//splits a string (http://www.cplusplus.com/articles/1UqpX9L8/)
// split: receives a char delimiter; returns a vector of strings
// By default ignores repeated delimiters (rep == 0)
void splitstr(std::string str, char delim, std::vector<std::string> & flds) {
    std::string buf = "";
    int rep = 0;
    int i = 0;
    while (i < str.length()) {
        if (str[i] != delim)
            buf += str[i];
        else if (rep == 1) {
            flds.push_back(buf);
            buf = "";
        } else if (buf.length() > 0) {
            flds.push_back(buf);
            buf = "";
        }
        i++;
    }
    if (!buf.empty())
        flds.push_back(buf);
    return;
}

// writes comment-style line for the --help option
void write_comment_line(std::string comment) {
	std::cout << "* " << comment << std::endl;	
}

// writes usage of program to standard output
void write_usage(void) {
	write_comment_line("So far tested on Ubuntu 14.04 only");
	write_comment_line("Based on: http://doc.cgal.org/latest/Surface_mesh_segmentation/index.html");
	write_comment_line("\t`...an implementation of the algorithm relying on the Shape Diameter Function [1] (SDF)'");
	write_comment_line("\tL. Shapira, A. Shamir, and D. Cohen-Or. Consistent mesh partitioning and skeletonisation using the shape diameter function. The Visual Computer, 24(4):249–259, 2008");
	write_comment_line("");
	write_comment_line("Dependencies: If you want to convert from non .OFF files, then this program will require Meshlab Server to be installed");
	write_comment_line("");
	write_comment_line("Segments every mesh in an input folder and writes the segmented .OFF files to an output folder");	
	write_comment_line("It is possible to use this program to run a nested loop of using different number of clusters and smoothing parameters");	
	write_comment_line("The program can also use MeshlabServer to convert input meshes to .OFF before segmenting them");
	write_comment_line("There are two possible usages:");
	write_comment_line("\tSegmenting once for each mesh with a given number of clusters (k) and smoothing (lambda) parameters;");
	write_comment_line("\tSegmenting many times using different values for k and lambda in a nested loop");
	write_comment_line("");
	write_comment_line("Options:");
	write_comment_line("\t-i: Followed by the input directory"); 
	write_comment_line("\t-o: Followed by the output directory"); 
	write_comment_line("\t-e: Followed by the input mesh file extension (if not .OFF)");
	write_comment_line("\t-k: Followed by the k value (Integer) - integer number of clusters to use (2 <= k <= 9)");
	write_comment_line("\t-l: Followed by the lambda value (Real) - float smoothing parameter to use (0.01 <= lambda <= 1)");
	write_comment_line("\t-z: Followed by K_START:K_STEP:K_END,LAMBDA_START:LAMBDA_STEP:LAMBDA_END values for runnning many segmentations in a nested loop");
	write_comment_line("\t-f: (TO BE IMPLEMENTED) Followed by a percentage number ]0, 0.5[ representing the minimum proportional size of a segment (in number of points) above which it is going to be be fused with closest segment neighbours");
	write_comment_line("\tOptions -i and -o are the only mandatory ones");
	write_comment_line("\tIf options -k or -l are not present, then the recommended default value of k=5 and l=0.25 are used");
	write_comment_line("\tIf option -z is present, options -k and -l are ignored");
	write_comment_line("");
	write_comment_line("Example usages:");
	write_comment_line("\tsegment_meshes -i data -o data_segmented");
	write_comment_line("\t\tThis will segment every .OFF file in 'data' once, using the recommended values for k and lambda, and output the segmented .OFF files in 'data_segmented'");
	write_comment_line("\tsegment_meshes -i data -o data_segmented -k 5 -l 0.25 -e ply");
	write_comment_line("\t\tThis will convert every .PLY file in 'data', convert them to .OFF and segment each one once, using k=8 and lambda=0.7, and output the segmented .OFF files in 'data_segmented'");
	write_comment_line("\tsegment_meshes -i data -o data_segmented -z 3:2:7,0.2:0.1:0.8");
	write_comment_line("\t\tThis will segment every .PLY file in 'data' (without converting them - i.e. assuming they are .OFF files already), using k ranging from 2 to 8 in steps of 2, and so on for lambda");
	write_comment_line("");
	write_comment_line("By Paulo Abelha (github.com/pauloabelha)");
}

// parse function params
// this function will set all global variables
// see global variables above to know the meaning of each option
int parse_args(int argc, char *argv[]) {
	if (argc < 2) {
		std::cout << "ERROR: No arguments given" << std::endl;
		write_usage();
		return 0;
	}
	if (argc > 10) {
		std::cout << "ERROR: There too many arguments passed to the program: " << argc << std::endl;
		write_usage();
		return 0;
	}
	int i = 1;
	bool is_option_arg = false;
	bool is_help_option = false;
	bool is_verbose_option = false;
	bool loop_option = false;
	int n_args = argc - 1;
	
	const int safe_n_iter = 10;
	int n_iter = 0;
	while (i<=n_args) {			
		if (n_iter++ > safe_n_iter) {
			std::cout << "ERROR: Avoiding infinite loop when parsing arguments (something bad happened):\t" << n_args << std::endl;
			write_usage();
			return 0;			
		}	
		std::string argv_str = std::string(argv[i]);
		char first_char = argv[i][0];		
		is_option_arg = first_char == '-';
		is_help_option = argv_str.compare("--help") == 0;
		is_verbose_option = argv_str.compare("--verbose") == 0;
		// arguments should either come in pairs (option arg) or be --verbose or --help
		if (!(is_verbose_option || is_help_option) && !(is_option_arg)) {
			std::cout << "ERROR: There is an option without argument (or vice versa):\t" << argv_str << std::endl;
			write_usage();
			return 0;
		}
		// parse --help
		if (is_help_option) {
			write_usage();
			return 0;
		}
		// parse --verbose
		if (is_verbose_option) {
			VERBOSE = true;
			i++;
			continue;
		}
		// parse options
		if (is_option_arg) {			
			// if last argument is an option (shouldn't be), write usage
			if ((i+1)>n_args) {
				std::cout << "ERROR: Last argument is an option:\t" << argv_str << std::endl;
				write_usage();
				return 0;
			}
			// if option argument is an option itself
			if (argv[i+1][0] == '-') {
				std::cout << "ERROR: Option has another options as argument:\t" << argv_str << " " << argv[i+1] << std::endl;
				write_usage();
				return 0;
			}
			// -z (loop option)
			if (argv_str.compare("-z") == 0) {
				loop_option = true;
				std::string loop_argument_str = std::string(argv[i+1]);
				std::vector<std::string> loop_args;
				splitstr(loop_argument_str, ',', loop_args);
				if (loop_args.size() != 2) {
					std::cout << "ERROR: Loop option should have one comma separating its two arguments:\t" << loop_argument_str << std::endl;
					write_usage();
					return 0;
				}
				// get loop cluster args
				std::vector<std::string> cluster_args;
				splitstr(loop_args[0], ':', cluster_args);
				if (cluster_args.size() != 3) {
					std::cout << "ERROR: Loop option for number of clusters (k) should have three values separated by colons:\t" << loop_argument_str << std::endl;
					write_usage();
					return 0;
				}
				K_START = std::atoi(cluster_args[0].c_str());
				K_STEP = std::atoi(cluster_args[1].c_str());
				K_END = std::atoi(cluster_args[2].c_str());
				if (K_START < 2) {
					std::cout << "ERROR: Initial number of clusters is less than two:\t" << loop_argument_str << std::endl;
					write_usage();
					return 0;
				}
				if (K_END < 2) {
					std::cout << "ERROR: Final number of clusters is less than two:\t" << loop_argument_str << std::endl;
					write_usage();
					return 0;
				}
				if (K_START > K_END) {
					std::cout << "ERROR: Initial number of clusters is greater than final number of clusters:\t" << loop_argument_str << std::endl;
					write_usage();
					return 0;
				}				
				// get loop lambda args
				std::vector<std::string> lambda_args;
				splitstr(loop_args[1], ':', lambda_args);
				if (lambda_args.size() != 3) {
					std::cout << "ERROR: Loop option for lambda (smoothing) should have three values separated by colons:\t" << loop_argument_str << std::endl;
					write_usage();
					return 0;
				}
				LAMBDA_START = std::atof(lambda_args[0].c_str());
				LAMBDA_STEP = std::atof(lambda_args[1].c_str());
				LAMBDA_END = std::atof(lambda_args[2].c_str());
				if (LAMBDA_START < 0.0099) {
					std::cout << "ERROR: Initial lambda is less than 0.01:\t" << loop_argument_str << std::endl;
					write_usage();
					return 0;
				}
				if (LAMBDA_STEP < 0.0099) {
					std::cout << "ERROR: Lambda step is too small (MIN: 0.01):\t" << loop_argument_str << std::endl;
					write_usage();
					return 0;
				}
				if (LAMBDA_END > 1.001) {
					std::cout << "ERROR: Final lambda is greater than 1:\t" << loop_argument_str << std::endl;
					write_usage();
					return 0;
				}
				if (LAMBDA_START > LAMBDA_END) {
					std::cout << "ERROR: Initial lambda is greater than final lambda:\t" << loop_argument_str << std::endl;
					write_usage();
					return 0;
				}
			}	
			else if (argv_str.compare("-k") == 0) {
				if(!loop_option) {
					K_START = std::atoi(argv[i+1]);
					if (K_START < 2) {
						std::cout << "ERROR: Given number of clusters is less than two: " << K_START << std::endl;
						write_usage();
						return 0;
					}
					K_STEP = 1;
					K_END = K_START;
					std::cout << K_START << std::endl;
				}
			}	
			else if (argv_str.compare("-l") == 0) {
				if(!loop_option) {
					LAMBDA_START = std::atof(argv[i+1]);
					if (LAMBDA_START < 0.0099 || LAMBDA_END > 1.001) {
						std::cout << "ERROR: Given lambda is outside allowed interval [0.01, 1]: " << LAMBDA_START << std::endl;
						write_usage();
						return 0;
					}
					LAMBDA_STEP = 1;
					LAMBDA_END = K_START;
					std::cout << LAMBDA_START << std::endl;
				}
			}
			else if ((argv_str.compare("-i") == 0)) {
				INPUT_DIR = std::string(argv[i+1]);
				if (INPUT_DIR.at(INPUT_DIR.length()-1) != '/')
					INPUT_DIR += "/";
			}	
			else if ((argv_str.compare("-o") == 0)) {
				OUTPUT_DIR = std::string(argv[i+1]);	
				if (OUTPUT_DIR.at(OUTPUT_DIR.length()-1) != '/')
					OUTPUT_DIR += "/";			
			}
			else if ((argv_str.compare("-e") == 0)) {
				INPUT_FILE_EXT = std::string(argv[i+1]);				
			}
			else {
				std::cout << "ERROR: Invalid option found: " << argv_str << std::endl;
				write_usage();
				return 0;
			}
			// increment to get next option
			i += 2;
		}
	}
	if (VERBOSE) {
		std::cout << "Arguments parsed (or default) that are going to be going to be used:" << std::endl;
		std::string loop_option_str = "No";
		if (loop_option)
			loop_option_str = "Yes! Loop de loop!";
		std::cout << "Looping: " << loop_option_str << std::endl;
		std::cout << "K_START: " << K_START << std::endl;
		std::cout << "K_STEP: " << K_STEP << std::endl;
		std::cout << "K_END: " << K_END << std::endl;
		std::cout << "LAMBDA_START: " << LAMBDA_START << std::endl;
		std::cout << "LAMBDA_STEP: " << LAMBDA_STEP << std::endl;
		std::cout << "LAMBDA_END: " << LAMBDA_END << std::endl;
		std::cout << "INPUT_DIR: " << INPUT_DIR << std::endl;
		std::cout << "OUTPUT_DIR: " << OUTPUT_DIR << std::endl;
		std::cout << "INPUT_FILE_EXT: " << INPUT_FILE_EXT;
		if (INPUT_FILE_EXT.empty())
			std::cout << "No mesh format conversion required"; 
		std::cout << std::endl << std::endl;	
	}
	if (INPUT_DIR.empty()) {
		std::cout << "Input directory cannot be empty. Please give an input directory through option -i" << std::endl;
		write_usage();
		return 0;
	}
	if (OUTPUT_DIR.empty()) {
		std::cout << "Output directory cannot be empty. Please give an output directory through option -o" << std::endl;
		write_usage();
		return 0;
	}
	return 1;
}

// convert a color as an int[3] RGB to a std::string
std::string color_intarray2string(int *color_int) {
	std::string color_str = "";
	for (int i=0;i<3;i++) {
		char int_char_array[4];
		snprintf(int_char_array, 4, "%d", color_int[i]);
		color_str += std::string(int_char_array) + " ";		
	}
	return color_str;	
}

// get a color as RGB in a int[3] array given an index
// color order: red - green - blue - yellow - cyan - magenta - white
// further colors (in order) are darker versions of each above
int* get_color(uint index) {
	index++;
	std::bitset<3> index_bits = index % 7;
	//std::cout << "Index in bits:\t" << index_bits[0] << " " << index_bits[1] << " " << index_bits[2] << std::endl;
	int darken_decay_factor = std::floor((double)index/7);
	double darken_factor = 1.0/std::pow(2.0,darken_decay_factor);
	//std::cout << "Darken decay and full factor:\t" << " " << darken_decay_factor << " " << darken_factor << std::endl;
	int *color = new int[3];
	for (int i=0;i<3;i++)		
		color[i] = index_bits[i] * std::floor(darken_factor*255);
	//std::cout << "Index color:\t" << color[0] << " " << color[1] << " " << color[2] << std::endl;
	return color;	
}

// adds a suffix to a filename that has an extension
// e.g. adding suffix "out" to filename "mymesh.off" outputs "mymesh_out.off"
std::string add_suffix_filename(std::string filename, std::string suffix) {
	std::string file_shortname = filename.substr(0,filename.length()-4);
	std::string file_ext = filename.substr(filename.length()-3,filename.length()-1);
	return file_shortname + suffix + "." + file_ext;	
}

std::string change_filename_ext(std::string filename, std::string new_ext) {
	if (new_ext.empty())
		return std::string(filename.substr(0,filename.length()-4));
	else
		return std::string(filename.substr(0,filename.length()-3)) + new_ext;	
}

// get all files in a given directory
// files are any string with lenght > 4 and with a '.' at end - 4
int get_files_in_dir (std::string dir, std::vector<std::string> &files, std::string ext){
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        std::cout << "Error (" << errno << ") opening folder: " << dir << std::endl;
        return 0;
    }
    
    while ((dirp = readdir(dp)) != NULL) {
		std::string file_name = std::string(dirp->d_name);
		bool file_has_min_lenght = file_name.length() > 4;
		if (file_has_min_lenght) {
			bool is_a_file = file_name[file_name.length() - 4] == '.';
			std::string file_ext = file_name.substr(file_name.length() - 3);
			bool file_has_ext = (file_ext.compare(ext) == 0);
			if (is_a_file && file_has_ext) {
				std::string filename = std::string(dirp->d_name);
				files.push_back(filename);
				if (VERBOSE) std::cout << filename << std::endl;
			}
			
		}
    }
    closedir(dp);
    return 1;
}

std::string get_filepath_from_k_and_lambda(std::string orig_filepath, int k, float lambda) {
	//std::string output_filename = mesh_filepath.substr(0,mesh_filepath.length()-4) + "_out.off";
	std::string output_filepath = orig_filepath;				
	char buffer_k [2];
	std::sprintf(buffer_k,"%d",k);
	std::string k_str = std::string(buffer_k);
	char buffer_lambda [4];
	int lambda_int = floor(lambda*100);
	std::sprintf(buffer_lambda,"%d",lambda_int);
	std::string lambda_str = std::string(buffer_lambda);
	std::string output_filepath_suffix = "_out_" + k_str + "_" + lambda_str;
	output_filepath = add_suffix_filename(output_filepath, output_filepath_suffix);
	return output_filepath;	
}

// write mesh to an .OFF file
void write_mesh_to_off_file(std::string orig_filepath, Polyhedron mesh, Facet_with_id_pmap<std::size_t> segment_property_map, int n_segms, int k, float lambda) {
	// get complete filepath
	std::string output_filepath = get_filepath_from_k_and_lambda(orig_filepath, k, lambda);
	// get stream output
	std::ofstream output(output_filepath.c_str());
	if (VERBOSE) std::cout << "Writing to file (" << output_filepath << ")" << std::endl;
	// Write polyhedron in Object File Format (OFF) - ASCII
	CGAL::set_ascii_mode( output );
	// write header
	output << "# File generated By Paulo Abelha (github/pauloabelha)" << std::endl;
	output << "# Code file: segment_meshes.cpp" << std::endl;
	output << "# Using the library CGAL 4.9" << std::endl;
	output << "# Based on http://doc.cgal.org/latest/Surface_mesh_segmentation/index.html" << std::endl;
	output << "# n_segms " << n_segms << std::endl;		
	output << "OFF" << std::endl << mesh.size_of_vertices() << ' ' << mesh.size_of_facets() << " 0" << std::endl;	
	//write points	
	if (VERBOSE) std::cout << "\tWriting " << mesh.size_of_vertices() << " points..." << std::endl;	  
	std::copy( mesh.points_begin(), mesh.points_end(), std::ostream_iterator<Point_3>( output, "\n"));
	// write faces		   
	if (VERBOSE) std::cout << "\tWriting " << mesh.size_of_facets() << " faces..." << std::endl;
	for (Facet_iterator i = mesh.facets_begin(); i != mesh.facets_end(); ++i) {
		Halfedge_facet_circulator j = i->facet_begin();
		// Facets in polyhedral surfaces are at least triangles
		CGAL_assertion( CGAL::circulator_size(j) >= 3);
		output << CGAL::circulator_size(j) << ' ';
		do {
			output << ' ' << std::distance(mesh.vertices_begin(), j->vertex());
		} while ( ++j != i->facet_begin());
		// put color in the face
		int *color = get_color(segment_property_map[i]);
		std::string color_str = color_intarray2string(color);
		output << ' ' << color_str;
		output << std::endl;
	}
	output.close();
	if (VERBOSE) std::cout << "OK" << std::endl;	
}


// return the segm ID for each vertex from the segm ID for each face
// we count, for each vertex, the number of times a given segm ID is given to each face to each it belongs
// the s of each vertex is then the one that happens the most
std::vector<int> get_vertices_segm_ids(Polyhedron mesh, Facet_with_id_pmap<std::size_t> segment_property_map, int n_segms) {
	// matrix V x S, V is number of vertices and S is number of segments
	// it holds for each segment, the number of times it has appeared in a face with the given segm ID
	std::vector< std::vector<int> > vertex_segm_ids(mesh.size_of_vertices(), std::vector<int>(n_segms));
	// run through faces and fill the matrix
	for (Facet_iterator i = mesh.facets_begin(); i != mesh.facets_end(); ++i) {
		Halfedge_facet_circulator j = i->facet_begin();
		// Facets in polyhedral surfaces are at least triangles
		CGAL_assertion( CGAL::circulator_size(j) >= 3);
		do {
			int vertex_ix = std::distance(mesh.vertices_begin(), j->vertex());
			vertex_segm_ids[vertex_ix][segment_property_map[i]]++;			
		} while ( ++j != i->facet_begin());
	}
	std::vector<int> vertices_segm_ids(mesh.size_of_vertices());
	for (int i=0; i<=mesh.size_of_vertices()-1; i++) {
		int max = -1;
		for (int j = 0; j<=n_segms - 1; j++) {		
			if (vertex_segm_ids[i][j] > max) {
				vertices_segm_ids[i] = j;
				max = vertex_segm_ids[i][j];
			}
		}
	}
	return vertices_segm_ids;		
}

// write a ply header
void write_ply_header(std::ofstream & output, int n_segms, int n_points, int n_faces) {	
	output << "ply" << std::endl;
	output << "format ascii 1.0" << std::endl;
	output << "comment File generated By Paulo Abelha (github/pauloabelha)" << std::endl;
	output << "comment Code file: segment_meshes.cpp" << std::endl;
	output << "comment Using the library CGAL 4.9" << std::endl;
	output << "comment Based on http://doc.cgal.org/latest/Surface_mesh_segmentation/index.html" << std::endl;
	output << "comment n_segms " << n_segms << std::endl;		
	output << "element vertex " << n_points << std::endl;
	output << "property float x" << std::endl;
	output << "property float y" << std::endl;
	output << "property float z" << std::endl;
	output << "property int segm" << std::endl;
	output << "property uchar red" << std::endl;
	output << "property uchar green" << std::endl;
	output << "property uchar blue" << std::endl;
	output << "element face " << n_faces << std::endl;
	output << "property list uchar int vertex_indices" << std::endl;
	output << "end_header" << std::endl;
}

// write mesh to an .PLY file
void write_mesh_to_ply_file(std::string orig_filepath, Polyhedron mesh, Facet_with_id_pmap<std::size_t> segment_property_map, int n_segms, int k, float lambda) {
	// get complete filepath
	std::string output_filepath = get_filepath_from_k_and_lambda(orig_filepath, k, lambda);
	// change file extension to .PLY
	output_filepath = change_filename_ext(output_filepath, "ply");
	// get stream output
	std::ofstream output(output_filepath.c_str());
	if (VERBOSE) std::cout << "Writing to file (" << output_filepath << ")" << std::endl;
	// Write polyhedron in Object File Format (OFF) - ASCII
	CGAL::set_ascii_mode( output );
	// write header
	write_ply_header(output, n_segms, mesh.size_of_vertices(), mesh.size_of_facets());
	//write points	
	if (VERBOSE) std::cout << "\tCalculating vertices colors from face colors for " << mesh.size_of_vertices() << " points and " << mesh.size_of_facets() << " faces..." << std::endl;
	std::vector<int> vertices_segm_ids = get_vertices_segm_ids(mesh, segment_property_map, n_segms);
	int vertex_ix = 0;
	if (VERBOSE) std::cout << "\tWriting " << mesh.size_of_vertices() << " points..." << std::endl;
	for ( Vertex_iterator v = mesh.vertices_begin(); v != mesh.vertices_end(); ++v) {
		int vertex_segm_id = vertices_segm_ids[vertex_ix];
		int *color = get_color(vertex_segm_id);
		std::string color_str = color_intarray2string(color);
		output << v->point() << " " << vertex_segm_id << " " << color_str << std::endl;
		vertex_ix++;
	}
	// write faces	
	if (VERBOSE) std::cout << "\tWriting " << mesh.size_of_facets() << " faces..." << std::endl;	   
	for (Facet_iterator i = mesh.facets_begin(); i != mesh.facets_end(); ++i) {
		Halfedge_facet_circulator j = i->facet_begin();
		// Facets in polyhedral surfaces are at least triangles
		CGAL_assertion( CGAL::circulator_size(j) >= 3);
		output << CGAL::circulator_size(j);
		do {
			output << ' ' << std::distance(mesh.vertices_begin(), j->vertex());
		} while ( ++j != i->facet_begin());
		output << std::endl;
	}
	output.close();
	if (VERBOSE) std::cout << "Done" << std::endl;	
}

// create and read mesh as a Polyhedron (defined through typedef above)
int get_mesh_from_off_file(std::string mesh_filepath, Polyhedron & mesh) {	
	if (VERBOSE) std::cout << "Reading mesh file (" << mesh_filepath << ") as a Polyhedron...";
	std::ifstream input(mesh_filepath.c_str());
	if ( !input ) {
		if (VERBOSE) std::cout << std::endl;
		std::cout << "ERROR: Could not read stream from file - going to next mesh" << std::endl << std::endl;
		return 0;
	}
	if ( !(input >> mesh)) {
		if (VERBOSE) std::cout << std::endl;
		std::cout << "ERROR: The .off file is invalid - going to next mesh" << std::endl << std::endl;
		return 0;
	}
	if ( mesh.empty() ) {
		if (VERBOSE) std::cout << std::endl;
		std::cout << "ERROR: Created Polyhedron is empty - going to next mesh" << std::endl << std::endl;
		return 0;
	}
	if (VERBOSE) std::cout << "OK" << std::endl;
	if (VERBOSE) std::cout << "Mesh has " << mesh.size_of_facets() << " faces" << std::endl;	
}

int convert_meshes(std::string exec_fullpath, std::string working_dir, std::string OUTPUT_DIR, std::vector<std::string> meshes_filenames, std::string ext_out) {
	for(std::vector<std::string>::iterator it = meshes_filenames.begin(); it != meshes_filenames.end(); ++it) {
		std::string mesh_filepath_in = working_dir + *it;
		std::string mesh_filepath_out = OUTPUT_DIR + *it;
		std::string meshlab_script_name = "make_cgal_friendly.mlx";
		std::string meshlabserver_command =
			"meshlabserver -i " + mesh_filepath_in +
			" -o " + change_filename_ext(mesh_filepath_out,ext_out) +
			" -s " + exec_fullpath + meshlab_script_name +
			" -om vn vf fn ff";
		if (system(NULL))
			system(meshlabserver_command.c_str());		
		else {
			std::cout << "ERROR: Could not get a shell to run MeshlabServer application" << std::endl;
			return 0;
		}
		if (VERBOSE) std::cout << "Meshlabserver command: " << meshlabserver_command << std::endl;
	}	
	return 1;
}

// convert meshes in a given directory
int convert_meshes_in_dir(std::string exec_fullpath, std::string & INPUT_DIR, std::string OUTPUT_DIR, std::string INPUT_FILE_EXT) {	
	if (!INPUT_FILE_EXT.empty()) {
		// get the meshes filenames to convert to .OFF
		std::vector<std::string> meshes_filenames_to_convert = std::vector<std::string>();	
		if (VERBOSE) std::cout << "Reading file names from directory " << INPUT_DIR << std::endl;	
		if(!get_files_in_dir(INPUT_DIR, meshes_filenames_to_convert, INPUT_FILE_EXT))
			return 0;
		// convert meshes to .OFF	
		if (VERBOSE) std::cout << "Converting meshes in " << INPUT_DIR << " and saving them in " << OUTPUT_DIR << std::endl;
		if (!convert_meshes(exec_fullpath, INPUT_DIR, OUTPUT_DIR, meshes_filenames_to_convert, "off")) {
			std::cout << "ERROR: Could not convert meshes" << std::endl;
			return 0;
		}
		if (VERBOSE) std::cout << std::endl;
		INPUT_DIR = OUTPUT_DIR;
	}
	return 1;
}

int get_meshes_filenames(std::string INPUT_DIR, std::vector<std::string> & meshes_filenames, bool sorted = false) {
	meshes_filenames = std::vector<std::string>();
	if(!get_files_in_dir(INPUT_DIR, meshes_filenames, "off"))
		return 0;
	if (VERBOSE) std::cout << std::endl;
	if (meshes_filenames.size() == 0) {
		std::cout << meshes_filenames.size() << " .OFF files found in directory: " << INPUT_DIR << std::endl;
		std::cout << "Segmentation requires .OFF files. Please add the extension you are using. For .PLY add: -e ply" << std::endl;
		return 0;
	}
	if (sorted)
		std::sort(meshes_filenames.begin(), meshes_filenames.end());
	return 1;
}

double sesc2HHMM(double secs, double & minutes, double & hours, int round) {
	hours = std::floor(secs/3600);
	minutes = ((int)secs % 3600)/60;
	double round_factor = std::pow(10,round);
	minutes = floorf( (minutes*round_factor)/round_factor );
}

void DisplayEstimatedTimeOfLoop( double tot_toc, int curr_ix, int tot_iter, std::string prefix ) {
	double minutes = 0;
	double hours = 0;
    if (curr_ix == tot_iter) {
		sesc2HHMM(tot_toc, minutes, hours, 2);
        std::cout << prefix << "Total elapsed time (HH:MM): " << hours << ":" << minutes  << std::endl;
	}
    else  { 
		double avg_toc = tot_toc/curr_ix;
		sesc2HHMM(avg_toc*(tot_iter-curr_ix), minutes, hours, 2);
        std::cout << prefix << "Estimated time (HH:MM): " << hours << ":" << minutes << " " << std::floor(curr_ix*100/tot_iter) << "%" << std::endl;
	}
}

// get full path to folder where the executable is running
// https://www.linuxquestions.org/questions/programming-9/get-full-path-of-a-command-in-c-117965/
std::string GetFullPathWhereProgramRuns() {
	const int MAXPATHLEN = 200;   /* make this larger if you need to. */
	int length;
	char fullpath[MAXPATHLEN];

	/* /proc/self is a symbolic link to the process-ID subdir
	* of /proc, e.g. /proc/4323 when the pid of the process
	* of this program is 4323.
	*
	* Inside /proc/<pid> there is a symbolic link to the
	* executable that is running as this <pid>.  This symbolic
	* link is called "exe".
	*
	* So if we read the path where the symlink /proc/self/exe
	* points to we have the full path of the executable.
	*/

	length = readlink("/proc/self/exe", fullpath, sizeof(fullpath));
	/* Catch some errors: */
	if (length < 0) {
		fprintf(stderr, "Error resolving symlink /proc/self/exe.\n");
		exit(EXIT_FAILURE);
	}
	if (length >= MAXPATHLEN) {
		fprintf(stderr, "Path too long. Truncated.\n");
		exit(EXIT_FAILURE);
	}

	/* I don't know why, but the string this readlink() function 
	* returns is appended with a '@'.
	*/
	fullpath[length] = '\0';       /* Strip '@' off the end. */
	
	std::string fullpath_str = fullpath;
	fullpath_str = fullpath_str.substr(0,fullpath_str.size()-14);
	return fullpath_str;;
	}

// segment every mesh in a given directory
int main(int argc, char *argv[]){
	// parse args
	if (!parse_args(argc, argv))
		return 0;
	
	// get full path to folder where the executable is running
	std::string exec_fullpath = GetFullPathWhereProgramRuns();
				
	// make output directory
	if (VERBOSE) std::cout << "Creating directory " << OUTPUT_DIR << std::endl;
	std::string command_mkdir = "mkdir " + OUTPUT_DIR;	
	system(command_mkdir.c_str());
		
	// convert meshes if necessary
	if(!convert_meshes_in_dir(exec_fullpath, INPUT_DIR, OUTPUT_DIR, INPUT_FILE_EXT))
		return 0;
		
	//get meshes filenames
	std::vector<std::string> meshes_filenames;
	if(!get_meshes_filenames(INPUT_DIR, meshes_filenames))
		return 0;
	
	// run through each mesh in INPUT_DIR (getting SDF values only once)
	// 		run through k number of clusters
	//			run through smoothing lambdas
	//				segment and write .OFF file
	double tot_toc = 0;
	int curr_ix = 0;
	int n_pcls = meshes_filenames.size();
	for(std::vector<std::string>::iterator it = meshes_filenames.begin(); it != meshes_filenames.end(); ++it) {
		// variables to measure time elapsed
		curr_ix++;
		std::clock_t tic = std::clock();
		
		// get filepath to write to
		std::string mesh_filename = *it;
		std::string mesh_filepath = INPUT_DIR + mesh_filename;	
		
		// get mesh from .OFF file
		// if there is a problem continue the loop for next mesh
		Polyhedron mesh;
		if(!get_mesh_from_off_file(mesh_filepath, mesh))
			continue;
		
		// assign id field for each face
		if (VERBOSE) std::cout << "Assigning a different ID to each face...";
		std::size_t facet_id = 0;
		for(Polyhedron::Facet_iterator facet_it = mesh.facets_begin(); facet_it != mesh.facets_end(); ++facet_it, ++facet_id)
			facet_it->id() = facet_id;
		if (VERBOSE) std::cout << "OK" << std::endl;

		// create a property-map for SDF values
		// to access SDF values with constant-complexity
		if (VERBOSE) std::cout << "Creating an index for SDF values to access them with constant-complexity...";
		std::vector<double> sdf_values(mesh.size_of_facets());		
		Facet_with_id_pmap<double> sdf_property_map(sdf_values);
		CGAL::sdf_values(mesh, sdf_property_map);
		if (VERBOSE) std::cout << "OK" << std::endl;
		
		// create a property-map for segment IDs
		// so we can access a face's segment ID with constant-complexity
		if (VERBOSE) std::cout << "Creating an index for face segment IDS to access them with constant-complexity...";
		std::vector<std::size_t> segment_ids(mesh.size_of_facets());
		Facet_with_id_pmap<std::size_t> segment_property_map(segment_ids);
		if (VERBOSE) std::cout << "OK" << std::endl;
		std::cout << std::endl;
		
		// run through ks and lambdas, segment and write to .OFF file
		double tot_toc_per_mesh = 0;
		int curr_ix_mesh = 0;
		int n_ks_tries = 0;
		for (int k=K_START; k <= K_END; k += K_STEP)
			n_ks_tries++;
			int n_lambda_tries = 0;
		for (float lambda=LAMBDA_START; lambda <= LAMBDA_END; lambda += LAMBDA_STEP)
			n_lambda_tries++;
		for (int k=K_START; k <= K_END; k += K_STEP) {			
			curr_ix_mesh++;
			int curr_ix_lambda = 0;
			double tot_toc_per_lambda = 0;
			for (float lambda=LAMBDA_START; lambda <= LAMBDA_END; lambda += LAMBDA_STEP) {			
				curr_ix_lambda++;
				// segment the mesh with params k and lambda
				if (VERBOSE) std::cout << "Segmenting...";
				int n_segms = CGAL::segmentation_from_sdf_values(mesh, sdf_property_map, segment_property_map, k, lambda);
				if (VERBOSE) std::cout << "OK" << std::endl << "Number of segments: " << n_segms << std::endl;
				
				// write mesh to .PLY or .OFF file
				if (INPUT_FILE_EXT.compare("ply") == 0)
					write_mesh_to_ply_file(OUTPUT_DIR + mesh_filename, mesh, segment_property_map, n_segms, k, lambda);
				else
					write_mesh_to_off_file(OUTPUT_DIR + mesh_filename, mesh, segment_property_map, n_segms, k, lambda);	
				
				
				if (VERBOSE) std::cout << std::endl;
				tot_toc_per_lambda += double(std::clock() - tic) / CLOCKS_PER_SEC;
				std::cout << std::endl;
				DisplayEstimatedTimeOfLoop( tot_toc_per_lambda, curr_ix_lambda, n_lambda_tries, "Loop for each lambda per mesh ");
			}
			tot_toc_per_mesh += double(std::clock() - tic) / CLOCKS_PER_SEC;
			std::cout << std::endl;
			DisplayEstimatedTimeOfLoop( tot_toc_per_mesh, curr_ix_mesh, n_ks_tries, "Loop for each K per mesh ");
			std::cout << std::endl;
		}
		tot_toc += double(std::clock() - tic) / CLOCKS_PER_SEC;
		std::cout << std::endl;
		DisplayEstimatedTimeOfLoop( tot_toc, curr_ix, n_pcls, "Loop for all meshes ");
		std::cout << std::endl;
	}
}
