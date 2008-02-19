#include "PrefsDialog.h"
#include "QTeXApp.h"
#include "PDFDocument.h"

#include <QSettings>
#include <QFontDatabase>
#include <QTextCodec>
#include <QSet>
#include <QFileDialog>
#include <QMainWindow>
#include <QToolBar>
#include <QDesktopWidget>
#include <QtAlgorithms>

PrefsDialog::PrefsDialog(QWidget *parent)
	: QDialog(parent)
{
	init();
}

void PrefsDialog::init()
{
	setupUi(this);
	
	connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
	
	connect(binPathList, SIGNAL(itemSelectionChanged()), this, SLOT(updatePathButtons()));
	connect(pathUp, SIGNAL(clicked()), this, SLOT(movePathUp()));
	connect(pathDown, SIGNAL(clicked()), this, SLOT(movePathDown()));
	connect(pathAdd, SIGNAL(clicked()), this, SLOT(addPath()));
	connect(pathRemove, SIGNAL(clicked()), this, SLOT(removePath()));

	connect(toolList, SIGNAL(itemSelectionChanged()), this, SLOT(updateToolButtons()));
	connect(toolUp, SIGNAL(clicked()), this, SLOT(moveToolUp()));
	connect(toolDown, SIGNAL(clicked()), this, SLOT(moveToolDown()));
	connect(toolAdd, SIGNAL(clicked()), this, SLOT(addTool()));
	connect(toolRemove, SIGNAL(clicked()), this, SLOT(removeTool()));
	connect(toolEdit, SIGNAL(clicked()), this, SLOT(editTool()));
	
	connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(changedTabPanel(int)));
	
	pathsChanged = toolsChanged = false;
}

void PrefsDialog::changedTabPanel(int index)
{
	// this all feels a bit hacky, but seems to keep things tidy on Mac OS X at least
	QWidget *page = tabWidget->widget(index);
	page->clearFocus();
	switch (index) {
		case 0: // General
			page->focusWidget()->clearFocus();
			break;
		case 1: // Editor
			editorFont->setFocus();
			editorFont->lineEdit()->selectAll();
			break;
		case 2: // Preview
			scale->setFocus();
			scale->selectAll();
			break;
		case 3: // Typesetting
			binPathList->setFocus();
			break;
	}
}

void PrefsDialog::updatePathButtons()
{
	int selRow = -1;
	if (binPathList->selectedItems().count() > 0)
		selRow = binPathList->currentRow();
	pathRemove->setEnabled(selRow != -1);
	pathUp->setEnabled(selRow > 0);
	pathDown->setEnabled(selRow != -1 && selRow < binPathList->count() - 1);
}

void PrefsDialog::movePathUp()
{
	int selRow = -1;
	if (binPathList->selectedItems().count() > 0)
		selRow = binPathList->currentRow();
	if (selRow > 0) {
		QListWidgetItem *item = binPathList->takeItem(selRow);
		binPathList->insertItem(selRow - 1, item);
		binPathList->setCurrentItem(binPathList->item(selRow - 1));
		pathsChanged = true;
	}
}

void PrefsDialog::movePathDown()
{
	int selRow = -1;
	if (binPathList->selectedItems().count() > 0)
		selRow = binPathList->currentRow();
	if (selRow != -1 &&  selRow < binPathList->count() - 1) {
		QListWidgetItem *item = binPathList->takeItem(selRow);
		binPathList->insertItem(selRow + 1, item);
		binPathList->setCurrentItem(binPathList->item(selRow + 1));
		pathsChanged = true;
	}
}

void PrefsDialog::addPath()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Choose Directory"),
					 "/usr", 0 /*QFileDialog::DontUseNativeDialog*/
								/*QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks*/);
	if (dir != "") {
		binPathList->addItem(dir);
		binPathList->setCurrentItem(binPathList->item(binPathList->count() - 1));
		pathsChanged = true;
	}
}

void PrefsDialog::removePath()
{
	if (binPathList->currentRow() > -1)
		if (binPathList->currentItem()->isSelected()) {
			binPathList->takeItem(binPathList->currentRow());
			pathsChanged = true;
		}
}

