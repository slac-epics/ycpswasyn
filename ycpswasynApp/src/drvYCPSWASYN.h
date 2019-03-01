/**
 *-----------------------------------------------------------------------------
 * Title      : YCPSW EPICS module driver
 * ----------------------------------------------------------------------------
 * File       : drvYCPSWASYN.h
 * Author     : Jesus Vasquez, jvasquez@slac.stanford.edu
 * Created    : 2016-08-25
 * ----------------------------------------------------------------------------
 * Description:
 * YCPSW EPICS module driver.
 * ----------------------------------------------------------------------------
 * This file is part of l2Mps. It is subject to
 * the license terms in the LICENSE.txt file found in the top-level directory
 * of this distribution and at:
    * https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
 * No part of l2Mps, including this file, may be
 * copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.txt file.
 * ----------------------------------------------------------------------------
**/

#include <stdio.h>
#include <string.h>
#include <fstream>
#include <boost/array.hpp>
#include "asynPortDriver.h"

#include <cpsw_api_builder.h>
#include <cpsw_api_user.h>
#include <yaml-cpp/yaml.h>

#define DRIVER_NAME     "YCPSWASYN"

// Key and substitution string to look for when creating the record name from it path
#define CARRIER_KEY     "CarrierCore"   // Carrier core
#define CARRIER_SUBS    "C"
#define BAY0_KEY        "Bay0"          // Bay 0
#define BAY0_SUBS       "B0"
#define BAY1_KEY        "Bay1"          // Bay 1
#define BAY1_SUBS       "B1"
#define APP_KEY         "App"           // Application
#define APP_SUBS        "A"
#define STREAM_KEY      "Stream"        // Stream
#define STREAM_SUBS     ""

#define DB_NAME_PREFIX_LENGTH_MAX   10      // Max lenght of the record prefix name provided by the user
#define DB_DESC_LENGTH_MAX          28      // Max lenght of the record description field
#define DB_MBBX_NELEM_MAX           16      // Max number of menu entries on a MBBx record
#define DB_NAME_PATH_TRIM_SIZE      3       // Number of chars that the name of the device will be trim to
#define DB_NAME_SUFFIX_LENGHT       3       // Length of the record name sufix

// Record and PV list dump file definitions
#define DUMP_FILE_PATH              "/tmp/"
#define REG_DUMP_TEXT_FILE_NAME     "regMap.txt"
#define REG_DUMP_YAML_FILE_NAME     "regMap.yaml"
#define PV_DUMP_FILE_NAME           "pvList.txt"
#define KEYS_NOT_FOUND_FILE_NAME    "keysNotFound.txt"
#define MAP_TOP_FILE_NAME           "map_top"
#define MAP_FILE_NAME               "map"

// These are the drvInfo strings that are used to identify the parameters.
// They are used by asyn clients, including standard asyn device support
#define loadConfigString        "CONFIG_LOAD"
#define saveConfigString        "CONFIG_SAVE"
#define loadConfigFileString    "CONFIG_LOAD_FILE"
#define saveConfigFileString    "CONFIG_SAVE_FILE"
#define loadConfigStatusString  "CONFIG_LOAD_STATUS"
#define saveConfigStatusString  "CONFIG_SAVE_STATUS"
#define loadConfigRootString    "CONFIG_LOAD_ROOT"
#define saveConfigRootString    "CONFIG_SAVE_ROOT"

#define mapTopFileVar           "YCPSWASYN_MAP_TOP_FILE"
#define mapFileVar              "YCPSWASYN_MAP_FILE"

// MBBx record menu value names
char const *mbbxValParam[] =
{
    "ZRVL",
    "ONVL",
    "TWVL",
    "THVL",
    "FRVL",
    "FVVL",
    "SXVL",
    "SVVL",
    "EIVL",
    "NIVL",
    "TEVL",
    "ELVL",
    "TVVL",
    "TTVL",
    "FTVL",
    "FFVL"
};

