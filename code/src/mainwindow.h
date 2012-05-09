#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <phonon>

#include "ui_mainwindow.h"
#include "customizeoptionsdialog.h"
#include "librarytreeview.h"
#include "settings.h"
#include "mediabutton.h"

/// Need to use this forward declaration (circular inclusion)
class CustomizeThemeDialog;


using namespace Phonon;

class MainWindow : public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT
public:
	MainWindow(QWidget *parent = 0);

	// Play, pause, stop, etc.
	QList<MediaButton*> mediaButtons;

private:
	CustomizeThemeDialog *customizeThemeDialog;
	CustomizeOptionsDialog *customizeOptionsDialog;

	/** Set up all actions and behaviour. */
	void setupActions();

	AudioOutput *audioOutput;

	// MP3 actions
	QAction *actionPlay;
	QAction *actionPause;
	QAction *actionStop;

protected:
	/** Redefined to be able to retransltate User Interface at runtime. */
	void changeEvent(QEvent *event);

public slots:
	void bindShortcut(const QString&, int keySequence);

private slots:
	/** Add a new playlist tab. */
	void addPlaylist();

	void drawLibrary(bool b=false);

	/** Add a file from the filesystem to the current playlist. */
	void addSelectedItemToPlaylist(const QModelIndex &item);

	/** When the user is clicking on the (+) button to add a new playlist. */
	void checkAddPlaylistButton(int i);

	/// Media actions
	/** This buttons switch the play function with the pause function because they are mutually exclusive. */
	void playAndPause();

	/** If playing, then stops the track. */
	void stop();

	/** Displays a simple message box about MmeMiamMiamMusicPlayer. */
	void aboutM4P();
};

#endif // MAINWINDOW_H
