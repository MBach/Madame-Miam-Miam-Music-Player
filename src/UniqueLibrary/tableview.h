#ifndef TABLEVIEW_H
#define TABLEVIEW_H

#include <QMenu>
#include <QTableView>

#include <library/jumptowidget.h>
#include "miamuniquelibrary_global.hpp"
#include "uniquelibraryitemmodel.h"

/**
 * \brief		The ListView class is used to display thousands of tracks in a single list.
 * \author      Matthieu Bachelier
 * \copyright   GNU General Public License v3
*/
class MIAMUNIQUELIBRARY_LIBRARY TableView : public QTableView
{
	Q_OBJECT
private:
	UniqueLibraryItemModel *_model;

	JumpToWidget *_jumpToWidget;

	int _skipCount;

	QMenu _menu;
	QAction *_actionSendToTagEditor;

public:
	explicit TableView(QWidget *parent = nullptr);

	virtual ~TableView();

	/** Adjust row height of last track when tracks in an album have an height lower than cover size. */
	void adjust();

	inline JumpToWidget* jumpToWidget() const { return _jumpToWidget; }

	/** Redefined to disable search in the table and trigger jumpToWidget's action. */
	virtual void keyboardSearch(const QString &search) override;

	inline UniqueLibraryItemModel *model() const { return _model; }

protected:
	virtual void contextMenuEvent(QContextMenuEvent *e) override;

	/** Redefined to override shortcuts that are mapped on simple keys. */
	virtual bool eventFilter(QObject *obj, QEvent *event) override;

	/** Redefined to keep displayed covers untouched. */
	virtual void mouseMoveEvent(QMouseEvent *event) override;

	/** Redefined to keep displayed covers untouched. */
	virtual void mousePressEvent(QMouseEvent *event) override;

	virtual void paintEvent(QPaintEvent *event) override;

public slots:
	void jumpTo(const QString &letter);

signals:
	void sendToTagEditor(const QModelIndexList &indexes, const QList<QUrl> &tracks);
};

#endif // TABLEVIEW_H
