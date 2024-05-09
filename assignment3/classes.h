#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
#include <fstream>
#include "constants.h"
#include "FileAccess.h"
using namespace std;

bool isDebug = false;

class Record {
public:
    int id, manager_id;
    std::string bio, name;

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


class LinearHashIndex {

private:
    ifstream dataRead;
    ofstream dataWrite;

    // Initialize the main page in memory
    char page[BLOCK_SIZE] = {};

    vector<int> blockDirectory; // Map the least-significant-bits of h(id) to a bucket location in EmployeeIndex (e.g., the jth bucket)
                                // can scan to correct bucket using offset (using seek function)
								// can initialize to a size of 256 (assume that we will never have more than 256 regular (i.e., non-overflow) buckets)
    int currentPageAddress = 0; // Address of current page in access
    int n;  // The number of indexes in blockDirectory currently being used
    int i;	// The number of least-significant-bits of h(id) to check. Will need to increase i once n > 2^i
    int numRecords = 0;    // Records currently in index file. Used to test whether to increase n
    int totalRecordsInPage = 0;    // Records currently in page
    int nextFreeBlock = 0; // Next place to write a bucket. Should increment it by BLOCK_SIZE whenever a bucket is written to EmployeeIndex
    string fName;      // Name of index file
    bool indexExisted = true; // Existence of the index file

    // Length of block directory in the first page of the index file
    int blockDirectoryPageLength = BLOCK_DIRECTORY_LENGTH_SIZE + NUMBER_RECORD_SIZE + PAGE_REMAINING_SIZE + LSBIT_TO_ADRESS_BUCKET_SIZE + NUMBER_BUCKET_SIZE + POINTER_ADDRESS_SIZE * 4;
    // Space remaining in the page. Must subtract space for the offset of free space and the number of
    // variable length records
    int pageRemaining = BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE;
    int totalPageRemaining = BLOCK_SIZE * 4 - (FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE) * 4;
    // Pointer to the newest record (offset, length)
    int latestRecordOffset = BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE;
    // Address of free space starting position
    int freeSpacePointer = 0;
    // Address of overflow page
    int overflowPagePointer = 0;

    void loadFromDisk(int address, void* dest, int size) {
        dataRead.open(fName, ios::in | ios::binary);
        dataRead.seekg(address, ios::beg);
        dataRead.read(reinterpret_cast<char*>(dest), size);
        dataRead.close();
    }

    void modifyFileOnDisk(int address, char* src, int size) {
        // ios::out | ios::in prevent truncating the file
        dataWrite.open(fName, ios::out | ios::in | ios::binary);
        if (!dataWrite.is_open()) {
            throw "Error creating " + fName;
        }
        dataWrite.seekp(address, ios::beg);
        dataWrite.write(src, size);
        dataWrite.close();
    }

    void saveToDiskAtTheEndOfFile(char* src, int size) {
        // ios::app to append to file
        dataWrite.open(fName, ios::app | ios::binary);
        if (!dataWrite.is_open()) {
            throw "Error creating " + fName;
        }
        dataWrite.write(src, size);
        dataWrite.close();
    }

    void resetSlotDirectory() {
        // Reset free space information and number of variable length records
        freeSpacePointer = 0;
        overflowPagePointer = 0;
        totalRecordsInPage = 0;

        // Move pointer to the end of page to save the free space pointer, overflow page address and the number of records
        latestRecordOffset = BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE;
        pageRemaining = BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE;
    }

    void saveSlotDirectory() {
        memcpy(page + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE, &freeSpacePointer, POINTER_ADDRESS_SIZE);
        memcpy(page + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE, &overflowPagePointer, POINTER_ADDRESS_SIZE);
        memcpy(page + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE, &totalRecordsInPage, NUMBER_RECORD_SIZE);
        memcpy(page + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE, &pageRemaining, PAGE_REMAINING_SIZE);
    }

    unsigned int hash(int key) {
        unsigned int hash = key % 216;
        
        // Creates a bitmask with n(th) least significant bit set to 1, other bits set to 0. Then
        // subtract by 1, flipping all bits to the right of the n(th) least significant bit to 1
        unsigned int mask = ((1 << i) - 1);
        return hash & mask;
    }

