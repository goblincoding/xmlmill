#ifndef GCMESSAGEDIALOG_H
#define GCMESSAGEDIALOG_H

#include <QDialog>

namespace Ui {
  class GCMessageDialog;
}

class GCMessageDialog : public QDialog
{
  Q_OBJECT
  
public:

  enum Icon
  {
    NoIcon,
    Information,
    Warning,
    Critical,
    Question
  };

  enum ButtonCombo
  {
    OKOnly,
    YesNo,
    OKCancel
  };

  enum Buttons
  {
    Yes,
    No,
    OK,
    Cancel
  };

  explicit GCMessageDialog( const QString &heading,
                            const QString &text,
                            ButtonCombo    buttons,
                            Buttons        defaultButton,
                            Icon           icon = NoIcon,
                            QWidget       *parent = 0 );
  ~GCMessageDialog();

signals:
  void rememberUserChoice( bool remember );

private slots:
  void setRememberUserChoice( bool remember );
  
private:
  Ui::GCMessageDialog *ui;
  bool m_rememberUserChoice;
};

#endif // GCMESSAGEDIALOG_H
