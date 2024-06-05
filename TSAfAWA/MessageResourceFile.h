//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: MRID_1
//
// MessageText:
//
// %1!s!
//
#define MRID_1                           0x00000001L

//
// MessageId: MRID_PROC_START
//
// MessageText:
//
// %1!s!
//
#define MRID_PROC_START                  0x00000010L

//
// MessageId: MRID_PROC_STOP
//
// MessageText:
//
// %1!s!
//
#define MRID_PROC_STOP                   0x00000011L

//
// MessageId: MRID_INVALID_TVROCK_SCH_FILE
//
// MessageText:
//
// %1!s!
//
#define MRID_INVALID_TVROCK_SCH_FILE     0x00000100L

//
// MessageId: MRID_CANNOT_ACCESS_TVROCK_REGISTORY
//
// MessageText:
//
// %1!s!
//
#define MRID_CANNOT_ACCESS_TVROCK_REGISTORY 0x00000101L

//
// MessageId: MRID_CANNOT_OPEN_TVROCK_SCH_FILE
//
// MessageText:
//
// %1!s!
//
#define MRID_CANNOT_OPEN_TVROCK_SCH_FILE 0x00000102L

//
// MessageId: MRID_ADJUSTED_SCHEDULE
//
// MessageText:
//
// %1!s!
//
#define MRID_ADJUSTED_SCHEDULE           0x00001000L

