#include "mainwindow.h"

#include <musicsearchengine.h>
#include <quickstart.h>
#include <settings.h>
#include <settingsprivate.h>
#include <libraryorderdialog.h>
#include <playlist.h>

#include "dialogs/customizethemedialog.h"
#include "dialogs/dragdropdialog.h"
#include "dialogs/equalizerdalog.h"
#include "views/viewloader.h"
#include "pluginmanager.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QStandardPaths>

#include <QtDebug>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, _mediaPlayer(new MediaPlayer(this))
	, _pluginManager(new PluginManager(this))
	, _currentView(nullptr)
{
	setupUi(this);
	actionPlay->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
	actionStop->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

	this->setAcceptDrops(true);
#ifndef Q_OS_MAC
	this->setWindowIcon(QIcon(":/icons/mp_win32"));
#else
	actionHideMenuBar->setVisible(false);
#endif

	// Fonts
	auto settings = SettingsPrivate::instance();
	this->updateFonts(settings->font(SettingsPrivate::FF_Menu));

	menubar->installEventFilter(this);
	menubar->setHidden(settings->value("isMenuHidden", false).toBool());
}

void MainWindow::activateLastView()
{
	// Find the last active view and connect database to it
	/// TODO
	Settings *settings = Settings::instance();
	//QString actionViewName = settings->lastActiveView();
	QString actionViewName = "actionViewPlaylists";
	for (QAction *actionView : menuView->actions()) {
		if (actionView->objectName() == actionViewName) {
			actionView->trigger();
			break;
		}
	}
}

void MainWindow::dispatchDrop(QDropEvent *event)
{
	/** Popup shown to one when tracks are dropped from another application to MiamPlayer. */
	DragDropDialog *dragDropDialog = new DragDropDialog;

	SettingsPrivate *settings = SettingsPrivate::instance();

	// Drag & Drop actions
	connect(dragDropDialog, &DragDropDialog::aboutToAddExtFoldersToLibrary, settings, &SettingsPrivate::addMusicLocations);
	/// FIXME
	//connect(dragDropDialog, &DragDropDialog::aboutToAddExtFoldersToPlaylist, tabPlaylists, &TabPlaylist::addExtFolders);

	bool onlyFiles = dragDropDialog->setMimeData(event->mimeData());
	if (onlyFiles) {
		QStringList tracks;
		for (QString file : dragDropDialog->externalLocations) {
			tracks << "file://" + file;
		}
		tracks.sort(Qt::CaseInsensitive);
		QList<QUrl> urls;
		for (QString t : tracks) {
			urls << QUrl::fromLocalFile(t);
		}
		/// FIXME
		//tabPlaylists->insertItemsToPlaylist(-1, urls);
	} else {
		QList<QDir> dirs;
		for (QString location : dragDropDialog->externalLocations) {
			dirs << location;
		}
		switch (SettingsPrivate::instance()->dragDropAction()) {
		case SettingsPrivate::DD_OpenPopup:
			dragDropDialog->show();
			dragDropDialog->raise();
			dragDropDialog->activateWindow();
			break;
		case SettingsPrivate::DD_AddToLibrary:
			settings->addMusicLocations(dirs);
			break;
		case SettingsPrivate::DD_AddToPlaylist:
			/// FIXME
			//tabPlaylists->addExtFolders(dirs);
			break;
		}
	}
}

void MainWindow::init()
{
	// Load playlists at startup if any, otherwise just add an empty one
	this->setupActions();

	// Init shortcuts
	Settings *settings = Settings::instance();
	QMapIterator<QString, QVariant> it(settings->shortcuts());
	while (it.hasNext()) {
		it.next();
		this->bindShortcut(it.key(), it.value().value<QKeySequence>());
	}

	bool isEmpty = SettingsPrivate::instance()->musicLocations().isEmpty();
	actionScanLibrary->setDisabled(isEmpty);
	if (isEmpty) {
		QuickStart *quickStart = new QuickStart(this);
		quickStart->searchMultimediaFiles();
	}
}

