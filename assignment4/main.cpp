/* This is a skeleton code for External Memory Sorting, you can make modifications as long as you meet 
   all question requirements*/  

#include "record_class.h"
#include <algorithm> // For sort function
#include <vector>
#include <limits>

using namespace std;

//defines how many blocks are available in the Main Memory 
#define buffer_size 22

int runNum = 0;
Records buffers[buffer_size]; //use this class object of size 22 as your main memory

/***You can change return type and arguments as you want.***/

/*
    This function reads records into buffer and return number of records read
*/
int loadRecordsIntoBuffers(fstream &empin) {
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
    This function sorts the buffer by ID
*/
void sortBuffersById(int numberOfRecords) {
    sort(buffers, buffers + numberOfRecords, compareByEid);
}

/*
    This function write the sorted buffer into temporary file
*/
void writeBufferToFile(const string &fileName, int recordNum) {
    fstream outFile(fileName, ios::out);
    if (!outFile.is_open()) {
        throw "Error creating " + fileName;
    }
    for (int i = 0; i < recordNum; ++i) {
        outFile << buffers[i].emp_record.eid << ","
                << buffers[i].emp_record.ename << ","
                << buffers[i].emp_record.age << ","
                << buffers[i].emp_record.salary << "\n";
    }
    outFile.close();
}

//PASS 1
//Sorting the buffers in main_memory and storing the sorted records into a temporary file (Runs) 
void Sort_Buffer(int recordNum){
    //Remember: You can use only [AT MOST] 22 blocks for sorting the records / tuples and create the runs
    sortBuffersById(recordNum); // sort buffer

    // create a run and add buffer to run
    stringstream ss;
    ss << "Run" << runNum << ".csv";
    string runFileName = ss.str();
    writeBufferToFile(runFileName, recordNum);
    cout << "Run " << runNum << " created!" << endl;
}

//PASS 2
//Use main memory to Merge the Runs 
//which are already sorted in 'runs' of the relation Emp.csv 

/*
    This function opens all the run files simultaneously for merge sort
*/
void openRunFiles(fstream* runFiles) {
    for (int i = 0; i < runNum; i++) {
        string fileName = "Run" + to_string(i) + ".csv";    // generate file name
        runFiles[i].open(fileName, ios::in);   // open file
        if (!runFiles[i].is_open()) {
            throw "Error opening " + fileName;
        } else {
            cout << "Added " << fileName << " to runFiles vector!" << endl;
        }
    }
}

// write buffer to EmpSorted file
void PrintSorted(ofstream& outFile, int minIdx) {
    // Write the record to the output file
    outFile << buffers[minIdx].emp_record.eid << "," 
            << buffers[minIdx].emp_record.ename << "," 
            << buffers[minIdx].emp_record.age << "," 
            << buffers[minIdx].emp_record.salary << endl;
}


void Merge_Runs(fstream* runFiles, ofstream& sortedFile){
    // read first record from each run into buffers
    for (int i = 0; i < runNum; ++i) {
        if (runFiles[i].is_open()) {
            cout << "In Merge_Runs: runFile[" << i << "]" << endl;
            buffers[i] = Grab_Emp_Record(runFiles[i]);
        }
    }

    // Run the loop until this flag is on, then stop all operations
    bool stopMerging;

    do {
        stopMerging = true;
        int lowestIdIdx = -1;
        Records::EmpRecord lowestIdRecord;
        lowestIdRecord.eid = std::numeric_limits<int>::max(); // Initialize with max possible value
        // read record from each run into buffer
        for (int i = 0; i < runNum; ++i) {
            if (buffers[i].no_values != -1 && buffers[i].emp_record.eid < lowestIdRecord.eid) {
                // still has record(s) for processing
                stopMerging = false; 
                lowestIdIdx = i;    // update the index of min record
                lowestIdRecord = buffers[i].emp_record; // get min record
                
            }
        }
        
        cout << "lowestIdRecord.eid: " << lowestIdRecord.eid << endl;
        cout << "lowestIdIdx: " << lowestIdIdx << endl;
        cout << "stopMerging: " << stopMerging << endl;
        
        if (!stopMerging) {
            // Write the smallest record to the sorted file
            PrintSorted(sortedFile, lowestIdIdx);

            // That buffer location will have that record replaced with the next one in the run file
            buffers[lowestIdIdx] = Grab_Emp_Record(runFiles[lowestIdIdx]);
        }

    } while(!stopMerging);

}

void deleteRuns() {
    for (int i = 0; i < runNum; ++i) {
        stringstream ss;
        ss << "Run" << i << ".csv";
        remove(ss.str().c_str());
    }
}

int main() {
    try
    {
        //Open file streams to read and write
        //Opening out the Emp.csv relation that we want to Sort
        fstream empin;
        empin.open("Emp.csv", ios::in);
        if (!empin.is_open()) {
            throw "Error opening Emp.csv";
        }

        int numRecordInBuffer;
        //Creating the EmpSorted.csv file where we will store our sorted results
        ofstream SortOut;
        SortOut.open("EmpSorted.csv", ios::out);
        if (!SortOut.is_open()) {
            throw "Error creating EmpSorted.csv";
        }

        //1. Create runs for Emp which are sorted using Sort_Buffer()
        while((numRecordInBuffer = loadRecordsIntoBuffers(empin)) > 0) {  // repeat until end of file
            cout << "Number of records read into buffers: " << numRecordInBuffer << endl;
            Sort_Buffer(numRecordInBuffer); // sort buffer and add to file
            runNum++;   // update runNum
        }
        cout << "runNum: " << runNum << endl;
        empin.close();

        //2. Use Merge_Runs() to Sort the runs of Emp relations 
        // open run files
        fstream runFiles[runNum];
        openRunFiles(runFiles);

        // merge sort
        Merge_Runs(runFiles, SortOut);

        // close files
        for (auto& file : runFiles) {
            if (file.is_open()) {
                file.close();
            }
        }
        SortOut.close();

        //Please delete the temporary files (runs) after you've sorted the Emp.csv
        deleteRuns();
 
        return 0;
    }
    catch(char const *e)
    {
        cerr << e << endl;
        return 1;
    }
}