// MBBx record menu string names
char const *mbbxNameParams[] =
{
    "ZRST",
    "ONST",
    "TWST",
    "THST",
    "FRST",
    "FVST",
    "SXST",
    "SVST",
    "EIST",
    "NIST",
    "TEST",
    "ELST",
    "TVST",
    "TTST",
    "FTST",
    "FFST"
};

// Types of register interfaces
enum registerInterfaceTypeList
{
    DEV_REG_RO,
    DEV_REG_RW,
    DEV_FLOAT_RO,
    DEV_FLOAT_RW,
    DEV_CMD,
    DEV_STM,
    DEV_CONFIG,
    DEV_SIZE
};

char const *regInterfaceTypeNames[] =
{
  "INT,RO",
  "INT,RW",
  "FLT,RO",
  "FLT,RW",
  "CMD,RW",
  "STM,RO",
  "CFG,RW",
};

// Type of registers
enum regTypeList
{
    REG_SINGLE,     // Single value register
    REG_ENUM,       // Enum register
    REG_ARRAY,      // Array registers
    REG_ARRAY_8,    // 8-bit array register
    REG_STRING,     // String register
    REG_STREAM,     // Stream register
    REG_SIZE
};

char const *regTypeNames[] =
{
  "SCL",
  "ENM",
  "ARR",
  "AR8",
  "STR",
  "STM"
};

// Type of interfaces
enum waveformTypeList
{
    WF_32_BIT,      // 32-bit data
    WF_16_BIT,      // 16-bit data
    WF_SIZE
};

// record template list (all interfaces but streams)
const char *templateList[DEV_SIZE - 1][REG_SIZE-1] =
{
     // DEV_SINGLE,             // REG_ENUM             // REG_ARRAY                        // REG_ARRAY_8                  // REG_STRING
    {"db/longin.template",      "db/mbbi.template",     "db/waveform_in.template",          "db/waveform_8_in.template",    "db/waveform_8_in.template"},   //DEV_REG_RO
    {"db/longout.template",     "db/mbbo.template",     "db/waveform_out.template",         "db/waveform_8_out.template",   "db/waveform_8_out.template"},  //DEV_REG_RW
    {"db/ai.template",          "",                     "db/waveform_in_float.template",    "",                             ""},                            //DEV_FLOAT_RO
    {"db/ao.template",          "",                     "db/waveform_out_float.template",   "",                             ""},                            //DEV_FLOAT_RW
    {"db/bo.template",          "",                     "",                                 "",                             ""},                            //DEV_CMD
};

// Record template list (oly for streans)
const char * templateListWaforms[WF_SIZE] =
{
    "db/waveform_stream32.template",    "db/waveform_stream16.template" //DEV_STM
};

#define PROCESS_CONFIG_MASK     0x03
enum processConfigurationStates
{
    CONFIG_STAT_IDLE,
    CONFIG_STAT_PROCESSING,
    CONFIG_STAT_SUCCESS,
    CONFIG_STAT_ERROR,
    CONFIGF_STAT_SIZE
};

// Argument list passed to the stream handling thread
typedef struct
{
    void    *pPvt;
    Stream  stm;
    int     param16index;
    int     param32index;
} ThreadArgs;

// Argument list passed to load a record
struct recordParams
{
    std::string recName;
    std::string recDesc;
    std::string recTemplate;
    std::string paramName;
    asynParamType paramType;
    std::string recRbvName;
};

#define MAX_SIGNALS         ((int)DEV_SIZE)                 // Max number of parameter list (size of register type list)
#define NUM_SCALVALS        5000                            // Max number of ScalVals
#define NUM_DOUBLEVALS      100                             // Max number of DoubleVals
#define NUM_CMD             500                             // Max number of commands
#define NUM_PARAMS          (NUM_SCALVALS + NUM_CMD)        // Max number of paramters
#define STREAM_MAX_SIZE     200UL*1024ULL*1024ULL           // Size of the stream buffers

