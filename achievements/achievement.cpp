#include "achievement.h"

#include <QLabel>




Achievement::~Achievement() {
}



Achievement::Achievement(int idDB, QString name, QString description, QString iconUrl) {

    this->idDB = idDB;
    this->name = name;
    this->description = description;
    this->iconUrl = iconUrl;
    this->completed = false;

}


int Achievement::getId() {
    return this->idDB;
}

QString Achievement::getName() {
    return this->name;
}
QString Achievement::getDescription() {
    return this->description;
}
QString Achievement::getIconUrl() {
    return this->iconUrl;
}
bool Achievement::isCompleted() {
    return this->completed;
}

void Achievement::setCompleted(bool completed) {
    this->completed = completed;
}
