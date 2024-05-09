/* This is a skeleton code for Optimized Merge Sort, you can make modifications as long as you meet 
   all question requirements*/  

#include <bits/stdc++.h>
#include "record_class.h"

using namespace std;

//defines how many blocks are available in the Main Memory 
#define buffer_size 22
int eRunNum = 0;    // number of runs from Emp
int dRunNum= 0;     // number of runs from Dept

Records buffers[buffer_size]; //use this class object of size 22 as your main memory

/***You can change return type and arguments as you want.***/

/*
    This function reads Emp records into buffer and return number of records read
*/
int loadEmpRecordsIntoBuffers(fstream &empin) {
    int count = 0;
    while (count < buffer_size) {
        Records record = Grab_Emp_Record(empin);    // grab record from csv file

        if (record.no_values == -1) {   // no more records available for processing
            break;
        }
        buffers[count] = record;    // add record to buffer
        count++;
        record.number_of_emp_records++;   // update number_of_emp_records
    }
    return count;
}

// Comparison function to compare two Records objects by their eid
bool compareByEid(const Records& a, const Records& b) {
    return a.emp_record.eid < b.emp_record.eid;
}
/*
    This function sorts the buffer by EID
*/
void sortBuffersByEid(int numberOfRecords) {
    sort(buffers, buffers + numberOfRecords, compareByEid);
}

/*
    This function reads Dept records into buffer and return number of records read
*/
int loadDeptRecordsIntoBuffers(fstream &deptin) {
    int count = 0;
    while (count < buffer_size) {
        Records record = Grab_Dept_Record(deptin);    // grab record from csv file

        if (record.no_values == -1) {   // no more records available for processing
            break;
        }
        buffers[count] = record;    // add record to buffer
        count++;
        record.number_of_dept_records++;   // update number_of_emp_records
    }
    return count;
}

// Comparison function to compare two Records objects by their eid
bool compareByManagerId(const Records& a, const Records& b) {
    return a.dept_record.managerid < b.dept_record.managerid;
}
/*
    This function sorts the buffer by DID
*/
void sortBuffersByManagerId(int numberOfRecords) {
    sort(buffers, buffers + numberOfRecords, compareByManagerId);
}

/*
    This function write the sorted buffer of emp into temporary file
*/
void writeEmpBufferToFile(const string &fileName, int recordNum) {
    fstream outFile(fileName, ios::out);
    if (!outFile.is_open()) {
        throw "Error creating " + fileName;
    }
    for (int i = 0; i < recordNum; ++i) {
        outFile << buffers[i].emp_record.eid << ","
                << buffers[i].emp_record.ename << ","
                << buffers[i].emp_record.age << ","
                << to_string(buffers[i].emp_record.salary) << "\n";
    }
    outFile.close();
}

/*
    This function write the sorted buffer of dept into temporary file
*/
void writeDeptBufferToFile(const string &fileName, int recordNum) {
    fstream outFile(fileName, ios::out);
    if (!outFile.is_open()) {
        throw "Error creating " + fileName;
    }
    for (int i = 0; i < recordNum; ++i) {
        outFile << buffers[i].dept_record.did << ","
                << buffers[i].dept_record.dname << ","
                << to_string(buffers[i].dept_record.budget) << ","
                << buffers[i].dept_record.managerid << "\n";
    }
    outFile.close();
}

//Sorting the buffers in main_memory and storing the sorted records into a temporary file (runs) 
// create runs for emp relation
void Sort_Emp_Buffer(int recordNum){
    //Remember: You can use only [AT MOST] 22 blocks for sorting the records / tuples and create the runs
    sortBuffersByEid(recordNum);

     // create a run and add buffer to run
    stringstream ss;
    ss << "eRun" << eRunNum << ".csv";
    string runFileName = ss.str();
    writeEmpBufferToFile(runFileName, recordNum);
    cout << "eRun " << eRunNum << " created!" << endl;
}

