/*
 * ============================================================
 *   CAMPUS-SPHERE  —  College ERP System
 *   Standard: C++17  |  No external libraries
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <map>
#include <cstdlib>
#include <algorithm>
#include <iomanip>
#include <cctype>
#include <filesystem>
namespace fs = std::filesystem;

using namespace std;

// ============================================================
// Path Constants
// ============================================================
const string STUDENT_PATH = "data/students/";
const string FACULTY_PATH = "data/faculty/";
const string BATCHES_PATH = "data/batches/";
const string REPORTS_PATH = "data/reports/";
const string COURSES_PATH = "data/courses/";
const string COURSES_FILE = "data/courses/courses.txt";

// ============================================================
// UI Helpers
// ============================================================

void printLine(char c = '-', int len = 60) {
    cout << string(len, c) << "\n";
}

void printHeader(const string &title) {
    printLine('=');
    int pad = (60 - (int)title.size()) / 2;
    cout << string(max(0,pad), ' ') << title << "\n";
    printLine('=');
}

void printSubHeader(const string &title) {
    printLine('-');
    cout << "  " << title << "\n";
    printLine('-');
}

string padString(const string &str, size_t length) {
    if (str.length() < length)
        return str + string(length - str.length(), ' ');
    return str.substr(0, length);
}

// ============================================================
// Utility: XOR encryption (unchanged from original)
// ============================================================
string encryptDecrypt(const string &pass) {

    string result = pass;
    char key = 'X';
    for (char &c : result) c ^= key;

    return result;
}

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// creates directories
void ensureDir(const string &path) {
    fs::create_directories(path);
}

// ============================================================
// Directory scan
// ============================================================

// scans .txt files in a folder
vector<string> listFiles(const string &dirPath) {
    vector<string> files;
    if (!fs::exists(dirPath)) return files;
    for (const auto &entry : fs::directory_iterator(dirPath)) {
        if (entry.path().extension() == ".txt")
            files.push_back(entry.path().filename().string());
    }
    sort(files.begin(), files.end());
    return files;
}

// ============================================================
// Structures
// ============================================================

struct AttendanceRecord {
    string date;
    string classType;
    string course;
    string status;
};

struct MarksRecord {
    string course;
    string examType;
    string examDate;
    int    marksObtained = 0;
    int    semester      = 1;
};

struct GradeRecord {
    string subject;
    int    credits    = 0;
    string grade;
    double gradePoint = 0.0;
    int    semester   = 1;
};

struct RankEntry {
    string name;
    string rollNo;
    double avgMarks   = 0.0;
    double attendance = 0.0;
    int    rank       = 0;
};

struct Course {
    string code;
    string name;
    int    credits       = 3;
    string assignedFacID;
};

// ============================================================
// Grade helpers
// ============================================================
double gradeToPoint(const string &g) {
    if (g == "A+")  return 10.0;
    if (g == "A") return 9.0;
    if (g == "B+")  return 8.0;
    if (g == "B") return 7.0;
    if (g == "C+")  return 6.0;
    if (g == "C")  return 5.0;
    return 0.0;
}

// ============================================================
// Course I/O
// ============================================================
vector<Course> loadAllCourses() {
    vector<Course> courses;
    ifstream f(COURSES_FILE);
    if (!f.is_open()) return courses;
    string line;
    while (getline(f, line)) {
        if (line.empty()) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        istringstream iss(line);
        string tok;
        Course c;
        if (getline(iss, tok, '|')) c.code = tok;
        if (getline(iss, tok, '|')) c.name = tok;
        if (getline(iss, tok, '|')) { try { c.credits = stoi(tok); } catch(...){} }
        if (getline(iss, tok, '|')) c.assignedFacID = tok;
        if (!c.code.empty()) courses.push_back(c);
    }
    return courses;
}

void saveAllCourses(const vector<Course> &courses) {
    ensureDir(COURSES_PATH);
    ofstream f(COURSES_FILE, ios::trunc);
    for (const auto &c : courses)
        f << c.code << "|" << c.name << "|" << c.credits << "|" << c.assignedFacID << "\n";
}

// ============================================================
// Abstract Base: User
// ============================================================
class User {
protected:
    string userID;
    string password;
    string name;
    int    age = 0;
    string phone;
    string email;
public:
    User() = default;
    User(const string &id, const string &pass)
        : userID(id), password(encryptDecrypt(pass)) {}

    string getUserID()  const { return userID; }
    string getPassword()const { return password; }
    string getName()    const { return name; }
    int    getAge()     const { return age; }
    string getPhone()   const { return phone; }
    string getEmail()   const { return email; }

    void setName (const string &n) { name  = n; }
    void setAge  (int a)           { age   = a; }
    void setPhone(const string &p) { phone = p; }
    void setEmail(const string &e) { email = e; }

    bool authenticate(const string &id, const string &pass) const {
        return (userID == id && encryptDecrypt(password) == pass);
    }

    virtual void changePassword(const string &newPass) {
        password = encryptDecrypt(newPass);
        cout << "Password changed successfully!\n";
    }

    virtual void saveToFile()                         = 0;
    virtual void loadFromFile(const string &filename) = 0;
    virtual void displayProfile()                     = 0;
    virtual ~User() = default;
};

// ============================================================
// Derived Class: Student
// ============================================================
class Student : public User {
private:
    string                   batch;
    vector<AttendanceRecord> attendance;
    vector<MarksRecord>      marks;
    vector<GradeRecord>      grades;
    vector<string>           registeredCourses;

public:
    Student() = default;
    Student(const string &id, const string &pass, const string &b)
        : User(id, pass), batch(b) {}

    void   setBatch(const string &b) { batch = b; }
    string getBatch() const          { return batch; }

    vector<AttendanceRecord>&  getAttendance()        { return attendance; }
    vector<MarksRecord>&       getMarks()             { return marks; }
    vector<GradeRecord>&       getGrades()            { return grades; }
    vector<string>&            getRegisteredCourses() { return registeredCourses; }

    void saveToFile() override {
        ofstream file(STUDENT_PATH + userID + ".txt");
        if (!file)
            throw runtime_error("Error opening file: " + STUDENT_PATH + userID + ".txt");

        file << userID << " " << password << "\n";
        file << "Batch: "  << batch  << "\n";
        file << "Name: "   << name   << "\n";
        file << "Age: "    << age    << "\n";
        file << "Phone: "  << phone  << "\n";
        file << "Email: "  << email  << "\n";

        file << "Attendance:\n";
        for (const auto &r : attendance)
            file << r.date << " " << r.classType << " " << r.course << " " << r.status << "\n";

        file << "Marks:\n";
        for (const auto &m : marks)
            file << m.course << " " << m.examType << " " << m.examDate
                 << " " << m.marksObtained << " " << m.semester << "\n";

        file << "Grades:\n";
        for (const auto &g : grades)
            file << g.subject << " " << g.credits << " " << g.grade
                 << " " << g.gradePoint << " " << g.semester << "\n";

        file << "RegisteredCourses:\n";
        for (const auto &c : registeredCourses)
            file << c << "\n";

        file.close();
    }

    void loadFromFile(const string &filename) override {
        ifstream file(STUDENT_PATH + filename);
        if (!file)
            throw runtime_error("Student file not found: " + STUDENT_PATH + filename);

        file >> userID >> password;
        string line;
        getline(file, line);

        auto trim = [](string &s){
            // strip leading whitespace
            size_t p = s.find_first_not_of(" \t\r");
            if (p != string::npos) s = s.substr(p); else s = "";
            // strip trailing \r\n
            while (!s.empty() && (s.back()=='\r'||s.back()=='\n')) s.pop_back();
        };

        getline(file, line);
        if (line.size() >= 6 && line.substr(0,6) == "Batch:")
            { batch = line.substr(6); trim(batch); }
        getline(file, line);
        if (line.size() >= 5 && line.substr(0,5) == "Name:")
            { name = line.substr(5); trim(name); }
        getline(file, line);
        if (line.size() >= 4 && line.substr(0,4) == "Age:")
            { try { age = stoi(line.substr(4)); } catch(...){} }
        getline(file, line);
        if (line.size() >= 6 && line.substr(0,6) == "Phone:")
            { phone = line.substr(6); trim(phone); }
        getline(file, line);
        if (line.size() >= 6 && line.substr(0,6) == "Email:")
            { email = line.substr(6); trim(email); }

        getline(file, line); // "Attendance:" header
        attendance.clear();
        while (getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line == "Marks:") break;
            if (line.empty()) continue;
            istringstream iss(line);
            AttendanceRecord r;
            iss >> r.date >> r.classType >> r.course >> r.status;
            attendance.push_back(r);
        }

        marks.clear();
        while (getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line == "Grades:" || line == "RegisteredCourses:") break;
            if (line.empty()) continue;
            istringstream iss(line);
            MarksRecord m;
            iss >> m.course >> m.examType >> m.examDate >> m.marksObtained;
            if (!(iss >> m.semester)) m.semester = 1;
            marks.push_back(m);
        }

        grades.clear();
        bool inRegCourses = false;
        if (line == "Grades:") {
            while (getline(file, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line == "RegisteredCourses:") { inRegCourses = true; break; }
                if (line.empty()) continue;
                istringstream iss(line);
                GradeRecord g;
                iss >> g.subject >> g.credits >> g.grade >> g.gradePoint;
                if (!(iss >> g.semester)) g.semester = 1;
                grades.push_back(g);
            }
        } else if (line == "RegisteredCourses:") {
            inRegCourses = true;
        }

        registeredCourses.clear();
        if (inRegCourses) {
            while (getline(file, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (!line.empty()) registeredCourses.push_back(line);
            }
        }

        file.close();
    }

    void displayProfile() override {
        printHeader("STUDENT PROFILE");
        cout << "  Enrollment No : " << userID   << "\n";
        cout << "  Name          : " << name     << "\n";
        cout << "  Batch         : " << batch    << "\n";
        cout << "  Age           : " << age      << "\n";
        cout << "  Phone         : " << phone    << "\n";
        cout << "  Email         : " << email    << "\n";
        printLine();

        // Attendance
        printSubHeader("ATTENDANCE RECORDS");
        if (attendance.empty()) {
            cout << "  No attendance records.\n";
        } else {
            map<string, vector<AttendanceRecord>> byCourse;
            for (const auto &r : attendance) byCourse[r.course].push_back(r);
            for (const auto &ce : byCourse) {
                cout << "\n  Course: " << ce.first << "\n";
                cout << "  " << string(40, '-') << "\n";
                cout << "  " << padString("Date",12) << padString("Type",12) << padString("Status",10) << "\n";
                cout << "  " << string(40, '-') << "\n";
                int total = 0, present = 0;
                for (const auto &r : ce.second) {
                    cout << "  " << padString(r.date,12) << padString(r.classType,12) << padString(r.status,10) << "\n";
                    total++;
                    if (toLower(r.status) == "present") present++;
                }
                double pct = total > 0 ? 100.0*present/total : 0.0;
                cout << "  Attendance: " << fixed << setprecision(1) << pct
                     << "% (" << present << "/" << total << ")\n";
            }
        }
        printLine();

        // Semester-wise marks
        printSubHeader("SEMESTER-WISE MARKS");
        if (marks.empty()) {
            cout << "  No marks records.\n";
        } else {
            map<int, map<string, vector<MarksRecord>>> bySemCourse;
            for (const auto &m : marks) bySemCourse[m.semester][m.course].push_back(m);
            for (const auto &se : bySemCourse) {
                cout << "\n  [ Semester " << se.first << " ]\n";
                for (const auto &ce : se.second) {
                    cout << "  Course: " << ce.first << "\n";
                    cout << "  " << string(44, '-') << "\n";
                    cout << "  " << padString("Exam Type",14) << padString("Date",14) << "Marks\n";
                    cout << "  " << string(44, '-') << "\n";
                    for (const auto &m : ce.second)
                        cout << "  " << padString(m.examType,14) << padString(m.examDate,14) << m.marksObtained << "\n";
                }
            }
        }
        printLine();

        // CGPA summary
        if (!grades.empty()) {
            printSubHeader("CGPA SUMMARY");
            map<int, vector<GradeRecord>> bySem;
            for (const auto &g : grades) bySem[g.semester].push_back(g);
            double totalCP = 0, totalC = 0;
            for (const auto &se : bySem) {
                double cp = 0, c = 0;
                for (const auto &g : se.second) { cp += g.gradePoint*g.credits; c += g.credits; }
                cout << "  Semester " << se.first << " GPA: " << fixed << setprecision(2) << (c>0?cp/c:0.0) << "\n";
                totalCP += cp; totalC += c;
            }
            cout << "  Overall CGPA  : " << fixed << setprecision(2) << (totalC>0?totalCP/totalC:0.0) << "\n";
            printLine();
        }

        // Registered courses
        if (!registeredCourses.empty()) {
            printSubHeader("REGISTERED COURSES");
            for (const auto &c : registeredCourses) cout << "  - " << c << "\n";
            printLine();
        }
    }
};

// ============================================================
// Derived Class: Faculty
// ============================================================
class Faculty : public User {
private:
    string                   department;
    vector<string>           courses;
    vector<AttendanceRecord> ownAttendance;
public:
    Faculty() = default;
    Faculty(const string &id, const string &pass, const string &dept)
        : User(id, pass), department(dept) {}

    void   setDepartment(const string &d) { department = d; }
    string getDepartment() const          { return department; }

    void            addCourse(const string &c) { courses.push_back(c); }
    vector<string>& getCourses()               { return courses; }

    vector<AttendanceRecord>& getOwnAttendance() { return ownAttendance; }

    void saveToFile() override {
        ofstream file(FACULTY_PATH + userID + ".txt");
        if (!file)
            throw runtime_error("Error opening file: " + FACULTY_PATH + userID + ".txt");
        file << userID << " " << password << "\n";
        file << "Department: " << department << "\n";
        file << "Name: "       << name       << "\n";
        file << "Age: "        << age        << "\n";
        file << "Phone: "      << phone      << "\n";
        file << "Email: "      << email      << "\n";
        file << "Courses:";
        for (const auto &c : courses) file << " " << c;
        file << "\n";
        file << "Attendance:\n";
        for (const auto &r : ownAttendance)
            file << r.date << " " << r.classType << "\n";
        file.close();
    }

    void loadFromFile(const string &filename) override {
        ifstream file(FACULTY_PATH + filename);
        if (!file)
            throw runtime_error("Faculty file not found: " + FACULTY_PATH + filename);
        file >> userID >> password;
        string line;
        getline(file, line);
        auto trim = [](string &s){
            size_t p = s.find_first_not_of(" \t\r");
            if (p != string::npos) s = s.substr(p); else s = "";
            while (!s.empty() && (s.back()=='\r'||s.back()=='\n')) s.pop_back();
        };
        getline(file, line);
        if (line.find("Department:") == 0) { department = line.substr(11); trim(department); }
        getline(file, line);
        if (line.substr(0,5) == "Name:") { name = line.substr(5); trim(name); }
        getline(file, line);
        if (line.substr(0,4) == "Age:") { try { age = stoi(line.substr(4)); } catch(...){} }
        getline(file, line);
        if (line.substr(0,6) == "Phone:") { phone = line.substr(6); trim(phone); }
        getline(file, line);
        if (line.substr(0,6) == "Email:") { email = line.substr(6); trim(email); }
        getline(file, line); // Courses:
        courses.clear();
        if (line.find("Courses:") == 0) {
            istringstream iss(line.substr(8));
            string c;
            while (iss >> c) courses.push_back(c);
        }
        getline(file, line); // Attendance: header
        ownAttendance.clear();
        while (getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            istringstream iss(line);
            AttendanceRecord r;
            iss >> r.date >> r.classType;
            ownAttendance.push_back(r);
        }
        file.close();
    }

    void displayProfile() override {
        printHeader("FACULTY PROFILE");
        cout << "  Faculty ID   : " << userID     << "\n";
        cout << "  Name         : " << name       << "\n";
        cout << "  Department   : " << department << "\n";
        cout << "  Age          : " << age        << "\n";
        cout << "  Phone        : " << phone      << "\n";
        cout << "  Email        : " << email      << "\n";
        printLine();
        printSubHeader("ASSIGNED COURSES");
        if (courses.empty()) cout << "  None assigned.\n";
        else for (const auto &c : courses) cout << "  - " << c << "\n";
        printLine();
        printSubHeader("OWN ATTENDANCE RECORDS");
        if (ownAttendance.empty()) {
            cout << "  No attendance records.\n";
        } else {
            cout << "  " << padString("Date",14) << "Class Type\n";
            cout << "  " << string(28, '-') << "\n";
            for (const auto &r : ownAttendance)
                cout << "  " << padString(r.date,14) << r.classType << "\n";
        }
        printLine();
    }
};

// ============================================================
// Derived Class: Admin
// ============================================================
class Admin : public User {
public:
    Admin(const string &id, const string &pass) : User(id, pass) {}
    void saveToFile() override {}
    void loadFromFile(const string &) override {}
    void displayProfile() override { cout << "Admin: " << userID << "\n"; }

    void assignDepartmentToFaculty() {
        string fid, dept;
        cout << "Faculty ID      : "; cin >> fid;
        Faculty f;
        try { f.loadFromFile(fid + ".txt"); }
        catch (const exception &e) { cout << e.what() << "\n"; return; }
        cout << "New Department  : "; cin >> dept;
        f.setDepartment(dept);
        f.saveToFile();
        cout << "Department updated for " << fid << ".\n";
    }
};

// ============================================================
// Batch Functions 
// ============================================================
string autoAssignBatch(const string &studentID) {
    int batchNumber = 1;

    while (true) {
        string batchName = "Batch" + to_string(batchNumber);
        string filename  = BATCHES_PATH + batchName + ".txt";
        int count = 0; string line;
        ifstream infile(filename);

        if (infile.is_open()) {
            while (getline(infile, line))
                if (!line.empty() && line != "\r") count++;
            infile.close();
        }

        if (count < 30) {
            ofstream outfile(filename, ios::app);
            outfile << studentID << "\n";
            outfile.close();
            return batchName;
        }
        
        batchNumber++;
    }
}

void removeStudentFromBatch(const string &batchName, const string &studentID) {
    string filename = BATCHES_PATH + batchName + ".txt";
    ifstream infile(filename);
    if (!infile.is_open()) return;
    vector<string> ids; string line;
    while (getline(infile, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line != studentID && !line.empty()) ids.push_back(line);
    }
    infile.close();
    ofstream outfile(filename, ios::trunc);
    for (const auto &id : ids) outfile << id << "\n";
    outfile.close();
}

// ============================================================
// ReportGenerator template 
// ============================================================
template <typename T>
class ReportGenerator {
public:
    void generateReport(T &obj) {
        printLine('=');
        obj.displayProfile();
        printLine('=');
    }
};

// ============================================================
// Directory helpers
// ============================================================
vector<string> getAllStudentIDs() {
    vector<string> ids;
    for (const auto &f : listFiles(STUDENT_PATH))
        ids.push_back(f.substr(0, f.size()-4));
    return ids;
}

vector<string> getAllFacultyIDs() {
    vector<string> ids;
    for (const auto &f : listFiles(FACULTY_PATH))
        ids.push_back(f.substr(0, f.size()-4));
    return ids;
}

// ============================================================
// Attendance percentage
// ============================================================
double calculateAttendancePct(Student &s) {
    auto &att = s.getAttendance();
    if (att.empty()) return 0.0;
    int total = (int)att.size(), present = 0;
    for (const auto &r : att)
        if (toLower(r.status) == "present") present++;
    return 100.0 * present / total;
}

// ============================================================
// Average marks
// ============================================================
double calculateAvgMarks(Student &s) {
    auto &mrks = s.getMarks();
    if (mrks.empty()) return 0.0;
    double sum = 0;
    for (const auto &m : mrks) sum += m.marksObtained;
    return sum / mrks.size();
}

// ============================================================
// Rank list
// ============================================================
void generateRankList(ostream *fileOut = nullptr) {
    vector<RankEntry> entries;
    for (const auto &id : getAllStudentIDs()) {
        Student s;
        try {
            s.loadFromFile(id + ".txt");
            RankEntry e;
            e.rollNo     = s.getUserID();
            e.name       = s.getName().empty() ? "(unnamed)" : s.getName();
            e.avgMarks   = calculateAvgMarks(s);
            e.attendance = calculateAttendancePct(s);
            entries.push_back(e);
        } catch (...) {}
    }
    sort(entries.begin(), entries.end(),
         [](const RankEntry &a, const RankEntry &b){ return a.avgMarks > b.avgMarks; });
    for (int i = 0; i < (int)entries.size(); i++) entries[i].rank = i+1;

    auto printTable = [&](ostream &out) {
        out << string(70,'=') << "\n";
        out << "  STUDENT RANK LIST\n";
        out << string(70,'=') << "\n";
        out << "  " << padString("Rank",6) << padString("Roll No",16)
            << padString("Name",24) << padString("Avg Marks",12) << "Attendance%\n";
        out << "  " << string(64,'-') << "\n";
        for (const auto &e : entries)
            out << "  " << padString(to_string(e.rank),6) << padString(e.rollNo,16)
                << padString(e.name,24) << padString(to_string((int)e.avgMarks),12)
                << fixed << setprecision(1) << e.attendance << "%\n";
        out << string(70,'=') << "\n";
    };
    printTable(cout);
    if (fileOut) printTable(*fileOut);
}

// ============================================================
// Defaulter list
// ============================================================
void generateDefaulterList(ostream *fileOut = nullptr) {
    vector<tuple<string,string,string,double>> defaulters;
    for (const auto &id : getAllStudentIDs()) {
        Student s;
        try {
            s.loadFromFile(id + ".txt");
            double pct = calculateAttendancePct(s);
            if (pct < 75.0)
                defaulters.emplace_back(s.getName(), s.getUserID(), s.getBatch(), pct);
        } catch (...) {}
    }
    auto printTable = [&](ostream &out) {
        out << string(70,'=') << "\n";
        out << "  ATTENDANCE DEFAULTER LIST  (Below 75%)\n";
        out << string(70,'=') << "\n";
        if (defaulters.empty()) { out << "  No defaulters found.\n"; return; }
        out << "  " << padString("Name",22) << padString("Roll No",16)
            << padString("Batch",10) << "Attendance%\n";
        out << "  " << string(60,'-') << "\n";
        for (const auto &d : defaulters)
            out << "  " << padString(get<0>(d),22) << padString(get<1>(d),16)
                << padString(get<2>(d),10) << fixed << setprecision(1) << get<3>(d) << "%\n";
        out << string(70,'=') << "\n";
    };
    printTable(cout);
    if (fileOut) printTable(*fileOut);
}

// ============================================================
// Statistics
// ============================================================
void generateStatistics(ostream *fileOut = nullptr) {
    double maxM=-1,minM=1e9,sumM=0, maxA=-1,minA=1e9,sumA=0;
    int pass=0,fail=0,total=0;
    string topperName="N/A", lowestAttN="N/A";
    double topperAvg=-1, lowestAtt=101;

    for (const auto &id : getAllStudentIDs()) {
        Student s;
        try {
            s.loadFromFile(id + ".txt");
            double avg = calculateAvgMarks(s);
            double att = calculateAttendancePct(s);
            if (avg > topperAvg) { topperAvg=avg; topperName=s.getName().empty()?id:s.getName(); }
            if (att < lowestAtt) { lowestAtt=att; lowestAttN=s.getName().empty()?id:s.getName(); }
            if (avg > maxM) maxM = avg;
            if (avg < minM) minM = avg;
            if (att > maxA) maxA = att;
            if (att < minA) minA = att;
            sumM+=avg; sumA+=att;
            if (avg>=40) pass++; else fail++;
            total++;
        } catch (...) {}
    }
    auto printStats = [&](ostream &out) {
        out << string(60,'=') << "\n";
        out << "  SYSTEM-WIDE STATISTICS\n";
        out << string(60,'=') << "\n";
        if (total==0) { out << "  No student data.\n"; return; }
        out << "  Total Students     : " << total << "\n";
        out << string(60,'-') << "\n";
        out << "  MARKS STATISTICS\n";
        out << "  Highest Avg Marks  : " << fixed << setprecision(1) << maxM << "\n";
        out << "  Lowest  Avg Marks  : " << minM << "\n";
        out << "  Average Marks      : " << sumM/total << "\n";
        out << "  Pass %             : " << 100.0*pass/total << "%\n";
        out << "  Fail %             : " << 100.0*fail/total << "%\n";
        out << "  Topper             : " << topperName << " (" << topperAvg << ")\n";
        out << string(60,'-') << "\n";
        out << "  ATTENDANCE STATISTICS\n";
        out << "  Highest Attendance : " << maxA << "%\n";
        out << "  Lowest  Attendance : " << minA << "%\n";
        out << "  Average Attendance : " << sumA/total << "%\n";
        out << "  Lowest Att. Student: " << lowestAttN << " (" << lowestAtt << "%)\n";
        out << string(60,'=') << "\n";
    };
    printStats(cout);
    if (fileOut) printStats(*fileOut);
}

// ============================================================
// Admin Dashboard
// ============================================================
void displayAdminDashboard() {
    auto studentIDs = getAllStudentIDs();
    auto facultyIDs = getAllFacultyIDs();
    auto batches    = listFiles(BATCHES_PATH);
    double sumM=0,sumA=0; int total=0;
    string topperName="N/A", lowestAttN="N/A";
    double topperAvg=-1, lowestAtt=101;

    for (const auto &id : studentIDs) {
        Student s;
        try {
            s.loadFromFile(id + ".txt");
            double avg = calculateAvgMarks(s);
            double att = calculateAttendancePct(s);
            sumM+=avg; sumA+=att;
            if (avg>topperAvg) { topperAvg=avg; topperName=s.getName().empty()?id:s.getName(); }
            if (att<lowestAtt) { lowestAtt=att; lowestAttN=s.getName().empty()?id:s.getName(); }
            total++;
        } catch (...) {}
    }

    printLine('*');
    printHeader("CAMPUS-SPHERE  --  ADMIN DASHBOARD");
    printLine('*');
    cout << "  Total Students     : " << studentIDs.size() << "\n";
    cout << "  Total Faculty      : " << facultyIDs.size() << "\n";
    cout << "  Total Batches      : " << batches.size()    << "\n";
    if (total > 0) {
        cout << "  Average Marks      : " << fixed << setprecision(1) << sumM/total << "\n";
        cout << "  Average Attendance : " << fixed << setprecision(1) << sumA/total << "%\n";
        cout << "  Topper             : " << topperName << " (" << fixed << setprecision(1) << topperAvg << ")\n";
        cout << "  Lowest Attendance  : " << lowestAttN << " (" << fixed << setprecision(1) << lowestAtt << "%)\n";
    }
    printLine('*');
    cout << "\n";
}

// ============================================================
// Enter grades (Faculty calls this)
// ============================================================
void enterStudentGrades(const string &sid) {
    Student s;
    try { s.loadFromFile(sid + ".txt"); }
    catch (const exception &e) { cout << e.what() << "\n"; return; }
    cout << "Semester number  : "; int sem; cin >> sem;
    cout << "Number of subjects: "; int n; cin >> n;
    for (int i = 0; i < n; i++) {
        GradeRecord g; g.semester = sem;
        cout << "  Subject : "; cin >> g.subject;
        cout << "  Credits : "; cin >> g.credits;
        cout << "  Grade (O/A+/A/B+/B/C/F): "; cin >> g.grade;
        g.gradePoint = gradeToPoint(g.grade);
        s.getGrades().push_back(g);
    }
    try { s.saveToFile(); cout << "Grades saved for " << sid << ".\n"; }
    catch (const exception &e) { cout << e.what() << "\n"; }
}

// ============================================================
// View CGPA
// ============================================================
void viewStudentCGPA(Student &s) {
    auto &grades = s.getGrades();
    if (grades.empty()) { cout << "  No grade data available.\n"; return; }
    map<int, vector<GradeRecord>> bySem;
    for (const auto &g : grades) bySem[g.semester].push_back(g);
    printSubHeader("CGPA DETAILS  --  " + s.getName());
    double totalCP=0, totalC=0;
    for (const auto &se : bySem) {
        cout << "\n  [ Semester " << se.first << " ]\n";
        cout << "  " << padString("Subject",20) << padString("Credits",10)
             << padString("Grade",8) << "Grade Point\n";
        cout << "  " << string(50,'-') << "\n";
        double cp=0, c=0;
        for (const auto &g : se.second) {
            cout << "  " << padString(g.subject,20) << padString(to_string(g.credits),10)
                 << padString(g.grade,8) << fixed << setprecision(1) << g.gradePoint << "\n";
            cp += g.gradePoint*g.credits; c += g.credits;
        }
        cout << "  Semester GPA: " << fixed << setprecision(2) << (c>0?cp/c:0.0) << "\n";
        totalCP+=cp; totalC+=c;
    }
    printLine();
    cout << "  Overall CGPA: " << fixed << setprecision(2) << (totalC>0?totalCP/totalC:0.0) << "\n";
    printLine();
}

// ============================================================
// CGPA report all students
// ============================================================
void generateCGPAReport(ostream *fileOut = nullptr) {
    auto printReport = [&](ostream &out) {
        out << string(60,'=') << "\n";
        out << "  CGPA REPORT -- ALL STUDENTS\n";
        out << string(60,'=') << "\n";
        out << "  " << padString("Roll No",16) << padString("Name",24) << "CGPA\n";
        out << "  " << string(44,'-') << "\n";
        for (const auto &id : getAllStudentIDs()) {
            Student s;
            try {
                s.loadFromFile(id + ".txt");
                double cp=0,c=0;
                for (const auto &g : s.getGrades()) { cp+=g.gradePoint*g.credits; c+=g.credits; }
                out << "  " << padString(s.getUserID(),16)
                    << padString(s.getName().empty()?"(unnamed)":s.getName(),24)
                    << fixed << setprecision(2) << (c>0?cp/c:0.0) << "\n";
            } catch (...) {}
        }
        out << string(60,'=') << "\n";
    };
    printReport(cout);
    if (fileOut) printReport(*fileOut);
}

// ============================================================
// Student search
// ============================================================
void searchStudents() {
    cout << "\n  Search by: 1.Roll No  2.Name  3.Batch  4.Phone  5.Email\n";
    cout << "  Choice: "; int choice; cin >> choice;
    cout << "  Search term: "; string term; cin >> term;
    string termLow = toLower(term);
    int found = 0;
    for (const auto &id : getAllStudentIDs()) {
        Student s;
        try { s.loadFromFile(id + ".txt"); } catch (...) { continue; }
        bool match = false;
        switch (choice) {
            case 1: match = (toLower(s.getUserID()) == termLow); break;
            case 2: match = (toLower(s.getName()).find(termLow) != string::npos); break;
            case 3: match = (toLower(s.getBatch()) == termLow); break;
            case 4: match = (s.getPhone().find(term) != string::npos); break;
            case 5: match = (toLower(s.getEmail()).find(termLow) != string::npos); break;
        }
        if (match) { s.displayProfile(); found++; }
    }
    if (found == 0) cout << "  No matching students found.\n";
    else cout << "  Total matches: " << found << "\n";
}

// ============================================================
// Faculty search
// ============================================================
void searchFaculty() {
    cout << "\n  Search by: 1.Faculty ID  2.Name  3.Department  4.Course\n";
    cout << "  Choice: "; int choice; cin >> choice;
    cout << "  Search term: "; string term; cin >> term;
    string termLow = toLower(term);
    int found = 0;
    for (const auto &id : getAllFacultyIDs()) {
        Faculty f;
        try { f.loadFromFile(id + ".txt"); } catch (...) { continue; }
        bool match = false;
        switch (choice) {
            case 1: match = (toLower(f.getUserID()) == termLow); break;
            case 2: match = (toLower(f.getName()).find(termLow) != string::npos); break;
            case 3: match = (toLower(f.getDepartment()).find(termLow) != string::npos); break;
            case 4: for (const auto &c : f.getCourses()) if (toLower(c).find(termLow)!=string::npos){ match=true; break; } break;
        }
        if (match) { f.displayProfile(); found++; }
    }
    if (found == 0) cout << "  No matching faculty found.\n";
    else cout << "  Total matches: " << found << "\n";
}

// ============================================================
// Course Management
// ============================================================
void manageCourses() {
    int choice;
    do {
        printSubHeader("COURSE MANAGEMENT");
        cout << "  1. Add Course\n  2. Delete Course\n  3. Assign Course to Faculty\n  4. View All Courses\n  0. Back\n";
        cout << "  Choice: "; cin >> choice;
        if (choice == 1) {
            Course c;
            cout << "Code    : "; cin >> c.code;
            cout << "Name    : "; cin >> c.name;
            cout << "Credits : "; cin >> c.credits;
            auto courses = loadAllCourses();
            courses.push_back(c);
            saveAllCourses(courses);
            cout << "Course added.\n";
        } else if (choice == 2) {
            string code; cout << "Course code: "; cin >> code;
            auto courses = loadAllCourses();
            auto it = remove_if(courses.begin(), courses.end(),
                [&](const Course &c){ return toLower(c.code)==toLower(code); });
            if (it == courses.end()) cout << "Not found.\n";
            else { courses.erase(it,courses.end()); saveAllCourses(courses); cout << "Deleted.\n"; }
        } else if (choice == 3) {
            string code, fid;
            cout << "Course code : "; cin >> code;
            cout << "Faculty ID  : "; cin >> fid;
            auto courses = loadAllCourses();
            bool found = false;
            for (auto &c : courses) {
                if (toLower(c.code)==toLower(code)) {
                    c.assignedFacID = fid;
                    found = true;
                    Faculty f;
                    try { f.loadFromFile(fid+".txt"); f.getCourses().push_back(code); f.saveToFile(); } catch (...) {}
                    break;
                }
            }
            if (found) { saveAllCourses(courses); cout << "Assigned.\n"; }
            else cout << "Course not found.\n";
        } else if (choice == 4) {
            auto courses = loadAllCourses();
            printSubHeader("ALL COURSES");
            cout << "  " << padString("Code",12) << padString("Name",30) << padString("Credits",10) << "Faculty\n";
            cout << "  " << string(64,'-') << "\n";
            for (const auto &c : courses)
                cout << "  " << padString(c.code,12) << padString(c.name,30)
                     << padString(to_string(c.credits),10) << c.assignedFacID << "\n";
        }
    } while (choice != 0);
}

// ============================================================
// Batch Management
// ============================================================
void manageBatches() {
    int choice;
    do {
        printSubHeader("BATCH MANAGEMENT");
        cout << "  1. Create Batch\n  2. Delete Batch\n  3. Transfer Student\n  4. View Batch Students\n  0. Back\n";
        cout << "  Choice: "; cin >> choice;
        if (choice == 1) {
            string bname; cout << "Batch name: "; cin >> bname;
            ofstream f(BATCHES_PATH + bname + ".txt");
            cout << (f ? "Batch created.\n" : "Error.\n");
        } else if (choice == 2) {
            string bname; cout << "Batch name: "; cin >> bname;
            cout << (remove((BATCHES_PATH+bname+".txt").c_str())==0 ? "Deleted.\n" : "Not found.\n");
        } else if (choice == 3) {
            string sid, from, to;
            cout << "Enrollment No : "; cin >> sid;
            cout << "From batch    : "; cin >> from;
            cout << "To batch      : "; cin >> to;
            removeStudentFromBatch(from, sid);
            ofstream out(BATCHES_PATH+to+".txt", ios::app);
            out << sid << "\n"; out.close();
            Student s;
            try { s.loadFromFile(sid+".txt"); s.setBatch(to); s.saveToFile(); cout << "Transferred.\n"; }
            catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 4) {
            string bname; cout << "Batch name: "; cin >> bname;
            ifstream in(BATCHES_PATH+bname+".txt");
            if (!in.is_open()) { cout << "Not found.\n"; continue; }
            printSubHeader("Students in " + bname);
            string line; int cnt=0;
            while (getline(in,line)) {
                if (!line.empty() && line.back()=='\r') line.pop_back();
                if (!line.empty()) { cout << "  " << line << "\n"; cnt++; }
            }
            cout << "  Total: " << cnt << "\n";
        }
    } while (choice != 0);
}

// ============================================================
// Export Reports
// ============================================================
void exportReports() {
    ensureDir(REPORTS_PATH);
    int choice;
    do {
        printSubHeader("EXPORT REPORTS");
        cout << "  1. Export All Student Reports\n";
        cout << "  2. Export All Faculty Reports\n";
        cout << "  3. Export Batch Report\n";
        cout << "  4. Export Rank List\n";
        cout << "  5. Export CGPA Report\n";
        cout << "  6. Export Statistics Report\n";
        cout << "  0. Back\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            ofstream out(REPORTS_PATH+"Student_Report.txt", ios::trunc);
            for (const auto &id : getAllStudentIDs()) {
                Student s;
                try {
                    s.loadFromFile(id+".txt");
                    out << string(60,'=') << "\n";
                    out << "Enrollment: " << s.getUserID() << "  Name: " << s.getName()
                        << "  Batch: " << s.getBatch() << "\n";
                    out << "Avg Marks : " << fixed << setprecision(1) << calculateAvgMarks(s)
                        << "  Attendance: " << calculateAttendancePct(s) << "%\n";
                } catch (...) {}
            }
            cout << "Saved: " << REPORTS_PATH << "Student_Report.txt\n";
        } else if (choice == 2) {
            ofstream out(REPORTS_PATH+"Faculty_Report.txt", ios::trunc);
            for (const auto &id : getAllFacultyIDs()) {
                Faculty f;
                try {
                    f.loadFromFile(id+".txt");
                    out << string(60,'=') << "\n";
                    out << "ID: " << f.getUserID() << "  Name: " << f.getName()
                        << "  Dept: " << f.getDepartment() << "\n";
                    out << "Courses: ";
                    for (const auto &c : f.getCourses()) out << c << " ";
                    out << "\n";
                } catch (...) {}
            }
            cout << "Saved: " << REPORTS_PATH << "Faculty_Report.txt\n";
        } else if (choice == 3) {
            ofstream out(REPORTS_PATH+"Batch_Report.txt", ios::trunc);
            for (const auto &bf : listFiles(BATCHES_PATH)) {
                string bname = bf.substr(0, bf.size()-4);
                out << string(60,'=') << "\n" << "Batch: " << bname << "\n";
                ifstream in(BATCHES_PATH+bf);
                string line; int cnt=0;
                while (getline(in,line)) {
                    if (!line.empty() && line.back()=='\r') line.pop_back();
                    if (!line.empty()) { out << "  " << line << "\n"; cnt++; }
                }
                out << "Total: " << cnt << "\n";
            }
            cout << "Saved: " << REPORTS_PATH << "Batch_Report.txt\n";
        } else if (choice == 4) {
            ofstream out(REPORTS_PATH+"Rank_List.txt", ios::trunc);
            generateRankList(&out);
            cout << "Saved: " << REPORTS_PATH << "Rank_List.txt\n";
        } else if (choice == 5) {
            ofstream out(REPORTS_PATH+"CGPA_Report.txt", ios::trunc);
            generateCGPAReport(&out);
            cout << "Saved: " << REPORTS_PATH << "CGPA_Report.txt\n";
        } else if (choice == 6) {
            ofstream out(REPORTS_PATH+"Statistics_Report.txt", ios::trunc);
            generateStatistics(&out);
            cout << "Saved: " << REPORTS_PATH << "Statistics_Report.txt\n";
        }
    } while (choice != 0);
}

// ============================================================
// Personal detail helpers 
// ============================================================
void updateStudentDetails(Student &s) {
    cin.ignore();
    cout << "Name  : "; string n; getline(cin,n); s.setName(n);
    cout << "Age   : "; int a; cin >> a; s.setAge(a);
    cout << "Phone : "; string p; cin >> p; s.setPhone(p);
    cout << "Email : "; string e; cin >> e; s.setEmail(e);
}

void updateFacultyDetails(Faculty &f) {
    cin.ignore();
    cout << "Name  : "; string n; getline(cin,n); f.setName(n);
    cout << "Age   : "; int a; cin >> a; f.setAge(a);
    cout << "Phone : "; string p; cin >> p; f.setPhone(p);
    cout << "Email : "; string e; cin >> e; f.setEmail(e);
}

// ============================================================
// individual report functions
// ============================================================
void generateIndividualStudentReport(const string &studentID) {
    Student s;
    try {
        s.loadFromFile(studentID+".txt");
        ReportGenerator<Student> rep; rep.generateReport(s);
        ensureDir(REPORTS_PATH);
        ofstream out(REPORTS_PATH+studentID+"_report.txt", ios::trunc);
        out << "Enrollment: " << s.getUserID() << "\nName: " << s.getName()
            << "\nBatch: " << s.getBatch() << "\nAvg Marks: " << fixed << setprecision(1)
            << calculateAvgMarks(s) << "\nAttendance: " << calculateAttendancePct(s) << "%\n";
        cout << "Saved: " << REPORTS_PATH << studentID << "_report.txt\n";
    } catch (const exception &e) { cout << "Error: " << e.what() << "\n"; }
}

void generateIndividualFacultyReport(const string &facultyID) {
    Faculty f;
    try {
        f.loadFromFile(facultyID+".txt");
        ReportGenerator<Faculty> rep; rep.generateReport(f);
        ensureDir(REPORTS_PATH);
        ofstream out(REPORTS_PATH+facultyID+"_report.txt", ios::trunc);
        out << "ID: " << f.getUserID() << "\nName: " << f.getName()
            << "\nDept: " << f.getDepartment() << "\n";
        cout << "Saved: " << REPORTS_PATH << facultyID << "_report.txt\n";
    } catch (const exception &e) { cout << "Error: " << e.what() << "\n"; }
}

// ============================================================
// ADMIN MENU
// ============================================================
void adminMenu() {
    Admin admin("admin", "admin123");
    displayAdminDashboard();  // F6

    int choice;
    do {
        printHeader("ADMIN MENU");
        cout << "  -- Student --\n";
        cout << "  1.  Add Student            2.  Delete Student\n";
        cout << "  3.  View Student Profile   4.  Search Students\n";
        cout << "  -- Faculty --\n";
        cout << "  5.  Add Faculty            6.  Delete Faculty\n";
        cout << "  7.  View Faculty Profile   8.  Search Faculty\n";
        cout << "  9.  Assign Department to Faculty\n";
        cout << "  -- Academic --\n";
        cout << "  10. Manage Courses         11. Manage Batches\n";
        cout << "  -- Reports --\n";
        cout << "  12. Reset User Password\n";
        cout << "  13. Individual Student Report\n";
        cout << "  14. Individual Faculty Report\n";
        cout << "  15. Generate Rank List\n";
        cout << "  16. View Attendance Defaulters\n";
        cout << "  17. View Statistics\n";
        cout << "  18. Generate CGPA Report\n";
        cout << "  19. Export Reports\n";
        cout << "  0.  Logout\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            string sid, pass;
            cout << "Enrollment No : "; cin >> sid;
            cout << "Password      : "; cin >> pass;
            string batch = autoAssignBatch(sid);
            Student s(sid, pass, batch);
            try { s.saveToFile(); cout << "Added to " << batch << ".\n"; }
            catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 2) {
            string sid; cout << "Enrollment No: "; cin >> sid;
            Student s;
            try {
                s.loadFromFile(sid+".txt");
                removeStudentFromBatch(s.getBatch(), s.getUserID());
                cout << (remove((STUDENT_PATH+sid+".txt").c_str())==0 ? "Deleted.\n" : "Failed.\n");
            } catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 3) {
            string sid; cout << "Enrollment No: "; cin >> sid;
            Student s;
            try { s.loadFromFile(sid+".txt"); s.displayProfile(); }
            catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 4) {
            searchStudents();
        } else if (choice == 5) {
            string fid, pass, dept;
            cout << "Faculty ID   : "; cin >> fid;
            cout << "Password     : "; cin >> pass;
            cout << "Department   : "; cin >> dept;
            Faculty f(fid, pass, dept);
            cout << "Courses (type 'done' to finish): ";
            string c;
            while (cin >> c && c != "done") f.addCourse(c);
            try { f.saveToFile(); cout << "Faculty " << fid << " added.\n"; }
            catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 6) {
            string fid; cout << "Faculty ID: "; cin >> fid;
            cout << (remove((FACULTY_PATH+fid+".txt").c_str())==0 ? "Deleted.\n" : "Not found.\n");
        } else if (choice == 7) {
            string fid; cout << "Faculty ID: "; cin >> fid;
            Faculty f;
            try { f.loadFromFile(fid+".txt"); f.displayProfile(); }
            catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 8) {
            searchFaculty();
        } else if (choice == 9) {
            admin.assignDepartmentToFaculty();
        } else if (choice == 10) {
            manageCourses();
        } else if (choice == 11) {
            manageBatches();
        } else if (choice == 12) {
            int role; cout << "1.Student  2.Faculty: "; cin >> role;
            if (role == 1) {
                string sid, np; cout << "Enrollment: "; cin >> sid;
                Student s;
                try { s.loadFromFile(sid+".txt"); cout << "New pass: "; cin >> np; s.changePassword(np); s.saveToFile(); }
                catch (const exception &e) { cout << e.what() << "\n"; }
            } else {
                string fid, np; cout << "Faculty ID: "; cin >> fid;
                Faculty f;
                try { f.loadFromFile(fid+".txt"); cout << "New pass: "; cin >> np; f.changePassword(np); f.saveToFile(); }
                catch (const exception &e) { cout << e.what() << "\n"; }
            }
        } else if (choice == 13) {
            string sid; cout << "Enrollment No: "; cin >> sid;
            generateIndividualStudentReport(sid);
        } else if (choice == 14) {
            string fid; cout << "Faculty ID: "; cin >> fid;
            generateIndividualFacultyReport(fid);
        } else if (choice == 15) {
            generateRankList();
        } else if (choice == 16) {
            generateDefaulterList();
        } else if (choice == 17) {
            generateStatistics();
        } else if (choice == 18) {
            generateCGPAReport();
        } else if (choice == 19) {
            exportReports();
        }
    } while (choice != 0);
}

// ============================================================
// FACULTY MENU
// ============================================================
void facultyMenu(Faculty &f) {
    int choice;
    do {
        printHeader("FACULTY MENU  --  " + f.getName());
        cout << "  1.  Mark Student Attendance\n";
        cout << "  2.  Update Student Marks\n";
        cout << "  3.  Mark Your Own Attendance\n";
        cout << "  4.  Update Personal Details\n";
        cout << "  5.  View Your Profile\n";
        cout << "  6.  View Student Profile\n";
        cout << "  7.  Change Password\n";
        cout << "  8.  Filter Students by Batch & Mark Attendance\n";
        cout << "  9.  Enter Student Grades (CGPA)\n";
        cout << "  10. View Attendance Defaulters\n";
        cout << "  11. View Assigned Courses\n";
        cout << "  0.  Logout\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            string sid; cout << "Enrollment No: "; cin >> sid;
            Student s;
            try { s.loadFromFile(sid+".txt"); } catch (const exception &e) { cout << e.what() << "\n"; continue; }
            AttendanceRecord r;
            cout << "Date (YYYY-MM-DD): "; cin >> r.date;
            cout << "Class Type       : "; cin >> r.classType;
            cout << "Course           : "; cin >> r.course;
            cout << "Status (present/absent): "; cin >> r.status;
            r.status = toLower(r.status);
            s.getAttendance().push_back(r);
            try { s.saveToFile(); cout << "Attendance marked.\n"; }
            catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 2) {
            string sid; cout << "Enrollment No: "; cin >> sid;
            Student s;
            try { s.loadFromFile(sid+".txt"); } catch (const exception &e) { cout << e.what() << "\n"; continue; }
            MarksRecord m;
            cout << "Course         : "; cin >> m.course;
            cout << "Exam Type      : "; cin >> m.examType;
            cout << "Exam Date      : "; cin >> m.examDate;
            cout << "Marks Obtained : "; cin >> m.marksObtained;
            cout << "Semester       : "; cin >> m.semester;
            s.getMarks().push_back(m);
            try { s.saveToFile(); cout << "Marks updated.\n"; }
            catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 3) {
            AttendanceRecord r;
            cout << "Date (YYYY-MM-DD): "; cin >> r.date;
            cout << "Class Type       : "; cin >> r.classType;
            f.getOwnAttendance().push_back(r);
            try { f.saveToFile(); cout << "Attendance marked.\n"; }
            catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 4) {
            updateFacultyDetails(f);
            try { f.saveToFile(); cout << "Updated.\n"; }
            catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 5) {
            f.displayProfile();
        } else if (choice == 6) {
            string sid; cout << "Enrollment No: "; cin >> sid;
            Student s;
            try { s.loadFromFile(sid+".txt"); s.displayProfile(); }
            catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 7) {
            string np; cout << "New password: "; cin >> np;
            f.changePassword(np);
            try { f.saveToFile(); } catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 8) {
            string bName; cout << "Batch name: "; cin >> bName;
            ifstream infile(BATCHES_PATH+bName+".txt");
            if (!infile.is_open()) { cout << "Batch not found.\n"; continue; }
            string line;
            cout << "Students in " << bName << ":\n";
            while (getline(infile,line)) {
                if (!line.empty() && line.back()=='\r') line.pop_back();
                if (!line.empty()) cout << "  " << line << "\n";
            }
            infile.close();
            char ans; cout << "Mark a student? (y/n): "; cin >> ans;
            if (tolower(ans)=='y') {
                string sid; cout << "Enrollment No: "; cin >> sid;
                Student s;
                try { s.loadFromFile(sid+".txt"); } catch (const exception &e) { cout << e.what() << "\n"; continue; }
                AttendanceRecord r;
                cout << "Date   : "; cin >> r.date;
                cout << "Type   : "; cin >> r.classType;
                cout << "Course : "; cin >> r.course;
                cout << "Status : "; cin >> r.status;
                r.status = toLower(r.status);
                s.getAttendance().push_back(r);
                try { s.saveToFile(); cout << "Marked.\n"; }
                catch (const exception &e) { cout << e.what() << "\n"; }
            }
        } else if (choice == 9) {
            string sid; cout << "Enrollment No: "; cin >> sid;
            enterStudentGrades(sid);
        } else if (choice == 10) {
            generateDefaulterList();
        } else if (choice == 11) {
            printSubHeader("YOUR ASSIGNED COURSES");
            auto courses = loadAllCourses();
            bool found = false;
            for (const auto &c : courses) {
                if (c.assignedFacID == f.getUserID()) {
                    cout << "  " << padString(c.code,12) << padString(c.name,30) << c.credits << " credits\n";
                    found = true;
                }
            }
            if (!found) {
                cout << "  No courses in master list. Courses from profile:\n";
                for (const auto &c : f.getCourses()) cout << "  - " << c << "\n";
            }
        }
    } while (choice != 0);
}

// ============================================================
// STUDENT MENU
// ============================================================
void studentMenu(Student &s) {
    int choice;
    do {
        printHeader("STUDENT MENU  --  " + s.getName());
        cout << "  1. View Your Profile\n";
        cout << "  2. Update Personal Details\n";
        cout << "  3. Change Password\n";
        cout << "  4. View Semester-wise Records\n";
        cout << "  5. View CGPA\n";
        cout << "  6. View Registered Courses\n";
        cout << "  0. Logout\n";
        cout << "  Choice: "; cin >> choice;

        if (choice == 1) {
            s.displayProfile();
        } else if (choice == 2) {
            updateStudentDetails(s);
            try { s.saveToFile(); cout << "Updated.\n"; }
            catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 3) {
            string np; cout << "New password: "; cin >> np;
            s.changePassword(np);
            try { s.saveToFile(); } catch (const exception &e) { cout << e.what() << "\n"; }
        } else if (choice == 4) {
            auto &marks = s.getMarks();
            if (marks.empty()) { cout << "  No marks records.\n"; continue; }
            map<int, map<string, vector<MarksRecord>>> bySemCourse;
            for (const auto &m : marks) bySemCourse[m.semester][m.course].push_back(m);
            printSubHeader("SEMESTER-WISE ACADEMIC RECORDS");
            for (const auto &se : bySemCourse) {
                cout << "\n  [ Semester " << se.first << " ]\n";
                for (const auto &ce : se.second) {
                    cout << "  Course: " << ce.first << "\n";
                    cout << "  " << string(44,'-') << "\n";
                    cout << "  " << padString("Exam Type",14) << padString("Date",14) << "Marks\n";
                    cout << "  " << string(44,'-') << "\n";
                    for (const auto &m : ce.second)
                        cout << "  " << padString(m.examType,14) << padString(m.examDate,14) << m.marksObtained << "\n";
                }
            }
        } else if (choice == 5) {
            viewStudentCGPA(s);
        } else if (choice == 6) {
            printSubHeader("YOUR REGISTERED COURSES");
            auto &rc = s.getRegisteredCourses();
            if (rc.empty()) {
                auto courses = loadAllCourses();
                if (courses.empty()) { cout << "  No courses available.\n"; continue; }
                cout << "  " << padString("Code",12) << padString("Name",30) << "Credits\n";
                cout << "  " << string(46,'-') << "\n";
                for (const auto &c : courses)
                    cout << "  " << padString(c.code,12) << padString(c.name,30) << c.credits << "\n";
            } else {
                for (const auto &c : rc) cout << "  - " << c << "\n";
            }
        }
    } while (choice != 0);
}

// ============================================================
// MAIN
// ============================================================
int main() {
    ensureDir("data");
    ensureDir(STUDENT_PATH);
    ensureDir(FACULTY_PATH);
    ensureDir(BATCHES_PATH);
    ensureDir(REPORTS_PATH);
    ensureDir(COURSES_PATH);

    int roleChoice;
    do {
        printLine('*');
        printHeader("WELCOME TO CAMPUS-SPHERE");
        printLine('*');
        cout << "  1. Admin\n  2. Faculty\n  3. Student\n  0. Exit\n";
        cout << "  Select Role: "; cin >> roleChoice;

        if (roleChoice == 1) {
            string id, pass;
            cout << "Admin ID   : "; cin >> id;
            cout << "Password   : "; cin >> pass;
            if (id == "admin" && pass == "admin123") adminMenu();
            else cout << "Invalid credentials.\n";
        } else if (roleChoice == 2) {
            string id, pass;
            cout << "Faculty ID : "; cin >> id;
            cout << "Password   : "; cin >> pass;
            Faculty f;
            try { f.loadFromFile(id+".txt"); }
            catch (const exception &e) { cout << e.what() << "\n"; continue; }
            if (f.authenticate(id, pass)) facultyMenu(f);
            else cout << "Incorrect password.\n";
        } else if (roleChoice == 3) {
            string id, pass;
            cout << "Enrollment No: "; cin >> id;
            cout << "Password     : "; cin >> pass;
            Student s;
            try { s.loadFromFile(id+".txt"); }
            catch (const exception &e) { cout << e.what() << "\n"; continue; }
            if (s.authenticate(id, pass)) studentMenu(s);
            else cout << "Incorrect password.\n";
        }
    } while (roleChoice != 0);

    cout << "\nExiting CAMPUS-SPHERE. Goodbye!\n";
    return 0;
}
