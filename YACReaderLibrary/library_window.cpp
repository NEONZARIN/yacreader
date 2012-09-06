#include "library_window.h"
#include "custom_widgets.h"
#include "treeitem.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QDir>
#include <QDirModel>
#include <QHeaderView>
#include <QProcess>
#include <QtCore>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QFileIconProvider>
#include <QMatrix>

#include <iterator>

#include "data_base_management.h"
#include "check_new_version.h"

//

LibraryWindow::LibraryWindow()
	:QMainWindow(),skip(0),fullscreen(false),fetching(false)
{
	setupUI();
	loadLibraries();
}

void LibraryWindow::setupUI()
{
	libraryCreator = new LibraryCreator();
	packageManager = new PackageManager();

	doModels();
	doDialogs();
	doLayout();
	createActions();
	createToolBars();
	createMenus();
	createConnections();

	setWindowTitle(tr("YACReader Library"));
}

void LibraryWindow::doLayout()
{
	sVertical = new QSplitter(Qt::Vertical);  //spliter derecha
	QSplitter * sHorizontal = new QSplitter(Qt::Horizontal);  //spliter principal
	//TODO: flowType is a global variable
	//CONFIG COMIC_FLOW--------------------------------------------------------
	comicFlow = new ComicFlow(0,flowType);
	comicFlow->setFocusPolicy(Qt::StrongFocus);
	comicFlow->setShowMarks(true);
	QMatrix m;
	m.rotate(-90);
	m.scale(-1,1);
	comicFlow->setMarkImage(QImage(":/images/setRead.png").transformed(m,Qt::SmoothTransformation));
	int heightDesktopResolution = QApplication::desktop()->screenGeometry().height();
	int height,width;
	height = heightDesktopResolution*0.39;
	width = height*0.65;
	slideSizeW = QSize(width,height);
	height = heightDesktopResolution*0.55;
	width = height*0.70;
	slideSizeF = QSize(width,height);
	comicFlow->setSlideSize(slideSizeW);
	setFocusProxy(comicFlow);
	//-------------------------------------------------------------------------

	//CONFIG TREE/TABLE VIEWS--------------------------------------------------
	comicView = new QTableView;
	foldersView = new QTreeView;
	//-------------------------------------------------------------------------

	//CONFIG FOLDERS/COMICS-------------------------------------------------------
	/*sVertical->setStretchFactor(0,1);
	sVertical->setStretchFactor(1,0);
	*/
		//views
	//foldersView->setAnimated(true);
	foldersView->setContextMenuPolicy(Qt::ActionsContextMenu);
	foldersView->setContextMenuPolicy(Qt::ActionsContextMenu);
	foldersView->header()->hide();
	foldersView->setUniformRowHeights(true);
	foldersView->setSelectionBehavior(QAbstractItemView::SelectRows);

	comicView->setAlternatingRowColors(true);
	//comicView->setStyleSheet("alternate-background-color: #e7e7d7;background-color: white;");
	//comicView->setItemDelegate(new YACReaderComicViewDelegate());
	comicView->setContextMenuPolicy(Qt::ActionsContextMenu);
	comicView->setShowGrid(false);
	//comicView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	comicView->horizontalHeader()->setStretchLastSection(true);
	comicView->horizontalHeader()->setClickable(false);
	//comicView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	comicView->verticalHeader()->setDefaultSectionSize(24);
	comicView->verticalHeader()->setClickable(false); //TODO comportamiento an�malo
	comicView->setCornerButtonEnabled(false);
	qApp->setStyleSheet ( qApp->styleSheet() +"  QTableView::item{border-right-style:solid;border-width:1;border-color: #d7d7c7;} QTableView::item:selected{background-color: #d7d7c7; color:#000000; border-right-style:solid;border-width:1;border-color: #8c8590;}");
	//qApp->setStyleSheet ( qApp->styleSheet() +" QTableView {selection-background-color: #d7d7c7; selection-color: #000000;}" );

	/*comicView->setStyleSheet("QTableView {selection-background-color: #d7d7c7; selection-color: #000000; selection-border: 1px solid;}\n  QTableView::item{"
"border-right-style:solid;"
"border-width:1;"
"border-color: #DEDEDE;"
"}");*/
//	comicView->verticalHeader()->setStyleSheet("QHeaderView::section"
//"{"
//    "background-color: white /* steelblue      */"
//"}");
	comicView->setSelectionBehavior(QAbstractItemView::SelectRows);
	comicView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	//-------------------------------------------------------------------------

	//CONFIG NAVEGACI�N/B�SQUEDA-----------------------------------------------
	left = new QWidget;
	QVBoxLayout * l = new QVBoxLayout;
	selectedLibrary = new QComboBox;
	l->setContentsMargins(2,2,0,0);
	l->addWidget(new QLabel(tr("Select a library:")));
	l->addWidget(selectedLibrary);
	treeActions = new QToolBar(left);
	treeActions->setIconSize(QSize(16,16));
	l->addWidget(treeActions);
	l->addWidget(foldersView);

	QVBoxLayout * searchLayout = new QVBoxLayout;

	QHBoxLayout * filter = new QHBoxLayout;
	filter->addWidget(foldersFilter = new QLineEdit());
	previousFilter = "";
	filter->addWidget(clearFoldersFilter = new QPushButton(tr("Clear")));

	searchLayout->addWidget(new QLabel(tr("Search folders/comics"),this));

	searchLayout->addLayout(filter);
	includeComicsCheckBox = new QCheckBox(tr("Include files (slower)"),this);
	includeComicsCheckBox->setChecked(true);
	searchLayout->addWidget(includeComicsCheckBox);

	l->addLayout(searchLayout);
	l->setSpacing(1);
	left->setLayout(l);
	//-------------------------------------------------------------------------

	//FINAL LAYOUT-------------------------------------------------------------
	sVertical->addWidget(comicFlow);
	comics = new QWidget;
	QVBoxLayout * comicsLayout = new QVBoxLayout;
	comicsLayout->setContentsMargins(2,2,0,0);
	comicsLayout->addWidget(editInfoToolBar = new QToolBar(comics));
	comicsLayout->addWidget(comicView);
	comics->setLayout(comicsLayout);
	sVertical->addWidget(comics);
	sHorizontal->addWidget(left);
	sHorizontal->addWidget(sVertical);
	sHorizontal->setStretchFactor(0,0);
	sHorizontal->setStretchFactor(1,1);
	setCentralWidget(sHorizontal);
	//FINAL LAYOUT-------------------------------------------------------------

	fullScreenToolTip = new QLabel(this);
	fullScreenToolTip->setText(tr("<font color='white'> press 'F' to close fullscreen mode </font>"));
	fullScreenToolTip->setPalette(QPalette(QColor(0,0,0)));
	fullScreenToolTip->setFont(QFont("courier new",15,234));
	fullScreenToolTip->setAutoFillBackground(true);
	fullScreenToolTip->hide();
	fullScreenToolTip->adjustSize();

	comicFlow->setFocus(Qt::OtherFocusReason);
}

