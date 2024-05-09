#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
#include <fstream>
using namespace std;

class Record {
public:
    int id, manager_id;     // Fixed size 8, 8 bytes
    std::string bio, name;  // Maximum size 500, 200 bytes, respectively

    Record(vector<std::string> fields) {
        id = stoi(fields[0]);           
        name = fields[1];               
        bio = fields[2];
        manager_id = stoi(fields[3]);
    }

    void print() {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
    }
};

class StorageBufferManager {

private:
    const int POINTER_ADDRESS_SIZE = sizeof(int);
    const int NUMBER_RECORD_SIZE = sizeof(int);
    const int RECORD_LENGTH_SIZE = sizeof(int);
    // Size of 2 pointers for (offset, length) of a record in the slot directory
    const int SLOT_DIRECTORY_POINTER_SIZE = POINTER_ADDRESS_SIZE * 2;
    // Size of offset address for 4 fields + 1 offset end address of a record
    const int FIELD_OFFSET_POINTER_SIZE = POINTER_ADDRESS_SIZE * 5;
    // Size of integer fields
    const int NUMBER_SIZE = 8;
    const int MAXIMUM_RECORD_SIZE = NUMBER_SIZE * 2 + 500 + 200;
    // Initialize the block with size allowed in main memory according to the question 
    static const int BLOCK_SIZE = 4096;
    char page[BLOCK_SIZE] = {};
    // Space remaining in the page. Must subtract space for the offset of free space and the number of
    // variable length records
    int pageRemaining = BLOCK_SIZE - POINTER_ADDRESS_SIZE - NUMBER_RECORD_SIZE;
    // Pointer to the newest record (offset, length)
    int latestRecordOffset = BLOCK_SIZE - POINTER_ADDRESS_SIZE - NUMBER_RECORD_SIZE;
    // Address of free space starting position
    int freeSpacePointer = 0;
    int numRecords = 0;
    
