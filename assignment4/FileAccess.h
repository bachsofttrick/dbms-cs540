#include <fstream>
using namespace std;

class FileAccess
{
private:
    ifstream dataRead;
    ofstream dataWrite;
    
public:
    string fName;
    
    FileAccess(string fName);
    void loadFromDisk(int address, void* dest, int size);
    void modifyFileOnDisk(int address, char* src, int size);
    void saveToDiskAtTheEndOfFile(char* src, int size);
};


