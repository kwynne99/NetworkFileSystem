#include "helper.h"

class disc_em {
public:
    disc_em(string fname);
    //creates disc.txt if it doesn't exist. Otherwise loads text into disc string
    string append(string fName, string text); //adds to an existing file if there is enough space
    //usage: append fileName textToAppend
    string touch(string fname, int num); //allocates given number of blocks for a file
    //usage:touch fileName #blocks
    string mkdir(string fname); //creates a directory
    string ls(); //displays items in current directory
    string cd(string dName); //switches to given directory. Does no accept a list of directories
    //usage: cd directoryName
    //you can't do cd d1/d2/d3
    string vi(string fName, string text); //replaces text of a file with text given to the command
    //old text is lost
    //usage: vi fileName newText
    string print(); //prints the disc
    ~disc_em(); //writes all changes to disc.txt
private:
    string disc;
    string fileName;
    int headPtr = 0;
    stack<int> path;
    vector<int> fOS = { 300, 80, 20, 8 };
    vector<int> nFat = { 1200, 220, 30, 12 };
};



disc_em::disc_em(string fname) {
    string temp_str;
    fileName = fname;
    ifstream read(fileName);
    while (getline(read, temp_str)) {
        disc += temp_str;
    }
    read.close();
    path.push(0);
    if (disc == "") {
        for (int i = 0; i < 1500; i++) {
            disc += '@';
        }
    }
}

string disc_em::append(string fName, string text) {
    for (int i = headPtr; i < (headPtr + fOS[path.size() - 1]); i++) {
        if (disc[i] == '^') {
            if (disc.substr(i + 1, fName.length()) == fName) {
                string start = "";
                i += (fName.length() + 2);
                while (isdigit(disc[i])) {
                    start = start + disc[i];
                    i++;
                }
                string end = "";
                i++;
                while (isdigit(disc[i])) {
                    end = end + disc[i];
                    i++;
                }
                string oldText = disc.substr((headPtr + fOS[path.size() - 1] + stoi(start)), (stoi(end) - stoi(start)));
                oldText = deblockify(oldText);
                int limit = oldText.length();
                oldText = removeEmpties(oldText);
                text = oldText + text;
                //cout<<text<<endl;
                if (text.length() > limit) {
                    return "New Text is too long.\n";
                }
                else {
                    while (text.length() < limit) {
                        text += "@";
                    }
                    text = blockifyText(text);
                    for (int i = (headPtr + fOS[path.size() - 1] + stoi(start)); i <= (headPtr + fOS[path.size() - 1] + stoi(end)); i++) {
                        disc[i] = text[i - (headPtr + fOS[path.size() - 1] + stoi(start))];
                    }
                    return "Text was modified\n";
                }
                i = disc.length();
            }
        }
        else if (disc[i] == '@') {
            i = disc.length();
            return "File not found.\n";
        }
    }
    return "File not found.\n";
}

string disc_em::touch(string fname, int num) {
    string text = "";
    for (int i = 0; i < num; i++) {
        text += "@@";
    }
    if (noReserved(fname)) {
        int emptyFatSpaces = 0;
        for (int i = headPtr; i < (headPtr + fOS[path.size() - 1]); i++) {
            if (disc[i] == '@') {
                emptyFatSpaces++;
            }
        }
        string start = "";
        for (int i = headPtr; i < (headPtr + fOS[path.size() - 1]); i++) {
            if (disc[i] == '@') {
                int last = i - 1;
                if (last < headPtr) {
                    start = "-1";
                }
                else {
                    while (isdigit(disc[last])) {
                        start = disc[last] + start;
                        last--;
                    }
                }
                i = disc.length();
            }
        }
        int begin = stoi(start) + 1;
        text = blockifyText(text);
        int end = begin + text.length() - 1;
        if (end < nFat[path.size() - 1]) {
            fname = fatifyFileName(fname, begin, end);
            int fnSize = fname.length();
            int countDown = fnSize;
            if (emptyFatSpaces >= fnSize) {
                for (int i = headPtr; i < (headPtr + fOS[path.size() - 1]); i++) {
                    if (disc[i] == '@' && countDown != 0) {
                        disc[i] = fname[fnSize - countDown];
                        countDown--;
                    }
                    else if (countDown == 0) {
                        i = disc.length();
                    }
                }
                for (int i = (headPtr + fOS[path.size() - 1] + begin); i <= (headPtr + fOS[path.size() - 1] + end); i++) {
                    disc[i] = text[i - (headPtr + fOS[path.size() - 1] + begin)];
                }
                return "File was added\n";
            }
            else {
                return "File name too long. Try Shortening it.\n";
            }
        }
        else {
            return "Too much text. Try reducing the amount of text\n";
        }
    }
    else {
        return "Reserved characters in file name or file text\n";
    }
}

