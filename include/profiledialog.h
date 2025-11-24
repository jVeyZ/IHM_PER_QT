#pragma once

#include <cstdio>

#include <QDialog>

#include "usermanager.h"

class QLineEdit;
class QDateEdit;
class QLabel;
class QPushButton;
class ProfileDialog : public QDialog {
    Q_OBJECT
public:
    ProfileDialog(UserManager &manager, UserRecord user, QWidget *parent = nullptr);

    const UserRecord &updatedUser() const;

private slots:
    void selectAvatar();
    void saveChanges();

private:
    void setupUi();
    bool validate(QString &errorMessage) const;

    UserManager &manager_;
    UserRecord user_;

    QLineEdit *nicknameEdit_ = nullptr;
    QLineEdit *emailEdit_ = nullptr;
    QLineEdit *passwordEdit_ = nullptr;
    QLineEdit *confirmPasswordEdit_ = nullptr;
    QDateEdit *birthdateEdit_ = nullptr;
    QLabel *avatarPreview_ = nullptr;
    QLabel *feedbackLabel_ = nullptr;
    QPushButton *saveButton_ = nullptr;

    QString avatarPath_;
};
