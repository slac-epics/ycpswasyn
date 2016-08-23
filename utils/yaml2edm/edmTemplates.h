#include <iostream>
#include <string>
#include <sstream>

#define OBJ_KEY		"%%KEY%%"
#define X_KEY		"%%X%%"
#define Y_KEY		"%%Y%%"
#define H_KEY		"%%H%%"
#define W_KEY		"%%W%%"

std::string header_template = "\
4 0 1\n\
beginScreenProperties\n\
major 4\n\
minor 0\n\
release 1\n\
x 2691\n\
y 228\n\
w %%W%%\n\
h %%H%%\n\
font \"helvetica-medium-r-18.0\"\n\
ctlFont \"helvetica-medium-r-12.0\"\n\
btnFont \"helvetica-medium-r-18.0\"\n\
fgColor index 14\n\
bgColor index 4\n\
textColor index 14\n\
ctlFgColor1 index 30\n\
ctlFgColor2 index 32\n\
ctlBgColor1 index 34\n\
ctlBgColor2 index 35\n\
topShadowColor index 37\n\
botShadowColor index 44\n\
showGrid\n\
gridSize 5\n\
endScreenProperties\n\
\n\
";

std::string title_template = "\
# (Static Text)\n\
object activeXTextClass\n\
beginObjectProperties\n\
major 4\n\
minor 1\n\
release 1\n\
x %%X%%\n\
y %%Y%%\n\
w 250\n\
h 40\n\
font \"helvetica-medium-r-18.0\"\n\
fontAlign \"center\"\n\
fgColor index 14\n\
bgColor index 0\n\
useDisplayBg\n\
value {\n\
  \"%%KEY%%\"\n\
}\n\
endObjectProperties\n\
\n\
";

std::string label_template = "\
# (Static Text)\n\
object activeXTextClass\n\
beginObjectProperties\n\
major 4\n\
minor 1\n\
release 1\n\
x %%X%%\n\
y %%Y%%\n\
w 116\n\
h 40\n\
font \"helvetica-bold-r-12.0\"\n\
fgColor index 14\n\
bgColor index 0\n\
useDisplayBg\n\
value {\n\
  \"%%KEY%%\"\n\
}\n\
endObjectProperties\n\
\n\
";

std::string textUpdate_template = "\
# (Text Update)\n\
object TextupdateClass\n\
beginObjectProperties\n\
major 10\n\
minor 0\n\
release 0\n\
x %%X%%\n\
y %%Y%%\n\
w 200\n\
h 40\n\
controlPv \"%%KEY%%\"\n\
displayMode \"hex\"\n\
fgColor index 14\n\
fgAlarm\n\
bgColor index 0\n\
fill\n\
font \"helvetica-medium-r-12.0\"\n\
endObjectProperties\n\
\n\
";

std::string textEntry_template = "\
# (Text Entry)\n\
object TextentryClass\n\
beginObjectProperties\n\
major 10\n\
minor 0\n\
release 0\n\
x %%X%%\n\
y %%Y%%\n\
w 200\n\
h 40\n\
controlPv \"%%KEY%%\"\n\
displayMode \"hex\"\n\
fgColor index 14\n\
bgColor index 0\n\
fill\n\
font \"helvetica-medium-r-12.0\"\n\
endObjectProperties\n\
\n\
";

std::string relatedDisplay_template = "\
# (Related Display)\n\
object relatedDisplayClass\n\
beginObjectProperties\n\
major 4\n\
minor 4\n\
release 0\n\
x %%X%%\n\
y %%Y%%\n\
w 200\n\
h 20\n\
fgColor index 14\n\
bgColor index 4\n\
topShadowColor index 2\n\
botShadowColor index 12\n\
font \"helvetica-medium-r-12.0\"\n\
buttonLabel \"%%KEY%%\"\n\
numPvs 4\n\
numDsps 1\n\
displayFileName {\n\
  0 \"%%KEY%%\"\n\
}\n\
icon\n\
endObjectProperties\n\
";