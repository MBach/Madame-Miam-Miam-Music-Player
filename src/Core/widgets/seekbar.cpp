#include "seekbar.h"
#include "settingsprivate.h"

#include <QApplication>
#include <QPropertyAnimation>
#include <QStyleOptionSlider>
#include <QStylePainter>
#include <QWheelEvent>

#include <QtDebug>

SeekBar::SeekBar(QWidget *parent)
	: MiamSlider(parent)
	, _mediaPlayer(nullptr)
{
	this->setMinimumHeight(30);
	this->setSingleStep(0);
	this->setPageStep(0);
}

void SeekBar::setMediaPlayer(MediaPlayer *mediaPlayer)
{
	_mediaPlayer = mediaPlayer;
	connect(_mediaPlayer, &MediaPlayer::positionChanged, this, &SeekBar::setPosition);
}

void SeekBar::keyPressEvent(QKeyEvent *e)
{
	_mediaPlayer->blockSignals(true);
	if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) {
		_mediaPlayer->blockSignals(true);
		_mediaPlayer->setMute(true);
		if (e->key() == Qt::Key_Left) {
			_mediaPlayer->seekBackward();
		} else {
			_mediaPlayer->seekForward();
		}
	} else {
		QSlider::keyPressEvent(e);
	}
}

void SeekBar::keyReleaseEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) {
		_mediaPlayer->setMute(false);
		_mediaPlayer->blockSignals(false);
	} else {
		QSlider::keyPressEvent(e);
	}
}

void SeekBar::mouseMoveEvent(QMouseEvent *)
{
	int xPos = mapFromGlobal(QCursor::pos()).x();
	static const int bound = 12;
	if (xPos >= bound && xPos <= width() - 2 * bound) {
		qreal p = (qreal) xPos / (width() - 2 * bound);
		float posButton = p * 1000;
		_mediaPlayer->seek(p);
		this->setValue(posButton);
	}
}

void SeekBar::mousePressEvent(QMouseEvent *)
{
	int xPos = mapFromGlobal(QCursor::pos()).x();
	static const int bound = 12;
	if (xPos >= bound && xPos <= width() - 2 * bound) {
		qreal p = (qreal) xPos / (width() - 2 * bound);
		float posButton = p * 1000;
		_mediaPlayer->blockSignals(true);
		_mediaPlayer->setMute(true);
		_mediaPlayer->seek(p);
		this->setValue(posButton);
	}
}

void SeekBar::mouseReleaseEvent(QMouseEvent *)
{
	_mediaPlayer->setMute(false);
	_mediaPlayer->blockSignals(false);
	if (_mediaPlayer->state() == QMediaPlayer::PausedState) {
		_mediaPlayer->togglePlayback();
	}
}

void SeekBar::paintEvent(QPaintEvent *)
{
	int h = height() / 3.0;
	QStylePainter p(this);
	QStyleOptionSlider o;
	initStyleOption(&o);
	o.palette = QApplication::palette();

	o.rect.adjust(10, 0, -10, 0);
	static const int bound = 12;

	// Inner rectangle
	int w = width() - 2 * bound;
	float posButton = (float) value() / 1000 * w + bound;

	p.fillRect(rect(), o.palette.window());

	QRectF rLeft = QRectF(o.rect.x(),
						  o.rect.y() + h,
						  h,
						  h);
	QRectF rRight = QRectF(o.rect.x() + o.rect.width() - h,
						   h,
						   h,
						   h);
	QRectF rMid = QRectF(rLeft.topRight(), rRight.bottomLeft());

	QPen pen(o.palette.mid().color());
	p.setPen(pen);
	p.save();
	p.setRenderHint(QPainter::Antialiasing, true);
	QPainterPath painterPath;
	// 2---1---->   Left curve is painted with 2 calls to cubicTo, starting in 1   <----10--9
	// |   |        First cubic call is with points p1, p2 and p3                       |   |
	// 3   +        Second is with points p3, p4 and p5                                 +   8
	// |   |        With that, a half circle can be filled with linear gradient         |   |
	// 4---5---->                                                                  <----6---7
	painterPath.moveTo(rMid.topLeft());
	painterPath.cubicTo(rMid.x(), rMid.y(),
				   rLeft.x() + rLeft.width() / 2.0f, rLeft.y(),
				   rLeft.x() + rLeft.width() / 2.0f, rLeft.y() + rLeft.height() / 2.0f);
	painterPath.cubicTo(rLeft.x() + rLeft.width() / 2.0f, rLeft.y() + rLeft.height() / 2.0f,
				   rLeft.x() + rLeft.width() / 2.0f, rLeft.y() + rLeft.height(),
				   rLeft.x() + rLeft.width(), rLeft.y() + rLeft.height());

	painterPath.lineTo(rRight.x(), rRight.y() + rRight.height());
	painterPath.cubicTo(rRight.x(), rRight.y() + rRight.height(),
					rRight.x() + rRight.width() / 2.0f, rRight.y() + rRight.height(),
					rRight.x() + rRight.width() / 2.0f, rRight.y() + rRight.height() / 2.0f);
	painterPath.cubicTo(rRight.x() + rRight.width() / 2.0f, rRight.y() + rRight.height() / 2.0f,
					rRight.x() + rRight.width() / 2.0f, rRight.y(),
					rRight.x(), rRight.y());
	painterPath.closeSubpath();

	// Increase the width of the pen because of Antialising
	pen.setWidthF(1.3);
	p.setPen(pen);
	p.drawPath(painterPath);

	p.setRenderHint(QPainter::Antialiasing, false);
	p.restore();

	// Exclude ErrorState from painting
	if (_mediaPlayer->state() == QMediaPlayer::PlayingState || _mediaPlayer->state() == QMediaPlayer::PausedState) {

		// Remove enabled state (bitwise operator)
		if (_mediaPlayer->state() == QMediaPlayer::PausedState) {
			o.state &= ~QStyle::State_Enabled;
		}
		QLinearGradient linearGradient = this->interpolatedLinearGradient(painterPath.boundingRect(), o);

		p.setRenderHint(QPainter::Antialiasing, true);
		p.fillPath(painterPath, linearGradient);
		p.setRenderHint(QPainter::Antialiasing, false);

		p.save();
		p.setRenderHint(QPainter::Antialiasing, true);
		QPointF center(posButton, height() * 0.5);
		QConicalGradient cGrad(center, 360 - 4 * (value() % 360));
		if (_mediaPlayer->state() == QMediaPlayer::PlayingState) {
			cGrad.setColorAt(0.0, o.palette.highlight().color());
			cGrad.setColorAt(1.0, o.palette.highlight().color().lighter());
		} else {
			cGrad.setColorAt(0.0, o.palette.mid().color());
			cGrad.setColorAt(1.0, o.palette.mid().color().lighter());
		}
		p.setBrush(cGrad);
		p.drawEllipse(center, height() * 0.3, height() * 0.3);
		p.restore();
	} else {
		p.setRenderHint(QPainter::Antialiasing, true);
		p.fillPath(painterPath, o.palette.window().color());
		p.setRenderHint(QPainter::Antialiasing, false);
	}
}

void SeekBar::wheelEvent(QWheelEvent *e)
{
	_mediaPlayer->setMute(true);
	if (e->angleDelta().y() > 0) {
		_mediaPlayer->seekForward();
	} else {
		_mediaPlayer->seekBackward();
	}
	_mediaPlayer->setMute(false);
}

void SeekBar::setPosition(qint64 pos, qint64 duration)
{
	if (duration > 0) {
		setValue(1000 * pos / duration);
	}
}