void LibraryWindow::doDialogs()
{
	createLibraryDialog = new CreateLibraryDialog(this);
	updateLibraryDialog = new UpdateLibraryDialog(this);
	renameLibraryDialog = new RenameLibraryDialog(this);
	propertiesDialog = new PropertiesDialog(this);
	exportLibraryDialog = new ExportLibraryDialog(this);
	importLibraryDialog = new ImportLibraryDialog(this);
	exportComicsInfoDialog = new ExportComicsInfoDialog(this);
	importComicsInfoDialog = new ImportComicsInfoDialog(this);
	addLibraryDialog = new AddLibraryDialog(this);
	optionsDialog = new OptionsDialog(this);
	optionsDialog->restoreOptions();
	had = new HelpAboutDialog(this); //TODO load data.
	QString sufix = QLocale::system().name();
	if(QFile(":/files/about_"+sufix+".html").exists())
		had->loadAboutInformation(":/files/about_"+sufix+".html");
	else
		had->loadAboutInformation(":/files/about.html");

	if(QFile(":/files/helpYACReaderLibrary_"+sufix+".html").exists())
		had->loadHelp(":/files/helpYACReaderLibrary_"+sufix+".html");
	else
		had->loadHelp(":/files/helpYACReaderLibrary.html");
}

void LibraryWindow::doModels()
{
	//dirmodels
	dm = new TreeModel();
	dmCV =  new TableModel();
	sm = new QItemSelectionModel(dm);


	/*proxyFilter = new  YACReaderTreeSearch();
	proxyFilter->setSourceModel(dm);
	proxyFilter->setFilterRole(Qt::DisplayRole);*/

	/*proxySort = new YACReaderSortComics();
	proxySort->setSourceModel(dmCV);
	proxySort->setFilterRole(Qt::DisplayRole);*/
	setFoldersFilter("");
}

void LibraryWindow::createActions()
{
	createLibraryAction = new QAction(this);
	createLibraryAction->setToolTip(tr("Create a new library"));
	createLibraryAction->setShortcut(Qt::Key_C);
	createLibraryAction->setIcon(QIcon(":/images/new.png"));

	openLibraryAction = new QAction(this);
	openLibraryAction->setToolTip(tr("Open an existing library"));
	openLibraryAction->setShortcut(Qt::Key_O);
	openLibraryAction->setIcon(QIcon(":/images/openLibrary.png"));

	exportComicsInfo = new QAction(tr("Export comics info"),this);
	exportComicsInfo->setToolTip(tr("Export comics info"));
	exportComicsInfo->setIcon(QIcon(":/images/exportComicsInfo.png"));

	importComicsInfo = new QAction(tr("Import comics info"),this);
	importComicsInfo->setToolTip(tr("Import comics info"));
	importComicsInfo->setIcon(QIcon(":/images/importComicsInfo.png"));

	exportLibraryAction = new QAction(tr("Pack covers"),this);
	exportLibraryAction->setToolTip(tr("Pack the covers of the selected library"));
	exportLibraryAction->setIcon(QIcon(":/images/exportLibrary.png"));

	importLibraryAction = new QAction(tr("Unpack covers"),this);
	importLibraryAction->setToolTip(tr("Unpack a catalog"));
	importLibraryAction->setIcon(QIcon(":/images/importLibrary.png"));

	updateLibraryAction = new QAction(this);
	updateLibraryAction->setToolTip(tr("Update current library"));
	updateLibraryAction->setShortcut(Qt::Key_U);
	updateLibraryAction->setIcon(QIcon(":/images/updateLibrary.png"));

	renameLibraryAction = new QAction(this);
	renameLibraryAction->setToolTip(tr("Rename current library"));
	renameLibraryAction->setShortcut(Qt::Key_R);
	renameLibraryAction->setIcon(QIcon(":/images/edit.png"));

	deleteLibraryAction = new QAction(this);
	deleteLibraryAction->setToolTip(tr("Delete current library from disk"));
	deleteLibraryAction->setIcon(QIcon(":/images/deleteLibrary.png"));

	removeLibraryAction = new QAction(this);
	removeLibraryAction->setToolTip(tr("Remove current library from your collection"));
	removeLibraryAction->setIcon(QIcon(":/images/removeLibrary.png"));

	openComicAction = new QAction(this);
	openComicAction->setToolTip(tr("Open current comic on YACReader"));
	openComicAction->setShortcut(Qt::Key_Return);
	openComicAction->setIcon(QIcon(":/images/icon.png"));

	setAsReadAction = new QAction(tr("Set as read"),this);
	setAsReadAction->setToolTip(tr("Set comic as read"));
	setAsReadAction->setIcon(QIcon(":/images/setRead.png"));

	setAsNonReadAction = new QAction(tr("Set as unread"),this);
	setAsNonReadAction->setToolTip(tr("Set comic as unread"));
	setAsNonReadAction->setIcon(QIcon(":/images/setUnread.png"));

	setAllAsReadAction = new QAction(tr("Set all as read"),this);
	setAllAsReadAction->setToolTip(tr("Set all comics as read"));
	setAllAsReadAction->setIcon(QIcon(":/images/setAllRead.png"));

	setAllAsNonReadAction = new QAction(tr("Set all as unread"),this);
	setAllAsNonReadAction->setToolTip(tr("Set all comics as unread"));
	setAllAsNonReadAction->setIcon(QIcon(":/images/setAllUnread.png"));

	showHideMarksAction = new QAction(tr("Show/Hide marks"),this);
	showHideMarksAction->setToolTip(tr("Show or hide readed marks"));
	showHideMarksAction->setShortcut(Qt::Key_M);
	showHideMarksAction->setCheckable(true);
	showHideMarksAction->setIcon(QIcon(":/images/showMarks.png"));
	showHideMarksAction->setChecked(true);


	showPropertiesAction = new QAction(this);
	showPropertiesAction->setToolTip(tr("Show properties of current comic"));
	showPropertiesAction->setShortcut(Qt::Key_P);
	showPropertiesAction->setIcon(QIcon(":/images/properties.png"));

	toggleFullScreenAction = new QAction(this);
	toggleFullScreenAction->setToolTip(tr("Fullscreen mode on/off (F)"));
	toggleFullScreenAction->setShortcut(Qt::Key_F);
	toggleFullScreenAction->setIcon(QIcon(":/images/fit.png"));

	helpAboutAction = new QAction(this);
	helpAboutAction->setToolTip(tr("Help, About YACReader"));
	helpAboutAction->setShortcut(Qt::Key_F1);
	helpAboutAction->setIcon(QIcon(":/images/help.png"));

	setRootIndexAction = new QAction(this);
	setRootIndexAction->setToolTip(tr("Select root node"));
	setRootIndexAction->setIcon(QIcon(":/images/setRoot.png"));

	expandAllNodesAction = new QAction(this);
	expandAllNodesAction->setToolTip(tr("Expand all nodes"));
	expandAllNodesAction->setIcon(QIcon(":/images/expand.png"));

	colapseAllNodesAction = new QAction(this);
	colapseAllNodesAction->setToolTip(tr("Colapse all nodes"));
	colapseAllNodesAction->setIcon(QIcon(":/images/colapse.png"));

	optionsAction = new QAction(this);
	optionsAction->setToolTip(tr("Show options dialog"));
	optionsAction->setIcon(QIcon(":/images/options.png"));

	//disable actions
	updateLibraryAction->setEnabled(false);
	renameLibraryAction->setEnabled(false);
	deleteLibraryAction->setEnabled(false);
	removeLibraryAction->setEnabled(false);
	openComicAction->setEnabled(false);
	showPropertiesAction->setEnabled(false);
	setAsReadAction->setEnabled(false);
	setAsNonReadAction->setEnabled(false);
	setAllAsReadAction->setEnabled(false);
	setAllAsNonReadAction->setEnabled(false);

	openContainingFolderAction = new QAction(this);
	openContainingFolderAction->setText(tr("Open folder..."));
	openContainingFolderAction->setIcon(QIcon(":/images/open.png"));

	openContainingFolderComicAction = new QAction(this);
	openContainingFolderComicAction->setText(tr("Open containing folder..."));
	openContainingFolderComicAction->setIcon(QIcon(":/images/open.png"));

	//Edit comics actions------------------------------------------------------
	selectAllComicsAction = new QAction(this);
	selectAllComicsAction->setText(tr("Select all comics"));
	selectAllComicsAction->setIcon(QIcon(":/images/selectAll.png"));

	editSelectedComicsAction = new QAction(this);
	editSelectedComicsAction->setText(tr("Edit"));
	editSelectedComicsAction->setIcon(QIcon(":/images/editComic.png"));

	asignOrderActions = new QAction(this);
	asignOrderActions->setText(tr("Asign current order to comics"));
	asignOrderActions->setIcon(QIcon(":/images/asignNumber.png"));

	forceConverExtractedAction = new QAction(this);
	forceConverExtractedAction->setText(tr("Update cover"));
	forceConverExtractedAction->setIcon(QIcon(":/images/importCover.png"));

	hideComicViewAction = new QAction(this);
	hideComicViewAction->setText(tr("Hide comic flow"));
	hideComicViewAction->setIcon(QIcon(":/images/hideComicFlow.png"));
	hideComicViewAction->setCheckable(true);
	hideComicViewAction->setChecked(false);
	//-------------------------------------------------------------------------
}