string disc_em::mkdir(string fname) {
    if (path.size() >= 4) {
        return "Mkdir no longer possible.\n";
    }
    else if (noReserved(fname)) {
        int emptyFatSpaces = 0;
        for (int i = headPtr; i < (headPtr + fOS[path.size() - 1]); i++) {
            if (disc[i] == '@') {
                emptyFatSpaces++;
            }
        }
        string start = "";
        for (int i = headPtr; i < (headPtr + fOS[path.size() - 1]); i++) {
            if (disc[i] == '@') {
                int last = i - 1;
                if (last < headPtr) {
                    start = "-1";
                }
                else {
                    while (isdigit(disc[last])) {
                        start = disc[last] + start;
                        last--;
                    }
                }
                i = disc.length();
            }
        }
        int begin = stoi(start) + 1;
        int end = begin + fOS[path.size() - 1] - 1;
        if (end < nFat[path.size() - 1]) {
            fname = fatifyDirName(fname, begin, end);
            int fnSize = fname.length();
            int countDown = fnSize;
            if (emptyFatSpaces >= fnSize) {
                for (int i = headPtr; i < (headPtr + fOS[path.size() - 1]); i++) {
                    if (disc[i] == '@' && countDown != 0) {
                        disc[i] = fname[fnSize - countDown];
                        countDown--;
                    }
                    else if (countDown == 0) {
                        i = disc.length();
                    }
                }
                return "Directory was added.\n";
            }
            else {
                return "Directory name too long. Try Shortening it.\n";
            }
        }
        else {
            return "Not enough space to create a directory.\n";
        }
    }
    else {
        return "Reserved characters in directory name.\n";
    }
}

string disc_em::ls() {
    string result = "";
    vector<string> items;
    for (int i = headPtr; i < (headPtr + fOS[path.size() - 1]); i++) {
        if (disc[i] == '@') {
            i = disc.length();
        }
        else if (disc[i] == '^' || disc[i] == '&') {
            string item;
            if (disc[i] == '^') {
                item += "f:";
            }
            else {
                item += "d:";
            }
            i++;
            while (disc[i] != '|') {
                item += disc[i];
                i++;
            }
            items.push_back(item);
        }
    }
    for (string x : items) {
        result = result + x + " ";
    }
    if (result == "") return "No items found.\n";
    return result + "\n";
}

string disc_em::cd(string dName) {
    if (dName == "..") {
        if (path.size() == 1) {
            return "You are in root. You cannot go back anymore.\n";
        }
        else {
            path.pop();
            headPtr = path.top();
            return "You went back.\n";
        }
    }
    else {
        for (int i = headPtr; i < (headPtr + fOS[path.size() - 1]); i++) {
            if (disc[i] == '&') {
                if (disc.substr(i + 1, dName.length()) == dName) {
                    string start = "";
                    i += (dName.length() + 2);
                    while (isdigit(disc[i])) {
                        start = start + disc[i];
                        i++;
                    }
                    i = disc.length();
                    headPtr = headPtr + fOS[path.size() - 1] + stoi(start);
                    path.push(headPtr);
                    return "Moved to chosen directory.\n";
                }
            }
            else if (disc[i] == '@') {
                i = disc.length();
            }
        }
        return "Directory not found.\n";
    }
}