    // Insert new record into index
    void insertRecord(Record record) {
        // Get page to store the record
        getBucket(record.id);

        // Take neccessary steps if capacity is reached:
		// increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip
        if (false && totalPageRemaining / (n * BLOCK_SIZE) <= FREE_SPACE_THRESHOLD_FOR_NEW_BUCKET) {
            n += 1;
            if (n > 2^i) i += 1;
            blockDirectory.push_back(nextFreeBlock);
            totalPageRemaining += (BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE);
            blockDirectoryPageLength += POINTER_ADDRESS_SIZE;
            saveBlockDirectory();

            // create a new page as destination to copy
            char *destPage = new char[BLOCK_SIZE];
            memset(destPage, '\0', BLOCK_SIZE);

            // Slot directory for destPage
            int freeSpacePointer2 = 0;
            int overflowPagePointer2 = 0;
            int totalRecordsInPage2 = 0;
            int latestRecordOffset2 = BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE;
            int pageRemaining2 = BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE;

            int currentPageAddress2 = nextFreeBlock;

            memcpy(destPage + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE, &freeSpacePointer2, POINTER_ADDRESS_SIZE);
            memcpy(destPage + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE, &overflowPagePointer2, POINTER_ADDRESS_SIZE);
            memcpy(destPage + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE, &totalRecordsInPage2, NUMBER_RECORD_SIZE);
            memcpy(destPage + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE, &pageRemaining2, PAGE_REMAINING_SIZE);
            
            // Save the new page to disk
            saveToDiskAtTheEndOfFile(&destPage[0], BLOCK_SIZE);

            // Copy data from page to destPage. one record at a time
            // As you copy each record, put -1 in record offset in slot directory in page (not destPage) and add (record directory, record length) to destPage's slot directory
            // If page has overflow page, copy all records in page to destPage then copy page's overflow page to page to continue copying like before
            // [CODE]
            bool isLastOverflowPage = false;
            while (!isLastOverflowPage)
            {
                isLastOverflowPage = overflowPagePointer == 0;

                // To store the n_th index of record into slot directory
                int trackRecordOffset = 0;
                
                // variable for record length in slot directory
                int recordLength = 0;

                // Pointer to record 
                int recordOffsetPointer = BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE - POINTER_ADDRESS_SIZE - RECORD_LENGTH_SIZE;
                
                // check all records in page
                for(int i = 0; i < totalRecordsInPage; i++) {
                    // Next Record Offset
                    memcpy(&trackRecordOffset, &page[recordOffsetPointer], POINTER_ADDRESS_SIZE);

                    // Next Record Length
                    memcpy(&recordLength, &page[recordOffsetPointer + POINTER_ADDRESS_SIZE], RECORD_LENGTH_SIZE);
                    int recordTrueSize = SLOT_DIRECTORY_POINTER_SIZE + recordLength;

                    // Get the variable-length record using (Record Offset, Record Length)
                    char *variableRecord = new char[MAXIMUM_RECORD_SIZE];
                    memcpy(&variableRecord[0], &page[trackRecordOffset], recordLength);

                    // Read all field pointers from record
                    int idPointer = 0;
                    memcpy(&idPointer, &variableRecord[0], POINTER_ADDRESS_SIZE);

                    // Read employee ID attribute
                    int employeeId = 0;
                    memcpy(&employeeId, &variableRecord[idPointer], sizeof(int));

                    // If the new hash of employeeId belongs to the new bucket, move the record to the new page
                    if (hash(employeeId) == (n-1)) {
                        // Create an overflow page if there is little free space in the new page [WIP]
                        if (pageRemaining < recordLength) {
                            overflowPagePointer2 = nextFreeBlock;
                            
                            memcpy(destPage + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE, &freeSpacePointer2, POINTER_ADDRESS_SIZE);
                            memcpy(destPage + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE, &overflowPagePointer2, POINTER_ADDRESS_SIZE);
                            memcpy(destPage + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE, &totalRecordsInPage2, NUMBER_RECORD_SIZE);
                            memcpy(destPage + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE, &pageRemaining2, PAGE_REMAINING_SIZE);
                            
                            modifyFileOnDisk(currentPageAddress2, destPage, BLOCK_SIZE);

                            // Reset page for a new overflow page
                            memset(page, '\0', BLOCK_SIZE);
                            resetSlotDirectory();
                            saveSlotDirectory();
                            // Save the overflow page to disk
                            saveToDiskAtTheEndOfFile(page, BLOCK_SIZE);
                            currentPageAddress = nextFreeBlock;
                            nextFreeBlock += BLOCK_SIZE;
                        }
                        // Copy variable length record to the memory page
                        memcpy(destPage + freeSpacePointer2, &variableRecord[0], recordLength);
                        int recordAddress = freeSpacePointer2;
                        freeSpacePointer2 += recordLength;
                        
                        // Get (offset, length) to the slot directory
                        memcpy(destPage + latestRecordOffset2 - POINTER_ADDRESS_SIZE - RECORD_LENGTH_SIZE, &recordAddress, POINTER_ADDRESS_SIZE);
                        memcpy(destPage + latestRecordOffset2 - POINTER_ADDRESS_SIZE, &recordLength, RECORD_LENGTH_SIZE);
                        latestRecordOffset2 = latestRecordOffset2 - POINTER_ADDRESS_SIZE - RECORD_LENGTH_SIZE;
                        
                        // Update free space pointer, the number of variable length records in the memory page,
                        // decrease free space of the page
                        totalRecordsInPage2 += 1;
                        pageRemaining2 = pageRemaining2 - recordTrueSize;
                        
                        memcpy(destPage + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE, &freeSpacePointer2, POINTER_ADDRESS_SIZE);
                        memcpy(destPage + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE, &overflowPagePointer2, POINTER_ADDRESS_SIZE);
                        memcpy(destPage + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE, &totalRecordsInPage2, NUMBER_RECORD_SIZE);
                        memcpy(destPage + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE, &pageRemaining2, PAGE_REMAINING_SIZE);

                        // Delete the record from the old page (it is still there physically, kinda like multi-session CD)
                        trackRecordOffset = -1;
                        memcpy(&page[recordOffsetPointer], &trackRecordOffset, POINTER_ADDRESS_SIZE);
                    }

                    recordOffsetPointer = recordOffsetPointer - POINTER_ADDRESS_SIZE - RECORD_LENGTH_SIZE;

                    delete[] variableRecord;
                }

                // Load overflow page if it exists to find more results
                if (overflowPagePointer > 0) {
                    modifyFileOnDisk(currentPageAddress, page, BLOCK_SIZE);
                    currentPageAddress = overflowPagePointer;
                    loadPage();
                }
            }

            // Save the old page, copy the new page to current page
            modifyFileOnDisk(currentPageAddress, page, BLOCK_SIZE);
            memcpy(page, &destPage[0], BLOCK_SIZE);
            currentPageAddress = nextFreeBlock;
            nextFreeBlock += BLOCK_SIZE;

            delete[] destPage;
        }

        // If there exists an overflow page, go there instead
        while (overflowPagePointer > 0) {
            currentPageAddress = overflowPagePointer;
            loadPage();
        }

        // Size of string
        int nameLength = record.name.size() + 1; // don't forget NULL termination
        int bioLength = record.bio.size() + 1;   // don't forget NULL termination

        // Size of the variable record (its length and how much it actually takes from page)
        int recordLength = FIELD_OFFSET_POINTER_SIZE + (NUMBER_SIZE * 2) + nameLength + bioLength;
        int recordTrueSize = SLOT_DIRECTORY_POINTER_SIZE + recordLength;

        // Create an overflow page if there is little free space in page
        if (pageRemaining < recordLength) {
            overflowPagePointer = nextFreeBlock;
            saveSlotDirectory();
            modifyFileOnDisk(currentPageAddress, page, BLOCK_SIZE);

            // Reset page for a new overflow page
            memset(page, '\0', BLOCK_SIZE);
            resetSlotDirectory();
            saveSlotDirectory();
            // Save the overflow page to disk
            saveToDiskAtTheEndOfFile(page, BLOCK_SIZE);
            currentPageAddress = nextFreeBlock;
            nextFreeBlock += BLOCK_SIZE;
        }

        addRecordToPage(record, recordLength);
    }

