#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QFontComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMenu>
#include <QMenuBar>
#include <QTextCodec>
#include <QLabel>
#include <QTextEdit>
#include <QToolBar>
#include <QTextCursor>
#include <QTextDocumentWriter>
#include <QTextList>
#include <QtDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QMimeData>
#ifndef QT_NO_PRINTER
#include <QPrintDialog>
#include <QPrinter>
#include <QPrintPreviewDialog>
#endif
#include "../ContactListEdit.hpp"

#include "MailEditor.hpp"
#include <fc/log/logger.hpp>

#include <bts/application.hpp>
#include <bts/profile.hpp>

#ifdef Q_OS_MAC
const QString rsrcPath = ":/images/mac";
#else
const QString rsrcPath = ":/images/win";
#endif

using namespace bts::bitchat;
using namespace bts::addressbook;

MailEditor::MailEditor(QWidget *parent, QCompleter* contact_completer)
: QDialog(parent),
  _contact_completer(contact_completer)
{
    to_values = new QTextDocument(this);
    cc_values = new QTextDocument(this);
    bcc_values = new QTextDocument(this);

//    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    layout = new QGridLayout(this);
    layout->setHorizontalSpacing(0);
    layout->setVerticalSpacing(0);
    layout->setContentsMargins(0,0,0,0);

    setupMailActions();
    setupEditActions();
    setupTextActions();
    setupMoneyToolBar();
    setupAddressBar();

    {
        //QMenu* helpMenu = new QMenu(tr("Help"), this);
        //menuBar()->addMenu(helpMenu);
        //helpMenu->addAction(tr("About"), this, SLOT(about()));
        //helpMenu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));
    }
    textEdit = new QTextEdit(this);

    layout->addWidget( textEdit, 4, 0 );
    connect(textEdit, SIGNAL(currentCharFormatChanged(QTextCharFormat)),
            this, SLOT(currentCharFormatChanged(QTextCharFormat)));
    connect(textEdit, SIGNAL(cursorPositionChanged()),
            this, SLOT(cursorPositionChanged()));

    //setCentralWidget(textEdit);
    //layout()->addWidget(textEdit);
    textEdit->setFocus();

    fontChanged(textEdit->font());
    colorChanged(textEdit->textColor());
    alignmentChanged(textEdit->alignment());

    connect(textEdit->document(), SIGNAL(modificationChanged(bool)),
            actionSave, SLOT(setEnabled(bool)));
    connect(textEdit->document(), SIGNAL(modificationChanged(bool)),
            this, SLOT(setWindowModified(bool)));
    connect(textEdit->document(), SIGNAL(undoAvailable(bool)),
            actionUndo, SLOT(setEnabled(bool)));
    connect(textEdit->document(), SIGNAL(redoAvailable(bool)),
            actionRedo, SLOT(setEnabled(bool)));

    setWindowModified(textEdit->document()->isModified());
    actionSave->setEnabled(textEdit->document()->isModified());
    actionUndo->setEnabled(textEdit->document()->isUndoAvailable());
    actionRedo->setEnabled(textEdit->document()->isRedoAvailable());

    connect(actionUndo, SIGNAL(triggered()), textEdit, SLOT(undo()));
    connect(actionRedo, SIGNAL(triggered()), textEdit, SLOT(redo()));

    actionCut->setEnabled(false);
    actionCopy->setEnabled(false);

    connect(actionCut, SIGNAL(triggered()), textEdit, SLOT(cut()));
    connect(actionCopy, SIGNAL(triggered()), textEdit, SLOT(copy()));
    connect(actionPaste, SIGNAL(triggered()), textEdit, SLOT(paste()));

    connect(textEdit, SIGNAL(copyAvailable(bool)), actionCut, SLOT(setEnabled(bool)));
    connect(textEdit, SIGNAL(copyAvailable(bool)), actionCopy, SLOT(setEnabled(bool)));

#ifndef QT_NO_CLIPBOARD
    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));
#endif

    enableFormat(false);
}

//set focus to first empty field in mail editor, then show window
void MailEditor::setFocusAndShow()
{
    if (to_field->toPlainText().trimmed().isEmpty())
        to_field->setFocus();
    else if (subject_field->text().trimmed().isEmpty())
        subject_field->setFocus();
    else
        textEdit->setFocus();
    show();
}

