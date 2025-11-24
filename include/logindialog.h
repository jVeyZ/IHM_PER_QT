#pragma once

#include <cstdio>

#include <QDialog>

#include <optional>

#include "usermanager.h"

class QLineEdit;
class QPushButton;
class QLabel;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(UserManager &userManager, QWidget *parent = nullptr);

    std::optional<UserRecord> loggedUser() const;

signals:
    void requestRegistration();

private slots:
    void handleLogin();
    void openRegistration();

private:
    void setupUi();
    void updateUiState(bool busy);

    UserManager &userManager_;
    QLineEdit *nicknameEdit_ = nullptr;
    QLineEdit *passwordEdit_ = nullptr;
    QPushButton *loginButton_ = nullptr;
    QPushButton *registerButton_ = nullptr;
    QLabel *feedbackLabel_ = nullptr;

    std::optional<UserRecord> loggedUser_;
};