    void addRecordToPage(Record record, int recordLength) {
        // Size of string
        int nameLength = record.name.size() + 1; // don't forget NULL termination
        int bioLength = record.bio.size() + 1;   // don't forget NULL termination

        int recordTrueSize = SLOT_DIRECTORY_POINTER_SIZE + recordLength;

        /*
         * Create a new variable-length record
         * The reason for a sepratate record instead of writing directly to page
         * is to make it easier to calculate 
         */
        char *variableRecord = new char[recordLength];
        memset(variableRecord, '\0', recordLength);
        // The address of the current field relative to the variable length record
        int addressOfCurrentField = FIELD_OFFSET_POINTER_SIZE;
        // The address of the current offset field relative to the variable length record
        int addressOfCurrentOffsetField = 0;

        // Add field to the record and write offset address of every field and the end of the record
        // record.id
        writeFieldToVariableRecord(&record.id, NUMBER_SIZE, true, &variableRecord[0], &addressOfCurrentField, &addressOfCurrentOffsetField);

        // record.name
        writeFieldToVariableRecord(&record.name[0], nameLength, false, &variableRecord[0], &addressOfCurrentField, &addressOfCurrentOffsetField);

        // record.bio
        writeFieldToVariableRecord(&record.bio[0], bioLength, false, &variableRecord[0], &addressOfCurrentField, &addressOfCurrentOffsetField);

        // record.manager_id
        writeFieldToVariableRecord(&record.manager_id, NUMBER_SIZE, true, &variableRecord[0], &addressOfCurrentField, &addressOfCurrentOffsetField);

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
        
        // Update total records in the index file, the number of variable length records in the memory page,
        // decrease free space of the page, total page
        numRecords += 1;
        totalRecordsInPage += 1;
        pageRemaining = pageRemaining - recordTrueSize;
        totalPageRemaining = totalPageRemaining - recordTrueSize;
        saveSlotDirectory();

        modifyFileOnDisk(currentPageAddress, page, BLOCK_SIZE);
        saveBlockDirectory();
        
        delete[] variableRecord;
    }

