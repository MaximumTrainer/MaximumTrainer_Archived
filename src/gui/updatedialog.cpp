#include "updatedialog.h"
#include "ui_updatedialog.h"

#include <QDebug>
#include <QDesktopServices>
#include "environnement.h"


UpdateDialog::~UpdateDialog()
{
    delete ui;
}


UpdateDialog::UpdateDialog(double versionNumber, QWidget *parent) : QDialog(parent), ui(new Ui::UpdateDialog) {

    ui->setupUi(this);


    ui->buttonBox->addButton(tr("Download"), QDialogButtonBox::AcceptRole);
    ui->label_firstLabelVersion->setText(tr("There is a new mandatory update available"));



    ui->label_firstLabelVersion->setText(ui->label_firstLabelVersion->text() + " (Version " + QString::number(versionNumber, 'f', 2) + ")");

    QString linkNews = "<a href='" + Environnement::getUrlNews()+  "'>";
    linkNews += tr("See new version release notes") + "</a>";

    ui->label_whatsNew->setText(linkNews);

}




//--------------------------------
void UpdateDialog::accept() {

    QDesktopServices::openUrl(QUrl(Environnement::getUrlDownload()));
    QDialog::accept();

}
