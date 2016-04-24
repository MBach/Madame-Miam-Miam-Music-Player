#include "searchdialog.h"

#include <QTimer>
#include <QStandardItemModel>
#include <QStylePainter>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

#include "viewplaylists.h"
#include <settings.h>
#include <model/sqldatabase.h>

#include <QtDebug>

/** Constructor. */
SearchDialog::SearchDialog(ViewPlaylists *viewPlaylists)
	: AbstractSearchDialog(viewPlaylists, Qt::Widget)
	, _viewPlaylists(viewPlaylists)
	, _checkBoxLibrary(new QCheckBox(tr("Library"), this))
	, _isMaximized(false)
{
	this->setupUi(this);
	_artists->setAttribute(Qt::WA_MacShowFocusRect, false);
	_albums->setAttribute(Qt::WA_MacShowFocusRect, false);
	_tracks->setAttribute(Qt::WA_MacShowFocusRect, false);

	_artists->setModel(new QStandardItemModel(this));
	_albums->setModel(new QStandardItemModel(this));
	_tracks->setModel(new QStandardItemModel(this));

	_artists->setUniformItemSizes(true);
	_albums->setUniformItemSizes(true);
	_tracks->setUniformItemSizes(true);

	// Init map with empty values
	for (QListView *list : this->findChildren<QListView*>()) {
		_hiddenItems.insert(list, QList<QStandardItem*>());
	}

	_checkBoxLibrary->setChecked(true);
	this->addSource(_checkBoxLibrary);

	connect(closeButton, &QPushButton::clicked, this, &SearchDialog::clear);
	connect(labelSearchMore, &QLabel::linkActivated, this, &SearchDialog::searchLabelWasClicked);

	// Unselect the 2 other lists when one is clicking on another one
	for (QListView *list : findChildren<QListView*>()) {
		connect(list->selectionModel(), &QItemSelectionModel::currentRowChanged, this, [=]() {
			for (QListView *otherList : findChildren<QListView*>()) {
				if (list != otherList) {
					otherList->selectionModel()->clear();
				}
			}
		});
	}

	connect(this, &SearchDialog::aboutToSearch, this, &SearchDialog::localSearch);

	// Update font size
	connect(SettingsPrivate::instance(), &SettingsPrivate::fontHasChanged, this, [=](SettingsPrivate::FontFamily ff, const QFont &newFont) {
		if (ff == SettingsPrivate::FF_Library) {
			for (QWidget *o : this->findChildren<QWidget*>()) {
				o->setFont(newFont);
			}
		}
	});

	_viewPlaylists->installEventFilter(this);

	this->setVisible(false);
	_oldRect = this->geometry();

	connect(_artists, &QListView::doubleClicked, this, &SearchDialog::artistWasDoubleClicked);
	connect(_albums, &QListView::doubleClicked, this, &SearchDialog::albumWasDoubleClicked);
	connect(_tracks, &QListView::doubleClicked, this, &SearchDialog::trackWasDoubleClicked);
}

SearchDialog::~SearchDialog()
{

}

/** Required interface from AbstractSearchDialog class. */
void SearchDialog::addSource(QCheckBox *checkBox)
{
	int i = sources_layout->count(); // Default are: HSpacer
	checkBox->setFont(SettingsPrivate::instance()->font(SettingsPrivate::FF_Library));
	sources_layout->insertWidget(i - 1, checkBox);

	connect(checkBox, &QCheckBox::toggled, this, &SearchDialog::toggleItems);
}

/** String to look for on every registered search engines. */
void SearchDialog::setSearchExpression(const QString &text)
{
	for (QListView *view : this->findChildren<QListView*>()) {
		auto model = view->model();
		while (model->rowCount() != 0) {
			model->removeRow(0);
		}
	}
	emit aboutToSearch(text);
}

bool SearchDialog::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == _viewPlaylists && event->type() == QEvent::Resize) {
		if (this->isVisible() && _isMaximized) {
			this->move(0, 0);
			this->resize(_viewPlaylists->rect().size());
		}
	}
	return AbstractSearchDialog::eventFilter(obj, event);
}

