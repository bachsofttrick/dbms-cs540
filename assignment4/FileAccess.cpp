#include <fstream>
#include "FileAccess.h"
using namespace std;

FileAccess::FileAccess(string fName) : fName(fName){}

void FileAccess::loadFromDisk(int address, void* dest, int size) {
    dataRead.open(fName, ios::in | ios::binary);
    if (!dataRead.is_open()) {
        dataRead.close();
        return;
    }
    dataRead.seekg(address, ios::beg);
    dataRead.read(reinterpret_cast<char*>(dest), size);
    dataRead.close();
}

void FileAccess::modifyFileOnDisk(int address, char* src, int size) {
    // ios::out | ios::in prevent truncating the file
    dataWrite.open(fName, ios::out | ios::in | ios::binary);
    if (!dataWrite.is_open()) {
        throw "Error creating " + fName;
    }
    dataWrite.seekp(address, ios::beg);
    dataWrite.write(src, size);
    dataWrite.close();
}

void FileAccess::saveToDiskAtTheEndOfFile(char* src, int size) {
    // ios::app to append to file
    dataWrite.open(fName, ios::app | ios::binary);
    if (!dataWrite.is_open()) {
        throw "Error creating " + fName;
    }
    dataWrite.write(src, size);
    dataWrite.close();
}