void MailEditor::addToContact(int contact_id)
{
    if (contact_id < 0)
        return;
    auto app = bts::application::instance();
    auto profile = app->get_profile();
    auto contacts = profile->get_addressbook()->get_contacts();
    QString to_string = contacts[contact_id].getFullName().c_str();
    to_field->insertCompletion(to_string);
}

void MailEditor::closeEvent(QCloseEvent* closeEvent)
{
    if (maybeSave())
        closeEvent->accept();
    else
        closeEvent->ignore();
}

void MailEditor::subjectChanged( const QString& subject )
{
   if( subject == QString() )
   {
      setWindowTitle( tr("(No Subject)") );
   }
   else
   {
      setWindowTitle( subject );
   }
}

void MailEditor::setupMailActions()
{
    QToolBar* tool_bar = new QToolBar(this);
    layout->addWidget( tool_bar, 0, 0 );
 //   tool_bar->setWindowTitle(tr("File Actions"));
 //   addToolBar(tool_bar);

    //QMenu* menu = new QMenu(tr("&File"), this);
    //menuBar()->addMenu(menu);

    QAction* action;

//    QIcon newIcon = QIcon::fromTheme("mail-send", QIcon(rsrcPath + "/filenew.png"));
 //   a = new QAction( newIcon, tr("&Send"), this);
  //  a->setPriority(QAction::LowPriority);
   // a->setShortcut(QKeySequence::Save);
    //connect(a, SIGNAL(triggered()), this, SLOT(fileNew()));
    //tool_bar->addAction(a);
    //menu->addAction(a);

//    a = new QAction(QIcon::fromTheme("document-open", QIcon(rsrcPath + "/fileopen.png")),
 //                   tr("&Open..."), this);
//    a->setShortcut(QKeySequence::Open);
//    connect(a, SIGNAL(triggered()), this, SLOT(fileOpen()));
 //   tool_bar->addAction(a);
    //menu->addAction(a);

    //menu->addSeparator();

    actionSave = action = new QAction(QIcon::fromTheme("mail-send", QIcon(":/images/128x128/send_mail.png")),
                                 tr("&Send"), this);
    action->setShortcut(QKeySequence::Save);
    connect(action, SIGNAL(triggered()), this, SLOT(sendMailMessage()));
    action->setEnabled(false);
    tool_bar->addAction(action);

    QWidget* spacer = new QWidget(tool_bar);
    spacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    tool_bar->addWidget(spacer);

    fieldsMenu = new QMenu( tr("Mail Fields") );
    actionToggleCc = fieldsMenu->addAction( "Cc:" );
    actionToggleBcc = fieldsMenu->addAction( "Bcc:" );
    actionToggleFrom = fieldsMenu->addAction( "From:" );
    actionToggleCc->setCheckable(true);
    actionToggleBcc->setCheckable(true);
    actionToggleFrom->setCheckable(true);

    fieldsButton = new QToolButton();
    fieldsButton->setIcon(QIcon( ":/images/gear.png" ) );
    fieldsButton->setMenu( fieldsMenu );
    fieldsButton->setPopupMode( QToolButton::InstantPopup );
    tool_bar->addWidget( fieldsButton );

    action = new QAction(QIcon::fromTheme("mail-format", QIcon(":/images/format_text.png")),
                                 tr("&Format"), this);
 //   a->setShortcut(QKeySequence::Save);
    connect(action, &QAction::toggled, this, &MailEditor::enableFormat);
    action->setCheckable(true);
    action->setEnabled(true);
    tool_bar->addAction(action);


    actionAttachMoney = action = new QAction(QIcon::fromTheme("mail-money", QIcon(":/images/money-in-envelope.png")),
                                 tr("&Attach Money"), this);
 //   a->setShortcut(QKeySequence::Save);
    connect(action, &QAction::toggled, this, &MailEditor::enableSendMoney );
    action->setCheckable(true);
    action->setEnabled(true);
    tool_bar->addAction(action);

    actionAttachFile = action = new QAction(QIcon::fromTheme("mail-file", QIcon(":/images/paperclip-icon.png")),
                                 tr("&Attach File"), this);
 //   a->setShortcut(QKeySequence::Save);
    connect(action, &QAction::toggled, this, &MailEditor::showAttachFileDialog );
 //   a->setCheckable(true);
    action->setEnabled(true);
    tool_bar->addAction(action);

    //menu->addAction(a);

//    action = new QAction(tr("Save &As..."), this);
 //   action->setPriority(QAction::LowPriority);
  //  connect(action, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    //menu->addAction(action);
    //menu->addSeparator();

//    action = new QAction(tr("&Quit"), this);
 //   action->setShortcut(Qt::CTRL + Qt::Key_Q);
  //  connect(action, SIGNAL(triggered()), this, SLOT(close()));
    //menu->addAction(action);
}