/** Plugins. */
void MainWindow::loadPlugins()
{
	/// FIXME
	QObjectList libraryObjectList;
	//libraryObjectList << library << library->properties;

	QObjectList tagEditorObjectList;
	//tagEditorObjectList << tagEditor->albumCover->contextMenu() << tagEditor->extensiblePushButtonArea << tagEditor->extensibleWidgetArea << tagEditor->tagEditorWidget << tagEditor;

	//_pluginManager->registerExtensionPoint(library->metaObject()->className(), libraryObjectList);
	//_pluginManager->registerExtensionPoint(tagEditor->metaObject()->className(), tagEditorObjectList);
	_pluginManager->init();
}

/** Set up all actions and behaviour. */
void MainWindow::setupActions()
{
	Settings *settings = Settings::instance();

	// Adds a group where view mode are mutually exclusive
	QActionGroup *viewModeGroup = new QActionGroup(this);
	actionViewPlaylists->setActionGroup(viewModeGroup);
	actionViewUniqueLibrary->setActionGroup(viewModeGroup);
	actionViewTagEditor->setActionGroup(viewModeGroup);

	connect(viewModeGroup, &QActionGroup::triggered, this, &MainWindow::activateView);

	QActionGroup *actionPlaybackGroup = new QActionGroup(this);
	for (QAction *actionPlayBack : findChildren<QAction*>(QRegExp("actionPlayback*", Qt::CaseSensitive, QRegExp::Wildcard))) {
		actionPlaybackGroup->addAction(actionPlayBack);
		connect(actionPlayBack, &QAction::triggered, this, [=]() {
			const QMetaObject &mo = QMediaPlaylist::staticMetaObject;
			QMetaEnum metaEnum = mo.enumerator(mo.indexOfEnumerator("PlaybackMode"));
			QString enu = actionPlayBack->property("PlaybackMode").toString();
			/// FIXME
			//_playbackModeWidgetFactory->setPlaybackMode((QMediaPlaylist::PlaybackMode)metaEnum.keyToValue(enu.toStdString().data()));
		});
	}

	// Link user interface
	// Actions from the menu
	connect(actionOpenFiles, &QAction::triggered, this, &MainWindow::openFiles);
	connect(actionOpenFolder, &QAction::triggered, this, &MainWindow::openFolderPopup);
	connect(actionExit, &QAction::triggered, this, [=]() {
		QCloseEvent event;
		this->closeEvent(&event);
		qApp->quit();
	});
	/// FIXME
	//connect(actionAddPlaylist, &QAction::triggered, tabPlaylists, &TabPlaylist::addPlaylist);
	//connect(actionDeleteCurrentPlaylist, &QAction::triggered, tabPlaylists, &TabPlaylist::removeCurrentPlaylist);
	connect(actionShowCustomize, &QAction::triggered, this, [=]() {
		CustomizeThemeDialog *customizeThemeDialog = new CustomizeThemeDialog(this);
		customizeThemeDialog->exec();
	});

	connect(actionShowOptions, &QAction::triggered, this, &MainWindow::createCustomizeOptionsDialog);
	connect(actionAboutQt, &QAction::triggered, &QApplication::aboutQt);
	connect(actionHideMenuBar, &QAction::triggered, this, &MainWindow::toggleMenuBar);
	connect(actionScanLibrary, &QAction::triggered, this, [=]() {
		/// FIXME
		//searchBar->clear();
		SqlDatabase::instance()->rebuild();
	});
	connect(actionShowHelp, &QAction::triggered, this, [=]() {
		QDesktopServices::openUrl(QUrl("http://miam-player.org/wiki/index.php"));
	});

	// Load music
	auto settingsPrivate = SettingsPrivate::instance();
	connect(settingsPrivate, &SettingsPrivate::musicLocationsHaveChanged, [=](const QStringList &oldLocations, const QStringList &newLocations) {
		qDebug() << Q_FUNC_INFO << oldLocations << newLocations;
		bool libraryIsEmpty = newLocations.isEmpty();
		/// FIXME
		//library->setVisible(!libraryIsEmpty);
		//libraryHeader->setVisible(!libraryIsEmpty);
		//changeHierarchyButton->setVisible(!libraryIsEmpty);
		actionScanLibrary->setDisabled(libraryIsEmpty);
		//widgetSearchBar->setVisible(!libraryIsEmpty);

		auto db = SqlDatabase::instance();
		if (libraryIsEmpty) {
			/// FIXME
			//leftTabs->setCurrentIndex(0);
			db->rebuild(oldLocations, QStringList());
			QuickStart *quickStart = new QuickStart(this);
			quickStart->searchMultimediaFiles();
		} else {
			db->rebuild(oldLocations, newLocations);
		}
	});

	// Media buttons and their shortcuts
	connect(menuPlayback, &QMenu::aboutToShow, this, [=]() {
		bool isPlaying = (_mediaPlayer->state() == QMediaPlayer::PlayingState || _mediaPlayer->state() == QMediaPlayer::PausedState);
		actionSeekBackward->setEnabled(isPlaying);
		actionStop->setEnabled(isPlaying);
		actionStopAfterCurrent->setEnabled(isPlaying);
		actionStopAfterCurrent->setChecked(_mediaPlayer->isStopAfterCurrent());
		actionSeekForward->setEnabled(isPlaying);

		bool notEmpty = _mediaPlayer->playlist() && !_mediaPlayer->playlist()->isEmpty();
		actionSkipBackward->setEnabled(notEmpty);
		actionPlay->setEnabled(notEmpty);
		actionSkipForward->setEnabled(notEmpty);
	});
	connect(actionSkipBackward, &QAction::triggered, _mediaPlayer, &MediaPlayer::skipBackward);
	connect(actionSeekBackward, &QAction::triggered, _mediaPlayer, &MediaPlayer::seekBackward);
	connect(actionPlay, &QAction::triggered, _mediaPlayer, &MediaPlayer::togglePlayback);
	connect(actionStop, &QAction::triggered, _mediaPlayer, &MediaPlayer::stop);
	connect(actionStopAfterCurrent, &QAction::triggered, _mediaPlayer, &MediaPlayer::stopAfterCurrent);
	connect(actionSeekForward, &QAction::triggered, _mediaPlayer, &MediaPlayer::seekForward);
	connect(actionSkipForward, &QAction::triggered, _mediaPlayer, &MediaPlayer::skipForward);

	connect(actionShowEqualizer, &QAction::triggered, this, [=]() {
		EqualizerDialog *equalizerDialog = new EqualizerDialog(_mediaPlayer, this);
		equalizerDialog->show();
		equalizerDialog->activateWindow();
	});

	// Playback
	connect(actionRemoveSelectedTracks, &QAction::triggered, this, [=]() {
		/// FIXME
		//if (tabPlaylists->currentPlayList()) {
		//	tabPlaylists->currentPlayList()->removeSelectedTracks();
		//}
	});
	/// FIXME
	//connect(actionMoveTracksUp, &QAction::triggered, tabPlaylists, &TabPlaylist::moveTracksUp);
	//connect(actionMoveTracksDown, &QAction::triggered, tabPlaylists, &TabPlaylist::moveTracksDown);
	connect(actionOpenPlaylistManager, &QAction::triggered, this, &MainWindow::openPlaylistManager);
	connect(actionMute, &QAction::triggered, _mediaPlayer, &MediaPlayer::toggleMute);

	connect(menuPlayback, &QMenu::aboutToShow, this, [=](){
		//QMediaPlaylist::PlaybackMode mode = tabPlaylists->currentPlayList()->mediaPlaylist()->playbackMode();
		const QMetaObject &mo = QMediaPlaylist::staticMetaObject;
		QMetaEnum metaEnum = mo.enumerator(mo.indexOfEnumerator("PlaybackMode"));
		//QAction *action = findChild<QAction*>(QString("actionPlayback").append(metaEnum.valueToKey(mode)));
		//action->setChecked(true);
	});
}

