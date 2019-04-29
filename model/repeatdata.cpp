#include "repeatdata.h"


RepeatData::RepeatData() {

    this->id = 0;
    this->firstRow = 0;
    this->lastRow = 0;
    this->numberRepeat = 0;
}

RepeatData::RepeatData(int id, int firstRow, int lastRow, int numberRepeat)
{
    this->id = id;
    this->firstRow = firstRow;
    this->lastRow = lastRow;
    this->numberRepeat = numberRepeat;
}