void MailEditor::setupAddressBar()
{
   address_bar = nullptr;//new QWidget(this);
   address_layout = nullptr;//new QFormLayout(address_bar);
   to_field = nullptr; //new ContactListEdit(address_bar);
   //to_field->setCompleter(_contact_completer);
   //to_field->setDocument(to_values);
   cc_field = nullptr; //new QLineEdit(address_bar);
   bcc_field = nullptr; //new QLineEdit(address_bar);
   from_field = new QComboBox(address_bar);
   auto idents = bts::application::instance()->get_profile()->identities();
   for( uint32_t i = 0; i < idents.size(); ++i )
   {
      // TODO: add user icon?
      elog( "From field... " );
      from_field->insertItem( i, idents[i].dac_id.c_str() );
   }
   subject_field = new QLineEdit(address_bar);
   setWindowTitle( tr( "New Message" ) ); 
   updateAddressBarLayout();

   connect( actionToggleCc, &QAction::toggled,  [=](bool state) { updateAddressBarLayout(); } );
   connect( actionToggleBcc, &QAction::toggled,  [=](bool state) { updateAddressBarLayout(); } );
   connect( actionToggleFrom, &QAction::toggled,  [=](bool state) { updateAddressBarLayout(); } );
}
void MailEditor::updateAddressBarLayout()
{
   QString subject_text = subject_field->text();

   delete address_bar;

   to_field = nullptr; 
   from_field = nullptr; 
   subject_field = nullptr; 
   cc_field = nullptr;
   bcc_field = nullptr;

   address_bar    = new QWidget(this);
   address_layout = new QFormLayout(address_bar);

   to_field = new ContactListEdit(address_bar);
   to_field->setCompleter(_contact_completer);
   to_field->setDocument(to_values);
   address_layout->addRow( "To:",  to_field );

   if( actionToggleCc->isChecked() )
   {
      cc_field = new ContactListEdit(address_bar);
      cc_field->setCompleter(_contact_completer);
      cc_field->setDocument(cc_values);
      address_layout->addRow( "Cc:",  cc_field );
   }

   if( actionToggleBcc->isChecked() )
   {
      bcc_field = new ContactListEdit(address_bar);
      bcc_field->setCompleter(_contact_completer);
      bcc_field->setDocument(bcc_values);
      address_layout->addRow( "Bcc:",  bcc_field );
   }
   subject_field = new QLineEdit(address_bar);
   subject_field->setText(subject_text);
   address_layout->addRow( "Subject:",  subject_field);
   connect( subject_field, &QLineEdit::textChanged, this, &MailEditor::subjectChanged );

   if( actionToggleFrom->isChecked() )
   {
      from_field = new QComboBox(address_bar);
      auto idents = bts::application::instance()->get_profile()->identities();
      for( uint32_t i = 0; i < idents.size(); ++i )
      {
         // TODO: add user icon?
         elog( "From field... " );
         from_field->insertItem( i, idents[i].dac_id.c_str() );
      }
     // from_field->setText( from_text );
      address_layout->addRow( "From:", from_field );
   }
   address_layout->setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );
   layout->addWidget( address_bar, 1, 0 );
}

void MailEditor::enableFormat(bool show_format)
{
    format_tool_bar->setVisible(show_format);
   // style_tb->setVisible(show_format);
}
void MailEditor::enableSendMoney(bool show_send_money )
{
    money_tool_bar->setVisible(show_send_money);
   // style_tb->setVisible(show_format);
}
void MailEditor::showAttachFileDialog(bool show_send_money )
{
   // money_tool_bar->setVisible(show_send_money);
   // style_tb->setVisible(show_format);
}