/** Update fonts for menu and context menus. */
void MainWindow::updateFonts(const QFont &font)
{
#ifndef Q_OS_OSX
	menuBar()->setFont(font);
	for (QAction *action : findChildren<QAction*>()) {
		action->setFont(font);
	}
#else
	Q_UNUSED(font)
#endif
}

/** Open a new Dialog where one can add a folder to current playlist. */
void MainWindow::openFolder(const QString &dir)
{
	Settings::instance()->setValue("lastOpenedLocation", dir);
	/// FIXME
	/*QDirIterator it(dir, QDirIterator::Subdirectories);
	QStringList suffixes = FileHelper::suffixes(FileHelper::All, false);
	QList<QUrl> localTracks;
	while (it.hasNext()) {
		it.next();
		if (suffixes.contains(it.fileInfo().suffix())) {
			localTracks << QUrl::fromLocalFile(it.filePath());
		}
	}
	if (Miam::showWarning(tr("playlist"), localTracks.count()) == QMessageBox::Ok) {
		tabPlaylists->insertItemsToPlaylist(-1, localTracks);
	}*/
}

/** Redefined to be able to retransltate User Interface at runtime. */
void MainWindow::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		this->retranslateUi(this);
		/// FIXME
		//tagEditor->retranslateUi(tagEditor);
		//tagEditor->tagConverter->retranslateUi(tagEditor->tagConverter);

		// (need to be tested with Arabic language)
		if (tr("LTR") == "RTL") {
			QApplication::setLayoutDirection(Qt::RightToLeft);
		}
	} else {
		QMainWindow::changeEvent(event);
	}
}