/** Custom rendering. */
void SearchDialog::paintEvent(QPaintEvent *)
{
	QStylePainter p(this);
	QPalette palette = QApplication::palette();
	p.setPen(palette.mid().color());
	p.setBrush(palette.base());
	p.drawRect(rect().adjusted(0, 0, -1, -1));
	p.setPen(palette.midlight().color());
	p.drawLine(1, labelSearchMore->height() - 1, rect().width() - 2, labelSearchMore->height() - 1);
	int y = rect().y() + rect().height() - aggregated->height();
	p.drawLine(39, rect().y() + labelSearchMore->height(), 39, y);
	p.drawLine(1, y, rect().width() - 2, y);
}

/** Process results sent back from various search engines (local, remote). */
void SearchDialog::processResults(Request type, const QStandardItemList &results)
{
	QListView *listToProcess = nullptr;
	switch (type) {
	case Artist:
		listToProcess = _artists;
		break;
	case Album:
		listToProcess = _albums;
		break;
	case Track:
		listToProcess = _tracks;
		break;
	}
	QStandardItemModel *m = qobject_cast<QStandardItemModel*>(listToProcess->model());
	for (int i = 0; i < results.size(); i++) {
		m->insertRow(0, results.at(i));
	}
	m->sort(0);
	listToProcess->setFixedHeight(listToProcess->model()->rowCount() * listToProcess->sizeHintForRow(0));
	qDebug() << "number of items" << listToProcess->model()->rowCount();
	qDebug() << "size h f r 1" << _artists->sizeHintForRow(0) << _albums->sizeHintForRow(0) << _tracks->sizeHintForRow(0);
	qDebug() << "size h f r 2" << iconArtists->height() << iconAlbums->height() << iconTracks->height();
	int ar = qMax(_artists->model()->rowCount() * _artists->sizeHintForRow(0), iconArtists->height());
	int al = qMax(_albums->model()->rowCount() * _albums->sizeHintForRow(0), iconAlbums->height());
	int tr = qMax(_tracks->model()->rowCount() * _tracks->sizeHintForRow(0), iconTracks->height());
	artistLayoutWidget->setFixedHeight(ar);
	albumLayoutWidget->setFixedHeight(al);
	trackLayoutWidget->setFixedHeight(tr);
	qDebug() << "ar al tr" << ar << al << tr;

	int h = ar + al + tr;
	//int h = 300;
	h += labelSearchMore->height() + aggregated->height() + 3;
	int minW = qMax(iconArtists->width() + _artists->sizeHintForColumn(0), 400);
	this->resize(minW, h);
}

void SearchDialog::aboutToProcessRemoteTracks(const std::list<TrackDAO> &tracks)
{
	Playlist *p = _viewPlaylists->tabPlaylists->currentPlayList();
	p->insertMedias(-1, QList<TrackDAO>::fromStdList(tracks));
	this->clear();
}

void SearchDialog::moveSearchDialog(int, int)
{
	QPoint tl = _viewPlaylists->widgetSearchBar->frameGeometry().topRight();
	tl.ry()--;
	QPoint tl2 = _viewPlaylists->widgetSearchBar->mapTo(_viewPlaylists, tl);
	this->move(tl2);
}

