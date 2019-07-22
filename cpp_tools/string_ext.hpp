#ifndef STRING_EXT_HPP
#define STRING_EXT_HPP

#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <sstream>
using namespace std;

bool has_suffix(const string &str, const string &suffix);

bool has_prefix(const string &str, const string &prefix);

string basename(string filename);
string descriptive(string filename);

pair<string,string> split_filename(string filename);
pair<string,string> split_ext(string filename);

string join(string dir, string basename);

string add_filename_prefix(string filename, string prefix);

vector<string> split_string(const string& s, char delimiter);

string get_part(string filename, string part, char delimiter);

#endif