void MainWindow::closeEvent(QCloseEvent *)
{
	auto settingsPrivate = SettingsPrivate::instance();
	if (settingsPrivate->playbackKeepPlaylists()) {
		QList<uint> list = settingsPrivate->lastPlaylistSession();
		list.clear();
		/// FIXME
		/*for (int i = 0; i < tabPlaylists->count(); i++) {
			Playlist *p = tabPlaylists->playlist(i);
			bool isOverwritting = p->id() != 0;
			uint id = tabPlaylists->playlistManager()->savePlaylist(p, isOverwritting, true);
			if (id != 0) {
				list.append(id);
			}
		}*/
		settingsPrivate->setLastPlaylistSession(list);
		/// FIXME
		/*int idx = tabPlaylists->currentIndex();
		Playlist *p = tabPlaylists->playlist(idx);
		settingsPrivate->setValue("lastActiveTab", idx);
		qDebug() << p->mediaPlaylist()->playbackMode();
		int m = p->mediaPlaylist()->playbackMode();
		settingsPrivate->setValue("lastActivePlaylistMode", m);*/
	}
	//auto settings = Settings::instance();
	//qDebug() << Q_FUNC_INFO << settings->lastActiveView();
	//settings->autoSaveGeometryForLastView();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
	if (event->mimeData()->hasFormat("playlist/x-tableview-item") || event->mimeData()->hasFormat("treeview/x-treeview-item")) {
		// Display a forbid cursor when one has started a drag from a playlist
		// Accepted drops are other playlists or the tabbar
		event->ignore();
	} else {
		event->acceptProposedAction();
	}
}

void MainWindow::dropEvent(QDropEvent *event)
{
	// Ignore Drag & Drop if the source is a part of this player
	if (event->source() != nullptr) {
		return;
	}
	this->dispatchDrop(event);
}

bool MainWindow::event(QEvent *e)
{
	bool b = QMainWindow::event(e);
	if (e->type() == QEvent::KeyPress) {
		if (!this->menuBar()->isVisible()) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(e);
			if (keyEvent->key() == Qt::Key_Alt) {
				qDebug() << Q_FUNC_INFO << "Alt was pressed";
				this->setProperty("altKey", true);
			} else {
				this->setProperty("altKey", false);
			}
		}
	} else if (e->type() == QEvent::KeyRelease) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(e);
		if (this->property("altKey").toBool() && keyEvent->key() == Qt::Key_Alt) {
			qDebug() << Q_FUNC_INFO << "Alt was released";
			this->menuBar()->show();
			//this->menuBar()->setProperty("dirtyHackMnemonic", true);
			this->menuBar()->setFocus();
			this->setProperty("altKey", false);
			actionHideMenuBar->setChecked(false);
			SettingsPrivate::instance()->setValue("isMenuHidden", false);
		}
	}
	return b;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == menubar) {
		//qDebug() << Q_FUNC_INFO << event->type();
	}
	return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
	//qDebug() << Q_FUNC_INFO << e->oldSize() << e->size();
	QMainWindow::resizeEvent(e);
}

