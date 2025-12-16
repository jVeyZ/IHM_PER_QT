#include "profiledialog.h"

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
#include <QVBoxLayout>

ProfileDialog::ProfileDialog(UserManager &manager, UserRecord user, QWidget *parent, bool readOnly)
    : QDialog(parent), manager_(manager), user_(std::move(user)), readOnly_(readOnly) {
    setWindowTitle(readOnly_ ? tr("Ver Perfil") : tr("Editar perfil"));
    setModal(true);
    if (readOnly_) {
        setupReadOnlyUi();
    } else {
        setupUi();
    }
}

const UserRecord &ProfileDialog::updatedUser() const {
    return user_;
}

void ProfileDialog::selectAvatar() {
    const QString file = QFileDialog::getOpenFileName(this, tr("Seleccionar avatar"), QString(),
                                                      tr("Imágenes (*.png *.jpg *.jpeg *.bmp *.svg)"));
    if (!file.isEmpty()) {
        avatarPath_ = file;
        QPixmap pixmap(file);
        avatarPreview_->setPixmap(pixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void ProfileDialog::saveChanges() {
    QString error;
    if (!validate(error)) {
        feedbackLabel_->setText(error);
        feedbackLabel_->setVisible(true);
        return;
    }

    const QString newEmail = emailEdit_->text().trimmed();
    const QString newPassword = passwordEdit_->text();
    const QDate newBirthdate = birthdateEdit_->date();
    std::optional<QString> passwordToStore;

    if (!newPassword.isEmpty()) {
        passwordToStore = newPassword;
    }

    if (!manager_.updateUser(user_.nickname, newEmail, passwordToStore, newBirthdate, avatarPath_, error)) {
        feedbackLabel_->setText(error);
        feedbackLabel_->setVisible(true);
        return;
    }

    if (auto refreshed = manager_.getUser(user_.nickname)) {
        user_ = refreshed.value();
    }

    feedbackLabel_->setVisible(false);
    accept();
}

void ProfileDialog::setupUi() {
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(tr("Actualiza tu perfil"));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold; color: #0b3d70;"));
    layout->addWidget(title);

    auto *form = new QFormLayout();

    nicknameEdit_ = new QLineEdit(user_.nickname);
    nicknameEdit_->setEnabled(false);

    emailEdit_ = new QLineEdit(user_.email);

    passwordEdit_ = new QLineEdit();
    passwordEdit_->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit_ = new QLineEdit();
    confirmPasswordEdit_->setEchoMode(QLineEdit::Password);

    birthdateEdit_ = new QDateEdit(user_.birthdate);
    birthdateEdit_->setCalendarPopup(true);
    birthdateEdit_->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));

    avatarPreview_ = new QLabel();
    avatarPreview_->setFixedSize(96, 96);
    avatarPreview_->setStyleSheet(QStringLiteral("border: 1px solid #9cc6eb; border-radius: 6px;"));
    const QString avatarPath = manager_.resolvedAvatarPath(user_.avatarPath);
    avatarPreview_->setPixmap(QPixmap(avatarPath).scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto *avatarButton = new QPushButton(tr("Cambiar avatar"));
    connect(avatarButton, &QPushButton::clicked, this, &ProfileDialog::selectAvatar);

    // Password visibility toggles (match RegisterDialog)
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

    // Reorder form to match register: Avatar, Fecha, Usuario, Correo, Nueva contraseña, Confirmar contraseña
    auto *avatarLayout = new QHBoxLayout();
    avatarLayout->setContentsMargins(0, 0, 0, 0);
    avatarLayout->setSpacing(8);
    // center avatar & button
    avatarLayout->addStretch(1);
    avatarLayout->addWidget(avatarPreview_);
    avatarLayout->addWidget(avatarButton);
    avatarLayout->addStretch(1);

    auto *avatarContainer = new QWidget();
    avatarContainer->setLayout(avatarLayout);

    // Add avatar centered without a label to match register dialog
    form->addRow(avatarContainer);

    form->addRow(tr("Fecha de nacimiento"), birthdateEdit_);

    form->addRow(tr("Usuario"), nicknameEdit_);

    form->addRow(tr("Correo electrónico"), emailEdit_);

    form->addRow(tr("Nueva contraseña"), passwordEdit_);

    form->addRow(tr("Confirmar contraseña"), confirmPasswordEdit_);

    // Accessible names for automation / accessibility
    emailEdit_->setAccessibleName(tr("email"));
    passwordEdit_->setAccessibleName(tr("password"));
    confirmPasswordEdit_->setAccessibleName(tr("confirm_password"));
    birthdateEdit_->setAccessibleName(tr("birthdate"));
    layout->addLayout(form);

    feedbackLabel_ = new QLabel();
    feedbackLabel_->setStyleSheet(QStringLiteral("color: #b00020;"));
    feedbackLabel_->setWordWrap(true);
    feedbackLabel_->setVisible(false);
    layout->addWidget(feedbackLabel_);

    // add extra spacing before the save button so fields and button do not feel cramped
    layout->addSpacing(12);

    saveButton_ = new QPushButton(tr("Guardar cambios"));
    layout->addWidget(saveButton_);

    connect(saveButton_, &QPushButton::clicked, this, &ProfileDialog::saveChanges);

    const auto validateTrigger = [this]() {
        QString error;
        const bool ok = validate(error);
        feedbackLabel_->setVisible(!ok);
        feedbackLabel_->setText(error);
        saveButton_->setEnabled(ok);
    };

    connect(emailEdit_, &QLineEdit::textChanged, this, validateTrigger);
    connect(passwordEdit_, &QLineEdit::textChanged, this, validateTrigger);
    connect(confirmPasswordEdit_, &QLineEdit::textChanged, this, validateTrigger);
    connect(birthdateEdit_, &QDateEdit::dateChanged, this, [this](const QDate &) {
        QString error;
        const bool ok = validate(error);
        feedbackLabel_->setVisible(!ok);
        feedbackLabel_->setText(error);
        saveButton_->setEnabled(ok);
    });

    QString initialError;
    const bool ok = validate(initialError);
    feedbackLabel_->setVisible(!ok);
    feedbackLabel_->setText(initialError);
    saveButton_->setEnabled(ok);
}

void ProfileDialog::setupReadOnlyUi() {
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(tr("Tu Perfil"));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: bold; color: #0b3d70;"));
    layout->addWidget(title);

    // Avatar display
    avatarPreview_ = new QLabel();
    avatarPreview_->setFixedSize(96, 96);
    avatarPreview_->setStyleSheet(QStringLiteral("border: 1px solid #9cc6eb; border-radius: 6px;"));
    const QString avatarPath = manager_.resolvedAvatarPath(user_.avatarPath);
    avatarPreview_->setPixmap(QPixmap(avatarPath).scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    avatarPreview_->setAlignment(Qt::AlignCenter);

    auto *avatarContainer = new QWidget();
    auto *avatarLayout = new QHBoxLayout(avatarContainer);
    avatarLayout->setContentsMargins(0, 0, 0, 0);
    avatarLayout->addStretch(1);
    avatarLayout->addWidget(avatarPreview_);
    avatarLayout->addStretch(1);
    layout->addWidget(avatarContainer);

    layout->addSpacing(10);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);

    auto createReadOnlyLabel = [](const QString &text) {
        auto *label = new QLabel(text);
        label->setStyleSheet(QStringLiteral("padding: 6px; background: #f5f9fc; border: 1px solid #d0e3f0; border-radius: 4px;"));
        return label;
    };

    form->addRow(tr("Usuario:"), createReadOnlyLabel(user_.nickname));
    form->addRow(tr("Correo electrónico:"), createReadOnlyLabel(user_.email));
    form->addRow(tr("Fecha de nacimiento:"), createReadOnlyLabel(user_.birthdate.toString(QStringLiteral("dd/MM/yyyy"))));

    layout->addLayout(form);
    layout->addStretch(1);

    auto *closeButton = new QPushButton(tr("Cerrar"));
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeButton);
}

bool ProfileDialog::validate(QString &errorMessage) const {
    const QString email = emailEdit_->text().trimmed();
    const QString password = passwordEdit_->text();
    const QString confirmPassword = confirmPasswordEdit_->text();
    const QDate birthdate = birthdateEdit_->date();

    static const QRegularExpression emailRegex(QStringLiteral("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$"));
    if (!emailRegex.match(email).hasMatch()) {
        errorMessage = tr("Correo electrónico no válido.");
        return false;
    }

    if (!password.isEmpty()) {
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
    } else if (!confirmPassword.isEmpty()) {
        errorMessage = tr("Introduce una nueva contraseña para confirmar.");
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