    // Insert new record 
    void insertRecord(Record record, ofstream *dataFile) {
        // No records written yet
        if (numRecords == 0) {
            // Initialize free space pointer
            freeSpacePointer = 0;

            // Move pointer to the end of page to save the free space pointer and the number of records
            latestRecordOffset = BLOCK_SIZE - POINTER_ADDRESS_SIZE - NUMBER_RECORD_SIZE;
            memcpy(page + BLOCK_SIZE - POINTER_ADDRESS_SIZE, &freeSpacePointer, POINTER_ADDRESS_SIZE);
            memcpy(page + BLOCK_SIZE - POINTER_ADDRESS_SIZE - NUMBER_RECORD_SIZE, &numRecords, NUMBER_RECORD_SIZE);
        }
        
        // Size of string
        int nameLength = record.name.size() + 1; // don't forget NULL termination
        int bioLength = record.bio.size() + 1;   // don't forget NULL termination

        // Size of the variable record (its length and how much it actually takes from page)
        int recordLength = FIELD_OFFSET_POINTER_SIZE + (NUMBER_SIZE * 2) + nameLength + bioLength;
        int recordTrueSize = SLOT_DIRECTORY_POINTER_SIZE + recordLength;

        // Copy data to disk if there is little free space in page
        if (pageRemaining < recordLength) {
            // Save page to disk
            (*dataFile).write(page, sizeof(page));
            // Seek to the end of file if there is a need to append to the data file
            (*dataFile).seekp(0, ios::end);
            
            // Reset free space information and number of variable length records
            pageRemaining = BLOCK_SIZE - POINTER_ADDRESS_SIZE - NUMBER_RECORD_SIZE;
            freeSpacePointer = 0;
            numRecords = 0;

            // Move pointer to the end of page to save the free space pointer and the number of records
            latestRecordOffset = BLOCK_SIZE - POINTER_ADDRESS_SIZE - NUMBER_RECORD_SIZE;
            memcpy(page + BLOCK_SIZE - POINTER_ADDRESS_SIZE, &freeSpacePointer, POINTER_ADDRESS_SIZE);
            memcpy(page + BLOCK_SIZE - POINTER_ADDRESS_SIZE - NUMBER_RECORD_SIZE, &numRecords, NUMBER_RECORD_SIZE);
        }
        
        // Create a new variable-length record
        char *variableRecord = new char[recordLength];
        // The address of the current field relative to the variable length record
        int addressOfCurrentField = FIELD_OFFSET_POINTER_SIZE;
        // The address of the current offset field relative to the variable length record
        int addressOfCurrentOffsetField = 0;

        // Add record to the block and write offset address of every field and the end of the record
        // record.id
        memcpy(&variableRecord[0] + addressOfCurrentField, &record.id, sizeof(int));
        memcpy(&variableRecord[0] + addressOfCurrentOffsetField, &addressOfCurrentField, POINTER_ADDRESS_SIZE);
        addressOfCurrentField += NUMBER_SIZE;
        addressOfCurrentOffsetField += POINTER_ADDRESS_SIZE;

        // record.name
        memcpy(&variableRecord[0] + addressOfCurrentField, &record.name[0], nameLength);
        memcpy(&variableRecord[0] + addressOfCurrentOffsetField, &addressOfCurrentField, POINTER_ADDRESS_SIZE);
        addressOfCurrentField += nameLength;
        addressOfCurrentOffsetField += POINTER_ADDRESS_SIZE;

        // record.bio
        memcpy(&variableRecord[0] + addressOfCurrentField, &record.bio[0], bioLength);
        memcpy(&variableRecord[0] + addressOfCurrentOffsetField, &addressOfCurrentField, POINTER_ADDRESS_SIZE);
        addressOfCurrentField += bioLength;
        addressOfCurrentOffsetField += POINTER_ADDRESS_SIZE;

        // record.manager_id
        memcpy(&variableRecord[0] + addressOfCurrentField, &record.manager_id, sizeof(int));
        memcpy(&variableRecord[0] + addressOfCurrentOffsetField, &addressOfCurrentField, POINTER_ADDRESS_SIZE);
        addressOfCurrentField += NUMBER_SIZE;
        addressOfCurrentOffsetField += POINTER_ADDRESS_SIZE;

        // Offset of end of record
        memcpy(&variableRecord[0] + addressOfCurrentOffsetField, &addressOfCurrentField, POINTER_ADDRESS_SIZE);

        // Copy variable length record to the memory page
        memcpy(page + freeSpacePointer, &variableRecord[0], recordLength);
        int recordAddress = freeSpacePointer;
        freeSpacePointer += recordLength;
        
        // Get (offset, length) to the slot directory
        memcpy(page + latestRecordOffset - POINTER_ADDRESS_SIZE - RECORD_LENGTH_SIZE, &recordAddress, POINTER_ADDRESS_SIZE);
        memcpy(page + latestRecordOffset - POINTER_ADDRESS_SIZE, &recordLength, RECORD_LENGTH_SIZE);
        latestRecordOffset = latestRecordOffset - POINTER_ADDRESS_SIZE - RECORD_LENGTH_SIZE;
        
        // Update free space pointer and the number of variable length records in the memory page
        numRecords += 1;
        memcpy(page + BLOCK_SIZE - POINTER_ADDRESS_SIZE, &freeSpacePointer, POINTER_ADDRESS_SIZE);
        memcpy(page + BLOCK_SIZE - POINTER_ADDRESS_SIZE - NUMBER_RECORD_SIZE, &numRecords, NUMBER_RECORD_SIZE);

        // Decrease free space of the page
        pageRemaining = pageRemaining - recordTrueSize;

        delete[] variableRecord;
    }
    
public:
    string databaseFileName;
    bool databaseExisted = true;

    StorageBufferManager(string NewFileName) {
        // File read handler for the database file
        ifstream iFile(databaseFileName, ios::in | ios::binary);

        // Check for existing database file 
        iFile.open(NewFileName);
        databaseFileName = NewFileName;
        if (!iFile.is_open()) {
            databaseExisted = false;
        }

        iFile.close();
    }

    // Read csv file (Employee.csv) and add records to the (EmployeeRelation)
    void createFromFile(string csvFName) {
        /* 
         * Check to see if the database file have already existed,
         * only read csv file when the database file doesn't exist or empty (newly created)
         */ 
        if (!databaseExisted) {
            // File read handler for csv file
            ifstream inputFile(csvFName);

            if (!inputFile.is_open()) {
                throw "Error opening " + csvFName;
            }

            // File write handler for database file
            ofstream dataFile(databaseFileName, ios::out | ios::binary);
            if (!dataFile.is_open()) {
                throw "Error creating " + databaseFileName;
            }

            // Going through each line to add record to page
            string line;
            while (getline(inputFile, line)) {
                vector<string> fields;
                stringstream ss(line);
                string field;
                while (getline(ss, field, ',')) {
                    fields.push_back(field);
                }
            
                Record record(fields);

                insertRecord(record, &dataFile);
            }
            // Final write the last page to disk
            dataFile.write(page, sizeof(page));

            inputFile.close();
            dataFile.close();
        }
    }