void MainWindow::createCustomizeOptionsDialog()
{
	CustomizeOptionsDialog *dialog = new CustomizeOptionsDialog(_pluginManager, this);
	connect(dialog, &CustomizeOptionsDialog::aboutToBindShortcut, this, &MainWindow::bindShortcut);
	/// FIXME
	//connect(dialog, &CustomizeOptionsDialog::defaultLocationFileExplorerHasChanged, addressBar, &AddressBar::init);
	dialog->show();
	dialog->raise();
	dialog->activateWindow();
}

void MainWindow::processArgs(const QStringList &args)
{
	QCommandLineParser parser;
	parser.setApplicationDescription(QCoreApplication::tr("Command line helper for Miam-Player"));
	parser.addHelpOption();

	QCommandLineOption directoryOption(QStringList() << "d" << "directory", tr("Directory to open."), tr("dir"));
	QCommandLineOption createNewPlaylist(QStringList() << "n" << "new-playlist", tr("Medias are added into a new playlist."));
	QCommandLineOption sendToTagEditor(QStringList() << "t" << "tag-editor", tr("Medias are sent to tag editor."));
	QCommandLineOption addToLibrary(QStringList() << "l" << "library", tr("Directory is sent to library."));
	QCommandLineOption playPause(QStringList() << "p" << "play", tr("Play or pause track in active playlist."));
	QCommandLineOption stop(QStringList() << "s" << "stop", tr("Stop playback."));
	QCommandLineOption skipForward(QStringList() << "f" << "forward", tr("Play next track."));
	QCommandLineOption skipBackward(QStringList() << "b" << "backward", tr("Play previous track."));
	QCommandLineOption volume(QStringList() << "v" << "volume", tr("Set volume of the player."), tr("volume"));

	parser.addOption(directoryOption);
	parser.addPositionalArgument("files", "Files to open", "[files]");
	parser.addOption(createNewPlaylist);
	parser.addOption(sendToTagEditor);
	parser.addOption(addToLibrary);
	parser.addOption(playPause);
	parser.addOption(stop);
	parser.addOption(skipForward);
	parser.addOption(skipBackward);
	parser.addOption(volume);
	parser.process(args);

	QStringList positionalArgs = parser.positionalArguments();
	bool isDirectoryOption = parser.isSet(directoryOption);
	bool isCreateNewPlaylist = parser.isSet(createNewPlaylist);
	bool isSendToTagEditor = parser.isSet(sendToTagEditor);
	bool isAddToLibrary = parser.isSet(addToLibrary);

	// -d <dir> and -f <files...> options are exclusive
	// It could be possible to use them at the same time but it can be confusing. Directory takes precedence
	if (isDirectoryOption) {
		QFileInfo fileInfo(parser.value(directoryOption));
		if (!fileInfo.isDir()) {
			parser.showHelp();
		}
		if (isSendToTagEditor) {
			/// FIXME
			//tagEditor->addDirectory(fileInfo.absoluteDir());
			actionViewTagEditor->trigger();
		} else if (isAddToLibrary) {
			SettingsPrivate::instance()->addMusicLocations(QList<QDir>() << QDir(fileInfo.absoluteFilePath()));
		} else {
			if (isCreateNewPlaylist) {
				/// FIXME
				//tabPlaylists->addPlaylist();
			}
			this->openFolder(fileInfo.absoluteFilePath());
		}
	} else if (!positionalArgs.isEmpty()) {
		if (isSendToTagEditor) {
			/// FIXME
			//tagEditor->addItemsToEditor(positionalArgs);
			actionViewTagEditor->trigger();
		} else {
			if (isCreateNewPlaylist) {
				/// FIXME
				//tabPlaylists->addPlaylist();
			}
			QList<QUrl> tracks;
			for (QString p : positionalArgs) {
				tracks << QUrl::fromLocalFile(p);
			}
			/// FIXME
			//tabPlaylists->insertItemsToPlaylist(-1, tracks);
		}
	} else if (parser.isSet(playPause)) {
		if (_mediaPlayer->state() == QMediaPlayer::PlayingState) {
			_mediaPlayer->pause();
		} else {
			_mediaPlayer->play();
		}
	} else if (parser.isSet(skipForward)) {
		_mediaPlayer->skipForward();
	} else if (parser.isSet(skipBackward)) {
		_mediaPlayer->skipBackward();
	} else if (parser.isSet(stop)) {
		_mediaPlayer->stop();
	} else if (parser.isSet(volume)) {
		bool ok = false;
		int vol = parser.value(volume).toInt(&ok);
		if (ok) {
			/// FIXME
			//volumeSlider->setValue(vol);
		}
	}
}