//TODO unificar con disableActions
void LibraryWindow::disableAllActions()
{
	updateLibraryAction->setEnabled(false);
	renameLibraryAction->setEnabled(false);
	deleteLibraryAction->setEnabled(false);
	removeLibraryAction->setEnabled(false);
	foldersFilter->setEnabled(false);
	clearFoldersFilter->setEnabled(false);
	setAsReadAction->setEnabled(false);
	setAsNonReadAction->setEnabled(false);
	setAllAsReadAction->setEnabled(false);
	setAllAsNonReadAction->setEnabled(false);
	selectAllComicsAction->setEnabled(false);
	editSelectedComicsAction->setEnabled(false);
	asignOrderActions->setEnabled(false);
}

//librer�a de s�lo lectura
void LibraryWindow::disableActions()
{
	updateLibraryAction->setEnabled(false);
	openComicAction->setEnabled(false);
	showPropertiesAction->setEnabled(false);
	openContainingFolderAction->setEnabled(false);
	openContainingFolderComicAction->setEnabled(false);
	setAsReadAction->setEnabled(false);
	setAsNonReadAction->setEnabled(false);
	setAllAsReadAction->setEnabled(false);
	setAllAsNonReadAction->setEnabled(false);
	selectAllComicsAction->setEnabled(false);
	editSelectedComicsAction->setEnabled(false);
	asignOrderActions->setEnabled(false);
}
//librer�a abierta
void LibraryWindow::enableActions()
{
	updateLibraryAction->setEnabled(true);
	openComicAction->setEnabled(true);
	showPropertiesAction->setEnabled(true);
	openContainingFolderAction->setEnabled(true);
	openContainingFolderComicAction->setEnabled(true);
	setAsReadAction->setEnabled(true);
	setAsNonReadAction->setEnabled(true);
	setAllAsReadAction->setEnabled(true);
	setAllAsNonReadAction->setEnabled(true);
}
void LibraryWindow::enableLibraryActions()
{
	renameLibraryAction->setEnabled(true);
	deleteLibraryAction->setEnabled(true);
	removeLibraryAction->setEnabled(true);
	foldersFilter->setEnabled(true);
	clearFoldersFilter->setEnabled(true);
}

void LibraryWindow::createToolBars()
{
	libraryToolBar = addToolBar(tr("Library"));
	libraryToolBar->setIconSize(QSize(32,32)); //TODO make icon size dynamic
	libraryToolBar->addAction(createLibraryAction);
	libraryToolBar->addAction(openLibraryAction);

	libraryToolBar->addSeparator();

	libraryToolBar->addAction(exportComicsInfo);
	libraryToolBar->addAction(importComicsInfo);

	libraryToolBar->addSeparator();

	libraryToolBar->addAction(exportLibraryAction);
	libraryToolBar->addAction(importLibraryAction);
	

	libraryToolBar->addSeparator();
	libraryToolBar->addAction(updateLibraryAction); 
	libraryToolBar->addAction(renameLibraryAction);
	libraryToolBar->addAction(removeLibraryAction);
	libraryToolBar->addAction(deleteLibraryAction);

	libraryToolBar->addSeparator();
	libraryToolBar->addAction(openComicAction);
	libraryToolBar->addAction(showPropertiesAction);

	QToolButton * tb = new QToolButton();
	tb->addAction(setAsReadAction);
	tb->addAction(setAllAsReadAction);
	tb->setPopupMode(QToolButton::MenuButtonPopup);
	tb->setDefaultAction(setAsReadAction);

	QToolButton * tb2 = new QToolButton();
	tb2->addAction(setAsNonReadAction);
	tb2->addAction(setAllAsNonReadAction);
	tb2->setPopupMode(QToolButton::MenuButtonPopup);
	tb2->setDefaultAction(setAsNonReadAction);

	libraryToolBar->addWidget(tb);
	libraryToolBar->addWidget(tb2);

	libraryToolBar->addAction(showHideMarksAction);

	libraryToolBar->addSeparator();
	libraryToolBar->addAction(toggleFullScreenAction);

	libraryToolBar->addWidget(new QToolBarStretch());
	libraryToolBar->addAction(optionsAction);
	libraryToolBar->addAction(helpAboutAction);


	libraryToolBar->setMovable(false);

	treeActions->addAction(setRootIndexAction);
	treeActions->addAction(expandAllNodesAction);
	treeActions->addAction(colapseAllNodesAction);

	comicFlow->addAction(toggleFullScreenAction);
	comicFlow->addAction(openComicAction);

	editInfoToolBar->addAction(openComicAction);
	editInfoToolBar->addSeparator();
	editInfoToolBar->addAction(editSelectedComicsAction);
	editInfoToolBar->addAction(selectAllComicsAction);
	editInfoToolBar->addSeparator();
	editInfoToolBar->addAction(asignOrderActions);
	editInfoToolBar->addWidget(new QToolBarStretch());
	editInfoToolBar->addAction(hideComicViewAction);
}