void MailEditor::setupEditActions()
{
//    QToolBar *tool_bar = new QToolBar(this);
//    tool_bar->setWindowTitle(tr("Edit Actions"));
 //   addToolBar(tool_bar);
    QMenu* menu = new QMenu(tr("&Edit"), this);
    //menuBar()->addMenu(menu);

    QAction* action;
    action = actionUndo = new QAction(QIcon::fromTheme("edit-undo", QIcon(rsrcPath + "/editundo.png")),
                                              tr("&Undo"), this);
    action->setShortcut(QKeySequence::Undo);
//    tool_bar->addAction(a);
    menu->addAction(action);
    action = actionRedo = new QAction(QIcon::fromTheme("edit-redo", QIcon(rsrcPath + "/editredo.png")),
                                              tr("&Redo"), this);
    action->setPriority(QAction::LowPriority);
    action->setShortcut(QKeySequence::Redo);
//    tool_bar->addAction(a);
    menu->addAction(action);
    menu->addSeparator();
    action = actionCut = new QAction(QIcon::fromTheme("edit-cut", QIcon(rsrcPath + "/editcut.png")),
                                             tr("Cu&t"), this);
    action->setPriority(QAction::LowPriority);
    action->setShortcut(QKeySequence::Cut);
//    tool_bar->addAction(a);
    menu->addAction(action);
    action = actionCopy = new QAction(QIcon::fromTheme("edit-copy", QIcon(rsrcPath + "/editcopy.png")),
                                 tr("&Copy"), this);
    action->setPriority(QAction::LowPriority);
    action->setShortcut(QKeySequence::Copy);
//    tool_bar->addAction(a);
    menu->addAction(action);
    action = actionPaste = new QAction(QIcon::fromTheme("edit-paste", QIcon(rsrcPath + "/editpaste.png")),
                                  tr("&Paste"), this);
    action->setPriority(QAction::LowPriority);
    action->setShortcut(QKeySequence::Paste);
//    tool_bar->addAction(a);
    menu->addAction(action);
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData* mime_data = QApplication::clipboard()->mimeData())
        actionPaste->setEnabled(mime_data->hasText());
#endif
}
void MailEditor::setupMoneyToolBar()
{
    auto tool_bar = money_tool_bar = new QToolBar(this);
    tool_bar->hide();
    tool_bar->setIconSize( QSize( 16,16 ) );
    layout->addWidget( tool_bar, 3, 0 );

    money_amount = new QLineEdit(tool_bar);
    money_amount->setPlaceholderText( "0.00" );
    money_unit   = new QComboBox(tool_bar);
    money_unit->insertItem( 0, QIcon::fromTheme("currency-bitcoin", QIcon( ":/images/bitcoin.png" ) ), QString("BTC") );
    money_unit->insertItem( 1, QIcon::fromTheme("currency-litecoin", QIcon( ":/images/litecoin128.png" ) ), QString("LTC") );
    money_unit->insertItem( 2, QIcon::fromTheme("currency-bitusd", QIcon( ":/images/bitusd.png" ) ), QString("BitUSD") );

    connect( money_unit, SIGNAL(currentIndexChanged(int)), this, SLOT(moneyUnitChanged(int)) );
    
    money_balance = new QLabel("Balance: 17.76 BTC",tool_bar );

    QWidget* spacer = new QWidget(tool_bar);
    QWidget* spacer2 = new QWidget(tool_bar);
    spacer->setMaximumWidth(10);
    tool_bar->addWidget(spacer);
    tool_bar->addWidget(money_amount);
    tool_bar->addWidget(money_unit);
    tool_bar->addWidget(spacer2);
    tool_bar->addWidget(money_balance);

    money_amount->setMaximumWidth(120);
}

void MailEditor::moneyUnitChanged( int index )
{
    switch( index )
    {
       case 0:
          money_balance->setText( "Balance: 17.76 BTC" );
          break;
       case 1:
          money_balance->setText( "Balance: 0.00 LTC" );
          break;
       case 2:
          money_balance->setText( "Balance: 0.00 BitUSD" );
          break;
    }
}