//Sorting the buffers in main_memory and storing the sorted records into a temporary file (runs) 
// create runs for dept relation
void Sort_Dept_Buffer(int recordNum){
    //Remember: You can use only [AT MOST] 22 blocks for sorting the records / tuples and create the runs
    sortBuffersByManagerId(recordNum);

     // create a run and add buffer to run
    stringstream ss;
    ss << "dRun" << dRunNum << ".csv";
    string runFileName = ss.str();
    writeDeptBufferToFile(runFileName, recordNum);
    cout << "dRun " << dRunNum << " created!" << endl;
}

//Prints out the attributes from empRecord and deptRecord when a join condition is met 
//and puts it in file Join.csv
void PrintJoin(ofstream& joinout, int minEIdx, int minDIdx) {
    joinout << buffers[minEIdx].emp_record.eid << "," 
            << buffers[minEIdx].emp_record.ename << "," 
            << buffers[minEIdx].emp_record.age << "," 
            << to_string(buffers[minEIdx].emp_record.salary) << ","
            << buffers[minDIdx].dept_record.did << ","
            << buffers[minDIdx].dept_record.dname << ","
            << to_string(buffers[minDIdx].dept_record.budget) << ","
            << buffers[minDIdx].dept_record.managerid << endl;
}

/*
    This function opens all the run files simultaneously for merge join
*/
void openRunFiles(fstream* eRunFiles, fstream* dRunFiles) {
    // open eRun files
    for (int i = 0; i < eRunNum; i++) {
        string eFileName = "eRun" + to_string(i) + ".csv";    // generate file name
        eRunFiles[i].open(eFileName, ios::in);   // open file
        if (!eRunFiles[i].is_open()) {
            throw "Error opening " + eFileName;
        } else {
            cout << "Added " << eFileName << " to eRunFiles!" << endl;
        }
    }
    // open dRun files
    for (int i = 0; i < dRunNum; i++) {
        string dFileName = "dRun" + to_string(i) + ".csv";    // generate file name
        dRunFiles[i].open(dFileName, ios::in);   // open file
        if (!dRunFiles[i].is_open()) {
            throw "Error opening " + dFileName;
        } else {
            cout << "Added " << dFileName << " to dRunFiles!" << endl;
        }
    }
}

//Use main memory to Merge and Join tuples 
//which are already sorted in 'runs' of the relations Dept and Emp 
void Merge_Join_Runs(fstream* eRunFiles, fstream* dRunFiles, ofstream& joinout){
    int deptStartIdx = eRunNum; // start index  dept records in buffer

    // read first record from each eRun into buffers
    for (int i = 0; i < eRunNum; ++i) {
        if (eRunFiles[i].is_open()) {
            cout << "In Merge_Runs: eRunFile[" << i << "]" << endl;
            buffers[i] = Grab_Emp_Record(eRunFiles[i]);
        }
    }
    // read first record from each dRun into buffers
    for (int i = 0; i < dRunNum; ++i) {
        if (dRunFiles[i].is_open()) {
            cout << "In Merge_Runs: dRunFile[" << i << "]" << endl;
            buffers[deptStartIdx + i] = Grab_Dept_Record(dRunFiles[i]);
        }
    }

    // Run the loop until this flag is on, then stop all operations
    bool stopMerge = false;

    while(!stopMerge) {
        int minEid = INT_MAX;
        int minManagerId = INT_MAX;
        int minEIdx = -1;   // smallest index of emp record
        int minDIdx = -1;   // smallest index of dept record

        Records::EmpRecord minERecord;
        Records::DeptRecord minDRecord;

        // find smallest Emp record
        for (int i = 0; i < eRunNum; ++i) {
            if (buffers[i].no_values != -1 && buffers[i].emp_record.eid < minEid) {
                minEid = buffers[i].emp_record.eid; // smallest eid
                minEIdx = i;    // index of smallest Emp record
                minERecord = buffers[i].emp_record;
            }
        }

        cout << "minManagerId BEFORE: " << minManagerId << endl;

        // find smallest Dept record
        for (int i = 0; i < dRunNum; ++i) {

            cout << "Looking for smallest dept record..." << endl;

            if (buffers[deptStartIdx + i].no_values != -1 && buffers[deptStartIdx + i].dept_record.managerid < minManagerId) {
                minManagerId = buffers[deptStartIdx + i].dept_record.managerid; // smallest managerId
                minDIdx = deptStartIdx + i;    // index of smallest dept record
                minDRecord = buffers[deptStartIdx + i].dept_record;
            }
        }

        cout << "minDIdx: " << minDIdx << endl;
        cout << "minEId: " << minEid << endl;
        cout << "minManagerId: " << minManagerId << endl;

        // stop if all runs from either emp or Dept are finished
        if (minEIdx == -1 || minDIdx == -1) {
            stopMerge = true;
        }

        // check join condition
        if (minEid == minManagerId) {
            cout << "found match!!!!" << endl;
            cout << "minEId = " << minEid << "\tminManagerId = " << minManagerId << endl;
            // in case Eid = managerid -> match -> join
            PrintJoin(joinout, minEIdx, minDIdx);    // write to join output
            buffers[minDIdx] = Grab_Dept_Record(dRunFiles[minDIdx - deptStartIdx]); // load the next record from the corresponding dRun of the record which was just processed into buffer
        } else {
            // in case Eid != managerid
            if (minEid < minManagerId) {
                // this Emp record is not a potential match, replace it with the next record from the same eRun
                buffers[minEIdx] = Grab_Emp_Record(eRunFiles[minEIdx]);
            } else {
                // Dept record is not a potential match, replace it with next record from the same dRun
                buffers[minDIdx] = Grab_Dept_Record(dRunFiles[minDIdx - deptStartIdx]);
            }
        }
    }
}