void PrefsDialog::updateToolButtons()
{
	int selRow = -1;
	if (toolList->selectedItems().count() > 0)
		selRow = toolList->currentRow();
	toolEdit->setEnabled(selRow != -1);
	toolRemove->setEnabled(selRow != -1);
	toolUp->setEnabled(selRow > 0);
	toolDown->setEnabled(selRow != -1 && selRow < toolList->count() - 1);
}

void PrefsDialog::moveToolUp()
{
	int selRow = -1;
	if (toolList->selectedItems().count() > 0)
		selRow = toolList->currentRow();
	if (selRow > 0) {
		QListWidgetItem *item = toolList->takeItem(selRow);
		toolList->insertItem(selRow - 1, item);
		toolList->setCurrentItem(toolList->item(selRow - 1));
		Engine e = engineList.takeAt(selRow);
		engineList.insert(selRow - 1, e);
		toolsChanged = true;
	}
}

void PrefsDialog::moveToolDown()
{
	int selRow = -1;
	if (toolList->selectedItems().count() > 0)
		selRow = toolList->currentRow();
	if (selRow != -1 &&  selRow < toolList->count() - 1) {
		QListWidgetItem *item = toolList->takeItem(selRow);
		toolList->insertItem(selRow + 1, item);
		toolList->setCurrentItem(toolList->item(selRow + 1));
		Engine e = engineList.takeAt(selRow);
		engineList.insert(selRow + 1, e);
		toolsChanged = true;
	}
}

void PrefsDialog::addTool()
{
	Engine e;
	e.setName(tr("New Tool"));
	e.setShowPdf(true);
	if (ToolConfig::doToolConfig(this, e) == QDialog::Accepted) {
		engineList.append(e);
		toolsChanged = true;
	}
}

void PrefsDialog::removeTool()
{
	if (toolList->currentRow() > -1)
		if (toolList->currentItem()->isSelected()) {
			engineList.removeAt(toolList->currentRow());
			toolList->takeItem(toolList->currentRow());
			toolsChanged = true;
		}
}

void PrefsDialog::editTool()
{
	if (toolList->currentRow() > -1)
		if (toolList->currentItem()->isSelected()) {
			Engine e = engineList[toolList->currentRow()];
			if (ToolConfig::doToolConfig(this, e) == QDialog::Accepted) {
				engineList[toolList->currentRow()] = e;
				toolList->currentItem()->setText(e.name());
				toolsChanged = true;
			}
		}
}

void PrefsDialog::buttonClicked(QAbstractButton *whichButton)
{
	if (buttonBox->buttonRole(whichButton) == QDialogButtonBox::ResetRole)
		restoreDefaults();
}

const int kDefault_LaunchOption = 1;
const int kDefault_ToolBarIcons = 2;
const bool kDefault_ToolBarText = false;
const bool kDefault_SyntaxColoring = true;
const bool kDefault_WrapLines = true;
const int kDefault_TabWidth = 32;
const int kDefault_PreviewScaleOption = 1;
const int kDefault_PreviewScale = 200;
const bool kDefault_HideConsole = true;

