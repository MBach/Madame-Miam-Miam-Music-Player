#ifndef TABPLAYLIST_H
#define TABPLAYLIST_H

#include <QDir>
#include <QTabWidget>
#include <QMouseEvent>

#include <mediabuttons/mediabutton.h>
#include <model/playlistdao.h>
#include <mediaplayer.h>
#include "playlist.h"
#include "playlistmanager.h"
#include "miamtabplaylists_global.hpp"

/**
 * \brief		The TabPlaylist class is used to manage mutiple playlists in the MainWindow class.
 * \author      Matthieu Bachelier
 * \copyright   GNU General Public License v3
 */
class MIAMTABPLAYLISTS_LIBRARY TabPlaylist : public QTabWidget
{
	Q_OBJECT

private:
	MediaPlayer *_mediaPlayer;
	PlaylistManager *_playlistManager;
	QMenu *_contextMenu;
	QAction *_deletePlaylist;

public:
	/** Default constructor. */
	explicit TabPlaylist(QWidget *parent = nullptr);

	virtual ~TabPlaylist();

	/** Get the current playlist. */
	Playlist *currentPlayList() const;

	//QIcon defaultIcon(QIcon::Mode mode) const;

	void init(MediaPlayer *mediaPlayer);

	/** Load a playlist saved in database. */
	void loadPlaylist(uint playlistId);

	/** Get the playlist at index. */
	Playlist *playlist(int index) const;

	QList<Playlist *> playlists() const;

	inline PlaylistManager *playlistManager() const { return _playlistManager; }

protected:
	/** Retranslate context menu. */
	virtual void changeEvent(QEvent *event) override;

	virtual void contextMenuEvent(QContextMenuEvent * event) override;

public slots:
	/** Add a new playlist tab. */
	Playlist* addPlaylist();

	/** Add external folders (from a drag and drop) to the current playlist. */
	void addExtFolders(const QList<QDir> &folders);

	void closePlaylist(int index);

	void deletePlaylist(uint playlistId);

public slots:
	void changeCurrentPlaylistPlaybackMode(QMediaPlaylist::PlaybackMode mode);

	/** Insert multiple tracks chosen by one from the library or the filesystem into a playlist. */
	void insertItemsToPlaylist(int rowIndex, const QList<QUrl> &tracks);

	/** Action sent from the menu. */
	void removeCurrentPlaylist();

	void renamePlaylist(Playlist *p);
	void renameTab(const PlaylistDAO &dao);

	/** Remove a playlist when clicking on a close button in the corner. */
	void removeTabFromCloseButton(int index);

	void savePlaylist(Playlist *p, bool overwrite);

signals:
	/** Forward the signal. */
	void aboutToChangeMenuLabels(int);

	void aboutToSavePlaylist(Playlist *p, int index, bool overwrite = false);

	void aboutToSendToTagEditor(const QList<QUrl> &tracks);

	void selectionChanged(bool isEmpty);

	void updatePlaybackModeButton(QMediaPlaylist::PlaybackMode mode);
};

#endif // TABPLAYLIST_H
