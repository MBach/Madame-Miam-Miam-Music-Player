#ifndef PLAYBACKMODEBUTTON_H
#define PLAYBACKMODEBUTTON_H

#include <mediabuttons/mediabutton.h>
#include <QMediaPlaylist>
#include <QMenu>

#include "miamcore_global.h"

/**
 * \brief		The PlaybackModeButton class is a custom class to choose a mode like Classic, Random, Play once, etc.
 * \author      Matthieu Bachelier
 * \copyright   GNU General Public License v3
 */
class MIAMCORE_LIBRARY PlaybackModeButton : public MediaButton
{
	Q_OBJECT
private:
	QMenu _menu;
	QMediaPlaylist::PlaybackMode _mode;
	bool _toggleShuffleOnly;

public:
	explicit PlaybackModeButton(QWidget *parent = 0);

	void setToggleShuffleOnly(bool b);

protected:
	virtual void contextMenuEvent(QContextMenuEvent *e) override;

public slots:
	/** Load an icon from a chosen theme in options. */
	virtual void setIconFromTheme(const QString &theme) override;

	void updateMode(QMediaPlaylist::PlaybackMode mode);

signals:
	void aboutToChangeCurrentPlaylistPlaybackMode(QMediaPlaylist::PlaybackMode mode);
};

#endif // PLAYBACKMODEBUTTON_H