void PrefsDialog::restoreDefaults()
{
	switch (tabWidget->currentIndex()) {
		case 0:
			// General
			switch (kDefault_ToolBarIcons) {
				case 1:
					smallIcons->setChecked(true);
					break;
				case 2:
					mediumIcons->setChecked(true);
					break;
				case 3:
					largeIcons->setChecked(true);
					break;
			}
			showText->setChecked(kDefault_ToolBarText);
			switch (kDefault_LaunchOption) {
				case 1:
					blankDocument->setChecked(true);
					break;
				case 2:
					templateDialog->setChecked(true);
					break;
				case 3:
					openDialog->setChecked(true);
					break;
			}
			blankDocument->setChecked(true);
			break;

		case 1:
			// Editor
			{
				QFont font;
				editorFont->setEditText(font.family());
				fontSize->setValue(font.pointSize());
			}
			tabWidth->setValue(kDefault_TabWidth);
			wrapLines->setChecked(kDefault_WrapLines);
			syntaxColoring->setChecked(kDefault_SyntaxColoring);
			encoding->setCurrentIndex(encoding->findText("UTF-8"));
			break;
	
		case 2:
			// Preview
			switch (kDefault_PreviewScaleOption) {
				default:
					actualSize->setChecked(true);
					break;
				case 2:
					fitWidth->setChecked(true);
					break;
				case 3:
					fitWindow->setChecked(true);
					break;
				case 4:
					fixedScale->setChecked(true);
					break;
			}
			scale->setValue(kDefault_PreviewScale);
			resolution->setValue(QApplication::desktop()->logicalDpiX());
			
			switch (kDefault_MagnifierSize) {
				case 1:
					smallMag->setChecked(true);
					break;
				default:
					mediumMag->setChecked(true);
					break;
				case 3:
					largeMag->setChecked(true);
					break;
			}
			circularMag->setChecked(kDefault_CircularMagnifier);
			break;
		
		case 3:
			// Typesetting
			{
				QTeXApp *app = qobject_cast<QTeXApp*>(qApp);
				if (app != NULL) {
					app->setDefaultEngineList();
					app->setDefaultPaths();
					initPathAndToolLists();
				}
			}
			autoHideOutput->setChecked(kDefault_HideConsole);
			break;
	}
}

void PrefsDialog::initPathAndToolLists()
{
	binPathList->clear();
	toolList->clear();
	QTeXApp *app = qobject_cast<QTeXApp*>(qApp);
	if (app != NULL) {
		binPathList->addItems(app->getBinaryPaths());
		engineList = app->getEngineList();
		foreach (Engine e, engineList) {
			toolList->addItem(e.name());
			defaultTool->addItem(e.name());
		}
	}
	if (binPathList->count() > 0)
		binPathList->setCurrentItem(binPathList->item(0));
	if (toolList->count() > 0)
		toolList->setCurrentItem(toolList->item(0));
	updatePathButtons();
	updateToolButtons();
}

