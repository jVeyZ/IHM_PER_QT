#pragma once

#include <cstdio>

#include <QDialog>

#include <optional>

#include "usermanager.h"

class QLineEdit;
class QDateEdit;
class QLabel;
class QPushButton;
class RegisterDialog : public QDialog {
    Q_OBJECT
public:
    explicit RegisterDialog(UserManager &userManager, QWidget *parent = nullptr);

    std::optional<UserRecord> createdUser() const;

private slots:
    void selectAvatar();
    void validateForm();
    void registerUser();

private:
    void setupUi();
    bool validateInputs(QString &errorMessage) const;

    UserManager &userManager_;
    QLineEdit *nicknameEdit_ = nullptr;
    QLineEdit *emailEdit_ = nullptr;
    QLineEdit *passwordEdit_ = nullptr;
    QLineEdit *confirmPasswordEdit_ = nullptr;
    QDateEdit *birthdateEdit_ = nullptr;
    QLabel *avatarPreview_ = nullptr;
    QLabel *feedbackLabel_ = nullptr;
    QPushButton *registerButton_ = nullptr;

    QString avatarPath_;
    std::optional<UserRecord> createdUser_;
};
