/*
* So far tested on Ubuntu 14.04 only
* 
* Fuses small segments with the closest ones of every .PLY file in an input folder and writes the fused .PLY file to an output folder
* 
* Options:
* 	-i: Followed by the input directory
* 	-o: Followed by the output directory
* Example usages:
* 	fuse_small_segments -i data_segmented -o data_fused
* 		This will fuse small segments of every .PLY file in 'data' and output the fused .PLY files in 'data_segmented'
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
#include <string>
#include <vector>
#include <algorithm>

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
//Shows information as the program runs (--verbose)
bool VERBOSE = 0;
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
std::vector<std::string> splitstr(std::string str, char delim) {
	std::vector<std::string> flds;
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
    return flds;
}

// writes comment-style line for the --help option
void write_comment_line(std::string comment) {
	std::cout << "* " << comment << std::endl;	
}

// writes usage of program to standard output
void write_usage(void) {
	write_comment_line("So far tested on Ubuntu 14.04 only");
	write_comment_line("");
	write_comment_line("Fuses small segments with the closest ones of every .PLY file in an input folder and writes the fused .PLY file to an output folder");	
	write_comment_line("");
	write_comment_line("Options:");
	write_comment_line("\t-i: Followed by the input directory"); 
	write_comment_line("\t-o: Followed by the output directory");
	write_comment_line("Example usages:");
	write_comment_line("\tfuse_small_segments -i data_segmented -o data_fused");
	write_comment_line("\t\tThis will fuse small segments of every .PLY file in 'data' and output the fused .PLY files in 'data_segmented'");
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
		std::cout << "INPUT_DIR: " << INPUT_DIR << std::endl;
		std::cout << "OUTPUT_DIR: " << OUTPUT_DIR << std::endl;
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

// gest the name of every .PLY file in the directory
int get_ply_filenames(std::string INPUT_DIR, std::vector<std::string> & pcl_filenames, bool sorted = false) {
	pcl_filenames = std::vector<std::string>();
	if(!get_files_in_dir(INPUT_DIR, pcl_filenames, "ply"))
		return 0;
	if (VERBOSE) std::cout << std::endl;
	if (pcl_filenames.size() == 0) {
		std::cout << pcl_filenames.size() << " .PLY files found in directory: " << INPUT_DIR << std::endl;
		return 0;
	}
	if (sorted)
		std::sort(pcl_filenames.begin(), pcl_filenames.end());
	return 1;
}

// get a color as RGB in a int[3] array given an index
// color order: red - green - blue - yellow - cyan - magenta - white
// further colors (in order) are darker versions of each above
int * get_color(uint index) {
	index++;
	std::bitset<3> index_bits = index % 7;
	int *color = new int[3];
	//std::cout << "Index in bits:\t" << index_bits[0] << " " << index_bits[1] << " " << index_bits[2] << std::endl;
	int darken_decay_factor = std::floor((double)index/7);
	double darken_factor = 1.0/std::pow(2.0,darken_decay_factor);
	//std::cout << "Darken decay and full factor:\t" << " " << darken_decay_factor << " " << darken_factor << std::endl;
	for (int i=0;i<3;i++)		
		color[i] = index_bits[i] * std::floor(darken_factor*255);
	//std::cout << "Index color:\t" << color[0] << " " << color[1] << " " << color[2] << std::endl;
	return color;
}

struct PLY {
	int n_points;
	int n_segms;
	int n_points_per_face;
	int n_faces;
	
	double ** points;
	int *segm_labels;
	int *points_per_segm_label;
	int **colors;
	int ** faces;
};

// read only non-binary .PLY
PLY * read_ply(std::string ply_filepath) {
	PLY *ply = new PLY();
	std::vector<std::string> line_split;
	std::ifstream infile(ply_filepath.c_str());
	std::string line;
	// read header
	ply->n_points = 0;
	ply->n_faces = 0;
	while (std::getline(infile, line)) {
		std::istringstream iss(line);
		if (line.compare("end_header")) {	
			line_split = splitstr(line, ' ');
			// get number of points
			if (line_split.size() > 2 && line_split[0].compare("element") == 0 && line_split[1].compare("vertex") == 0)
				ply->n_points = std::atoi(line_split[2].c_str());
			// get number of faces
			if (line_split.size() > 2 && line_split[0].compare("element") == 0 && line_split[1].compare("face") == 0)
				ply->n_faces = std::atoi(line_split[2].c_str());
		}
		else
			break;
	}
	// read points, segm labels and colors
	// read first line
	bool has_segm_label = false;
	bool has_color = false;
	std::getline(infile, line);
	
	line_split = splitstr(line, ' ');
	if (line_split.size() < 3) {
		std::cout << "Error reading point #" << 0 << std::endl;
		return 0;
	}
	// add point
	ply->points = new double*[ply->n_points];
	ply->points[0] = new double[3];
	for (int j=0;j<=2;j++) {		
		ply->points[0][j] = std::atof(line_split[j].c_str());
	}
	// add segm label	
	std::vector<int> unique_segm_labels;
	int n_segms = 0;
	if (line_split.size() > 3) {
		ply->segm_labels = new int[ply->n_points];
		has_segm_label = true;
		ply->segm_labels[0] = std::atoi(line_split[3].c_str());
		// add unique segm label		
		unique_segm_labels.push_back(ply->segm_labels[0]);
		n_segms++;
	}
	// add color
	ply->colors = NULL;
	if (line_split.size() > 6) {		
		has_color = true;
		ply->colors = new int*[ply->n_points];
		ply->colors[0] = new int[3];
		for (int j=4;j<=6;j++)
			ply->colors[0][j-4] = std::atoi(line_split[j].c_str());
	}

	// run from the second point to the last, gathering points, segm and color
	for (int i=1;i<=ply->n_points-1;i++) {		
		std::getline(infile, line);
		
		line_split = splitstr(line, ' ');		
		if (line_split.size() < 3) {
			std::cout << "Error reading point #" << i << std::endl;
			return 0;
		}
		// add point
		ply->points[i] = new double[3];
		for (int j=0;j<=2;j++)
			ply->points[i][j] = std::atof(line_split[j].c_str());
		// add segm label		
		if (has_segm_label)	{
			ply->segm_labels[i] = std::atoi(line_split[3].c_str());
			// add unique segm label
			if (std::find(unique_segm_labels.begin(), unique_segm_labels.end(), ply->segm_labels[i]) == unique_segm_labels.end()) {
				unique_segm_labels.push_back(ply->segm_labels[i]);
				n_segms++;
			}
		}
		// add color
		if (has_color)
			for (int j=4;j<=6;j++) {
				ply->colors[i] = new int[3];
				ply->colors[i][j-4] = std::atoi(line_split[j].c_str());		
			}
	}
	
	if(has_segm_label)
		ply->n_segms = n_segms;
	else
		ply->n_segms = 0;	 
	
	// run a second pass to get the number of points per segm label
	ply->points_per_segm_label = NULL;
	if (has_segm_label)	{
		ply->points_per_segm_label = new int[n_segms];
		for (int i=0;i<=ply->n_segms-1;i++) {
			ply->points_per_segm_label[i] = 0;
		}
		for (int i=0;i<=ply->n_points-1;i++)
			ply->points_per_segm_label[ply->segm_labels[i]]++;
	}

	
	
	// read faces
	// read first face and get number of points per face
	std::getline(infile, line);

	line_split = splitstr(line, ' ');
	// get number of points per face	
	const int n_points_per_face = line_split.size() - 1;
	ply->n_points_per_face = n_points_per_face;
	// allocate faces
	ply->faces = new int*[ply->n_faces];	
	// add points to first face
	ply->faces[0] = new int[3];
	for (int j=1;j<=line_split.size()-1;j++) {		
		ply->faces[0][j-1] = std::atoi(line_split[j].c_str());
	}
	for (int i=1;i<=ply->n_faces-1;i++) {
		std::getline(infile, line);
		std::istringstream iss(line);		
		line_split = splitstr(line, ' ');
		// add points
		ply->faces[i] = new int[3];
		for (int j=1;j<=line_split.size()-1;j++) {
			ply->faces[i][j-1] = std::atoi(line_split[j].c_str());			
		}
	}	
	infile.close();
	return ply;
}

// write a ply header
void write_ply_header(std::ofstream & output, int n_segms, int n_points, int n_faces, bool color) {	
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
	if (n_segms > 0)
		output << "property int segm" << std::endl;
	if (color) {
		output << "property uchar red" << std::endl;
		output << "property uchar green" << std::endl;
		output << "property uchar blue" << std::endl;
	}
	if (n_faces > 0) {
		output << "element face " << n_faces << std::endl;
		output << "property list uchar int vertex_indices" << std::endl;
	}
	output << "end_header" << std::endl;
}

void write_ply(PLY *ply, std::string output_filepath) {
	// get stream output
	std::ofstream output(output_filepath.c_str());
	// write header
	write_ply_header(output, ply->n_segms, ply->n_points, ply->n_faces, ply->colors != NULL);
	// write points, segm labels and colors
	for (int i=0;i<=ply->n_points-1;i++)
		output << ply->points[i][0] << " " << ply->points[i][1] << " " << ply->points[i][2] << " " << ply->segm_labels[i] << " " << ply->colors[i][0] << " " << ply->colors[i][1] << " " << ply->colors[i][2] << std::endl;
	// write faces
	for (int i=0;i<=ply->n_faces-1;i++) {
		output << ply->n_points_per_face;
		for (int j=0;j<=2;j++)
			output << " " << ply->faces[i][j];
		output << std::endl;
	}
	output.close();
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

// get the label of the first small segm
int get_small_segm_label(PLY *ply, double max_prop) {
	int small_segm_label = -1;
	double prop_segm_size = 0;
	for (int i=0;i<=ply->n_segms-1;i++) {
		// there needs to be at lest one point for that segment
		if (ply->points_per_segm_label[i] > 0) {
			prop_segm_size = (double)ply->points_per_segm_label[i]/ply->n_points;
			if (prop_segm_size < max_prop) {
				small_segm_label = i;
				break;
			}
		}	
	}	
	if (small_segm_label >= 0) {
		if (VERBOSE) std::cout << "Small segment label: " << small_segm_label << std::endl;
		if (VERBOSE) std::cout << "Segment proportional size: " << prop_segm_size << std::endl;
	}
	else
		if (VERBOSE) std::cout << "There are no small segments" << std::endl;
	return small_segm_label;
}

// get ix of the closest neighbour from another segment for all points
int * get_closest_diff_neighbour(PLY *ply) {
	// initialize
	int *closest_diff_neighbour = new int[ply->n_points];	
	// get the ix of the closest neighbour for each point
	for (int i=0;i<=ply->n_points-1;i++) {
		double min_dist = 1e6;
		// run through every point from another segment (ignoring itself)
		for (int j=0;j<=ply->n_points-1;j++) {
			if (i != j && ply->segm_labels[i] != ply->segm_labels[j]) {
				// get dist between points
				double dist = std::sqrt(std::pow(ply->points[i][0] - ply->points[j][0], 2) + std::pow(ply->points[i][1] - ply->points[j][1], 2) + std::pow(ply->points[i][2] - ply->points[j][2], 2));
				if (dist < min_dist) {
					min_dist = dist;
					closest_diff_neighbour[i] = j;
				}
			}				
		}		
	}
	return closest_diff_neighbour;
}

// fuse segm into others (segm is a segm label)
void fuse_segm_into_others(PLY *ply, int small_segm_label, int *closest_diff_neighbour) {		
	for (int i=0;i<=ply->n_points-1;i++) {
		if (ply->segm_labels[i] == small_segm_label) {
			ply->segm_labels[i] = ply->segm_labels[closest_diff_neighbour[i]];
			ply->points_per_segm_label[ply->segm_labels[i]]++;
		}
	}	
	// update labels (left shifting label values > small_segm_label)
	for (int i=0;i<=ply->n_points-1;i++) {
		if (ply->segm_labels[i] > small_segm_label)
			ply->segm_labels[i]--;
	}	
	// delete small segm label
	int *new_segm_labels = new int[ply->n_segms-1];
	for (int i=0;i<small_segm_label;i++)
		new_segm_labels[i] = ply->points_per_segm_label[i];
	for (int i=small_segm_label+1;i<=ply->n_segms-1;i++)
		new_segm_labels[i-1] = ply->points_per_segm_label[i];
	delete [] ply->points_per_segm_label;
	ply->points_per_segm_label = new_segm_labels;
	std::cout << std::endl;
	// update number of segments
	ply->n_segms--;
}

void update_ply_color(PLY *ply) {
	for (int i=0;i<=ply->n_points-1;i++) {
		int *color = get_color(ply->segm_labels[i]);
		for (int j=0;j<=2;j++) {			
			ply->colors[i][j] = color[j];
		}
		delete color;
	}
}

void print_ply_info(PLY *ply) {
	if (VERBOSE) {
		std::cout << "PLY info:" << std::endl;
		std::cout << "\tNumber of points: " << ply->n_points << std::endl;
		std::cout << "\tNumber of faces: " << ply->n_faces << std::endl;
		std::cout << "\tNumber of segments: " << ply->n_segms << std::endl;		
		std::cout << "\tPoints per segment label: ";			
		int sum_points_per_segm_label = 0;
		for (int i=0;i<=ply->n_segms-1;i++) {
			std::cout << ply->points_per_segm_label[i] << " ";
			sum_points_per_segm_label += ply->points_per_segm_label[i];
		}
		std::cout << std::endl;
		std::cout << "\tSum of points per segment label: " << sum_points_per_segm_label << std::endl;
		std::cout << std::endl;
	}
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


// segment every mesh in a given directory
int main(int argc, char *argv[]){
	// parse args
	if (!parse_args(argc, argv))
		return 0;
	
	// make output directory
	if (VERBOSE) std::cout << "Creating directory " << OUTPUT_DIR << std::endl;
	std::string command_mkdir = "mkdir " + OUTPUT_DIR;	
	system(command_mkdir.c_str());
		
	//get ply filenames sorted (ascend)
	std::vector<std::string> pcl_filenames;
	if(!get_ply_filenames(INPUT_DIR, pcl_filenames, true))
		return 0;
	
	// run through each mesh in INPUT_DIR (getting SDF values only once)
	// 		run through k number of clusters
	//			run through smoothing lambdas
	//				segment and write .OFF file
	double tot_toc_per_pcl = 0;
	int curr_ix = 0;
	for(std::vector<std::string>::iterator it = pcl_filenames.begin(); it != pcl_filenames.end(); ++it) {
		// for outputting estimated time
		curr_ix++;
		std::clock_t tic = std::clock();
		
		// get filepath to write to
		std::string pcl_filename = *it;
		std::string pcl_filepath = INPUT_DIR + pcl_filename;
		
		// read PLY		
		if (VERBOSE) std::cout << "Reading .PLY file: " << pcl_filepath << std::endl;

		PLY *ply = read_ply(pcl_filepath);
		// print PLY info
		print_ply_info(ply);
		
		if (VERBOSE) std::cout << "Fusing small segments into others" << std::endl;
		
		int small_segm_label = get_small_segm_label(ply, 0.05);
		bool has_small_segms = small_segm_label >= 0;
		
		// calculate indexes of closest neighbours
		int *closest_diff_neighbour = get_closest_diff_neighbour(ply);

		while (has_small_segms) {			
			fuse_segm_into_others(ply, small_segm_label, closest_diff_neighbour);
			print_ply_info(ply);
			small_segm_label = get_small_segm_label(ply, 0.05);			
			has_small_segms = small_segm_label >= 0;			
		}
		delete [] closest_diff_neighbour;
		//update color
		update_ply_color(ply);
		// write PLY
		std::string output_filepath = add_suffix_filename(OUTPUT_DIR + pcl_filename, "_fused");
		if (VERBOSE) std::cout << "Writing .PLY file: "  << output_filepath << std::endl;
		write_ply(ply, output_filepath);
		// destruct PLY 
		for(int i = 0; i < ply->n_points; i++) {
			delete [] ply->points[i];
		}
		delete [] ply->points;
		delete [] ply->segm_labels;
		delete [] ply->colors;
		delete [] ply->points_per_segm_label;
		for(int i = 0; i < ply->n_faces; i++) {
			delete [] ply->faces[i];
		}
		delete [] ply->faces; 
		delete ply;
		if (VERBOSE) std::cout << std::endl;
		if (VERBOSE) {
			tot_toc_per_pcl += double(std::clock() - tic) / CLOCKS_PER_SEC;
			std::cout << std::endl;
			DisplayEstimatedTimeOfLoop( tot_toc_per_pcl, curr_ix, pcl_filenames.size(), "Loop per point cloud ");
			std::cout << std::endl;
		}
	}
}













