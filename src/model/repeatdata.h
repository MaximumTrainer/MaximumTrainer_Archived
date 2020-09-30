#ifndef REPEATDATA_H
#define REPEATDATA_H

#include <QtCore>

class RepeatData
{

public:
    ~RepeatData() {}
    RepeatData();
    RepeatData(int id, int firstRow, int lastRow, int numberRepeat);

    ///Setters
    void setId(int id) {
        this->id = id;
    }
    void setFirstRow(int firstRow) {
        this->firstRow = firstRow;
    }
    void setLastRow(int lastRow) {
        this->lastRow = lastRow;
    }
    void setNumberRepeat(int numberRepeat) {
        this->numberRepeat = numberRepeat;
    }
    ///Getters
    int getId() const {
        return this->id;
    }
    int getFirstRow() const {
        return this->firstRow;
    }
    int getLastRow() const {
        return this->lastRow;
    }
    int getNumberRepeat() const {
        return this->numberRepeat;
    }
    int getNumberOfRows() const {
        return (this->lastRow - this->firstRow +1);
    }


private :

    int id;
    int firstRow;
    int lastRow;
    int numberRepeat;



};
Q_DECLARE_METATYPE(RepeatData)


#endif // REPEATDATA_H





