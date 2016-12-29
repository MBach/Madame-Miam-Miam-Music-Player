#include <QtWidgets>

#include "playlist.h"
#include "stardelegate.h"
#include "starrating.h"
#include "stareditor.h"

#include <settings.h>
#include <filehelper.h>

int StarDelegate::maxStarCount = 5;

StarDelegate::StarDelegate(Playlist *playlist, QMediaPlaylist *parent)
	: QStyledItemDelegate(parent), _playlist(playlist), _mediaPlaylist(parent), _editorIsOpened(false)
{
	for (int i = 0; i < 5; ++i) {
		QLineF l(0.5, 0.5, 0.5, 0);
		l.setAngle(i * 72 + 18);
		starPolygon << l.p2();

		QLineF l2(0.5, 0.5, 0.5, 0.71);
		l2.setAngle(i * 72 + 54);
		starPolygon << l2.p2();
	}
	diamondPolygon << QPointF(0.4, 0.5) << QPointF(0.5, 0.4) << QPointF(0.6, 0.5) << QPointF(0.5, 0.6) << QPointF(0.4, 0.5);
}

/** Redefined. */
QWidget* StarDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const
{
	StarEditor *editor = new StarEditor(index.data().value<StarRating>(), parent);
	/*connect(editor, &StarEditor::editingFinished, [=]() {
		qDebug() << "ratings to update" << _mediaPlaylist->media(index.row()).canonicalUrl();
		QMediaContent mediaContent = _mediaPlaylist->media(index.row());
		FileHelper fh(QString(QFile::encodeName(mediaContent.canonicalUrl().toLocalFile())));
		fh.setRating(editor->starRating().starCount());
		delete editor;
	});*/
	return editor;
}

/** Redefined. */
void StarDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	// Removes the dotted rectangle on the selected cell because the entire row is selected
	QStyleOptionViewItem opt = option;
	opt.state &= ~QStyle::State_HasFocus;
	if (opt.state.testFlag(QStyle::State_Selected) && opt.state.testFlag(QStyle::State_Active)) {
		painter->save();
		painter->setPen(opt.palette.highlight().color());
		painter->fillRect(opt.rect, opt.palette.highlight().color().lighter());
		painter->restore();
	} else {
		if (!Settings::getInstance()->colorsAlternateBG() || index.row() % 2 == 0) {
			painter->fillRect(opt.rect, opt.palette.base());
		} else {
			painter->fillRect(opt.rect, opt.palette.alternateBase());
		}
	}

	if (opt.state.testFlag(QStyle::State_Selected) && opt.state.testFlag(QStyle::State_Active)) {
		painter->save();
		painter->setPen(opt.palette.highlight().color());
		// Don't display the upper line is the track above is selected
		QModelIndex top = index.sibling(index.row() - 1, index.column());
		if (!top.isValid() || !_playlist->selectionModel()->selectedIndexes().contains(top)) {
			painter->drawLine(opt.rect.topLeft(), opt.rect.topRight());
		}
		if (opt.rect.left() == 0) {
			painter->drawLine(opt.rect.topLeft(), opt.rect.bottomLeft());
		} else if (opt.rect.right() == _playlist->viewport()->rect().right()) {
			painter->drawLine(opt.rect.topRight(), opt.rect.bottomRight());
		}
		// Don't display the bottom line is the track underneath is selected
		QModelIndex bottom = index.sibling(index.row() + 1, index.column());
		if (!bottom.isValid() || !_playlist->selectionModel()->selectedIndexes().contains(bottom)) {
			painter->drawLine(opt.rect.bottomLeft(), opt.rect.bottomRight());
		}
		painter->restore();
	}

	// Don't paint anything if index cannot be converted in rating and item isn't selected
	// Otherwise, paint color filled stars for existing rating, or "wireframe" stars for selected items
	if (index.data().canConvert<StarRating>() || opt.state.testFlag(QStyle::State_Selected) && opt.state.testFlag(QStyle::State_Active)) {
		this->paintStars(painter, option, index);
	}
}

