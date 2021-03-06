#include "MailInbox.hpp"
#include "../ui_MailInbox.h"
#include "InboxModel.hpp"

MailInbox::MailInbox( QWidget* parent )
: ui( new Ui::MailInbox() ),
  _type(Inbox)
{
   ui->setupUi( this );
}

MailInbox::~MailInbox()
{
}

void MailInbox::setModel( QAbstractItemModel* model, InboxType type )
{
   _type = type;
   ui->inbox_table->setModel(model);

   ui->inbox_table->horizontalHeader()->resizeSection( InboxModel::To, 120 );
   ui->inbox_table->horizontalHeader()->resizeSection( InboxModel::Subject, 300 );
   ui->inbox_table->horizontalHeader()->resizeSection( InboxModel::DateReceived, 120 );
   ui->inbox_table->horizontalHeader()->resizeSection( InboxModel::From, 120 );
   ui->inbox_table->horizontalHeader()->resizeSection( InboxModel::DateSent, 120 );
   if( _type == Inbox )
   {
      ui->inbox_table->horizontalHeader()->hideSection( InboxModel::Status );
      ui->inbox_table->horizontalHeader()->hideSection( InboxModel::DateSent );
   }
   if( _type == Sent )
   {
      ui->inbox_table->horizontalHeader()->swapSections( InboxModel::To, InboxModel::From );
      ui->inbox_table->horizontalHeader()->swapSections( InboxModel::DateReceived, InboxModel::DateSent );
      ui->inbox_table->horizontalHeader()->hideSection( InboxModel::DateReceived );
   }
   if( _type == Drafts )
   {
      ui->inbox_table->horizontalHeader()->swapSections( InboxModel::To, InboxModel::From );
      ui->inbox_table->horizontalHeader()->swapSections( InboxModel::DateReceived, InboxModel::DateSent );
      ui->inbox_table->horizontalHeader()->hideSection( InboxModel::DateReceived );
      ui->inbox_table->horizontalHeader()->hideSection( InboxModel::Status );
   }

   ui->inbox_table->horizontalHeader()->setSectionsMovable(true);
   ui->inbox_table->horizontalHeader()->setSortIndicatorShown(false);
   ui->inbox_table->horizontalHeader()->setSectionsClickable(true);
   ui->inbox_table->horizontalHeader()->setHighlightSections(true);
}



