#include "radio.h"



Radio::Radio(QString name, QString genre, bool gotAds, int bitrate, QString language, QString url) {

    this->name = name;
    this->genre = genre;
    this->gotAds = gotAds;
    this->bitrate = bitrate;
    this->language = language;
    this->url = url;
}