class YCPSWASYNRAIIFile;
class YCPSWKeysNotFound;

class YCPSWASYN : public asynPortDriver
{
    public:
        // Constructor
        YCPSWASYN(const char *portName, Path p, const char *recordPrefix, int autogenerationMode, const char* dictionary);

        // Methods that we override from asynPortDriver
        virtual asynStatus  readInt32           (asynUser *pasynUser, epicsInt32 *value);
        virtual asynStatus  writeInt32          (asynUser *pasynUser, epicsInt32 value);
        virtual asynStatus  readFloat64         (asynUser *pasynUser, epicsFloat64 *value);
        virtual asynStatus  writeFloat64        (asynUser *pasynUser, epicsFloat64 value);
        virtual asynStatus  readInt32Array      (asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn);
        virtual asynStatus  writeInt32Array     (asynUser *pasynUser, epicsInt32 *value, size_t nElements);
        virtual asynStatus  readOctet           (asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason);
        virtual asynStatus  writeOctet          (asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
        virtual asynStatus  readFloat64Array    (asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn);
        virtual asynStatus  writeFloat64Array   (asynUser *pasynUser, epicsFloat64 *value, size_t nElements);
        virtual asynStatus  writeUInt32Digital  (asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask);
        virtual asynStatus  readUInt32Digital   (asynUser *pasynUser, epicsUInt32 *value, epicsUInt32 mask);
        virtual asynStatus  getBounds           (asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high);
        virtual void        report              (FILE *fp, int details);

        // New Methods for this class
        // Streamn hanlding function
        virtual void streamTask(Stream stm, int param16index, int param32index);

        // Initialization routine
        static int YCPSWASYNInit(const char* rootPath, Path *p);

        // Create a record from a Path
        virtual int  CreateRecord(Path p);

        // Default parameters, which can be changed from the IOC shell
        static double       defaultScan;      // Default SCAN value for PVs
        static unsigned int recordNameLenMax; // Max lenght of the record name
        static std::string  mapFilePath;      // Path to map file used in autogeneration mode
        static std::string  debugFilePath;    // Path to dump debug information files

    private:
        const char                          *driverName_;               // Name of the driver (passed from st.cmd)
        Path                                p_;                         // Path on root
        const char                          *portName_;                 // Name of the port (passed from st.cmd)
        std::string                         recordPrefix_;              // Record name prefix defined by the user (passed from st.cmd)
        long                                nRO, nRW, nCMD, nSTM;       // Counter for RO/RW register, command and Stremas found on the YAML file
        long                                nFO, nFW;                   // Counter for Floating point RO/RW registers
        long                                recordCount;                // Counter for the total number of register loaded
        ScalVal                             rw[NUM_SCALVALS];           // Array of ScalVals (RW)
        ScalVal_RO                          ro[NUM_SCALVALS];           // Array of ScalVals (RO)
        DoubleVal                           fw[NUM_DOUBLEVALS];         // Array of DoubleVals (RW)
        DoubleVal_RO                        fo[NUM_DOUBLEVALS];         // Array of DoubleVals (RO)
        Command                             cmd[NUM_CMD];               // Array of Commands
        YCPSWASYNRAIIFile                   *pvDumpFile;                // File with the list of Pvs
        YCPSWKeysNotFound                   *keysNotFound;              // Set of name of elements not found on the substitution map
        std::map<std::string, std::string>  mapTop, map;                // Substitution maps
        int                                 loadConfigValue_;           // Load configuration parameter index
        int                                 saveConfigValue_;           // Save configuration parameter index
        int                                 loadConfigFileValue_;       // Save configuration file name parameter index
        int                                 saveConfigFileValue_;       // Load configuration file name parameter index
        int                                 loadConfigStatusValue_;     // Load configuration status parameter index
        int                                 saveConfigStatusValue_;     // Save configuration status parameter index
        int                                 loadConfigRootValue_;       // Load configuration cpsw root parameter index
        int                                 saveConfigRootValue_;       // Save configuration cpsw root parameter index
        std::string                         loadConfigFileName;         // Load configuration file name
        std::string                         saveConfigFileName;         // Save configuration file name
        std::string                         loadConfigRootPath;         // Load configuration cpsw root
        std::string                         saveConfigRootPath;         // Save configuration cpsw root
        int                                 autogenerationMode_;        // DB autogeneration mode

        // Automatic generation of database from YAML definition  routine
        int autogenerateDatabase(void);

        // Load database from dictionary file routine
        int loadDBFromFile(const char* dictionary);

        // Create the record name from its path
        std::string generateRecordName(const Path& p, const std::string& suffix);

        // Create a record from a register pointer
        template <typename T>
        int CreateRecord(const T& reg);

        // Create a record from a register pointer and its path.
        // This is for Command and Stream which don't support the getPath() function
        template <typename T>
        int CreateRecord(const T& reg, const Path& p);

        // Create a record from a float register pointer
        template <typename T>
        int CreateRecordFloat(const T& reg);

        // Load a EPICS record with the provided infomation
        int LoadRecord(int regType, const recordParams& rp, const std::string& dbParams, Path p);

        // Get the register type
        template <typename T>
        int getRegType(const T& reg);

        // Push the register pointer to its respective list based on the parameter index
        template <typename T>
        void pushParameter(const T& reg, const int& paramIndex);

        // Extract record parameters related to MBBx records
        std::string extractMbbxDbParams(const Enum& isEnum);

        // Load configuration from YAML
        void loadConfiguration();

        // Save configuration to YAML
        void saveConfiguration();

        // Calculate the closest SCAN value for the EPICS record from the YAML pollSecs value
        std::string getEpicsScan(double scan);

        // Creates a parameter for the given register an add its pointer to it list
        template <typename T>
        void addParameter(const T& reg, const std::string& paramName, const asynParamType& paramType);

        // Creates a asyn paramter for the given register
        template <typename T>
        void createRegisterParameter(const T& reg, const std::string& paramName);

        // Creates a asyn paramter for the given float register
        template <typename T>
        void createRegisterParameterFloat(const T& reg, const std::string& paramName);

};

class YCPSWASYNRAIIFile
{
    private:
        FILE      *f_;
        std::string name_;