    // Add field to the new record and write offset address of every field and the end of the record
    void writeFieldToVariableRecord(void* fieldToWrite,
        int size,
        bool isInt,
        char* variableRecord,
        int* addressOfCurrentField,
        int* addressOfCurrentOffsetField
    ) {
        // Determine the size of variable to write to record
        // And actual size of that field to add to current offset field
        // e.g The size of a number field is 8 bytes, while int size is 4 bytes
        int sizeOfVariable = isInt ? sizeof(int) : size;
        int actualSizeOfField = isInt ? NUMBER_SIZE : size;
        memcpy(variableRecord + *addressOfCurrentField, fieldToWrite, sizeOfVariable);
        memcpy(variableRecord + *addressOfCurrentOffsetField, addressOfCurrentField, POINTER_ADDRESS_SIZE);
        *addressOfCurrentField += actualSizeOfField;
        *addressOfCurrentOffsetField += POINTER_ADDRESS_SIZE;
    }

    void getBucketAddress(int key) {
        unsigned int bucketNumber = hash(key);

        // If result is higher than number of bucket, flip the most significant bit to get a lower result 
        if (bucketNumber >= n) bucketNumber = bucketNumber ^ (1 << (i - 1));
        currentPageAddress = blockDirectory[bucketNumber];
    }

    void loadPage() {
        // Get the page from the bucket chosen
        loadFromDisk(currentPageAddress, page, BLOCK_SIZE);

        // Load slot directory to memory
        memcpy(&freeSpacePointer, page + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE, POINTER_ADDRESS_SIZE);
        memcpy(&overflowPagePointer, page + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE, POINTER_ADDRESS_SIZE);
        memcpy(&totalRecordsInPage, page + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE, NUMBER_RECORD_SIZE);
        memcpy(&pageRemaining, page + BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE, PAGE_REMAINING_SIZE);
        latestRecordOffset = BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE - (POINTER_ADDRESS_SIZE + RECORD_LENGTH_SIZE) * totalRecordsInPage;
    }

    void getBucket(int key) {
        getBucketAddress(key);
        loadPage();
    }

