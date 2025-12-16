#include "logindialog.h"

#include "registerdialog.h"
#include "usermanager.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginDialog::LoginDialog(UserManager &userManager, QWidget *parent)
    : QDialog(parent), userManager_(userManager) {
    setWindowTitle(tr("Acceso a Proyecto PER"));
    setModal(true);
    setupUi();
}

std::optional<UserRecord> LoginDialog::loggedUser() const {
    return loggedUser_;
}

void LoginDialog::handleLogin() {
    QString error;
    const auto user = userManager_.authenticate(nicknameEdit_->text().trimmed(), passwordEdit_->text(), error);
    if (!user.has_value()) {
        feedbackLabel_->setText(error);
        feedbackLabel_->setVisible(true);
        return;
    }

    loggedUser_ = user;
    accept();
}

void LoginDialog::openRegistration() {
    RegisterDialog dialog(userManager_, this);
    if (dialog.exec() == QDialog::Accepted) {
        if (const auto created = dialog.createdUser()) {
            loggedUser_ = created;
            accept();
            return;
        }
    }
}

void LoginDialog::setupUi() {
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(tr("Bienvenido/a"));
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("loginTitle"));
    title->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: bold; color: #0b3d70;"));
    layout->addWidget(title);

    auto *form = new QFormLayout();
    nicknameEdit_ = new QLineEdit();
    nicknameEdit_->setPlaceholderText(tr("Usuario"));
    passwordEdit_ = new QLineEdit();
    passwordEdit_->setPlaceholderText(tr("Contraseña"));
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setClearButtonEnabled(false);
    
    togglePasswordAction_ = passwordEdit_->addAction(
        QIcon(QStringLiteral(":/resources/images/icon_eye_closed.svg")),
        QLineEdit::TrailingPosition);
    togglePasswordAction_->setCheckable(true);
    connect(togglePasswordAction_, &QAction::toggled, this, [this](bool checked) {
        passwordEdit_->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        togglePasswordAction_->setIcon(QIcon(checked
            ? QStringLiteral(":/resources/images/icon_eye_open.svg")
            : QStringLiteral(":/resources/images/icon_eye_closed.svg")));
    });

    auto *nicknameHint = new QLabel(tr("Usuario"));
    nicknameHint->setStyleSheet(QStringLiteral("color: #6b7280; font-size: 12px;"));
    form->addRow(nicknameHint);

    form->addRow(tr("Usuario"), nicknameEdit_);

    auto *passwordHint = new QLabel(tr("Contraseña"));
    passwordHint->setStyleSheet(QStringLiteral("color: #6b7280; font-size: 12px;"));
    form->addRow(passwordHint);

    form->addRow(tr("Contraseña"), passwordEdit_);

    // Accessible names for automated tests / assistive tech
    nicknameEdit_->setAccessibleName(tr("usuario"));
    passwordEdit_->setAccessibleName(tr("password"));

    layout->addLayout(form);

    feedbackLabel_ = new QLabel();
    feedbackLabel_->setStyleSheet(QStringLiteral("color: #b00020;"));
    feedbackLabel_->setWordWrap(true);
    feedbackLabel_->setVisible(false);
    layout->addWidget(feedbackLabel_);

    loginButton_ = new QPushButton(tr("Acceder"));
    loginButton_->setDefault(true);
    loginButton_->setEnabled(false);

    registerButton_ = new QPushButton(tr("Registrarse"));

    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(registerButton_);
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(loginButton_);
    layout->addLayout(buttonsLayout);

    connect(loginButton_, &QPushButton::clicked, this, &LoginDialog::handleLogin);
    connect(registerButton_, &QPushButton::clicked, this, &LoginDialog::openRegistration);
    connect(nicknameEdit_, &QLineEdit::textChanged, this, [this]() {
        feedbackLabel_->setVisible(false);
        updateUiState(false);
    });
    connect(passwordEdit_, &QLineEdit::textChanged, this, [this]() {
        feedbackLabel_->setVisible(false);
        updateUiState(false);
    });

    updateUiState(false);
}

void LoginDialog::updateUiState(bool busy) {
    const bool hasData = !nicknameEdit_->text().trimmed().isEmpty() && !passwordEdit_->text().isEmpty();
    loginButton_->setEnabled(!busy && hasData);
    registerButton_->setEnabled(!busy);
}
