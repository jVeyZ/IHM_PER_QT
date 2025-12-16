#include "registerdialog.h"

#include "usermanager.h"

#include <QAction>
#include <QDate>
#include <QDateEdit>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

RegisterDialog::RegisterDialog(UserManager &userManager, QWidget *parent)
    : QDialog(parent), userManager_(userManager) {
    setWindowTitle(tr("Registrar nuevo usuario"));
    setModal(true);
    setupUi();
}

std::optional<UserRecord> RegisterDialog::createdUser() const {
    return createdUser_;
}

void RegisterDialog::selectAvatar() {
    const QString file = QFileDialog::getOpenFileName(this, tr("Seleccionar avatar"), QString(),
                                                      tr("Imágenes (*.png *.jpg *.jpeg *.bmp *.svg)"));
    if (!file.isEmpty()) {
        avatarPath_ = file;
        QPixmap pixmap(file);
        avatarPreview_->setPixmap(pixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void RegisterDialog::validateForm() {
    const bool allFilled = !nicknameEdit_->text().trimmed().isEmpty() &&
                           !emailEdit_->text().trimmed().isEmpty() &&
                           !passwordEdit_->text().isEmpty() &&
                           !confirmPasswordEdit_->text().isEmpty();

    if (!allFilled) {
        registerButton_->setEnabled(false);
        feedbackLabel_->setVisible(false);
        return;
    }

    QString error;
    const bool valid = validateInputs(error);
    registerButton_->setEnabled(valid);
    feedbackLabel_->setVisible(!error.isEmpty());
    feedbackLabel_->setText(error);
}

void RegisterDialog::registerUser() {
    QString error;
    if (!validateInputs(error)) {
        feedbackLabel_->setText(error);
        feedbackLabel_->setVisible(true);
        return;
    }

    const auto nickname = nicknameEdit_->text().trimmed();
    const auto email = emailEdit_->text().trimmed();
    const auto password = passwordEdit_->text();
    const auto birthdate = birthdateEdit_->date();

    if (!userManager_.registerUser(nickname, email, password, birthdate, avatarPath_, error)) {
        feedbackLabel_->setText(error);
        feedbackLabel_->setVisible(true);
        return;
    }

    createdUser_ = userManager_.getUser(nickname);
    accept();
}

void RegisterDialog::setupUi() {
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(tr("Completa tus datos"));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold; color: #0b3d70;"));
    layout->addWidget(title);

    auto *form = new QFormLayout();

    nicknameEdit_ = new QLineEdit();
    nicknameEdit_->setPlaceholderText(tr("Entre 6 y 15 caracteres"));
    emailEdit_ = new QLineEdit();
    passwordEdit_ = new QLineEdit();
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
    
    confirmPasswordEdit_ = new QLineEdit();
    confirmPasswordEdit_->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit_->setClearButtonEnabled(false);
    
    toggleConfirmPasswordAction_ = confirmPasswordEdit_->addAction(
        QIcon(QStringLiteral(":/resources/images/icon_eye_closed.svg")),
        QLineEdit::TrailingPosition);
    toggleConfirmPasswordAction_->setCheckable(true);
    connect(toggleConfirmPasswordAction_, &QAction::toggled, this, [this](bool checked) {
        confirmPasswordEdit_->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        toggleConfirmPasswordAction_->setIcon(QIcon(checked
            ? QStringLiteral(":/resources/images/icon_eye_open.svg")
            : QStringLiteral(":/resources/images/icon_eye_closed.svg")));
    });

    birthdateEdit_ = new QDateEdit(QDate::currentDate().addYears(-18));
    birthdateEdit_->setCalendarPopup(true);
    birthdateEdit_->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));

    avatarPreview_ = new QLabel();
    avatarPreview_->setFixedSize(96, 96);
    avatarPreview_->setStyleSheet(QStringLiteral("border: 1px solid #9cc6eb; border-radius: 6px;"));
    avatarPreview_->setPixmap(QPixmap(":/resources/images/default_avatar.svg").scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto *avatarButton = new QPushButton(tr("Seleccionar avatar"));
    connect(avatarButton, &QPushButton::clicked, this, &RegisterDialog::selectAvatar);

    auto *nickHint = new QLabel(tr("Usuario"));
    nickHint->setStyleSheet(QStringLiteral("color: #6b7280; font-size: 12px;"));
    form->addRow(nickHint);
    form->addRow(tr("Usuario"), nicknameEdit_);

    auto *emailHint = new QLabel(tr("Correo electrónico"));
    emailHint->setStyleSheet(QStringLiteral("color: #6b7280; font-size: 12px;"));
    form->addRow(emailHint);
    form->addRow(tr("Correo electrónico"), emailEdit_);

    auto *passwordHint = new QLabel(tr("Contraseña"));
    passwordHint->setStyleSheet(QStringLiteral("color: #6b7280; font-size: 12px;"));
    form->addRow(passwordHint);
    form->addRow(tr("Contraseña"), passwordEdit_);

    auto *confirmHint = new QLabel(tr("Confirmar contraseña"));
    confirmHint->setStyleSheet(QStringLiteral("color: #6b7280; font-size: 12px;"));
    form->addRow(confirmHint);
    form->addRow(tr("Confirmar contraseña"), confirmPasswordEdit_);

    form->addRow(tr("Fecha de nacimiento"), birthdateEdit_);

    // Accessible names for automation / accessibility
    nicknameEdit_->setAccessibleName(tr("usuario"));
    emailEdit_->setAccessibleName(tr("email"));
    passwordEdit_->setAccessibleName(tr("password"));
    confirmPasswordEdit_->setAccessibleName(tr("confirm_password"));
    birthdateEdit_->setAccessibleName(tr("birthdate"));

    auto *avatarLayout = new QHBoxLayout();
    avatarLayout->setContentsMargins(0, 0, 0, 0);
    avatarLayout->setSpacing(8);
    avatarLayout->addWidget(avatarPreview_);
    avatarLayout->addWidget(avatarButton);
    avatarLayout->addStretch(1);

    auto *avatarContainer = new QWidget();
    avatarContainer->setLayout(avatarLayout);
    form->addRow(tr("Avatar"), avatarContainer);

    layout->addLayout(form);

    feedbackLabel_ = new QLabel();
    feedbackLabel_->setStyleSheet(QStringLiteral("color: #b00020;"));
    feedbackLabel_->setWordWrap(true);
    feedbackLabel_->setVisible(false);
    layout->addWidget(feedbackLabel_);

    registerButton_ = new QPushButton(tr("Crear cuenta"));
    registerButton_->setEnabled(false);
    layout->addWidget(registerButton_);

    connect(registerButton_, &QPushButton::clicked, this, &RegisterDialog::registerUser);

    const auto triggerValidation = [this]() {
        validateForm();
    };

    connect(nicknameEdit_, &QLineEdit::textChanged, this, triggerValidation);
    connect(emailEdit_, &QLineEdit::textChanged, this, triggerValidation);
    connect(passwordEdit_, &QLineEdit::textChanged, this, triggerValidation);
    connect(confirmPasswordEdit_, &QLineEdit::textChanged, this, triggerValidation);
    connect(birthdateEdit_, &QDateEdit::dateChanged, this, [this](const QDate &) { validateForm(); });

    validateForm();
}

bool RegisterDialog::validateInputs(QString &errorMessage) const {
    const QString nickname = nicknameEdit_->text().trimmed();
    const QString email = emailEdit_->text().trimmed();
    const QString password = passwordEdit_->text();
    const QString confirmPassword = confirmPasswordEdit_->text();
    const QDate birthdate = birthdateEdit_->date();

    if (nickname.size() < 6 || nickname.size() > 15) {
        errorMessage = tr("El usuario debe tener entre 6 y 15 caracteres.");
        return false;
    }

    static const QRegularExpression nicknameRegex(QStringLiteral("^[A-Za-z0-9_-]+$"));
    if (!nicknameRegex.match(nickname).hasMatch()) {
        errorMessage = tr("El usuario solo puede contener letras, números, guiones y guiones bajos.");
        return false;
    }

    static const QRegularExpression emailRegex(QStringLiteral("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$"));
    if (!emailRegex.match(email).hasMatch()) {
        errorMessage = tr("Correo electrónico no válido.");
        return false;
    }

    if (password.size() < 8 || password.size() > 20) {
        errorMessage = tr("La contraseña debe tener entre 8 y 20 caracteres.");
        return false;
    }

    static const QRegularExpression upperRegex(QStringLiteral("[A-Z]"));
    static const QRegularExpression lowerRegex(QStringLiteral("[a-z]"));
    static const QRegularExpression digitRegex(QStringLiteral("[0-9]"));
    static const QRegularExpression specialRegex(QStringLiteral("[-!@#$%&*()+=]"));

    if (!upperRegex.match(password).hasMatch() || !lowerRegex.match(password).hasMatch() ||
        !digitRegex.match(password).hasMatch() || !specialRegex.match(password).hasMatch()) {
        errorMessage = tr("La contraseña debe incluir mayúsculas, minúsculas, dígitos y caracteres especiales.");
        return false;
    }

    if (password != confirmPassword) {
        errorMessage = tr("Las contraseñas no coinciden.");
        return false;
    }

    const int age = birthdate.daysTo(QDate::currentDate()) / 365;
    if (age < 16) {
        errorMessage = tr("Debes ser mayor de 16 años.");
        return false;
    }

    errorMessage.clear();
    return true;
}
