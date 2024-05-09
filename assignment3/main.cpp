/*
Skeleton code for linear hash indexing
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
        // Create the index
        LinearHashIndex emp_index("EmployeeIndex");
        emp_index.createFromFile("Employee.csv");
        
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
                vector<Record> results = emp_index.findRecordsById(stoi(input));
                int resultCount = results.size();
                if (resultCount == 0) {
                    cout << "Record not found\n";
                    continue;
                }
                for (int i = 0; i < resultCount; i++)
                {
                    results[i].print();
                    cout << endl;
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
