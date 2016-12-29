#ifndef FILESYSTEMTREEVIEW_H
#define FILESYSTEMTREEVIEW_H

#include <treeview.h>

#include <QContextMenuEvent>
#include <QFileSystemModel>
#include <QMenu>

#include "miamtabplaylists_global.hpp"

/**
 * \brief		The FileSystemTreeView class is displaying a standard file-system in a tree.
 * \details     This view allows one to browse folders and files from a standard file-system. Non playable files are disabled,
 *				excepted for covers. One can send files, folders, and both to playlist or tag editor.
 * \author      Matthieu Bachelier
 * \copyright   GNU General Public License v3
 */
class MIAMTABPLAYLISTS_LIBRARY FileSystemTreeView : public TreeView
{
	Q_OBJECT
private:
	QString _toLibrary;
	QString _toPlaylist;
	QString _toTagEditor;
	QModelIndex _theIndex;
	QMenu *_properties;
	QFileSystemModel *_fileSystemModel;

public:
	explicit FileSystemTreeView(QWidget *parent = nullptr);

	/** Reimplemented with a QDirIterator to gather informations about tracks. */
	virtual void findAll(const QModelIndex &index, QList<QUrl> *tracks) const override;

	virtual void updateSelectedTracks() override;

protected:
	/** Reimplemented to display up to 3 actions. */
	virtual void contextMenuEvent(QContextMenuEvent *event) override;

	/** Reimplemented with a QDirIterator to quick count tracks. */
	virtual int countAll(const QModelIndexList &indexes) const override;

	virtual void keyPressEvent(QKeyEvent *event) override;

private:
	void scrollAndHighlight(const QChar &c);

public slots:
	/** Reload tree when the path has changed in the address bar. */
	void reloadWithNewPath(const QDir &path);

private slots:
	/** Get the folder which is the target of one's double-click. */
	void convertIndex(const QModelIndex &index);

signals:
	/** Append the selected folder to the existing music locations. */
	void aboutToAddMusicLocations(const QList<QDir> &);

	void folderChanged(const QString &);
};

#endif // FILESYSTEMTREEVIEW_H