    void saveBlockDirectory() {
        char* blockDirectoryRecord = new char[blockDirectoryPageLength];
        memset(blockDirectoryRecord, '\0', blockDirectoryPageLength);

        // Save block directory info to a record
        memcpy(&blockDirectoryRecord[0], &blockDirectoryPageLength, BLOCK_DIRECTORY_LENGTH_SIZE);
        memcpy(&blockDirectoryRecord[0] + BLOCK_DIRECTORY_LENGTH_SIZE, &numRecords, NUMBER_RECORD_SIZE);
        memcpy(&blockDirectoryRecord[0] + BLOCK_DIRECTORY_LENGTH_SIZE + NUMBER_RECORD_SIZE, &totalPageRemaining, PAGE_REMAINING_SIZE);
        memcpy(&blockDirectoryRecord[0] + BLOCK_DIRECTORY_LENGTH_SIZE + NUMBER_RECORD_SIZE + PAGE_REMAINING_SIZE, &i, LSBIT_TO_ADRESS_BUCKET_SIZE);
        memcpy(&blockDirectoryRecord[0] + BLOCK_DIRECTORY_LENGTH_SIZE + NUMBER_RECORD_SIZE + PAGE_REMAINING_SIZE + LSBIT_TO_ADRESS_BUCKET_SIZE, &n, NUMBER_BUCKET_SIZE);
        
        // Save bucket to block directory record
        for (int count = 0; count < n; count++)
        {
            memcpy(&blockDirectoryRecord[0] + BLOCK_DIRECTORY_LENGTH_SIZE + NUMBER_RECORD_SIZE + PAGE_REMAINING_SIZE + LSBIT_TO_ADRESS_BUCKET_SIZE + NUMBER_BUCKET_SIZE + POINTER_ADDRESS_SIZE * count, &blockDirectory[count], POINTER_ADDRESS_SIZE);
        }

        // Save page to disk at the beginning of file
        modifyFileOnDisk(0, &blockDirectoryRecord[0], blockDirectoryPageLength);

        delete[] blockDirectoryRecord;
    }

    void loadBlockDirectory() {
        // Load block directory length first
        loadFromDisk(0, &blockDirectoryPageLength, BLOCK_DIRECTORY_LENGTH_SIZE);

        char* blockDirectoryRecord = new char[blockDirectoryPageLength];
        loadFromDisk(0, &blockDirectoryRecord[0], blockDirectoryPageLength);

        // Load block directory info from a record
        memcpy(&numRecords, &blockDirectoryRecord[0] + BLOCK_DIRECTORY_LENGTH_SIZE, NUMBER_RECORD_SIZE);
        memcpy(&totalPageRemaining, &blockDirectoryRecord[0] + BLOCK_DIRECTORY_LENGTH_SIZE + NUMBER_RECORD_SIZE, PAGE_REMAINING_SIZE);
        memcpy(&i, &blockDirectoryRecord[0] + BLOCK_DIRECTORY_LENGTH_SIZE + NUMBER_RECORD_SIZE + PAGE_REMAINING_SIZE, LSBIT_TO_ADRESS_BUCKET_SIZE);
        memcpy(&n, &blockDirectoryRecord[0] + BLOCK_DIRECTORY_LENGTH_SIZE + NUMBER_RECORD_SIZE + PAGE_REMAINING_SIZE + LSBIT_TO_ADRESS_BUCKET_SIZE, NUMBER_BUCKET_SIZE);
        
        // Load bucket from block directory record, push_back when BlockAddress is not big enough
        int blockDirectoryArraySize = blockDirectory.size();
        int getBlockAddress;
        for (int count = 0; count < n; count++)
        {
            if (count < blockDirectoryArraySize) {
                memcpy(&blockDirectory[count], &blockDirectoryRecord[0] + BLOCK_DIRECTORY_LENGTH_SIZE + NUMBER_RECORD_SIZE + PAGE_REMAINING_SIZE + LSBIT_TO_ADRESS_BUCKET_SIZE + NUMBER_BUCKET_SIZE + POINTER_ADDRESS_SIZE * count, POINTER_ADDRESS_SIZE);
            } else {
                memcpy(&getBlockAddress, &blockDirectoryRecord[0] + BLOCK_DIRECTORY_LENGTH_SIZE + NUMBER_RECORD_SIZE + PAGE_REMAINING_SIZE + LSBIT_TO_ADRESS_BUCKET_SIZE + NUMBER_BUCKET_SIZE + POINTER_ADDRESS_SIZE * count, POINTER_ADDRESS_SIZE);
                blockDirectory.push_back(getBlockAddress);
            }
        }

        delete[] blockDirectoryRecord;
    }

public:
    LinearHashIndex(string indexFileName) {
        n = 4; // Start with 4 buckets in index
        i = 2; // Need 2 bits to address 4 buckets
        numRecords = 0;
        nextFreeBlock = 0;        

        // File read handler for the database file
        dataRead.open(indexFileName, ios::in | ios::binary);

        // Check for existing database file 
        fName = indexFileName;
        if (isDebug || !dataRead.is_open()) {
            indexExisted = false;

            // Create an empty file
            dataWrite.open(fName, ios::out | ios::binary);
            if (!dataWrite.is_open()) {
                throw "Error creating " + fName;
            }
            dataWrite.close();

            // Create a new index file with 1 bucket array page and 4 empty pages
            // make sure to account for the created buckets by incrementing nextFreeBlock appropriately

            // Save empty pages  with default slot directory information to disk
            saveSlotDirectory();
            for (int count = 0; count < n + 1; count++)
            {
                if (count > 0) {
                    saveToDiskAtTheEndOfFile(page, BLOCK_SIZE);
                    // Add page offset to block directory
                    blockDirectory.push_back(nextFreeBlock);
                    nextFreeBlock += BLOCK_SIZE;
                } else {
                    saveToDiskAtTheEndOfFile(page, BLOCK_DIRECTORY_PAGE_SIZE);
                    nextFreeBlock += BLOCK_DIRECTORY_PAGE_SIZE;
                }
            }

            // This function opens and closes its own dataWrite
            saveBlockDirectory();
        }

        dataRead.close();
        loadBlockDirectory();
    }