void LibraryWindow::createMenus()
{
	comicView->addAction(openContainingFolderComicAction);
	foldersView->addAction(openContainingFolderAction);
}

void LibraryWindow::createConnections()
{
	//libraryCreator connections
	connect(createLibraryDialog,SIGNAL(createLibrary(QString,QString,QString)),this,SLOT(create(QString,QString,QString)));
	connect(importComicsInfoDialog,SIGNAL(finished(int)),this,SLOT(reloadCurrentLibrary()));
	connect(libraryCreator,SIGNAL(coverExtracted(QString)),createLibraryDialog,SLOT(showCurrentFile(QString)));
	connect(libraryCreator,SIGNAL(finished()),createLibraryDialog,SLOT(close()));
	connect(libraryCreator,SIGNAL(coverExtracted(QString)),updateLibraryDialog,SLOT(showCurrentFile(QString)));
	connect(libraryCreator,SIGNAL(finished()),updateLibraryDialog,SLOT(close()));
	connect(libraryCreator,SIGNAL(updated()),this,SLOT(reloadCurrentLibrary()));
	connect(libraryCreator,SIGNAL(created()),this,SLOT(openLastCreated()));

	//packageManager connections
	connect(exportLibraryDialog,SIGNAL(exportPath(QString)),this,SLOT(exportLibrary(QString)));
	connect(exportLibraryDialog,SIGNAL(rejected()),packageManager,SLOT(cancel()));
	connect(packageManager,SIGNAL(exported()),exportLibraryDialog,SLOT(close()));
	connect(importLibraryDialog,SIGNAL(unpackCLC(QString,QString,QString)),this,SLOT(importLibrary(QString,QString,QString)));
	connect(importLibraryDialog,SIGNAL(rejected()),packageManager,SLOT(cancel()));
	connect(importLibraryDialog,SIGNAL(rejected()),this,SLOT(deleteCurrentLibrary()));
	connect(packageManager,SIGNAL(imported()),importLibraryDialog,SLOT(hide()));
	connect(packageManager,SIGNAL(imported()),this,SLOT(openLastCreated()));


	//create and update dialogs
	connect(createLibraryDialog,SIGNAL(cancelCreate()),this,SLOT(cancelCreating()));
	connect(updateLibraryDialog,SIGNAL(cancelUpdate()),this,SLOT(stopLibraryCreator()));

	//open existing library from dialog.
	connect(addLibraryDialog,SIGNAL(addLibrary(QString,QString)),this,SLOT(openLibrary(QString,QString)));

	//load library when selected library changes
	connect(selectedLibrary,SIGNAL(currentIndexChanged(QString)),this,SLOT(loadLibrary(QString)));

	//rename library dialog
	connect(renameLibraryDialog,SIGNAL(renameLibrary(QString)),this,SLOT(rename(QString)));

	//navigations between view modes (tree,list and flow)
	connect(foldersView, SIGNAL(pressed(QModelIndex)), this, SLOT(loadCovers(QModelIndex)));
	connect(comicView, SIGNAL(pressed(QModelIndex)), this, SLOT(centerComicFlow(QModelIndex)));
	connect(comicFlow, SIGNAL(centerIndexChanged(int)), this, SLOT(updateComicView(int)));

	//actions
	connect(createLibraryAction,SIGNAL(triggered()),this,SLOT(createLibrary()));
	connect(exportLibraryAction,SIGNAL(triggered()),exportLibraryDialog,SLOT(show()));
	connect(importLibraryAction,SIGNAL(triggered()),importLibraryDialog,SLOT(show()));

	connect(openLibraryAction,SIGNAL(triggered()),this,SLOT(showAddLibrary()));
	connect(showPropertiesAction,SIGNAL(triggered()),this,SLOT(showProperties()));
	connect(setAsReadAction,SIGNAL(triggered()),this,SLOT(setCurrentComicReaded()));
	connect(setAsNonReadAction,SIGNAL(triggered()),this,SLOT(setCurrentComicUnreaded()));
	connect(setAllAsReadAction,SIGNAL(triggered()),this,SLOT(setComicsReaded()));
	connect(setAllAsNonReadAction,SIGNAL(triggered()),this,SLOT(setComicsUnreaded()));

	connect(showHideMarksAction,SIGNAL(toggled(bool)),comicFlow,SLOT(setShowMarks(bool)));
	
	//comicsInfoManagement
	connect(exportComicsInfo,SIGNAL(triggered()),this,SLOT(showExportComicsInfo()));
	connect(importComicsInfo,SIGNAL(triggered()),this,SLOT(showImportComicsInfo()));

	//properties
	connect(propertiesDialog,SIGNAL(accepted()),this,SLOT(reloadCovers()));

	connect(updateLibraryAction,SIGNAL(triggered()),this,SLOT(updateLibrary()));
	connect(renameLibraryAction,SIGNAL(triggered()),this,SLOT(renameLibrary()));
	connect(deleteLibraryAction,SIGNAL(triggered()),this,SLOT(deleteLibrary()));
	connect(removeLibraryAction,SIGNAL(triggered()),this,SLOT(removeLibrary()));
	connect(openComicAction,SIGNAL(triggered()),this,SLOT(openComic()));
	connect(helpAboutAction,SIGNAL(triggered()),had,SLOT(show()));
	connect(setRootIndexAction,SIGNAL(triggered()),this,SLOT(setRootIndex()));
	connect(expandAllNodesAction,SIGNAL(triggered()),foldersView,SLOT(expandAll()));
	connect(colapseAllNodesAction,SIGNAL(triggered()),foldersView,SLOT(collapseAll()));
	connect(toggleFullScreenAction,SIGNAL(triggered()),this,SLOT(toggleFullScreen()));
	connect(optionsAction, SIGNAL(triggered()),optionsDialog,SLOT(show()));
	connect(optionsDialog, SIGNAL(optionsChanged()),this,SLOT(reloadOptions()));
	//ComicFlow
	connect(comicFlow,SIGNAL(selected(unsigned int)),this,SLOT(openComic()));
	connect(comicView,SIGNAL(doubleClicked(QModelIndex)),this,SLOT(openComic()));
	//Folders filter
	connect(clearFoldersFilter,SIGNAL(clicked()),foldersFilter,SLOT(clear()));
	connect(foldersFilter,SIGNAL(textChanged(QString)),this,SLOT(setFoldersFilter(QString)));
	connect(includeComicsCheckBox,SIGNAL(stateChanged(int)),this,SLOT(searchInFiles(int)));

	//ContextMenus
	connect(openContainingFolderComicAction,SIGNAL(triggered()),this,SLOT(openContainingFolderComic()));
	connect(openContainingFolderAction,SIGNAL(triggered()),this,SLOT(openContainingFolder()));

	//connect(dm,SIGNAL(directoryLoaded(QString)),foldersView,SLOT(expandAll()));
	//connect(dm,SIGNAL(directoryLoaded(QString)),this,SLOT(updateFoldersView(QString)));
	//Comicts edition
	connect(selectAllComicsAction,SIGNAL(triggered()),comicView,SLOT(selectAll()));
	connect(editSelectedComicsAction,SIGNAL(triggered()),this,SLOT(showProperties()));
	connect(asignOrderActions,SIGNAL(triggered()),this,SLOT(asignNumbers()));

	connect(hideComicViewAction, SIGNAL(toggled(bool)),this, SLOT(hideComicFlow(bool)));

}