    // Given an ID, find the relevant record and print it
    Record findRecordById(int id) {
        // Open File for reading
        ifstream dataFile(databaseFileName, ios::in | ios::binary);
        if (!dataFile.is_open()) {
            throw "Error opening " + databaseFileName;
        }

        // Read a page from file
        while (!dataFile.eof()) {
            dataFile.read(&page[0], BLOCK_SIZE);

            // Get the Number of variable length records
            memcpy(&numRecords, &page[BLOCK_SIZE - POINTER_ADDRESS_SIZE - NUMBER_RECORD_SIZE], NUMBER_RECORD_SIZE);
            
            // Get the free space pointer of the page
            memcpy(&freeSpacePointer, &page[BLOCK_SIZE - POINTER_ADDRESS_SIZE], POINTER_ADDRESS_SIZE);

            // To store the n_th index of record into slot directory
            int trackRecordOffset = 0;
            
            // variable for record length in slot directory
            int recordLength = 0;

            // Pointer to record 
            int recordOffsetPointer = BLOCK_SIZE - POINTER_ADDRESS_SIZE * 2 - NUMBER_RECORD_SIZE - RECORD_LENGTH_SIZE;
            
            // check all records in page
            for(int i = 0; i < numRecords; i++) {
                // Next Record Offset
                memcpy(&trackRecordOffset, &page[recordOffsetPointer], sizeof(int));

                // Next Record Length
                memcpy(&recordLength, &page[recordOffsetPointer + sizeof(int)], sizeof(int));

                // Get the variable-length record using (Record Offset, Record Length)
                char *variableRecord = new char[recordLength];
                memcpy(&variableRecord[0], &page[trackRecordOffset], recordLength);

                // Read all field pointers from record
                int idPointer = 0;
                int namePointer = 0;
                int bioPointer = 0;
                int managerIdPointer = 0;
                int endRecordPointer = 0;
                memcpy(&idPointer, &variableRecord[0], POINTER_ADDRESS_SIZE);
                memcpy(&namePointer, &variableRecord[POINTER_ADDRESS_SIZE], POINTER_ADDRESS_SIZE);
                memcpy(&bioPointer, &variableRecord[2 * POINTER_ADDRESS_SIZE], POINTER_ADDRESS_SIZE);
                memcpy(&managerIdPointer, &variableRecord[3 * POINTER_ADDRESS_SIZE], POINTER_ADDRESS_SIZE);
                memcpy(&endRecordPointer, &variableRecord[4 * POINTER_ADDRESS_SIZE], POINTER_ADDRESS_SIZE);

                trackRecordOffset = endRecordPointer;

                // Read employee ID attribute
                int employeeId = 0;
                memcpy(&employeeId, &variableRecord[idPointer], sizeof(int));

                if (employeeId == id) {
                    vector<string> employeeInfo;
                    int eId, eManager_id;
                    string eName, eBio;
                    // read the string
                    memcpy(&eId, &variableRecord[idPointer], sizeof(int));
                    eName.resize(bioPointer - namePointer);
                    memcpy(&eName[0], &variableRecord[namePointer], bioPointer - namePointer);
                    eBio.resize(managerIdPointer - bioPointer);
                    memcpy(&eBio[0], &variableRecord[bioPointer], managerIdPointer - bioPointer);
                    memcpy(&eManager_id, &variableRecord[managerIdPointer], sizeof(int));
                
                    // Assign the string into empty record;
                    employeeInfo.push_back(to_string(eId));
                    employeeInfo.push_back(eName);
                    employeeInfo.push_back(eBio);
                    employeeInfo.push_back(to_string(eManager_id));

                    dataFile.close();
                    delete[] variableRecord;
                    return Record(employeeInfo);
                } else {
                    recordOffsetPointer = recordOffsetPointer - POINTER_ADDRESS_SIZE - RECORD_LENGTH_SIZE;
                }
            }
        }

        dataFile.close();

        return Record({"-1","","","0"});
    }    
};
