#ifndef LIBRARYORDERDIALOG_H
#define LIBRARYORDERDIALOG_H

#include <QDialog>

namespace Ui {
	class LibraryOrderDialog;
}

class LibraryOrderDialog : public QDialog
{
	Q_OBJECT
private:
	Ui::LibraryOrderDialog *_ui;

public:
	explicit LibraryOrderDialog(QWidget *parent = 0);

	QString headerValue() const;
};

#endif // LIBRARYORDERDIALOG_H
