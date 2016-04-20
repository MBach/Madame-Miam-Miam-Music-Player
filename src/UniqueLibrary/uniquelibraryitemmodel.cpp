#include "uniquelibraryitemmodel.h"

#include <model/sqldatabase.h>
#include <albumitem.h>
#include <artistitem.h>
#include <discitem.h>
#include <trackitem.h>
#include "coveritem.h"

#include <QSqlQuery>
#include <QSqlRecord>

#include <QtDebug>

UniqueLibraryItemModel::UniqueLibraryItemModel(QObject *parent)
	: MiamItemModel(parent)
	, _proxy(new UniqueLibraryFilterProxyModel(this))
{
	setColumnCount(2);
	_proxy->setSourceModel(this);
	this->load();
}

QChar UniqueLibraryItemModel::currentLetter(const QModelIndex &index) const
{
	QStandardItem *item = itemFromIndex(_proxy->mapToSource(index));
	if (item && item->type() == Miam::IT_Separator && index.data(Miam::DF_NormalizedString).toString() == "0") {
		return QChar();
	} else if (!index.isValid()) {
		return QChar();
	} else {
		// An item without a valid parent is a top level item, therefore we can extract the letter.
		if (!index.data(Miam::DF_NormalizedString).toString().isEmpty()) {
			return index.data(Miam::DF_NormalizedString).toString().toUpper().at(0);
		} else {
			return QChar();
		}
	}
}

UniqueLibraryFilterProxyModel *UniqueLibraryItemModel::proxy() const
{
	return _proxy;
}

void UniqueLibraryItemModel::insertAlbums(const QList<AlbumDAO> &nodes)
{
	for (AlbumDAO album : nodes) {
		if (album.cover().isEmpty()) {
			appendRow({ nullptr, new AlbumItem(&album) });
		} else {
			appendRow({ new CoverItem(album.cover()), new AlbumItem(&album) });
		}
	}
}

void UniqueLibraryItemModel::insertArtists(const QList<ArtistDAO> &nodes)
{
	for (ArtistDAO artist : nodes) {
		appendRow({ nullptr, new ArtistItem(&artist) });
	}
}

void UniqueLibraryItemModel::insertDiscs(const QList<AlbumDAO> &nodes)
{
	for (AlbumDAO disc : nodes) {
		appendRow({ nullptr, new DiscItem(&disc) });
	}
}

void UniqueLibraryItemModel::insertTracks(const QList<TrackDAO> &nodes)
{
	for (TrackDAO track : nodes) {
		appendRow({ nullptr, new TrackItem(&track) });
	}
	this->proxy()->sort(this->proxy()->defaultSortColumn());
	this->proxy()->setDynamicSortFilter(true);
}

void UniqueLibraryItemModel::load()
{
	this->deleteCache();

	SqlDatabase db;
	db.init();

	QSqlQuery query(db);
	query.setForwardOnly(true);
	if (query.exec("SELECT DISTINCT artistAlbum, artistNormalized, icon host FROM cache")) {
		QList<ArtistDAO> artists;
		while (query.next()) {
			ArtistDAO artist;
			int i = -1;
			//artist.setId(query.record().value(++i).toString());
			artist.setTitle(query.record().value(++i).toString());
			artist.setTitleNormalized(query.record().value(++i).toString());
			artist.setIcon(query.record().value(++i).toString());
			artist.setHost(query.record().value(++i).toString());
			artists.append(artist);
		}
		insertArtists(artists);
	}

	if (query.exec("SELECT DISTINCT albumNormalized, album, year, host, icon, cover FROM cache")) {
		QList<AlbumDAO> albums;
		while (query.next()) {
			AlbumDAO album;
			int i = -1;
			//album.setId(query.record().value(++i).toString());
			album.setTitleNormalized(query.record().value(++i).toString());
			album.setTitle(query.record().value(++i).toString());
			album.setArtist(query.record().value(++i).toString());
			album.setYear(query.record().value(++i).toString());
			album.setHost(query.record().value(++i).toString());
			album.setIcon(query.record().value(++i).toString());
			album.setCover(query.record().value(++i).toString());
			albums.append(album);
		}
		this->insertAlbums(albums);
	}
	if (query.exec("SELECT DISTINCT artistNormalized, artist, disc FROM cache WHERE disc > 0")) {
		QList<AlbumDAO> discs;
		while (query.next()) {
			AlbumDAO disc;
			int i = -1;
			disc.setTitleNormalized(query.record().value(++i).toString());
			disc.setArtist(query.record().value(++i).toString());
			//disc.setId(query.record().value(++i).toString());
			disc.setDisc(query.record().value(++i).toString());
			discs.append(disc);
		}
		this->insertDiscs(discs);
	}

	if (query.exec("SELECT uri, trackNumber, trackTitle, artistAlbum, album, trackLength, rating, disc, host, icon " \
				   "FROM cache")) {
		QList<TrackDAO> tracks;
		while (query.next()) {
			TrackDAO track;
			int i = -1;
			//track.setTitleNormalized(query.record().value(++i).toString());
			track.setUri(query.record().value(++i).toString());
			track.setTrackNumber(query.record().value(++i).toString());
			track.setTitle(query.record().value(++i).toString());
			track.setArtist(query.record().value(++i).toString());
			track.setAlbum(query.record().value(++i).toString());
			track.setLength(query.record().value(++i).toString());
			track.setRating(query.record().value(++i).toInt());
			track.setDisc(query.record().value(++i).toString());
			track.setHost(query.record().value(++i).toString());
			tracks.append(track);
		}
		this->insertTracks(tracks);
	}
}
