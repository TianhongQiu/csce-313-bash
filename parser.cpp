#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>

using namespace std;

vector<string> remove_whitespace(vector<string> parsed_line){
    for (int i = 0; i < parsed_line.size(); i++){
        if (parsed_line[i] == "")
            parsed_line.erase(parsed_line.begin() + i);
    }
    return parsed_line;
}

vector<string> parse_line(string line){
    vector<string> parsed_string = {""};
    bool single_quote = false;
    bool double_quote = false;
    int i = 0;
    for (char c : line){
        if (c == ' ' && !single_quote && !double_quote){
            parsed_string.push_back("");
            i++;
        }
        else if ((c == '|' || c == '<' || c == '>') && !single_quote && !double_quote){
            parsed_string.push_back(string(1,c));
            i++;
            parsed_string.push_back("");
            i++;
        }
        else if (c == '\'' && !single_quote) single_quote = true;
        else if (c == '\'' && single_quote) single_quote = false;
        else if (c == '\"' && !double_quote) double_quote = true;
        else if (c == '\"' && double_quote) double_quote = false;
        else
            parsed_string[i] += c;
    }
    return remove_whitespace(parsed_string);
}