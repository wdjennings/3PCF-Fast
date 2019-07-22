// Extensions for strings 
#include "string_ext.hpp"

bool has_suffix(const string &str, const string &suffix){
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool has_prefix(const string &str, const string &prefix){
	return mismatch(prefix.begin(),prefix.end(),str.begin()).first==prefix.end();
}

string basename(string filename){
    size_t index = filename.find_last_of("/");
    return filename.substr(index+1, string::npos); ;
}

string descriptive(string filename){
  string basename_st = basename(filename);
  return split_ext(basename_st).first;
}

pair<string,string> split_filename(string filename){
    size_t index = filename.find_last_of("/");
    string dir = filename.substr(0, index); 
    string basename = filename.substr(index+1, string::npos);
    return make_pair(dir, basename);
}

pair<string,string> split_ext(string filename){
    size_t index = filename.find_last_of(".");
    string descr = filename.substr(0, index); 
    string ext = filename.substr(index+1, string::npos);
    return make_pair(descr, ext);
}

string join(string dir, string basename){
	if (has_suffix(dir,"/")){
		return dir + basename;
	} else {
		return dir + "/" + basename;
	}
}

vector<string> split_string(const string& s, char delimiter){
   vector<string> tokens;
   string token;
   istringstream tokenStream(s);
   while (getline(tokenStream, token, delimiter)){
      tokens.push_back(token);
   }
   return tokens;
}

string get_part(string filename, string part, char delimiter){
	string descriptive_part = descriptive(filename);
	vector<string> parts = split_string(descriptive_part, delimiter);
	for(vector<string>::iterator it  = parts.begin(); it  != parts.end(); ++it ) {
		string part_value = *it;
		if (has_prefix(part_value, part)){ 
			return part_value.substr(part.length(), string::npos); 
		}
	}
	return "";
}

string add_filename_prefix(string filename, string prefix){
	pair<string,string> parts = split_filename(filename);
	return join(parts.first, prefix + parts.second);
}
