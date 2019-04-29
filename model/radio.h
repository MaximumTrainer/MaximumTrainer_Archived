#ifndef RADIO_H
#define RADIO_H

#include <QString>

class Radio
{
public:
    Radio(QString name, QString genre, bool gotAds, int bitrate, QString language, QString url);


    /// GETTERS
    QString getName() const {
        return this->name;
    }
    QString getGenre() const {
        return this->genre;
    }
    bool getGotAds() const {
        return this->gotAds;
    }
    int getBitrate() const {
        return this->bitrate;
    }
    QString getLanguage() const {
        return this->language;
    }
    QString getUrl() const {
        return this->url;
    }



private :
    QString name;
    QString genre;
    bool gotAds;
    int bitrate;
    QString language;
    QString url;

};

#endif // RADIO_H
