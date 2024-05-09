// Block size allowed in main memory according to the question 
static const int BLOCK_SIZE = 4096;
const int BLOCK_DIRECTORY_PAGE_SIZE = 256;
const int BLOCK_DIRECTORY_LENGTH_SIZE = sizeof(int);
const int POINTER_ADDRESS_SIZE = sizeof(int);
const int NUMBER_RECORD_SIZE = sizeof(int);
const int NUMBER_BUCKET_SIZE = sizeof(int);
const int LSBIT_TO_ADRESS_BUCKET_SIZE = sizeof(int);
const int RECORD_LENGTH_SIZE = sizeof(int);
const int PAGE_REMAINING_SIZE = sizeof(int);
const int FREE_SPACE_POINTER_SIZE = POINTER_ADDRESS_SIZE;
const int OVERFLOW_PAGE_POINTER_SIZE = POINTER_ADDRESS_SIZE;
// Size of 2 pointers for (offset, length) of a record in the slot directory
const int SLOT_DIRECTORY_POINTER_SIZE = POINTER_ADDRESS_SIZE * 2;
// Size of offset address for 4 fields + 1 offset end address of a record
const int FIELD_OFFSET_POINTER_SIZE = POINTER_ADDRESS_SIZE * 5;
// Size of integer fields
const int NUMBER_SIZE = 8;
static const int MAXIMUM_RECORD_SIZE = NUMBER_SIZE * 2 + 500 + 200;
// Threshold to create new bucket
const int FREE_SPACE_THRESHOLD_FOR_NEW_BUCKET = 0.3;