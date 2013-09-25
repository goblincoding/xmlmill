/* Copyright (c) 2012 - 2013 by William Hallatt.
 *
 * This file forms part of "XML Mill".
 *
 * The official website for this project is <http://www.goblincoding.com> and,
 * although not compulsory, it would be appreciated if all works of whatever
 * nature using this source code (in whole or in part) include a reference to
 * this site.
 *
 * Should you wish to contact me for whatever reason, please do so via:
 *
 *                 <http://www.goblincoding.com/contact>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (GNUGPL.txt).  If not, see
 *
 *                    <http://www.gnu.org/licenses/>
 */

#include "gcmessagespace.h"
#include "ui_gcmessagedialog.h"
#include "utils/gcglobalspace.h"

#include <QSettings>
#include <QDialog>
#include <QMessageBox>

/// Provides a user dialog prompt with the option to save the user's preference.
class GCMessageDialog : public QDialog
{
Q_OBJECT
public:

  /*! Constructor.
      @param remember - this flag should be passed in from the calling object and will be set
                        when the user checks the relevant box
      @param heading - the message box header
      @param text - the actual message text
      @param buttons - the buttons that should be displayed for this particular message
      @param defaultButton - the button that should be highlighted as the default
      @param icon - the icon associated with this particular message. */
  explicit GCMessageDialog( bool* remember,
                            const QString& heading,
                            const QString& text,
                            GCMessageSpace::ButtonCombo buttons,
                            GCMessageSpace::Buttons defaultButton,
                            GCMessageSpace::Icon icon = GCMessageSpace::NoIcon )
  : ui( new Ui::GCMessageDialog ),
    m_remember( remember )
  {
    ui->setupUi( this );
    ui->textLabel->setText( text );

    switch( buttons )
    {
      case GCMessageSpace::YesNo:
        ui->acceptButton->setText( "Yes" );
        ui->rejectButton->setText( "No" );
        break;
      case GCMessageSpace::OKCancel:
        ui->acceptButton->setText( "OK" );
        ui->rejectButton->setText( "Cancel" );
        break;
      case GCMessageSpace::OKOnly:
        ui->acceptButton->setText( "OK" );
        ui->rejectButton->setVisible( false );
    }

    switch( defaultButton )
    {
      case GCMessageSpace::Yes:
        /* Deliberate fall-through. */
      case GCMessageSpace::OK:
        ui->acceptButton->setDefault( true );
        break;
      case GCMessageSpace::No:
        /* Deliberate fall-through. */
      case GCMessageSpace::Cancel:
        ui->rejectButton->setDefault( true );
    }

    switch( icon )
    {
      case GCMessageSpace::Information:
        ui->iconLabel->setPixmap( style()->standardIcon( QStyle::SP_MessageBoxInformation ).pixmap( 32, 32 ) );
        break;
      case GCMessageSpace::Warning:
        ui->iconLabel->setPixmap( style()->standardIcon( QStyle::SP_MessageBoxWarning ).pixmap( 32, 32 ) );
        break;
      case GCMessageSpace::Critical:
        ui->iconLabel->setPixmap( style()->standardIcon( QStyle::SP_MessageBoxCritical ).pixmap( 32, 32 ) );
        break;
      case GCMessageSpace::Question:
        ui->iconLabel->setPixmap( style()->standardIcon( QStyle::SP_MessageBoxQuestion ).pixmap( 32, 32 ) );
        break;
      case GCMessageSpace::NoIcon:
      default:
        ui->iconLabel->setPixmap( QPixmap() );
    }

    connect( ui->checkBox, SIGNAL( toggled( bool ) ), this, SLOT( setRememberUserChoice( bool ) ) );
    connect( ui->acceptButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( ui->rejectButton, SIGNAL( clicked() ), this, SLOT( reject() ) );

    setWindowTitle( heading );
  }

  /*! Destructor. */
  ~GCMessageDialog()
  {
    /* The variable that "m_remember" points to is owned externally. */
    delete ui;
  }

private slots:
  /*! Triggered when the user checks or unchecks the "Don't ask me again" box. */
  void setRememberUserChoice( bool remember )
  {
    *m_remember = remember;
  }

private:
  Ui::GCMessageDialog* ui;
  bool* m_remember;
};

/* Standard trick for classes defined in .cpp files (resolves "Undefined Reference
  to vtable for xxx issue). The file that seems "missing" at the moment is generated
  by MOC when qMake is run. Must include after class definition. */
#include "gcmessagespace.moc"

/*--------------------------------------------------------------------------------------*/

namespace GCMessageSpace
{
  /* Hides our "member" variables. */
  namespace
  {
    QSettings settings( GCGlobalSpace::getOrganisationName(), GCGlobalSpace::getApplicationName() );
    bool settingsInitialised( false );
  }

  /*------------------------------------------------------------------------------------*/

  /* Uses QSettings to save the user preference to the registry (Windows) or
    relevant XML files (Mac) or ini (Unix). */
  bool userAccepted( const QString& uniqueMessageKey,
                     const QString& heading,
                     const QString& text,
                     ButtonCombo buttons,
                     Buttons defaultButton,
                     Icon icon,
                     bool saveCancel )
  {
    if( !settingsInitialised )
    {
      settings.setValue( "dialogPreferences", "" );
      settingsInitialised = true;
    }

    /* Check if the user previously requested that his/her choice must be saved. */
    QString key = QString( "dialogPreferences/%1" ).arg( uniqueMessageKey );
    QString valueKey = QString( key + "/preference" );

    bool remembered = settings.value( key, false ).toBool();

    if( !remembered )
    {
      GCMessageDialog dialog( &remembered, heading, text, buttons, defaultButton, icon );
      QDialog::DialogCode accept = static_cast< QDialog::DialogCode >( dialog.exec() );

      if( accept == QDialog::Accepted )
      {
        if( remembered )
        {
          settings.setValue( key, true );
          settings.setValue( valueKey, true );
        }

        return true;
      }
      else
      {
        /* For some message prompts, it isn't desirable to save "Cancel" values and
          it is therefore necessary to ensure that none of these values are remembered
          by accident. */
        if( remembered )
        {
          if( saveCancel )
          {
            settings.setValue( key, true );
            settings.setValue( valueKey, false );
          }
          else
          {
            settings.setValue( key, false );
          }
        }

        return false;
      }
    }
    else
    {
      /* If we do have a remembered setting, act accordingly. */
      return settings.value( valueKey ).toBool();
    }
  }

  /*------------------------------------------------------------------------------------*/

  void forgetAllPreferences()
  {
    settings.beginGroup( "dialogPreferences" );
    settings.remove( "" );
    settings.endGroup();
  }

  /*------------------------------------------------------------------------------------*/

  void showErrorMessageBox( QWidget* parent, const QString& message )
  {
    QMessageBox::critical( parent, "Error!", message );
  }

  /*------------------------------------------------------------------------------------*/
}