void MailEditor::setupTextActions()
{
    auto tool_bar = format_tool_bar = new QToolBar(this);
    tool_bar->setIconSize( QSize( 16,16 ) );
    layout->addWidget( tool_bar, 2, 0 );


//    tool_bar->setWindowTitle(tr("Format Actions"));
    //addToolBar(tool_bar);

    QMenu* menu = new QMenu(tr("F&ormat"), this);
    //menuBar()->addMenu(menu);

    actionTextBold = new QAction(QIcon::fromTheme("format-text-bold", QIcon(rsrcPath + "/textbold.png")),
                                 tr("&Bold"), this);
    actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
    actionTextBold->setPriority(QAction::LowPriority);
	QFont bold;
    bold.setBold(true);
    actionTextBold->setFont(bold);
    connect(actionTextBold, SIGNAL(triggered()), this, SLOT(textBold()));
    tool_bar->addAction(actionTextBold);
    menu->addAction(actionTextBold);
    actionTextBold->setCheckable(true);

    actionTextItalic = new QAction(QIcon::fromTheme("format-text-italic",
                                                    QIcon(rsrcPath + "/textitalic.png")),
                                   tr("&Italic"), this);
    actionTextItalic->setPriority(QAction::LowPriority);
    actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    actionTextItalic->setFont(italic);
    connect(actionTextItalic, SIGNAL(triggered()), this, SLOT(textItalic()));
    tool_bar->addAction(actionTextItalic);
    menu->addAction(actionTextItalic);
    actionTextItalic->setCheckable(true);

    actionTextUnderline = new QAction(QIcon::fromTheme("format-text-underline",
                                                       QIcon(rsrcPath + "/textunder.png")),
                                      tr("&Underline"), this);
    actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
    actionTextUnderline->setPriority(QAction::LowPriority);
    QFont underline;
    underline.setUnderline(true);
    actionTextUnderline->setFont(underline);
    connect(actionTextUnderline, SIGNAL(triggered()), this, SLOT(textUnderline()));
    tool_bar->addAction(actionTextUnderline);
    menu->addAction(actionTextUnderline);
    actionTextUnderline->setCheckable(true);

    menu->addSeparator();

    QActionGroup *grp = new QActionGroup(this);
    connect(grp, SIGNAL(triggered(QAction*)), this, SLOT(textAlign(QAction*)));

    // Make sure the alignLeft  is always left of the alignRight
    if (QApplication::isLeftToRight()) {
        actionAlignLeft = new QAction(QIcon::fromTheme("format-justify-left",
                                                       QIcon(rsrcPath + "/textleft.png")),
                                      tr("&Left"), grp);
        actionAlignCenter = new QAction(QIcon::fromTheme("format-justify-center",
                                                         QIcon(rsrcPath + "/textcenter.png")),
                                        tr("C&enter"), grp);
        actionAlignRight = new QAction(QIcon::fromTheme("format-justify-right",
                                                        QIcon(rsrcPath + "/textright.png")),
                                       tr("&Right"), grp);
    } else {
        actionAlignRight = new QAction(QIcon::fromTheme("format-justify-right",
                                                        QIcon(rsrcPath + "/textright.png")),
                                       tr("&Right"), grp);
        actionAlignCenter = new QAction(QIcon::fromTheme("format-justify-center",
                                                         QIcon(rsrcPath + "/textcenter.png")),
                                        tr("C&enter"), grp);
        actionAlignLeft = new QAction(QIcon::fromTheme("format-justify-left",
                                                       QIcon(rsrcPath + "/textleft.png")),
                                      tr("&Left"), grp);
    }
    actionAlignJustify = new QAction(QIcon::fromTheme("format-justify-fill",
                                                      QIcon(rsrcPath + "/textjustify.png")),
                                     tr("&Justify"), grp);

    actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
    actionAlignLeft->setCheckable(true);
    actionAlignLeft->setPriority(QAction::LowPriority);
    actionAlignCenter->setShortcut(Qt::CTRL + Qt::Key_E);
    actionAlignCenter->setCheckable(true);
    actionAlignCenter->setPriority(QAction::LowPriority);
    actionAlignRight->setShortcut(Qt::CTRL + Qt::Key_R);
    actionAlignRight->setCheckable(true);
    actionAlignRight->setPriority(QAction::LowPriority);
    actionAlignJustify->setShortcut(Qt::CTRL + Qt::Key_J);
    actionAlignJustify->setCheckable(true);
    actionAlignJustify->setPriority(QAction::LowPriority);

    tool_bar->addActions(grp->actions());
    menu->addActions(grp->actions());

    menu->addSeparator();

    QPixmap pix(16, 16);
    pix.fill(Qt::black);
    actionTextColor = new QAction(pix, tr("&Color..."), this);
    connect(actionTextColor, SIGNAL(triggered()), this, SLOT(textColor()));
    tool_bar->addAction(actionTextColor);
    menu->addAction(actionTextColor);

    //format_tool_bar = tool_bar = new QToolBar(this);
    //layout->addWidget( tool_bar, 3, 0 );
    //tool_bar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    //tool_bar->setWindowTitle(tr("Format Actions"));
    //addToolBarBreak(Qt::TopToolBarArea);
    //addToolBar(tool_bar);


    comboSize = new QComboBox(tool_bar);
    comboSize->setObjectName("comboSize");
    tool_bar->addWidget(comboSize);
    comboSize->setEditable(true);

    QFontDatabase db;
    foreach(int size, db.standardSizes())
        comboSize->addItem(QString::number(size));

    connect(comboSize, SIGNAL(activated(QString)), this, SLOT(textSize(QString)));
    comboSize->setCurrentIndex(comboSize->findText(QString::number(QApplication::font()
                                                                   .pointSize())));

    comboFont = new QFontComboBox(tool_bar);
    tool_bar->addWidget(comboFont);
    connect(comboFont, SIGNAL(activated(QString)), this, SLOT(textFamily(QString)));
}

