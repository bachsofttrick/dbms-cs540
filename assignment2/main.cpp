/*
Skeleton code for storage and buffer management
*/

#include <string>
#include <ios>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include "classes.h"
using namespace std;

int main(int argc, char* const argv[]) {
    try
    {
        // Create the EmployeeRelation file from Employee.csv
        StorageBufferManager manager("EmployeeRelation");
        manager.createFromFile("Employee.csv");
        
        // Loop to lookup IDs until user is ready to quit
        string userInput;
        while (true)
        {
            cout << "Enter the records to print: ";

            // Use getline to read a line of input
            getline(cin, userInput);

            // Declare a vector to store the inputs
            vector<string> inputs;
            stringstream ss(userInput);
            string field;

            // Split the input based on commas
            while (getline(ss, field, ',')) {
                inputs.push_back(field);
            }

            // Accessing elements in vector and print
            for (const auto& input : inputs) {
                Record result = manager.findRecordById(stoi(input));
                if (result.id > 0) {
                    result.print();
                    cout << endl;
                } else {
                    cout << "Record not found\n";
                }
                
            }
        }
        
        return 0;
    }
    catch(char const *e)
    {
        cerr << e << endl;
        return 1;
    }
}
