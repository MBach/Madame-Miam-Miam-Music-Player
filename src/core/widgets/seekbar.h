#ifndef SEEKBAR_H
#define SEEKBAR_H

#include <styling/miamslider.h>
#include <mediaplayer.h>

/**
 * \brief       The SeekBar class is used to display a nice seek bar instead of default slider.
 * \author		Matthieu Bachelier
 * \copyright   GNU General Public License v3
 */
class MIAMCORE_LIBRARY SeekBar : public MiamSlider
{
	Q_OBJECT
private:
	MediaPlayer *_mediaPlayer;

public:
	explicit SeekBar(QWidget *parent = nullptr);

	void setMediaPlayer(MediaPlayer *mediaPlayer);

protected:
	/** Redefined to seek in current playing file. */
	virtual void keyPressEvent(QKeyEvent *e) override;

	virtual void keyReleaseEvent(QKeyEvent *e) override;

	virtual void mouseMoveEvent(QMouseEvent *) override;

	virtual void mousePressEvent(QMouseEvent *) override;

	virtual void mouseReleaseEvent(QMouseEvent *) override;

	virtual void paintEvent(QPaintEvent *) override;

	virtual void wheelEvent(QWheelEvent *e) override;

public slots:
	void setPosition(qint64 pos, qint64 duration);
};


#endif // SEEKBAR_H