bool MailEditor::load(const QString &f)
{
    if (!QFile::exists(f))
        return false;
    QFile file(f);
    if (!file.open(QFile::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    QTextCodec *codec = Qt::codecForHtml(data);
    QString str = codec->toUnicode(data);
    if (Qt::mightBeRichText(str)) {
        textEdit->setHtml(str);
    } else {
        str = QString::fromLocal8Bit(data);
        textEdit->setPlainText(str);
    }

    return true;
}

bool MailEditor::maybeSave()
{
    if (!textEdit->document()->isModified())
        return true;
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Application"),
                               tr("The document has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return fileSave();
    else if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

//DLNFIX
void MailEditor::sendMailMessage()
{
    auto app = bts::application::instance();
    auto profile = app->get_profile();
    auto idents = profile->identities();
    private_email_message msg;
    msg.subject = subject_field->text().toStdString();
    msg.body = textEdit->document()->toHtml().toStdString();
    if( idents.size() )
    {         
        auto my_priv_key = profile->get_keychain().get_identity_key( idents[0].dac_id );
        //foreach(to, toList)
        QStringList images;
        QTextBlock b = to_field->document()->begin();
        while (b.isValid()) 
        {
            for (QTextBlock::iterator i = b.begin(); !i.atEnd(); ++i) 
            {
                QTextCharFormat format = i.fragment().charFormat();
                bool isImage = format.isImageFormat();
                if (isImage)
                    images << format.toImageFormat().name();
            }
            b = b.next();
        }
        foreach(auto recipient,images)
        {
            std::string to = recipient.toStdString();
            //check first to see if we have a dac_id
            auto to_contact = profile->get_addressbook()->get_contact_by_dac_id(to);
            if (!to_contact.valid())
            { //TODO if not dac_id, check if we have a full name

            }
            assert(to_contact.valid());
            app->send_email(msg, to_contact->public_key, my_priv_key);
        }
        //TODO add code to save to SentItems
        textEdit->document()->setModified(false);
        close();
    }
}

bool MailEditor::fileSave()
{
    if (_fileName.isEmpty())
        return fileSaveAs();

    QTextDocumentWriter writer(_fileName);
    bool success = writer.write(textEdit->document());
    if (success)
        textEdit->document()->setModified(false);
    else
    {
        elog( "error writing document!" );
    }
    return success;
}

bool MailEditor::fileSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."), QString(),
                                              tr("ODF files (*.odt);;HTML-Files "
                                                 "(*.htm *.html);;All Files (*)"));
    if (fn.isEmpty())
        return false;
    if (!(fn.endsWith(".odt", Qt::CaseInsensitive)
          || fn.endsWith(".htm", Qt::CaseInsensitive)
          || fn.endsWith(".html", Qt::CaseInsensitive))) {
        fn += ".html"; // default
    }
    _fileName = fn;
    return fileSave();
}

void MailEditor::filePrint()
{
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    if (textEdit->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted)
        textEdit->print(&printer);
    delete dlg;
#endif
}

void MailEditor::filePrintPreview()
{
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, SIGNAL(paintRequested(QPrinter*)), SLOT(printPreview(QPrinter*)));
    preview.exec();
#endif
}

void MailEditor::printPreview(QPrinter *printer)
{
#ifdef QT_NO_PRINTER
    Q_UNUSED(printer);
#else
    textEdit->print(printer);
#endif
}


void MailEditor::filePrintPdf()
{
#ifndef QT_NO_PRINTER
//! [0]
    QString fileName = QFileDialog::getSaveFileName(this, "Export PDF",
                                                    QString(), "*.pdf");
    if (!fileName.isEmpty()) {
        if (QFileInfo(fileName).suffix().isEmpty())
            fileName.append(".pdf");
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        textEdit->document()->print(&printer);
    }
//! [0]
#endif
}

void MailEditor::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void MailEditor::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(actionTextUnderline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MailEditor::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MailEditor::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
}

void MailEditor::textSize(const QString &p)
{
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void MailEditor::textStyle(int styleIndex)
{
    QTextCursor cursor = textEdit->textCursor();

    if (styleIndex != 0) {
        QTextListFormat::Style style = QTextListFormat::ListDisc;

        switch (styleIndex) {
            default:
            case 1:
                style = QTextListFormat::ListDisc;
                break;
            case 2:
                style = QTextListFormat::ListCircle;
                break;
            case 3:
                style = QTextListFormat::ListSquare;
                break;
            case 4:
                style = QTextListFormat::ListDecimal;
                break;
            case 5:
                style = QTextListFormat::ListLowerAlpha;
                break;
            case 6:
                style = QTextListFormat::ListUpperAlpha;
                break;
            case 7:
                style = QTextListFormat::ListLowerRoman;
                break;
            case 8:
                style = QTextListFormat::ListUpperRoman;
                break;
        }

        cursor.beginEditBlock();

        QTextBlockFormat blockFmt = cursor.blockFormat();

        QTextListFormat listFmt;

        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
        } else {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }

        listFmt.setStyle(style);

        cursor.createList(listFmt);

        cursor.endEditBlock();
    } else {
        // ####
        QTextBlockFormat bfmt;
        bfmt.setObjectIndex(-1);
        cursor.mergeBlockFormat(bfmt);
    }
}

void MailEditor::textColor()
{
    QColor col = QColorDialog::getColor(textEdit->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}

void MailEditor::textAlign(QAction* a)
{
    if (a == actionAlignLeft)
        textEdit->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    else if (a == actionAlignCenter)
        textEdit->setAlignment(Qt::AlignHCenter);
    else if (a == actionAlignRight)
        textEdit->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    else if (a == actionAlignJustify)
        textEdit->setAlignment(Qt::AlignJustify);
}

void MailEditor::currentCharFormatChanged(const QTextCharFormat& format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void MailEditor::cursorPositionChanged()
{
    alignmentChanged(textEdit->alignment());
}

void MailEditor::clipboardDataChanged()
{
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        actionPaste->setEnabled(md->hasText());
#endif
}

void MailEditor::about()
{
    QMessageBox::about(this, tr("About"), tr("This example demonstrates Qt's "
        "rich text editing facilities in action, providing an example "
        "document for you to experiment with."));
}

void MailEditor::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = textEdit->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    textEdit->mergeCurrentCharFormat(format);
}

void MailEditor::fontChanged(const QFont& new_font)
{
    comboFont->setCurrentIndex(comboFont->findText(QFontInfo(new_font).family()));
    comboSize->setCurrentIndex(comboSize->findText(QString::number(new_font.pointSize())));
    actionTextBold->setChecked(new_font.bold());
    actionTextItalic->setChecked(new_font.italic());
    actionTextUnderline->setChecked(new_font.underline());
}

void MailEditor::colorChanged(const QColor& new_color)
{
    QPixmap pixmap(16, 16);
    pixmap.fill(new_color);
    actionTextColor->setIcon(pixmap);
}

void MailEditor::alignmentChanged(Qt::Alignment alignment)
{
    if (alignment & Qt::AlignLeft)
        actionAlignLeft->setChecked(true);
    else if (alignment & Qt::AlignHCenter)
        actionAlignCenter->setChecked(true);
    else if (alignment & Qt::AlignRight)
        actionAlignRight->setChecked(true);
    else if (alignment & Qt::AlignJustify)
        actionAlignJustify->setChecked(true);
}