void SearchDialog::clear()
{
	//this->close();
	_isMaximized = false;
	iconSearchMore->setPixmap(QPixmap(":/icons/search"));
	labelSearchMore->setText(tr("<a href='#more' style='text-decoration: none; color:#3399FF;'>Search for more results...</a>"));
	this->setVisible(false);
	this->setGeometry(_oldRect);
	_artists->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_albums->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_tracks->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void SearchDialog::artistWasDoubleClicked(const QModelIndex &artistIndex)
{
	SqlDatabase db;
	db.init();
	QSqlQuery selectTracks(db);
	selectTracks.prepare("SELECT t.uri FROM tracks t INNER JOIN albums al ON t.albumId = al.id " \
		"INNER JOIN artists a ON t.artistId = a.id WHERE a.id = ? ORDER BY al.year");
	QString artistId = artistIndex.data(DT_Identifier).toString();
	selectTracks.addBindValue(artistId);
	if (selectTracks.exec()) {
		QList<QMediaContent> tracks;
		while (selectTracks.next()) {
			tracks << QMediaContent(QUrl::fromLocalFile(selectTracks.record().value(0).toString()));
		}

		Playlist *p = _viewPlaylists->tabPlaylists->currentPlayList();
		p->insertMedias(-1, tracks);
		this->clear();
	}
}

void SearchDialog::albumWasDoubleClicked(const QModelIndex &albumIndex)
{
	SqlDatabase db;
	db.init();
	QSqlQuery selectTracks(db);
	selectTracks.prepare("SELECT t.uri FROM tracks t INNER JOIN albums al ON t.albumId = al.id WHERE al.id = ?");
	QString albumId = albumIndex.data(DT_Identifier).toString();
	selectTracks.addBindValue(albumId);
	if (selectTracks.exec()) {
		QList<QMediaContent> tracks;
		while (selectTracks.next()) {
			tracks << QMediaContent(QUrl::fromLocalFile(selectTracks.record().value(0).toString()));
		}

		Playlist *p = _viewPlaylists->tabPlaylists->currentPlayList();
		p->insertMedias(-1, tracks);
		this->clear();
	}
	this->clear();
}

void SearchDialog::trackWasDoubleClicked(const QModelIndex &track)
{
	Playlist *p = _viewPlaylists->tabPlaylists->currentPlayList();
	p->insertMedias(-1, { QMediaContent(QUrl::fromLocalFile(track.data(DT_Identifier).toString())) });
	this->clear();
}

void SearchDialog::appendSelectedItem(const QModelIndex &index)
{
	const QStandardItemModel *m = qobject_cast<const QStandardItemModel*>(index.model());

	// At this point, we have to decide if the object that has been double clicked is local or remote
	QStandardItem *item = m->itemFromIndex(index);
	qDebug() << Q_FUNC_INFO << item->text();

	QListView *list = qobject_cast<QListView*>(sender());

	Playlist *p = _viewPlaylists->tabPlaylists->currentPlayList();
	if (item->data(AbstractSearchDialog::DT_Origin).toString() == _checkBoxLibrary->text()) {
		QList<QMediaContent> tracks;
		// Local items: easy to process! (SQL request)
		if (list == _artists) {
			// Select all tracks from this Artist
		} else if (list == _albums) {
			// Select all tracks from this Album
		} else /*if (list == _tracks)*/ {
			// Nothing special
		}
		p->insertMedias(-1, tracks);
	} else {
		// Remote items: apply strategy pattern to get remote information depending on the caller
		///FIXME
		//QList<TrackDAO> tracks;
		//p->insertMedias(-1, tracks);
	}
}

/** Local search for matching expressions. */
void SearchDialog::localSearch(const QString &text)
{
	if (!_checkBoxLibrary->isChecked()) {
		return;
	}

	SqlDatabase db;
	db.init();

	/// XXX: Factorize this, 3 times the (almost) same code
	QSqlQuery qSearchForArtists(db);
	qSearchForArtists.prepare("SELECT DISTINCT artist FROM cache WHERE artist LIKE :t LIMIT 5");
	qSearchForArtists.bindValue(":t", "%" + text + "%");
	if (qSearchForArtists.exec()) {
		QList<QStandardItem*> artistList;
		while (qSearchForArtists.next()) {
			QStandardItem *artist = new QStandardItem(qSearchForArtists.record().value(0).toString());
			artist->setData(_checkBoxLibrary->text(), DT_Origin);
			artistList.append(artist);
		}
		this->processResults(Artist, artistList);
	}

	QSqlQuery qSearchForAlbums(db);
	qSearchForAlbums.prepare("SELECT DISTINCT album, artist FROM cache WHERE album LIKE :t LIMIT 5");
	qSearchForAlbums.bindValue(":t", "%" + text + "%");
	if (qSearchForAlbums.exec()) {
		QList<QStandardItem*> albumList;
		while (qSearchForAlbums.next()) {
			QStandardItem *album = new QStandardItem(qSearchForAlbums.record().value(0).toString() + " – " + qSearchForAlbums.record().value(1).toString());
			album->setData(_checkBoxLibrary->text(), DT_Origin);
			albumList.append(album);
		}
		this->processResults(Album, albumList);
	}

	QSqlQuery qSearchForTracks(db);
	qSearchForTracks.prepare("SELECT DISTINCT trackTitle, COALESCE(artistAlbum, artist), uri FROM cache WHERE trackTitle LIKE :t LIMIT 5");
	qSearchForTracks.bindValue(":t", "%" + text + "%");
	if (qSearchForTracks.exec()) {
		QList<QStandardItem*> trackList;
		while (qSearchForTracks.next()) {
			QSqlRecord r = qSearchForTracks.record();
			QStandardItem *track = new QStandardItem(r.value(0).toString() + " – " + r.value(1).toString());
			track->setData(_checkBoxLibrary->text(), DT_Origin);
			trackList.append(track);
		}
		this->processResults(Track, trackList);
	}
}

/** Expand this dialog to all available space. */
void SearchDialog::searchLabelWasClicked(const QString &link)
{
	if (link == "#more") {
		_isMaximized = true;
		iconSearchMore->setPixmap(QPixmap(":/icons/back"));
		labelSearchMore->setText(tr("<a href='#less' style='text-decoration: none; color:#3399FF;'>Show less results</a>"));
		this->move(0, 0);
		this->resize(_viewPlaylists->rect().size());
		this->searchMoreResults();
		_artists->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		_albums->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		_tracks->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	} else {
		_isMaximized = false;
		iconSearchMore->setPixmap(QPixmap(":/icons/search"));
		labelSearchMore->setText(tr("<a href='#more' style='text-decoration: none; color:#3399FF;'>Search for more results...</a>"));
		this->resize(_oldRect.size());
		this->moveSearchDialog();
		_artists->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		_albums->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		_tracks->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}
}

/** Start search again more but fetch more results. */
void SearchDialog::searchMoreResults()
{
	/// TODO
	qDebug() << Q_FUNC_INFO << "not implemented";
}

void SearchDialog::toggleItems(bool enabled)
{
	QCheckBox *checkBox = qobject_cast<QCheckBox*>(sender());
	for (QListView *list : this->findChildren<QListView*>()) {
		QStandardItemModel *m = qobject_cast<QStandardItemModel*>(list->model());
		// Hiding / restoring items has to be done in 2-steps
		// First step is for finding items that are about to be moved
		// Second step is for iterating backward on marked items -> you cannot remove items on a single for loop
		if (enabled) {
			// Restore hidden items for every list
			QList<QStandardItem*> items = _hiddenItems.value(list);
			QList<int> indexes;
			for (int i = 0; i < items.size(); i++) {
				QStandardItem *item = items.at(i);
				// Extract only matching items
				if (item->data(AbstractSearchDialog::DT_Origin) == checkBox->text()) {
					indexes.prepend(i);
				}
			}

			// Moving back from hidden to visible
			for (int i = 0; i < indexes.size(); i++) {
				QStandardItem *item = items.takeAt(indexes.at(i));
				m->appendRow(item);
			}

			// Replace existing values with potentially empty list
			_hiddenItems.insert(list, items);
			m->sort(0);
		} else {
			// Hide items for every list
			QStandardItemModel *m = qobject_cast<QStandardItemModel*>(list->model());
			QList<QStandardItem*> items;
			QList<QPersistentModelIndex> indexes;
			for (int i = 0; i < m->rowCount(); i++) {
				QStandardItem *item = m->item(i, 0);
				if (item->data(AbstractSearchDialog::DT_Origin).toString() == checkBox->text()) {
					indexes << m->index(i, 0);
					// Default copy-constructor is protected!
					QStandardItem *copy = new QStandardItem(item->text());
					copy->setData(checkBox->text(), AbstractSearchDialog::DT_Origin);
					copy->setIcon(item->icon());
					items.append(copy);
				}
			}

			for (const QPersistentModelIndex &i : indexes) {
				m->removeRow(i.row());
			}

			// Finally, hide selected items
			if (!items.isEmpty()) {
				QList<QStandardItem*> hItems = _hiddenItems.value(list);
				hItems.append(items);
				_hiddenItems.insert(list, hItems);
			}
		}
	}
}