    public:
        YCPSWASYNRAIIFile(const std::string &name, const char *mode);

        void write(const char * format, ...);

        FILE *f()
        {
            return f_;
        }

        virtual ~YCPSWASYNRAIIFile();
};

class YCPSWASYNGenerateDB : public IPathVisitor
{
    private:
        YCPSWASYN         *drv_;
        int                indent_;
        std::vector<Path>  pathStack_;
        Path               workingPrefix_;
        char               idx_[256];
        YCPSWASYNRAIIFile  textFile;

        void processStack(unsigned level);

    public:
        YCPSWASYNGenerateDB(const std::string &pre, YCPSWASYN *drv);

        virtual bool visitPre(ConstPath here);
        virtual void visitPost(ConstPath here);

        void genereate(Path prefix);

        int
        yamlIndent()
        {
            return indent_;
        }

        int
        pushYamlIndent()
        {
            indent_ += 2;
            return indent_;
        }

        int
        popYamlIndent()
        {
            indent_ -= 2;
            return indent_;
        }

        YCPSWASYNRAIIFile  yamlFile;
};

class YCPSWKeysNotFound
{
    private:
        std::set<std::string> list_;
        std::string           fileName_;

    public:
        YCPSWKeysNotFound(const std::string &fileName);
        void insert(const std::string &element);
        void dump();
};

// Remove '[a-b]' of the name when leaf are arrays
std::string getNameWithoutLeafIndexes(const Path& p);

// Stream handling function caller
static void streamTaskC(void *args);
