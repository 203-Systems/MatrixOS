#define DEBUG

#define DEVICENAME "Matrix"
#define MAUNFACTURERNAME "203 Electronic"
#define SERIALSTRING "Matr1x"
#define VID 0x0203
#define VID2 0x0203 // for device ID USE
#define PID 0x0100
#define PID2 0x0100 // for device ID USE
const u8 SYSEXID[3] = {0x00, 0x02, 0x03};

#define FLASHVERSION 0 //Each Flash data strcture change will cause this to increase
#define FWVERSION_STRING "1.0 Dev" //String(MAJOR_VER)+ "." +MINOR_VER+"." +PATCH_VER+(BUILD_VER == 0)?"":("b"+BUILD_VER) 
#define MAJOR_VER 1
#define MINOR_VER 0
#define PATCH_VER 0
#define BUILD_VER 0 //0 for Release, any other number will repensent beta ver