void deleteRuns() {
    // delete eRun files
    for (int i = 0; i < eRunNum; ++i) {
        stringstream ss;
        ss << "eRun" << i << ".csv";
        remove(ss.str().c_str());
        cout << "Removed " << ss.str() << "!" << endl;
    }
    // delete dRun files
    for (int i = 0; i < dRunNum; ++i) {
        stringstream ss;
        ss << "dRun" << i << ".csv";
        remove(ss.str().c_str());
        cout << "Removed " << ss.str() << "!" << endl;
    }
}

int main() {
    //Open file streams to read and write
    //Opening out two relations Emp.csv and Dept.csv which we want to join
    fstream empin;
    fstream deptin;
    empin.open("Emp.csv", ios::in);
    deptin.open("Dept.csv", ios::in);
   
    //Creating the Join.csv file where we will store our joined results
    ofstream joinout;
    joinout.open("Join.csv", ios::out);

    int numRecordInBuffer = 0;

    //1. Create runs for Dept and Emp which are sorted using Sort_Buffer()
    // creating runs for Emp
    cout << "Creating runs for Emp..." << endl; 
    while((numRecordInBuffer = loadEmpRecordsIntoBuffers(empin)) > 0) {  // repeat until end of file
        cout << "Number of records read into buffers: " << numRecordInBuffer << endl;
        Sort_Emp_Buffer(numRecordInBuffer); // sort buffer and add to file
        eRunNum++;   // update runNum
    }
    cout << "eRunNum: " << eRunNum << endl;
    empin.close();

    numRecordInBuffer = 0;

    // creating runs for Dept
    cout << "Creating runs for Dept..." << endl; 
    while((numRecordInBuffer = loadDeptRecordsIntoBuffers(deptin)) > 0) {  // repeat until end of file
        cout << "Number of records read into buffers: " << numRecordInBuffer << endl;
        Sort_Dept_Buffer(numRecordInBuffer); // sort buffer and add to file
        dRunNum++;   // update runNum
    }
    cout << "dRunNum: " << dRunNum << endl;
    deptin.close();

    //2. Use Merge_Join_Runs() to Join the runs of Dept and Emp relations 
    // open run files
    fstream eRunFiles[eRunNum];
    fstream dRunFiles[dRunNum];
    openRunFiles(eRunFiles, dRunFiles);

    Merge_Join_Runs(eRunFiles, dRunFiles, joinout);

    // close files
        for (auto& file : eRunFiles) {
            if (file.is_open()) {
                file.close();
            }
        }
        for (auto& file : dRunFiles) {
            if (file.is_open()) {
                file.close();
            }
        }


        joinout.close();

    //Please delete the temporary files (runs) after you've joined both Emp.csv and Dept.csv
    deleteRuns();

    return 0;
}