void LibraryWindow::loadLibrary(const QString & name)
{
	if(libraries.size()>0)  //si hay bibliotecas...
	{	
		QString path=libraries.value(name)+"/.yacreaderlibrary";
		QDir d; //TODO change this by static methods (utils class?? with delTree for example)
		QString dbVersion;
		if(d.exists(path) && (dbVersion = DataBaseManagement::checkValidDB(path+"/library.ydb")) != "") //si existe en disco la biblioteca seleccionada, y es v�lida..
		{
			int comparation;
			if((comparation = DataBaseManagement::compareVersions(dbVersion,VERSION)) == 0) //en caso de que la versi�n se igual que la actual
			{
				index = 0;
				sm->clear();

				dm->setupModelData(path);
				foldersView->setModel(dm);

				d.setCurrent(libraries.value(name));
				d.setFilter(QDir::AllDirs | QDir::Files | QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot);
				if(d.count()<=1) //librer�a de s�lo lectura
				{
					//QMessageBox::critical(NULL,QString::number(d.count()),QString::number(d.count()));
					disableActions();
					importedCovers = true;
				}
				else //librer�a normal abierta
				{
					enableActions();
					importedCovers = false;
				}
				enableLibraryActions();

				loadCovers(QModelIndex());

				//includeComicsCheckBox->setCheckState(Qt::Unchecked);
				foldersFilter->clear();
			}
			else
			{
				if(comparation < 0)
				{
					int ret = QMessageBox::question(this,tr("Update needed"),tr("This library was created with a previous version of YACReaderLibrary. It needs to be updated. Update now?"));
					if(ret == QMessageBox::Yes)
					{
					}
					else
					{
						comicView->setModel(NULL);
						foldersView->setModel(NULL);
						comicFlow->clear();
						disableAllActions();//TODO comprobar que se deben deshabilitar
					}
				}
				else
				{
					int ret = QMessageBox::question(this,tr("Download new version"),tr("This library was created with a newer version of YACReaderLibrary. Download the new version now?"),QMessageBox::Yes,QMessageBox::No);
					if(ret == QMessageBox::Yes)
						QDesktopServices::openUrl(QUrl("http://www.yacreader.com"));

					comicView->setModel(NULL);
					foldersView->setModel(NULL);
					comicFlow->clear();
					disableAllActions();//TODO comprobar que se deben deshabilitar

				}
			}
		}
		else
		{
			comicView->setModel(NULL);
			foldersView->setModel(NULL);
			comicFlow->clear();
			disableAllActions();//TODO comprobar que se deben deshabilitar

			//si la librer�a no existe en disco, se ofrece al usuario la posibiliad de eliminarla
			if(!d.exists(path))
			{
				QString currentLibrary = selectedLibrary->currentText();
				if(QMessageBox::question(this,tr("Library not available"),tr("Library ")+currentLibrary+tr(" is no longer available. Do you want to remove it?"),QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes)
				{
					deleteCurrentLibrary();
				}
			}
			else//si existe el path, puede ser que la librer�a sea alguna versi�n pre-5.0 � que est� corrupta
			{
				QString currentLibrary = selectedLibrary->currentText();
				QString path = libraries.value(selectedLibrary->currentText());
				if(QMessageBox::question(this,tr("Old library or corrupted"),tr("Library ")+currentLibrary+tr(" is corrupted or has been created with an older version of YACReaderLibrary. It must be created again. Do you want to create the library now?"),QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes)
				{
					QDir d(path+"/.yacreaderlibrary");
					delTree(d);
					d.rmdir(path+"/.yacreaderlibrary");
					createLibraryDialog->setDataAndStart(currentLibrary,path);
					//create(path,path+"/.yacreaderlibrary",currentLibrary);
				}
			}
		}
	}
	else //en caso de que no exista ninguna biblioteca se desactivan los botones pertinentes
	{
		disableAllActions();
	}
}

void LibraryWindow::loadCovers(const QModelIndex & mi)
{
	_rootIndexCV = mi;
	unsigned long long int folderId = 1;
	if(mi.isValid())
	{
		TreeItem *item = static_cast<TreeItem*>(mi.internalPointer());
		folderId = item->id;
	}

	//cambiado de orden, ya que al llamar a foldersFilter->clear() se invalidan los model index
	if(foldersFilter->text()!="")
	{
		//setFoldersFilter("");
		if(mi.isValid())
		{
			index = static_cast<TreeItem *>(mi.internalPointer())->originalItem;
			column = mi.column();
			foldersFilter->clear();
		}
	}
	else
	{
		index = static_cast<TreeItem *>(mi.internalPointer());
		column = mi.column();
	}

	//comicView->setModel(NULL);
	dmCV->setupModelData(folderId,dm->getDatabase());
	comicView->setModel(dmCV);
	comicView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	for(int i = 4;i<comicView->horizontalHeader()->count();i++)
		comicView->horizontalHeader()->hideSection(i);


	//debido a un bug, qt4 no es capaz de ajustar el ancho teniendo en cuenta todas la filas (no s�lo las visibles)
	//as� que se ecala la primera vez y despu�s se deja el control al usuario.
	comicView->resizeColumnsToContents();
	comicView->horizontalHeader()->setStretchLastSection(true);

	QStringList paths = dmCV->getPaths(currentPath());
	comicFlow->setImagePaths(paths);
	comicFlow->setMarks(dmCV->getReadList());
	comicFlow->setFocus(Qt::OtherFocusReason);

	if(paths.size()>0 && !importedCovers)
	{
		openComicAction->setEnabled(true);
		showPropertiesAction->setEnabled(true);
		setAsReadAction->setEnabled(true);
		setAsNonReadAction->setEnabled(true);
		setAllAsReadAction->setEnabled(true);
		setAllAsNonReadAction->setEnabled(true);
		selectAllComicsAction->setEnabled(true);
		editSelectedComicsAction->setEnabled(true);
		asignOrderActions->setEnabled(true);
	}
	else
	{
		openComicAction->setEnabled(false);
		showPropertiesAction->setEnabled(false);
		setAsReadAction->setEnabled(false);
		setAsNonReadAction->setEnabled(false);
		setAllAsReadAction->setEnabled(false);
		setAllAsNonReadAction->setEnabled(false);
		selectAllComicsAction->setEnabled(false);
		editSelectedComicsAction->setEnabled(false);
		asignOrderActions->setEnabled(false);
	}
	if(paths.size()>0)
		comicView->setCurrentIndex(dmCV->index(0,0));
}

void LibraryWindow::reloadCovers()
{
	loadCovers(_rootIndexCV);

	QModelIndex mi = dmCV->getIndexFromId(_comicIdEdited);
	comicView->scrollTo(mi,QAbstractItemView::PositionAtCenter);
	comicView->setCurrentIndex(mi);
	//centerComicFlow(mi);
	comicFlow->setCenterIndex(mi.row());
}

void LibraryWindow::centerComicFlow(const QModelIndex & mi)
{
	int distance = comicFlow->centerIndex()-mi.row();
	if(abs(distance)>10)
	{
		if(distance<0)
			comicFlow->setCenterIndex(comicFlow->centerIndex()+(-distance)-10);
		else
			comicFlow->setCenterIndex(comicFlow->centerIndex()-distance+10);
		skip = 10;
	}
	else
		skip = abs(comicFlow->centerIndex()-mi.row());
	comicFlow->showSlide(mi.row());
	comicFlow->setFocus(Qt::OtherFocusReason);
}

void LibraryWindow::updateComicView(int i)
{

	if(skip==0)
	{
		QModelIndex mi = dmCV->index(i,2);
		comicView->setCurrentIndex(mi);
		comicView->scrollTo(mi,QAbstractItemView::EnsureVisible);
	}
	skip?(--skip):0;
}

void LibraryWindow::openComic()
{
	if(!importedCovers)
	{
		QString path = currentPath() + dmCV->getComicPath(comicView->currentIndex());
		
#ifdef Q_WS_MAC
		
		QProcess::startDetached("open", QStringList() << "-n" << QDir::cleanPath(QCoreApplication::applicationDirPath()+"/../../../YACReader.app") << "--args" << path);//,QStringList() << path);
		//Comic is readed
#else
        
		QProcess::startDetached(QDir::cleanPath(QCoreApplication::applicationDirPath())+"/YACReader",QStringList() << path);
#endif        
		//Comic is readed
		setCurrentComicReaded();
	}
}

void LibraryWindow::setCurrentComicReaded()
{
	comicFlow->markSlide(comicFlow->centerIndex());
	comicFlow->updateMarks();

	ComicDB c = dmCV->getComic(comicView->currentIndex());
	c.info.read = true;
	QSqlDatabase db = DataBaseManagement::loadDatabase(dm->getDatabase());
	db.open();
	c.info.updateRead(db);
	db.close();
	QSqlDatabase::removeDatabase(dm->getDatabase());
}

void LibraryWindow::setComicsReaded()
{
	comicFlow->setMarks(dmCV->setAllComicsRead(true));
	comicFlow->updateMarks();
}

void LibraryWindow::setCurrentComicUnreaded()
{
	comicFlow->unmarkSlide(comicFlow->centerIndex());
	comicFlow->updateMarks();

	ComicDB c = dmCV->getComic(comicView->currentIndex());
	c.info.read = false;
	QSqlDatabase db = DataBaseManagement::loadDatabase(dm->getDatabase());
	db.open();
	c.info.updateRead(db);
	db.close();
	QSqlDatabase::removeDatabase(dm->getDatabase());

}

void LibraryWindow::setComicsUnreaded()
{
	comicFlow->setMarks(dmCV->setAllComicsRead(false));
	comicFlow->updateMarks();
}

void LibraryWindow::createLibrary()
{
	createLibraryDialog->show();
}

void LibraryWindow::create(QString source, QString dest, QString name)
{
	libraryCreator->createLibrary(source,dest);
	libraryCreator->start();
	_lastAdded = name;
	_sourceLastAdded = source;

}

void LibraryWindow::reloadCurrentLibrary()
{
	loadLibrary(selectedLibrary->currentText());
}

void LibraryWindow::openLastCreated()
{
	
	selectedLibrary->disconnect();

	selectedLibrary->setCurrentIndex(selectedLibrary->findText(_lastAdded));
	libraries.insert(_lastAdded,_sourceLastAdded);
	selectedLibrary->addItem(_lastAdded,_sourceLastAdded);
	selectedLibrary->setCurrentIndex(selectedLibrary->findText(_lastAdded));
	saveLibraries();

	connect(selectedLibrary,SIGNAL(currentIndexChanged(QString)),this,SLOT(loadLibrary(QString)));
	
	loadLibrary(_lastAdded);
}

void LibraryWindow::showAddLibrary()
{
	addLibraryDialog->show();
}

void LibraryWindow::openLibrary(QString path, QString name)
{	
	path.remove("/.yacreaderlibrary");
	QDir d; //TODO change this by static methods (utils class?? with delTree for example) 
	if(d.exists(path + "/.yacreaderlibrary"))
	{
		_lastAdded = name;
		_sourceLastAdded = path;
		openLastCreated();
	}
	else
		QMessageBox::warning(this,tr("Library not found"),tr("The selected folder doesn't contain any library."));
}

void LibraryWindow::loadLibraries()
{
	QFile f(QCoreApplication::applicationDirPath()+"/libraries.yacr");
	f.open(QIODevice::ReadOnly);
	QTextStream txtS(&f);
	QString content = txtS.readAll();
	QStringList lines = content.split('\n');
	QString line,name;
	int i=0;
	bool librariesAvailable = false;
	foreach(line,lines)
	{
		if((i%2)==0)
		{
			name = line;
		}
		else
		{
			librariesAvailable = true;
			libraries.insert(name.trimmed(),line.trimmed());
			selectedLibrary->addItem(name.trimmed(),line.trimmed());
		}
		i++;
	}

	if(!librariesAvailable)
	{
		disableAllActions();
	}
}

void LibraryWindow::saveLibraries()
{
	QFile f(QCoreApplication::applicationDirPath()+"/libraries.yacr");
	if(!f.open(QIODevice::WriteOnly))
	{
		QMessageBox::critical(NULL,tr("Saving libraries file...."),tr("There was a problem saving YACReaderLibrary libraries file. Please, check if you have enough permissions in the YACReader root folder."));
	}
	else
	{
		QTextStream txtS(&f);
		for(QMap<QString,QString>::iterator i = libraries.begin();i!=libraries.end();i++)
		{
			txtS << i.key() << "\n";
			txtS << i.value() << "\n";
		}
	}
}

void LibraryWindow::updateLibrary()
{
	updateLibraryDialog->show();
	QString currentLibrary = selectedLibrary->currentText();
	QString path = libraries.value(currentLibrary);
	_lastAdded = currentLibrary;
	libraryCreator->updateLibrary(path,path+"/.yacreaderlibrary");
	libraryCreator->start();
}

void LibraryWindow::deleteLibrary()
{
	QString currentLibrary = selectedLibrary->currentText();
	if(QMessageBox::question(this,tr("Are you sure?"),tr("Do you want delete ")+currentLibrary+" library?",QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes)
	{
		deleteCurrentLibrary();
	}
}

void LibraryWindow::deleteCurrentLibrary()
{
	//QSqlDatabase db = dm->getDatabase();
	//db.commit();
	//db.close();
	//QSqlDatabase::removeDatabase(db.connectionName());
	//if(!dm->getDatabase().isOpen())
	//{
	QString path = libraries.value(selectedLibrary->currentText());
	libraries.remove(selectedLibrary->currentText());
	selectedLibrary->removeItem(selectedLibrary->currentIndex());
	selectedLibrary->setCurrentIndex(0);
	path = path+"/.yacreaderlibrary";

	QDir d(path);
	delTree(d);
	d.rmdir(path);
	if(libraries.size()==0)//no more libraries avaliable.
	{
		comicView->setModel(NULL);
		foldersView->setModel(NULL);
		comicFlow->clear();
	}
	saveLibraries();
	//}
}

void LibraryWindow::removeLibrary()
{
	QString currentLibrary = selectedLibrary->currentText();
	if(QMessageBox::question(this,tr("Are you sure?"),tr("Do you want remove ")+currentLibrary+tr(" library?\nFiles won't be erased from disk."),QMessageBox::Yes,QMessageBox::No)==QMessageBox::Yes)
	{
		libraries.remove(currentLibrary);
		selectedLibrary->removeItem(selectedLibrary->currentIndex());
		selectedLibrary->setCurrentIndex(0);
		if(libraries.size()==0)//no more libraries avaliable.
		{
			comicView->setModel(NULL);
			foldersView->setModel(NULL);
			comicFlow->clear();
		}
		saveLibraries();
	}
}

void LibraryWindow::renameLibrary()
{
	renameLibraryDialog->show();
}

void LibraryWindow::rename(QString newName)
{
	QString currentLibrary = selectedLibrary->currentText();
	QString path = libraries.value(currentLibrary);
	libraries.remove(currentLibrary);
	selectedLibrary->removeItem(selectedLibrary->currentIndex());
	libraries.insert(newName,path);
	selectedLibrary->addItem(newName,path);
	saveLibraries();
	selectedLibrary->setCurrentIndex(selectedLibrary->findText(newName));
}

void LibraryWindow::cancelCreating()
{
	stopLibraryCreator();
}

void LibraryWindow::stopLibraryCreator()
{
	libraryCreator->stop();
	libraryCreator->wait();
}

void LibraryWindow::setRootIndex()
{
	if(libraries.size()>0)
	{	
		QString path=libraries.value(selectedLibrary->currentText())+"/.yacreaderlibrary";
		QDir d; //TODO change this by static methods (utils class?? with delTree for example) 
		if(d.exists(path))
		{
			loadCovers(QModelIndex());
		}
		else
		{
			comicView->setModel(NULL);
			comicFlow->clear();
		}
	}
}


void LibraryWindow::toggleFullScreen()
{
	fullscreen?toNormal():toFullScreen();
	fullscreen = !fullscreen;
}

void LibraryWindow::toFullScreen()
{
	comicFlow->hide();
	comicFlow->setSlideSize(slideSizeF);
	comicFlow->setCenterIndex(comicFlow->centerIndex());
	comics->hide();
	left->hide();
	libraryToolBar->hide();

	showFullScreen();

	comicFlow->show();
	comicFlow->setFocus(Qt::OtherFocusReason);
	fullScreenToolTip->move((width()-fullScreenToolTip->width())/2,0);
	fullScreenToolTip->adjustSize();
	fullScreenToolTip->show();
}

void LibraryWindow::toNormal()
{
	comicFlow->hide();
	comicFlow->setSlideSize(slideSizeW);
	comicFlow->setCenterIndex(comicFlow->centerIndex());
	comicFlow->render();
	comics->show();
	left->show();
	fullScreenToolTip->hide();
	libraryToolBar->show();
	comicFlow->show();

	showMaximized();
}

void LibraryWindow::setFoldersFilter(QString filter)
{
	if(filter.isEmpty() && dm->isFilterEnabled())
	{
		dm->resetFilter();
		//foldersView->collapseAll();
		if(index != 0)
		{
			QModelIndex mi = dm->indexFromItem(index,column);
			foldersView->scrollTo(mi,QAbstractItemView::PositionAtTop);
			sm->select(mi,QItemSelectionModel::Select);
			foldersView->setSelectionModel(sm);
		}
	}
	else
	{
		if(!filter.isEmpty())
		{
			dm->setFilter(filter, includeComicsCheckBox->isChecked());
			foldersView->expandAll();
		}
	}
}

void LibraryWindow::showProperties()
{
	QModelIndexList indexList = comicView->selectionModel()->selectedRows();

	QList<ComicDB> comics = dmCV->getComics(indexList);
	ComicDB c = comics[0];
	_comicIdEdited = c.id;//static_cast<TableItem*>(indexList[0].internalPointer())->data(4).toULongLong();

	//QModelIndex mi = comicView->currentIndex();
	//QString path = QDir::cleanPath(currentPath()+dmCV->getComicPath(mi));

	//ThumbnailCreator tc(path,"");
	//tc.create();
	propertiesDialog->databasePath = dm->getDatabase();
	propertiesDialog->basePath = currentPath();
	propertiesDialog->setComics(comics);
	
	/*propertiesDialog->setCover(tc.getCover());
	propertiesDialog->setFilename(path.split("/").last());
	propertiesDialog->setNumpages(tc.getNumPages());
	QFile file(path);
	propertiesDialog->setSize(file.size()/(1024.0*1024));
	file.close();*/
	propertiesDialog->show();
}

void LibraryWindow::asignNumbers()
{
	QModelIndexList indexList = comicView->selectionModel()->selectedRows();

	QList<ComicDB> comics = dmCV->getComics(indexList);
	ComicDB c = comics[0];
	_comicIdEdited = c.id;

	int startingNumber = dmCV->getIndexFromId(comics[0].id).row()+1;
	if(comics.count()>1)
	{
		bool ok;
		int n = QInputDialog::getInt(this, tr("Asign comics numbers"),
			tr("Asign numbers starting in:"), dmCV->getIndexFromId(comics[0].id).row()+1,0,2147483647,1,&ok);
		if (ok)
			startingNumber = n;
		else
			return;
	}

	QSqlDatabase db = DataBaseManagement::loadDatabase(dm->getDatabase());
	db.transaction();

	for(int i = 0;i<comics.length();i++)
	{
		ComicDB c = comics[i];
		c.info.setNumber(startingNumber+i);
		c.info.edited = true;
		c.info.update(db);
		/*QString hash = comics[i].info.hash;
		comics[i].info.setNumber(i+1);
		comics[i].info.update(db);*/
	}

	db.commit();
	db.close();
	QSqlDatabase::removeDatabase(dm->getDatabase());
	
	reloadCovers();
}

void LibraryWindow::openContainingFolderComic()
{
	QModelIndex modelIndex = comicView->currentIndex();
	QFileInfo file = QDir::cleanPath(currentPath() + dmCV->getComicPath(modelIndex)); 
	QString path = file.absolutePath();
	QDesktopServices::openUrl(QUrl("file:///"+path, QUrl::TolerantMode));
}

void LibraryWindow::openContainingFolder()
{
	QModelIndex modelIndex = foldersView->currentIndex();
	QString path = QDir::cleanPath(currentPath() + dm->getFolderPath(modelIndex));
	QDesktopServices::openUrl(QUrl("file:///"+path, QUrl::TolerantMode));
}

void LibraryWindow::exportLibrary(QString destPath)
{
	QString currentLibrary = selectedLibrary->currentText();
	QString path = libraries.value(currentLibrary)+"/.yacreaderlibrary";
	packageManager->createPackage(path,destPath+"/"+currentLibrary);
}

void LibraryWindow::importLibrary(QString clc,QString destPath,QString name)
{
	packageManager->extractPackage(clc,destPath+"/"+name);
	openLibrary(destPath+"/"+name,name);
}

void LibraryWindow::reloadOptions()
{
	comicFlow->setFlowType(flowType);
}

//TODO esto sobra
void LibraryWindow::updateFoldersView(QString path)
{
	//QModelIndex mi = dm->index(path);
	//int rowCount = dm->rowCount(mi);
	//if(!fetching)
	//{
	//	//fetching = true;
	//	for(int i=0;i<rowCount;i++)
	//	{
	//		dm->fetchMore(dm->index(i,0,mi));
	//		//int childCount = dm->rowCount(dm->index(i,0,mi));
	//		//if(childCount>0)
	//		//	QMessageBox::critical(NULL,tr("..."),tr("-----"));
	//		fetching = false;
	//	}
	//}

}

void LibraryWindow::searchInFiles(int state)
{

	if(state == Qt::Checked)
	{
		if(!foldersFilter->text().isEmpty())
		{
			dm->setFilter(foldersFilter->text(), true);
			foldersView->expandAll();
		}
	}
	else
	{
		if(!foldersFilter->text().isEmpty())
		{
			dm->setFilter(foldersFilter->text(), false);
			foldersView->expandAll();
		}
	}
}

QString LibraryWindow::currentPath()
{
	return libraries.value(selectedLibrary->currentText());
}

void LibraryWindow::hideComicFlow(bool hide)
{
	if(hide)
	{
		QList<int> sizes;
		sizes.append(0);
		int total = sVertical->sizes().at(0) + sVertical->sizes().at(1);
		sizes.append(total);
		sVertical->setSizes(sizes);	
	}
	else
	{
		QList<int> sizes;
		int total = sVertical->sizes().at(0) + sVertical->sizes().at(1);
		sizes.append(2*total/3);
		sizes.append(total/3);
		sVertical->setSizes(sizes);	
	}

}

void LibraryWindow::showExportComicsInfo()
{
	exportComicsInfoDialog->source = currentPath() + "/.yacreaderlibrary/library.ydb";
	exportComicsInfoDialog->show();
}

void LibraryWindow::showImportComicsInfo()
{
	importComicsInfoDialog->dest = currentPath() + "/.yacreaderlibrary/library.ydb";
	importComicsInfoDialog->show();
}

QList<LibraryItem *> LibraryWindow::getFolderContentFromLibrary(const QString & libraryName, qulonglong folderId)
{
	QSqlDatabase db = DataBaseManagement::loadDatabase(libraries.value(libraryName)+"/.yacreaderlibrary");
	
	QList<LibraryItem *> list = Folder::getFoldersFromParent(folderId,db);
	
	db.close();
	QSqlDatabase::removeDatabase(libraries.value(libraryName));
	return list;

}

QList<LibraryItem *> LibraryWindow::getFolderComicsFromLibrary(const QString & libraryName, qulonglong folderId)
{
	QSqlDatabase db = DataBaseManagement::loadDatabase(libraries.value(libraryName)+"/.yacreaderlibrary");

	QList<LibraryItem *> list = ComicDB::getComicsFromParent(folderId,db);

	db.close();
	QSqlDatabase::removeDatabase(libraries.value(libraryName));
	return list;
}

qulonglong LibraryWindow::getParentFromComicFolderId(const QString & libraryName, qulonglong id)
{
	QSqlDatabase db = DataBaseManagement::loadDatabase(libraries.value(libraryName)+"/.yacreaderlibrary");

	Folder f(id,db);

	db.close();
	QSqlDatabase::removeDatabase(libraries.value(libraryName));
	return f.parentId;
}

ComicDB LibraryWindow::getComicInfo(const QString & libraryName, qulonglong id)
{
	QSqlDatabase db = DataBaseManagement::loadDatabase(libraries.value(libraryName)+"/.yacreaderlibrary");

	ComicDB comic;
	comic.load(id,db);

	db.close();
	QSqlDatabase::removeDatabase(libraries.value(libraryName));
	return comic;
}