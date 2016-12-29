#ifndef ADDRESSBAR_H
#define ADDRESSBAR_H

#include <QDir>
#include <QHBoxLayout>
#include <QMenu>
#include <QWidget>

#include "addressbarbutton.h"
#include "addressbarlineedit.h"
#include "addressbarmenu.h"

#include <QStack>

#include "miamtabplaylists_global.hpp"

/**
 * \brief		The AddressBar class is the place where subfolders (instance of AddressBarButton) will be appended.
 * \details		The path to a folder is splitted into folders. When there is not enough space to display the entire path,
 *				then first folders next to root are visually removed and stacked into a menu. When one triggers the root item,
 *				previously saved items are displayed in reverse order.
 * \author      Matthieu Bachelier
 * \copyright   GNU General Public License v3
 * \see			AddressBarMenu
 */
class MIAMTABPLAYLISTS_LIBRARY AddressBar : public QWidget
{
	Q_OBJECT
private:
	QHBoxLayout *_hBoxLayout;
	AddressBarMenu *_menu;
	QStack<QDir> _hiddenFolders;
	AddressBarButton *_lastHighlightedButton;
	AddressBarLineEdit *_lineEdit;
	bool _isDown;
	QDir _currentDir;

public:
	explicit AddressBar(QWidget *parent = nullptr);

	static QString getVolumeInfo(const QString &drive);

	/** Called by the popup menu when one is moving the mouse cursor. */
	void findAndHighlightButton(const QPoint &p);

	inline QDir currentDir() const { return _currentDir; }

	inline bool isDown() const { return _isDown; }
	inline void setDown(bool down) { _isDown = down; }

	inline bool hasHiddenFolders() const { return !_hiddenFolders.isEmpty(); }

protected:
	virtual void mousePressEvent(QMouseEvent *) override;

	virtual void paintEvent(QPaintEvent *) override;

	virtual void resizeEvent(QResizeEvent *event) override;

private:
	/** Delete subdirectories located after the arrow button. */
	void clear();

	/** Create a special root arrow button.*/
	void createRoot();

	/** Append a button to the address bar to navigate through the filesystem. */
	int createSubDirButtons(const QDir &path);

public slots:
	/** Init with an absolute path. Also used as a callback to a view. */
	void init(const QDir &initDir);

private slots:
	void feedLineEdit();

	/** Show logical drives (on Windows) or root item (on Unix). Also, when the path is too long, first folders are sent to this submenu. */
	void showDrivesAndPreviousFolders();

	/** Show a popup menu with the content of the selected directory. */
	void showSubDirMenu(AddressBarButton *button);

signals:
	void aboutToChangePath(const QDir &);
};

#endif // ADDRESSBAR_H