void MainWindow::activateView(QAction *menuAction)
{
	if (this->centralWidget()) {
		QWidget *w = this->takeCentralWidget();
		w->deleteLater();
	}
	ViewLoader v(_mediaPlayer);
	_currentView = v.load(menuPlaylist, menuAction->objectName());
	if (!_currentView) {
		return;
	}
	this->setCentralWidget(_currentView);
	SettingsPrivate *settingsPrivate = SettingsPrivate::instance();
	this->restoreGeometry(settingsPrivate->lastActiveView(menuAction->objectName()));

	connect(actionIncreaseVolume, &QAction::triggered, _currentView, &AbstractView::volumeSliderIncrease);
	connect(actionIncreaseVolume, &QAction::triggered, _currentView, &AbstractView::volumeSliderDecrease);

	connect(qApp, &QApplication::aboutToQuit, this, [=] {
		if (_currentView) {
			QActionGroup *actionGroup = this->findChild<QActionGroup*>();
			settingsPrivate->setLastActiveView(actionGroup->checkedAction()->objectName(), this->saveGeometry());
			settingsPrivate->sync();
		}
	});

	/*connect(actionViewUniqueLibrary, &QAction::triggered, this, [=]() {
		stackedWidgetRight->setVisible(false);
		stackedWidget->setCurrentIndex(1);
		_uniqueLibrary->uniqueTable->createConnectionsToDB();
		_uniqueLibrary->uniqueTable->setFocus();
		_mediaPlayer->setPlaylist(nullptr);

		QModelIndex iTop = _uniqueLibrary->uniqueTable->indexAt(_uniqueLibrary->uniqueTable->viewport()->rect().topLeft());
		_uniqueLibrary->uniqueTable->jumpToWidget()->setCurrentLetter(_uniqueLibrary->uniqueTable->model()->currentLetter(iTop));
	});
	connect(actionViewTagEditor, &QAction::triggered, this, [=]() {
		stackedWidget->setCurrentIndex(0);
		stackedWidgetRight->setVisible(true);
		stackedWidgetRight->setCurrentIndex(1);
		actionViewTagEditor->setChecked(true);
		library->createConnectionsToDB();
	});*/
}

void MainWindow::bindShortcut(const QString &objectName, const QKeySequence &keySequence)
{
	QAction *action = findChild<QAction*>("action" + objectName.left(1).toUpper() + objectName.mid(1));
	// Connect actions first
	if (action) {
		action->setShortcut(keySequence);
		// Some default shortcuts might interfer with other widgets, so we need to restrict where it applies
		if (action == actionIncreaseVolume || action == actionDecreaseVolume) {
			action->setShortcutContext(Qt::WidgetShortcut);
		} else if (action == actionRemoveSelectedTracks) {
			action->setShortcutContext(Qt::ApplicationShortcut);
		}
	// Specific actions not defined in main menu
	} /// FIXME
	/*else if (objectName == "showTabLibrary" || objectName == "showTabFilesystem") {
		leftTabs->setShortcut(objectName, keySequence);
	} else if (objectName == "sendToCurrentPlaylist") {
		library->sendToCurrentPlaylist->setKey(keySequence);
	} else if (objectName == "sendToTagEditor") {
		library->openTagEditor->setKey(keySequence);
	} else if (objectName == "search") {
		searchBar->shortcut->setKey(keySequence);
	}*/
}