QDialog::DialogCode PrefsDialog::doPrefsDialog(QWidget *parent)
{
	PrefsDialog dlg(NULL);
	
	QSet<QTextCodec*> codecs;
	foreach (QByteArray codecName, QTextCodec::availableCodecs())
		codecs.insert(QTextCodec::codecForName(codecName));
	QStringList nameList;
	foreach (QTextCodec *codec, codecs)
		nameList.append(codec->name());
	nameList.sort();
	dlg.encoding->addItems(nameList);
	
	QSettings settings;
	// initialize controls based on the current settings
	
	// General
	int oldIconSize = settings.value("toolBarIconSize", kDefault_ToolBarIcons).toInt();
	switch (oldIconSize) {
		case 1:
			dlg.smallIcons->setChecked(true);
			break;
		case 3:
			dlg.largeIcons->setChecked(true);
			break;
		default:
			dlg.mediumIcons->setChecked(true);
			break;
	}
	bool oldShowText = settings.value("toolBarShowText", kDefault_ToolBarText).toBool();
	dlg.showText->setChecked(oldShowText);

	switch (settings.value("launchOption", kDefault_LaunchOption).toInt()) {
		default:
			dlg.blankDocument->setChecked(true);
			break;
		case 2:
			dlg.templateDialog->setChecked(true);
			break;
		case 3:
			dlg.openDialog->setChecked(true);
			break;
	}
	
	// Editor
	dlg.syntaxColoring->setChecked(settings.value("syntaxColoring", kDefault_SyntaxColoring).toBool());
	dlg.wrapLines->setChecked(settings.value("wrapLines", kDefault_WrapLines).toBool());
	dlg.tabWidth->setValue(settings.value("tabWidth", kDefault_TabWidth).toInt());
	QFontDatabase fdb;
	dlg.editorFont->addItems(fdb.families());
	QString fontString = settings.value("font").toString();
	QFont font;
	if (fontString != "")
		font.fromString(fontString);
	dlg.editorFont->setCurrentIndex(fdb.families().indexOf(font.family()));
	dlg.fontSize->setValue(font.pointSize());
	dlg.encoding->setCurrentIndex(nameList.indexOf("UTF-8"));

	// Preview
	switch (settings.value("scaleOption", kDefault_PreviewScaleOption).toInt()) {
		default:
			dlg.actualSize->setChecked(true);
			break;
		case 2:
			dlg.fitWidth->setChecked(true);
			break;
		case 3:
			dlg.fitWindow->setChecked(true);
			break;
		case 4:
			dlg.fixedScale->setChecked(true);
			break;
	}
	dlg.scale->setValue(settings.value("previewScale", kDefault_PreviewScale).toInt());

	int oldResolution = settings.value("previewResolution", QApplication::desktop()->logicalDpiX()).toInt();
	dlg.resolution->setValue(oldResolution);
	
	int oldMagSize = settings.value("magnifierSize", kDefault_MagnifierSize).toInt();
	switch (oldMagSize) {
		case 1:
			dlg.smallMag->setChecked(true);
			break;
		default:
			dlg.mediumMag->setChecked(true);
			break;
		case 3:
			dlg.largeMag->setChecked(true);
			break;
	}
	bool oldCircular = settings.value("circularMagnifier", kDefault_CircularMagnifier).toBool();
	dlg.circularMag->setChecked(oldCircular);
	
	// Typesetting
	dlg.initPathAndToolLists();
	dlg.autoHideOutput->setChecked(settings.value("autoHideConsole", kDefault_HideConsole).toBool());

	if (parent && parent->inherits("TeXDocument"))
		dlg.tabWidget->setCurrentIndex(1);
	else if (parent && parent->inherits("PDFDocument"))
		dlg.tabWidget->setCurrentIndex(2);
	
	dlg.show();

	DialogCode	result = (DialogCode)dlg.exec();

	if (result == Accepted) {
		// General
		int iconSize = 2;
		if (dlg.smallIcons->isChecked())
			iconSize = 1;
		else if (dlg.largeIcons->isChecked())
			iconSize = 3;
		bool showText = dlg.showText->isChecked();
		if (iconSize != oldIconSize || showText != oldShowText) {
			settings.setValue("toolBarIconSize", iconSize);
			settings.setValue("toolBarShowText", showText);
			foreach (QWidget *widget, qApp->topLevelWidgets()) {
				QMainWindow *theWindow = qobject_cast<QMainWindow*>(widget);
				if (theWindow != NULL)
					QTeXUtils::applyToolbarOptions(theWindow, iconSize, showText);
			}
		}
		
		int launchOption = 1;
		if (dlg.templateDialog->isChecked())
			launchOption = 2;
		else if (dlg.openDialog->isChecked())
			launchOption = 3;
		settings.setValue("launchOption", launchOption);
		
		// Editor
		settings.setValue("syntaxColoring", dlg.syntaxColoring->isChecked());
		settings.setValue("wrapLines", dlg.wrapLines->isChecked());
		settings.setValue("tabWidth", dlg.tabWidth->value());
		font = QFont(dlg.editorFont->currentText());
		font.setPointSize(dlg.fontSize->value());
		settings.setValue("font", font.toString());
		
		// Preview
		int scaleOption = 1;
		if (dlg.fitWidth->isChecked())
			scaleOption = 2;
		else if (dlg.fitWindow->isChecked())
			scaleOption = 3;
		else if (dlg.fixedScale->isChecked())
			scaleOption = 4;
		int scale = dlg.scale->value();
		settings.setValue("scaleOption", scaleOption);
		settings.setValue("previewScale", scale);
		
		int resolution = dlg.resolution->value();
		if (resolution != oldResolution) {
			settings.setValue("previewResolution", resolution);
			foreach (QWidget *widget, qApp->topLevelWidgets()) {
				PDFDocument *thePdfDoc = qobject_cast<PDFDocument*>(widget);
				if (thePdfDoc != NULL)
					thePdfDoc->setResolution(resolution);
			}
		}
		
		int magSize = 2;
		if (dlg.smallMag->isChecked())
			magSize = 1;
		else if (dlg.largeMag->isChecked())
			magSize = 3;
		settings.setValue("magnifierSize", magSize);
		bool circular = dlg.circularMag->isChecked();
		settings.setValue("circularMagnifier", circular);
		if (oldMagSize != magSize || oldCircular != circular) {
			foreach (QWidget *widget, qApp->topLevelWidgets()) {
				PDFDocument *thePdfDoc = qobject_cast<PDFDocument*>(widget);
				if (thePdfDoc != NULL)
					thePdfDoc->resetMagnifier();
			}
		}

		// Typesetting
		QTeXApp *app = qobject_cast<QTeXApp*>(qApp);
		if (app != NULL) {
			if (dlg.pathsChanged) {
				QStringList paths;
				for (int i = 0; i < dlg.binPathList->count(); ++i)
					paths << dlg.binPathList->item(i)->text();
				app->setBinaryPaths(paths);
			}
			if (dlg.toolsChanged) {
				app->setEngineList(dlg.engineList);
			}
		}
	}

	return result;
}

