#include "libraryorderdialog.h"

#include <qstandarditemmodel.h>
#include "settings.h"

#include "ui_libraryorderdialog.h"
#include "librarytreeview.h"

#include <QtDebug>

LibraryOrderDialog::LibraryOrderDialog(QWidget *parent) :
	QDialog(parent, Qt::Popup), _ui(new Ui::LibraryOrderDialog)
{
	_ui->setupUi(this);

	// Artists \ Albums \ Tracks
	QStandardItemModel *artistModel = new QStandardItemModel(this);
	artistModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Artists \\ Albums")));
	artistModel->setHeaderData(0, Qt::Horizontal, SqlDatabase::IP_Artists, Qt::UserRole + 1);
	QStandardItem *artist = new QStandardItem("Artist");
	artistModel->appendRow(artist);
	for (int i = 1; i <= 1; i++) {
		QStandardItem *album = new QStandardItem("Album");
		artist->appendRow(album);
		for (int j = 1; j <= 2; j++) {
			album->appendRow(new QStandardItem("0" + QString::number(j) + ". track #" + QString::number(j)));
		}
	}
	_ui->artistTreeView->setModel(artistModel);

	bool disabled = false;

	// Albums \ Tracks
	QStandardItemModel *albumModel = new QStandardItemModel(this);
	albumModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Albums")));
	albumModel->setHeaderData(0, Qt::Horizontal, SqlDatabase::IP_Albums, Qt::UserRole + 1);
	QStandardItem *album = new QStandardItem("Album");
	albumModel->appendRow(album);
	for (int i = 1; i <= 2; i++) {
		album->appendRow(new QStandardItem("0" + QString::number(i) + ". track #" + QString::number(i)));
	}
	_ui->albumTreeView->setModel(albumModel);
	_ui->albumTreeView->setDisabled(disabled);

	// Artists - Albums \ Tracks
	QStandardItemModel *artistAlbumModel = new QStandardItemModel(this);
	artistAlbumModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Artists – Albums")));
	artistAlbumModel->setHeaderData(0, Qt::Horizontal, SqlDatabase::IP_ArtistsAlbums, Qt::UserRole + 1);
	QStandardItem *artistAlbum_1 = new QStandardItem("Artist – Album");
	artistAlbumModel->appendRow(artistAlbum_1);
	for (int i = 1; i <= 2; i++) {
		artistAlbum_1->appendRow(new QStandardItem("0" + QString::number(i) + ". track #" + QString::number(i)));
	}
	_ui->artistAlbumTreeView->setModel(artistAlbumModel);
	_ui->artistAlbumTreeView->setDisabled(disabled);

	// Year \ Artist - Album \ Tracks
	QStandardItemModel *yearModel = new QStandardItemModel(this);
	yearModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Years")));
	yearModel->setHeaderData(0, Qt::Horizontal, SqlDatabase::IP_Years, Qt::UserRole + 1);
	QStandardItem *year = new QStandardItem("2014");
	yearModel->appendRow(year);
	QStandardItem *artistAlbum_2 = new QStandardItem("Artist – Album");
	year->appendRow(artistAlbum_2);
	for (int j = 1; j <= 2; j++) {
		artistAlbum_2->appendRow(new QStandardItem("0" + QString::number(j) + ". track #" + QString::number(j)));
	}
	_ui->yearTreeView->setModel(yearModel);
	_ui->yearTreeView->setDisabled(disabled);

	Settings *settings = Settings::instance();
	foreach (QTreeView *treeView, findChildren<QTreeView*>()) {
		treeView->expandAll();
		connect(treeView, &QTreeView::clicked, [=]() {
			foreach (QTreeView *treeView_2, findChildren<QTreeView*>()) {
				if (treeView == treeView_2) {
					treeView_2->clearSelection();
					int i = treeView_2->model()->headerData(0, Qt::Horizontal, Qt::UserRole + 1).toInt();
					SqlDatabase::InsertPolicy insertPolicy = (SqlDatabase::InsertPolicy) i;
					// Rebuild library only if the click was on another treeview
					if (insertPolicy != settings->value("insertPolicy").toInt()) {
						settings->setValue("insertPolicy", insertPolicy);
						emit accept();
					}
				}
			}
			this->close();
		});
	}
}

QString LibraryOrderDialog::headerValue() const
{
	switch (Settings::instance()->value("insertPolicy").toInt()) {
	case SqlDatabase::IP_Albums:
		return tr("Album");
	case SqlDatabase::IP_ArtistsAlbums:
		return tr("Artist – Album");
	case SqlDatabase::IP_Years:
		return tr("Year");
	case SqlDatabase::IP_Artists:
	default:
		return tr("Artist \\ Album");
	}
}