void MainWindow::openFiles()
{
	QString audioFiles = tr("Audio files");
	Settings *settings = Settings::instance();
	QString lastOpenedLocation;
	QString defaultMusicLocation = QStandardPaths::standardLocations(QStandardPaths::MusicLocation).first();
	if (settings->value("lastOpenedLocation").toString().isEmpty()) {
		lastOpenedLocation = defaultMusicLocation;
	} else {
		lastOpenedLocation = settings->value("lastOpenedLocation").toString();
	}

	audioFiles.append(" (" + FileHelper::suffixes(FileHelper::Standard, true).join(" ") + ")");
	audioFiles.append(";;Game Music Emu (" + FileHelper::suffixes(FileHelper::GameMusicEmu, true).join(" ") + ");;");
	audioFiles.append(tr("Every file type (*)"));

	QStringList files = QFileDialog::getOpenFileNames(this, tr("Choose some files to open"), lastOpenedLocation,
													  audioFiles);
	if (files.isEmpty()) {
		settings->setValue("lastOpenedLocation", defaultMusicLocation);
	} else {
		QFileInfo fileInfo(files.first());
		settings->setValue("lastOpenedLocation", fileInfo.absolutePath());
		QList<QUrl> tracks;
		for (QString file : files) {
			tracks << QUrl::fromLocalFile(file);
		}
		/// FIXME
		//tabPlaylists->insertItemsToPlaylist(-1, tracks);
	}
}

void MainWindow::openFolderPopup()
{
	Settings *settings = Settings::instance();
	QString lastOpenedLocation;
	QString defaultMusicLocation = QStandardPaths::standardLocations(QStandardPaths::MusicLocation).first();
	if (settings->value("lastOpenedLocation").toString().isEmpty()) {
		lastOpenedLocation = defaultMusicLocation;
	} else {
		lastOpenedLocation = settings->value("lastOpenedLocation").toString();
	}
	QString dir = QFileDialog::getExistingDirectory(this, tr("Choose a folder to open"), lastOpenedLocation);
	if (dir.isEmpty()) {
		settings->setValue("lastOpenedLocation", defaultMusicLocation);
	} else {
		this->openFolder(dir);
	}
}

void MainWindow::openPlaylistManager()
{
	PlaylistDialog *playlistDialog = new PlaylistDialog(this);
	/// FIXME
	/*playlistDialog->setPlaylists(tabPlaylists->playlists());
	connect(playlistDialog, &PlaylistDialog::aboutToLoadPlaylist, tabPlaylists, &TabPlaylist::loadPlaylist);
	connect(playlistDialog, &PlaylistDialog::aboutToDeletePlaylist, tabPlaylists, &TabPlaylist::deletePlaylist);
	connect(playlistDialog, &PlaylistDialog::aboutToRenamePlaylist, tabPlaylists, &TabPlaylist::renamePlaylist);
	connect(playlistDialog, &PlaylistDialog::aboutToRenameTab, tabPlaylists, &TabPlaylist::renameTab);
	connect(playlistDialog, &PlaylistDialog::aboutToSavePlaylist, tabPlaylists, &TabPlaylist::savePlaylist);*/
	playlistDialog->exec();
}

void MainWindow::showTabPlaylists()
{
	if (!actionViewPlaylists->isChecked()) {
		actionViewPlaylists->setChecked(true);
	}
	/// FIXME
	//stackedWidgetRight->setCurrentIndex(0);
}

void MainWindow::showTagEditor()
{
	if (!actionViewTagEditor->isChecked()) {
		actionViewTagEditor->setChecked(true);
	}
	/// FIXME
	//stackedWidgetRight->setCurrentIndex(1);
}

void MainWindow::toggleMenuBar(bool checked)
{
	menuBar()->setVisible(!checked);
	SettingsPrivate::instance()->setValue("isMenuHidden", checked);
}