#include <iostream>
#include <string>
#include <sstream>

#define CARRIER_KEY		"CarrierCore"
#define	CARRIER_SUBS	"C"
#define BAY0_KEY		"Bay0"
#define	BAY0_SUBS		"B0"
#define BAY1_KEY		"Bay1"
#define	BAY1_SUBS		"B1"
#define APP_KEY			"App"
#define	APP_SUBS		"A"
#define STREAM_KEY		"Stream"

#define MAX_NUM_FILES	100

#define WINDOWS_W		880	
#define WINDOWS_H		1500
#define WINDOWS_X0		10
#define	WINDOWS_X1		120
#define	WINDOWS_X2		340
#define	WINDOWS_X3		450
#define	WINDOWS_X4		670
#define WINDOWS_Y0		10
#define WINDOWS_Y_STEP	45

#define MENU_W			500
#define MENU_H			1500
#define MENU_X0			10
#define MENU_Y0			10
#define MENU_Y_STEP		30
#define MENU_X_STEP		20

enum objectTypeList 
{
	HEADER_OBJ,
	TITLE_OBJ,
	LABEL_OBJ,
	TEXTUPDATE_OBJ,
	TEXTENTRY_OBJ,
	RELATEDDISPLAY_OBJ
};

int insertHeader(FILE *f, int w, int h);
int insertObject(FILE *f, int obj_type, const char *name, int x, int y);
void replaceKey(std::string *str, const char *key, const char *value);
void printChildrenPath(Path p);

void generateEDM(Path p);
std::string generatePrefix(Path p);
std::string trimPath(Path p, size_t pos);