ToolConfig::ToolConfig(QWidget *parent)
	: QDialog(parent)	
{
	init();
}
	
void ToolConfig::init()
{
	setupUi(this);
	
	connect(arguments, SIGNAL(itemSelectionChanged()), this, SLOT(updateArgButtons()));
	connect(argUp, SIGNAL(clicked()), this, SLOT(moveArgUp()));
	connect(argDown, SIGNAL(clicked()), this, SLOT(moveArgDown()));
	connect(argAdd, SIGNAL(clicked()), this, SLOT(addArg()));
	connect(argRemove, SIGNAL(clicked()), this, SLOT(removeArg()));
}

void ToolConfig::updateArgButtons()
{
	int selRow = -1;
	if (arguments->selectedItems().count() > 0)
		selRow = arguments->currentRow();
	argRemove->setEnabled(selRow != -1);
	argUp->setEnabled(selRow > 0);
	argDown->setEnabled(selRow != -1 && selRow < arguments->count() - 1);
}

void ToolConfig::moveArgUp()
{
	int selRow = -1;
	if (arguments->selectedItems().count() > 0)
		selRow = arguments->currentRow();
	if (selRow > 0) {
		QListWidgetItem *item = arguments->takeItem(selRow);
		arguments->insertItem(selRow - 1, item);
		arguments->setCurrentItem(arguments->item(selRow - 1));
	}
}

void ToolConfig::moveArgDown()
{
	int selRow = -1;
	if (arguments->selectedItems().count() > 0)
		selRow = arguments->currentRow();
	if (selRow != -1 &&  selRow < arguments->count() - 1) {
		QListWidgetItem *item = arguments->takeItem(selRow);
		arguments->insertItem(selRow + 1, item);
		arguments->setCurrentItem(arguments->item(selRow + 1));
	}
}

void ToolConfig::addArg()
{
	arguments->addItem(tr("NewArgument"));
	QListWidgetItem* item = arguments->item(arguments->count() - 1);
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	arguments->setCurrentItem(item);
}

void ToolConfig::removeArg()
{
	if (arguments->currentRow() > -1)
		if (arguments->currentItem()->isSelected())
			arguments->takeItem(arguments->currentRow());
}

QDialog::DialogCode ToolConfig::doToolConfig(QWidget *parent, Engine &engine)
{
	ToolConfig dlg(parent);
	
	dlg.toolName->setText(engine.name());
	dlg.program->setText(engine.program());
	dlg.arguments->addItems(engine.arguments());
	for (int i = 0; i < dlg.arguments->count(); ++i) {
		QListWidgetItem *item = dlg.arguments->item(i);
		item->setFlags(item->flags() | Qt::ItemIsEditable);
	}
	dlg.viewPdf->setChecked(engine.showPdf());
	
	dlg.show();

	DialogCode	result = (DialogCode)dlg.exec();
	if (result == Accepted) {
		engine.setName(dlg.toolName->text());
		engine.setProgram(dlg.program->text());
		QStringList args;
		for (int i = 0; i < dlg.arguments->count(); ++i)
			args << dlg.arguments->item(i)->text();
		engine.setArguments(args);
		engine.setShowPdf(dlg.viewPdf->isChecked());
	}

	return result;
}