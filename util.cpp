#include "util.h"

using namespace std;

/*
    Helper function which given a string and a delimiter, creates a iterator of the split tokens
    Sourced from: https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
    Need to use longer version bc: https://stackoverflow.com/questions/53582365/regex-token-iterator-attempting-to-reference-a-deleted-function
*/
vector<string> split(const string str, const string regex_str)
{
    regex regexz(regex_str);
    vector<string> list(sregex_token_iterator(str.begin(), str.end(), regexz, -1), sregex_token_iterator());
    return list;
}

/*
    Helper function to remove whitespace from string on both ends in place
    Sourced from: https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
*/
void trim(string &s)
{
    // trim from start
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !isspace(ch);
            }));

    // trim from end
    s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !isspace(ch);
            }).base(),
            s.end());
}

/*
    Helper function to run trim on all elements of a vector
*/
void trim_vector(vector<string> &vec)
{
    for (auto &v : vec)
    {
        trim(v);
    }
}

/*
    Helper function to remove ending semicolon
*/
void remove_ending_semicolon(string &s)
{
    if (s.find(";") == s.length() - 1)
    {
        s.pop_back();
    }
}

/*
    Helper function to remove ending semicolon from vector
*/
void remove_ending_semicolon_vector(vector<string> &vec)
{
    for (auto &v : vec)
    {
        remove_ending_semicolon(v);
    }
}

/*
    Helper function to get s[l:r] substring where l is inclusive and r is exclusive
*/
string substr_between_indices(const string s, int l, int r)
{
    return s.substr(0, r).substr(l);
}

/*
    Helper function to determine if a string is a substring of another
*/
bool is_substr(const string s1, const string s2)
{
    return s1.find(s2) != string::npos;
}

/*
    Helper function to determine if a line of code is accessing an array
*/
bool is_array_accessor(const string s)
{
    return is_substr(s, "[") && is_substr(s, "]");
}

/*
    Helper function to determine if a line of code is accessing an array element via another variable
    i.e a[x] returns true while a[0] returns false
*/
bool is_array_accessor_dynamic(const string s)
{
    return is_substr(s, "[") && is_substr(s, "]") && !is_int(substr_between_indices(s, s.find("[") + 1, s.find("]")));
}

/*
    Helper function to determine if string is int
    Sourced from: https://stackoverflow.com/questions/2844817/how-do-i-check-if-a-c-string-is-an-int
*/
bool is_int(const string s)
{
    return s.find_first_not_of("0123456789") == string::npos;
}

/*
    Helper function to read a file and load all its contents
    line-by-line into a vector.
*/
vector<string> loadFile(string filename, int &maxlen)
{
    ifstream inputFile;
    inputFile.open(filename);
    string inputLine;

    if (inputFile.fail())
    {
        cout << "Failed to open file." << endl;
    }
    else
    {
        cout << "Opened " + filename + " successfully." << endl;
    }

    vector<string> sourceCode;
    while (!inputFile.eof())
    {
        getline(inputFile, inputLine);
        trim(inputLine);
        // cout << inputLine << endl;
        sourceCode.push_back(inputLine);
        maxlen++;
    }

    inputFile.close();

    return sourceCode;
}

/*
    Helper function to write assembly instruction to a file
    Add tabs before instruction as spacing
    Leaves labels as if
    Adds tab between operation and first operand
    Adds 2 tabs for jump instructions
*/
void writeFile(string filename, vector<string> assembly, string f_name)
{
    ofstream fileOUT(filename, ios::app); // open filename.txt in append mode

    for (string s : assembly)
    {
        if (is_substr(s, f_name) || s.find(".L", 0) == 0)
        {
            fileOUT << s << endl; // append "some stuff" to the end of the file
        }
        else if (is_substr(s, "#"))
        {
            fileOUT << "\t\t" << s << endl;
        }
        else
        {
            int space_idx = s.find(" ");
            if (space_idx != string::npos)
            {
                s.replace(space_idx, 1, "\t");
                if (is_substr(s, ".L"))
                {
                    s.insert(space_idx, 1, '\t');
                }
                fileOUT << "\t\t" << s << endl;
            }
            else
            {
                fileOUT << "\t\t" << s << endl;
            }
        }
    }

    fileOUT.close(); // close the file
}