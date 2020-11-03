#include<iostream>
#include<fstream>
#include<stack>
#include<string>
#include<vector>
using namespace std;

#ifndef HELPER_H_
#define HELPER_H_

bool noReserved(string text) {
    for (char c : text) {
        if (c == '^' || c == '&' || c == '|' || c == '*' ||
            c == '/' || c == '@' || isdigit(c)) {
            return false;
        }
    }
    return true;
}

string fatifyFileName(string name, int start, int end) {
    return "^" + name + "|" + to_string(start)
        + "*" + to_string(end);
}

string fatifyDirName(string name, int start, int end) {
    return "&" + name + "|" + to_string(start)
        + "*" + to_string(end);
}

string blockifyText(string text) {
    string result;
    for (int i = 0; i < text.length(); i += 2) {
        result += text[i];
        if (i + 1 < text.length() && i + 1 != text.length() - 1) {
            result += text[i + 1];
            result += '/';
        }
        else if (i + 1 < text.length()) {
            result += text[i + 1];
        }
        else {
            result += '@';
        }
    }
    return result += "/";
}

string removeEmpties(string text) {
    string result = "";
    for (int i = 0; i < text.length(); i++) {
        if (text[i] != '@') {
            result += text[i];
        }
    }
    return result;
}

string deblockify(string text) {
    string result;
    for (int i = 0; i < text.length(); i++) {
        if (text[i] != '/') {
            result += text[i];
        }
    }
    return result;
}

string help() {
    char help[] = "Please do not use the following characters: ^ & | * @\nAlso please do not use whitespace or numbers in file or directory names.\nUsage:\ntouch filename numblocks\n mkdir filename\ncd directoryname\n vi filename newFileText\nappend filename textToAppend\nprint: this will print the file contents to the screen\nType help to show this message again\n";
    /*
    cout << "Please do not use the following characters: ^ & | * @\n";
    cout << "also please do not put whitespace or numbers in file or directory names\n";
    cout << "usage:\n";
    cout << "touch filename numblocks\n";
    cout << "mkdir filename\n";
    cout << "cd directoryname\n";
    cout << "vi filename newfiletext\n";
    cout << "append filename textToAppend\n";
    cout << "print      this will print the file contents to the screen\n";
    cout << "input help to show this again\n";
    */
    return help;
}

#endif