void StarDelegate::paintStars(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);

	/// XXX: extract this somewhere?
	QPen pen(QColor(171, 122, 77));
	QLinearGradient linearGradientBrush(0, 0, 0, 1);
	QLinearGradient linearGradientPen(0, 0, 0, 1);

	pen.setWidthF(pen.widthF() / opt.rect.height());

	EditMode mode;
	int stars = 0;
	if (index.data().canConvert<StarRating>()) {
		/// XXX
		mode = ReadOnly;
		StarRating sr = index.data().value<StarRating>();
		stars = sr.starCount();
	} else {
		mode = NoStarsYet;
	}
	switch (mode) {
	case Editable:
		painter->fillRect(opt.rect, QApplication::style()->standardPalette().highlight().color().lighter());

		linearGradientBrush.setColorAt(0, Qt::white);
		linearGradientBrush.setColorAt(1, QColor(253, 230, 116));

		linearGradientPen.setColorAt(0, QColor(227, 178, 94));
		linearGradientPen.setColorAt(1, QColor(166, 122, 87));

		pen.setColor(QColor(171, 122, 77));
		pen.setBrush(QBrush(linearGradientPen));
		painter->setBrush(QBrush(linearGradientBrush));
		break;
	case NoStarsYet:
		painter->setBrush(QBrush(QColor::fromRgbF(1, 1, 1, 0.9)));
		break;
	case ReadOnly:
		linearGradientBrush.setColorAt(0, Qt::white);
		linearGradientBrush.setColorAt(1, QColor(253, 230, 116));

		linearGradientPen.setColorAt(0, QColor(227, 178, 94));
		linearGradientPen.setColorAt(1, QColor(166, 122, 87));

		pen.setColor(QColor(171, 122, 77));
		pen.setBrush(QBrush(linearGradientPen));
		painter->setBrush(QBrush(linearGradientBrush));
		break;
	}
	painter->setPen(pen);

	//opt.rect.adjust(0, 3, 0, -3);

	int yOffset = (opt.rect.height() - opt.rect.height() * starPolygon.boundingRect().height()) / 2;
	painter->translate(opt.rect.x(), opt.rect.y() + yOffset);
	if (opt.rect.height() < opt.rect.width() / 5) {
		painter->scale(opt.rect.height(), opt.rect.height());
	} else {
		painter->scale(opt.rect.width() / maxStarCount, opt.rect.width() / maxStarCount);
	}

	for (int i = 0; i < maxStarCount; ++i) {
		if (i < stars || mode == NoStarsYet) {
			painter->drawPolygon(starPolygon);
		} else if (mode == Editable) {
			painter->drawPolygon(diamondPolygon, Qt::WindingFill);
		}
		painter->translate(1.0, 0);
	}
	painter->restore();
}

/** Redefined. */
void StarDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	qDebug() << Q_FUNC_INFO << editor;
	StarRating starRating = qvariant_cast<StarRating>(index.data());
	//StarEditor *starEditor = qobject_cast<StarEditor *>(editor);
	//starEditor->setStarRating(starRating);
}

/** Redefined. */
void StarDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	qDebug() << Q_FUNC_INFO;
	//StarEditor *starEditor = qobject_cast<StarEditor *>(editor);
	//model->setData(index, QVariant::fromValue(starEditor->starRating()));
}

/** Redefined. */
QSize StarDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (index.data().canConvert<StarRating>()) {
		return option.rect.size();
	} else {
		return QStyledItemDelegate::sizeHint(option, index);
	}
}

bool StarDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
	if (event->type() == QEvent::MouseButtonRelease) {
		qDebug() << "editor can be opened";
		_editorIsOpened = true;
	}
	return QStyledItemDelegate::editorEvent(event, model, option, index);
}
