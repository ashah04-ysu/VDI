#ifndef VDIFILE
#define VDIFILE
#include <fstream>
#include "vdiheader.h"

using namespace std;

struct VDIFile{
  int file;
  VDIHeader header;
  int fileSize;
  int cursor;
};

#endif
