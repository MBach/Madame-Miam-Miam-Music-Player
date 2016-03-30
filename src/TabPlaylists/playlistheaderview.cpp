#include "playlistheaderview.h"

#include <settingsprivate.h>
#include <QApplication>
#include <QStylePainter>

#include <QtDebug>

QStringList PlaylistHeaderView::labels = QStringList() << "#"
													   << QT_TRANSLATE_NOOP("PlaylistHeaderView", "Title")
													   << QT_TRANSLATE_NOOP("PlaylistHeaderView", "Album")
													   << QT_TRANSLATE_NOOP("PlaylistHeaderView", "Length")
													   << QT_TRANSLATE_NOOP("PlaylistHeaderView", "Artist")
													   << QT_TRANSLATE_NOOP("PlaylistHeaderView", "Rating")
													   << QT_TRANSLATE_NOOP("PlaylistHeaderView", "Year")
													   << QT_TRANSLATE_NOOP("PlaylistHeaderView", "Source")
													   << "TrackDAO";

PlaylistHeaderView::PlaylistHeaderView(Playlist *parent) :
	QHeaderView(Qt::Horizontal, parent)
{
	this->setHighlightSections(false);
	this->setFrameShape(QFrame::NoFrame);
	this->setMinimumSectionSize(this->height());
	this->setMouseTracking(true);
	this->setSectionResizeMode(Interactive);
	this->setSectionsMovable(true);
	this->setStretchLastSection(true);
	this->installEventFilter(this);

	// Context menu on header of columns
	_columns = new QMenu(this);
	connect(_columns, &QMenu::triggered, this, [=](const QAction *action) {
		int columnIndex = action->data().toInt();
		this->setSectionHidden(columnIndex, !this->isSectionHidden(columnIndex));
		parent->resizeColumnToContents(columnIndex);
	});

	// Initialize font from settings
	SettingsPrivate *settings = SettingsPrivate::instance();
	this->setFont(settings->font(SettingsPrivate::FF_Playlist));

	connect(settings, &SettingsPrivate::fontHasChanged, this, [=](SettingsPrivate::FontFamily ff, const QFont &newFont) {
		if (ff == SettingsPrivate::FF_Playlist) {
			this->setFont(newFont);
		}
	});
}

void PlaylistHeaderView::setFont(const QFont &newFont)
{
	QFont font = newFont;
	font.setPointSizeF(font.pointSizeF() * 0.8);
	QHeaderView::setFont(newFont);
	int h = fontMetrics().height() * 1.25;
	if (h >= 30) {
		this->setMinimumHeight(h);
		this->setMaximumHeight(h);
	} else {
		this->setMinimumHeight(30);
		this->setMaximumHeight(30);
	}
}

QSize PlaylistHeaderView::sectionSizeFromContents(int logicalIndex) const
{
	if (logicalIndex == 5) {
		QSize s = QHeaderView::sectionSizeFromContents(logicalIndex);
		auto playlist = qobject_cast<Playlist*>(parentWidget());
		if (playlist->rowHeight(0) != 0) {
			s.setWidth(5 * playlist->rowHeight(0));
		}
		return s;
	}
	return QHeaderView::sectionSizeFromContents(logicalIndex);
}

/** Redefined for dynamic translation. */
void PlaylistHeaderView::changeEvent(QEvent *event)
{
	QHeaderView::changeEvent(event);
	if (model() && event->type() == QEvent::LanguageChange) {
		for (int i = 0; i < count(); i++) {
			model()->setHeaderData(i, Qt::Horizontal, tr(labels.at(i).toStdString().data()), Qt::DisplayRole);
		}
	}
}

void PlaylistHeaderView::setModel(QAbstractItemModel *model)
{
	QHeaderView::setModel(model);
	for (int i = 0; i < count(); i++) {
		QString label = labels.at(i);

		// Exclude hidden columns (should be improved?)
		if (QString::compare(label, "TrackDAO") != 0) {
			model->setHeaderData(i, Qt::Horizontal, tr(label.toStdString().data()), Qt::DisplayRole);

			// Match actions with columns using index of labels
			QAction *actionColumn = new QAction(label, this);
			actionColumn->setData(i);
			actionColumn->setEnabled(actionColumn->text() != tr("Title"));
			actionColumn->setCheckable(true);
			actionColumn->setChecked(!isSectionHidden(i));

			// Then populate the context menu
			_columns->addAction(actionColumn);
		}
	}
}

void PlaylistHeaderView::contextMenuEvent(QContextMenuEvent *event)
{
	// Initialize values for the Header (label and horizontal resize mode)
	for (int i = 0; i < _columns->actions().count(); i++) {
		QAction *action = _columns->actions().at(i);
		if (action) {
			action->setText(tr(labels.at(i).toStdString().data()));
		}
	}

	for (int i = 0; i < _columns->actions().count(); i++) {
		QAction *action = _columns->actions().at(i);
		if (action) {
			action->setChecked(!this->isSectionHidden(i));
		}
	}
	_columns->exec(mapToGlobal(event->pos()));
}

/** Redefined. */
void PlaylistHeaderView::paintEvent(QPaintEvent *)
{
	QStylePainter p(this->viewport());

	QLinearGradient vLinearGradient(viewport()->rect().topLeft(), viewport()->rect().bottomLeft());
	QPalette palette = QApplication::palette();
	vLinearGradient.setColorAt(0, palette.base().color());
	vLinearGradient.setColorAt(1, palette.window().color());

	QStyleOptionHeader opt;
	opt.initFrom(this->viewport());
	p.fillRect(this->viewport()->rect(), QBrush(vLinearGradient));

	p.setPen(opt.palette.text().color());
	QRect r;
	p.save();
	if (QGuiApplication::isLeftToRight()) {
		p.translate(-offset(), 0);
	} else {
		p.translate(offset(), 0);
	}
	for (int i = 0; i < count(); i++) {
		QRect r2(sectionPosition(i), viewport()->rect().y(), sectionSize(i), viewport()->rect().height());
		QString t = fontMetrics().elidedText(model()->headerData(i, Qt::Horizontal).toString(), Qt::ElideRight, r2.width());
		p.drawText(r2, Qt::AlignCenter, t);
		if (r2.contains(mapFromGlobal(QCursor::pos()))) {
			r = r2;
		}
	}
	p.restore();
	if (!r.isNull()) {
		p.save();
		p.setPen(palette.highlight().color());
		p.drawLine(r.x(), r.y() + r.height() / 4,
				   r.x(), r.y() + 3 * r.height() / 4);
		p.drawLine(r.x() + r.width() - 1, r.y() + r.height() / 4,
				   r.x() + r.width() - 1, r.y() + 3 * r.height() / 4);
		p.restore();
	}

	// Bottom frame
	p.setPen(QApplication::palette().mid().color());
	p.drawLine(rect().x(), rect().y() + rect().height() - extra, rect().x() + rect().width(), rect().y() + rect().height() - extra);

	// Vertical frame
	if (QGuiApplication::isLeftToRight()) {
		p.drawLine(rect().x(), rect().y(), rect().x(), rect().y() + rect().height());
	} else {
		p.drawLine(rect().x() + rect().width(), rect().y(), rect().x() + rect().width(), rect().y() + rect().height());
	}
}
