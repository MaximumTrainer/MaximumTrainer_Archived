#include "updatedialog.h"
#include "ui_updatedialog.h"

#include <QDebug>
#include <QDesktopServices>
#include "environnement.h"


UpdateDialog::~UpdateDialog()
{
    delete ui;
}


UpdateDialog::UpdateDialog(const QString &latestVersion, QWidget *parent) : QDialog(parent), ui(new Ui::UpdateDialog) {

    ui->setupUi(this);

    ui->buttonBox->addButton(tr("Download"), QDialogButtonBox::AcceptRole);
    ui->buttonBox->addButton(tr("Use Software As-Is"), QDialogButtonBox::RejectRole);
    ui->label_firstLabelVersion->setText(
        tr("There is a new version available") + " (" + latestVersion + ")");

    QString linkNews = "<a href='" + urlGitHubReleasesPage + "'>";
    linkNews += tr("See new version release notes") + "</a>";

    ui->label_whatsNew->setText(linkNews);

}




//--------------------------------
void UpdateDialog::accept() {

    QDesktopServices::openUrl(QUrl(urlGitHubReleasesPage));
    QDialog::accept();

}