    // Read csv file and add records to the index
    void createFromFile(string csvFName) {
        /* 
         * Check to see if the index file have already existed,
         * only read csv file when the index file doesn't exist or empty (newly created)
         */ 
        if (!indexExisted) {
            // File read handler for csv file
            ifstream inputFile(csvFName);

            if (!inputFile.is_open()) {
                throw "Error opening " + csvFName;
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
                insertRecord(record);
            }

            inputFile.close();
        }
    }

    // Given an ID, find the relevant records and print them
    vector<Record> findRecordsById(int id) {
        vector<Record> results;
        getBucket(id);

        // Flag to check if this is the last page in a chain of (overflow )pages
        bool isLastOverflowPage = false;

        while (!isLastOverflowPage)
        {
            isLastOverflowPage = overflowPagePointer == 0;

            // To store the n_th index of record into slot directory
            int trackRecordOffset = 0;
            
            // variable for record length in slot directory
            int recordLength = 0;

            // Pointer to record 
            int recordOffsetPointer = BLOCK_SIZE - FREE_SPACE_POINTER_SIZE - OVERFLOW_PAGE_POINTER_SIZE - NUMBER_RECORD_SIZE - PAGE_REMAINING_SIZE - POINTER_ADDRESS_SIZE - RECORD_LENGTH_SIZE;
            
            // check all records in page
            for(int i = 0; i < totalRecordsInPage; i++) {
                // Next Record Offset
                memcpy(&trackRecordOffset, &page[recordOffsetPointer], POINTER_ADDRESS_SIZE);

                // Next Record Length
                memcpy(&recordLength, &page[recordOffsetPointer + POINTER_ADDRESS_SIZE], RECORD_LENGTH_SIZE);

                // Get the variable-length record using (Record Offset, Record Length)
                char *variableRecord = new char[MAXIMUM_RECORD_SIZE];
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

                    results.push_back(Record(employeeInfo));
                }

                recordOffsetPointer = recordOffsetPointer - POINTER_ADDRESS_SIZE - RECORD_LENGTH_SIZE;

                delete[] variableRecord;
            }

            // Load overflow page if it exists to find more results
            if (overflowPagePointer > 0) {
                currentPageAddress = overflowPagePointer;
                loadPage();
            }
        }

        dataRead.close();
        return results;
    }
};