string disc_em::vi(string fName, string text) {
    for (int i = headPtr; i < (headPtr + fOS[path.size() - 1]); i++) {
        if (disc[i] == '^') {
            if (disc.substr(i + 1, fName.length()) == fName) {
                string start = "";
                i += (fName.length() + 2);
                while (isdigit(disc[i])) {
                    start = start + disc[i];
                    i++;
                }
                string end = "";
                i++;
                while (isdigit(disc[i])) {
                    end = end + disc[i];
                    i++;
                }
                //cout<<start<<" "<<end<<endl;
                string oldText = disc.substr((headPtr + fOS[path.size() - 1] + stoi(start)), (stoi(end) - stoi(start)));
                //cout<<oldText<<endl;
                oldText = deblockify(oldText);
                //cout<<text<<endl<<oldText<<endl;
                if (text.length() > oldText.length()) {
                    return "New Text is too long.\n";
                }
                else {
                    while (text.length() < oldText.length()) {
                        text += "@";
                    }
                    text = blockifyText(text);
                    for (int i = (headPtr + fOS[path.size() - 1] + stoi(start)); i <= (headPtr + fOS[path.size() - 1] + stoi(end)); i++) {
                        disc[i] = text[i - (headPtr + fOS[path.size() - 1] + stoi(start))];
                    }
                    return "Text was modified\n";
                }
                i = disc.length();
            }
        }
        else if (disc[i] == '@') {
            i = disc.length();
            return "File not found.\n";
        }
    }
    return "File not found.\n";
}

string disc_em::print() {
    string temp;
    for (int i = 0; i < disc.length(); i += 150) {
        temp += (disc.substr(i, 150) + "\n");
    }
    //cout << endl;
    return temp;
}

disc_em::~disc_em() {
    ofstream write(fileName);
    for (int i = 0; i < disc.size(); i += 150) {
        write << disc.substr(i, 150) << endl;
    }
    write.close();
}

/*int main() {
    string command;
    help();
    cout << endl;
    disc_em disc = disc_em("disc.txt");
    bool operating = true;
    while (operating) {
        cout << "Enter a command\n";
        getline(cin, command);
        if (command.substr(0, 5) == "touch") {
            string fname = "", text = "";
            int i = 6;
            while (command[i] != ' ') {
                fname += command[i];
                i++;
            }
            i++;
            while (i < command.length()) {
                text += command[i];
                i++;
            }
            cout << disc.touch(fname, stoi(text));
            disc.print();
        }
        if (command.substr(0, 6) == "append") {
            string fname = "", text = "";
            int i = 7;
            while (command[i] != ' ') {
                fname += command[i];
                i++;
            }
            i++;
            while (i < command.length()) {
                text += command[i];
                i++;
            }
            cout << disc.append(fname, text);
            disc.print();
        }
        else if (command.substr(0, 5) == "mkdir") {
            string dname = "";
            int i = 6;
            while (i < command.length()) {
                dname += command[i];
                i++;
            }
            cout << disc.mkdir(dname);
            disc.print();
        }
        else if (command.substr(0, 2) == "ls") {
            cout << disc.ls();
        }
        else if (command.substr(0, 2) == "cd") {
            string dName = "";
            for (int i = 3; i < command.length(); i++) {
                dName += command[i];
            }
            cout << disc.cd(dName);
        }
        else if (command.substr(0, 2) == "vi") {
            string fname = "", text = "";
            int i = 3;
            while (command[i] != ' ') {
                fname += command[i];
                i++;
            }
            i++;
            while (i < command.length()) {
                text += command[i];
                i++;
            }
            cout << disc.vi(fname, text);
            disc.print();
        }
        else if (command == "exit") {
            operating = false;
            cout << "Disc turning off\n";
        }
        else if (command == "help") {
            help();
        }
        else if (command == "print") {
            disc.print();
        }
        cout << endl;
    }
    // disc.mkdir("stuff");
    // disc.touch("name","john, jan, jill, jason");
    // disc.touch("fruits", "apples, oranges, plums");
    // disc.touch("sounds","aahh, ooh, eeeek");
    // disc.vi("sounds", "froi, vik, plapo");
    // cout<<disc.ls();
    // cout<<disc.cd("stuff");
    // disc.touch("name","john, jan, jill, jason");
    // disc.touch("fruits", "apples, oranges, plums");
    // disc.touch("sounds","aahh, ooh, eeeek");
    // disc.vi("fruits", "peachs, arongse, lumps");
    // cout<<disc.ls();
}*/
