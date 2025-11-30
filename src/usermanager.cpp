#include "usermanager.h"

#include <QCoreApplication>
#include <exception>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStringList>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace {
constexpr auto kDefaultAvatarResource = ":/resources/images/default_avatar.svg";
constexpr auto kHistoryTableName = "question_history";

QString sessionKey(const QDateTime &timestamp) {
	return timestamp.isValid() ? timestamp.toString(Qt::ISODateWithMs) : QString();
}

QString attemptKey(const QDateTime &timestamp) {
	return timestamp.isValid() ? timestamp.toString(Qt::ISODateWithMs) : QString();
}
} // namespace

UserManager::UserManager(Navigation &navigation, QString avatarsDirectory)
	: navigation_(navigation), avatarsDirectory_(std::move(avatarsDirectory)) {
	if (!avatarsDirectory_.isEmpty()) {
		QDir().mkpath(avatarsDirectory_);
	}
	databasePath_ = resolveDatabasePath();
}

bool UserManager::load() {
	users_.clear();
	navigation_.reload();

	const auto &navUsers = navigation_.users();
	users_.reserve(navUsers.size());
	for (auto it = navUsers.constBegin(); it != navUsers.constEnd(); ++it) {
		users_.push_back(makeRecordFromNavUser(it.value()));
	}

	return true;
}

bool UserManager::registerUser(const QString &nickname,
							   const QString &email,
							   const QString &password,
							   const QDate &birthdate,
							   const QString &avatarSource,
							   QString &errorMessage) {
	if (navigation_.findUser(nickname)) {
		errorMessage = QObject::tr("El nombre de usuario ya está en uso.");
		return false;
	}

	UserRecord user;
	user.nickname = nickname;
	user.email = email;
	user.birthdate = birthdate;
	user.salt = generateSalt();
	user.passwordHash = hashPassword(password, user.salt);

	if (!avatarSource.isEmpty()) {
		QString avatarError;
		user.avatarPath = ensureAvatarStored(nickname, avatarSource, avatarError);
		if (user.avatarPath.isEmpty()) {
			errorMessage = avatarError;
			return false;
		}
	} else {
		user.avatarPath = QString::fromLatin1(kDefaultAvatarResource);
	}

	const QImage avatarImage = loadAvatarImage(resolvedAvatarPath(user.avatarPath));
	User navUser(user.nickname,
				 user.email,
				 encodePasswordPayload(user.salt, user.passwordHash),
				 avatarImage,
				 user.birthdate);

	try {
		navigation_.addUser(navUser);
	} catch (const std::exception &ex) {
		errorMessage = QObject::tr("No se pudo registrar al usuario: %1").arg(QString::fromUtf8(ex.what()));
		return false;
	}

	return load();
}

std::optional<UserRecord> UserManager::authenticate(const QString &nickname,
													const QString &password,
													QString &errorMessage) const {
	const int index = findIndex(nickname);
	if (index == -1) {
		errorMessage = QObject::tr("Usuario o contraseña incorrectos.");
		return std::nullopt;
	}

	const auto &user = users_.at(index);
	const auto hash = hashPassword(password, user.salt);
	if (!hash.isEmpty() && hash == user.passwordHash) {
		return user;
	}

	errorMessage = QObject::tr("Usuario o contraseña incorrectos.");
	return std::nullopt;
}

bool UserManager::updateUser(const QString &nickname,
							 const QString &email,
							 const std::optional<QString> &newPassword,
							 const QDate &birthdate,
							 const QString &avatarSource,
							 QString &errorMessage) {
	User *navUser = navigation_.findUser(nickname);
	if (!navUser) {
		errorMessage = QObject::tr("El usuario no existe.");
		return false;
	}

	navUser->setEmail(email);
	navUser->setBirthdate(birthdate);

	if (newPassword.has_value()) {
		const QString salt = generateSalt();
		const QString hash = hashPassword(newPassword.value(), salt);
		navUser->setPassword(encodePasswordPayload(salt, hash));
	}

	if (!avatarSource.isEmpty()) {
		QString avatarError;
		const auto storedPath = ensureAvatarStored(nickname, avatarSource, avatarError);
		if (storedPath.isEmpty()) {
			errorMessage = avatarError;
			return false;
		}
		const QImage avatarImage = loadAvatarImage(resolvedAvatarPath(storedPath));
		navUser->setAvatar(avatarImage);
	}

	try {
		navigation_.updateUser(*navUser);
	} catch (const std::exception &ex) {
		errorMessage = QObject::tr("No se pudieron guardar los cambios del perfil: %1").arg(QString::fromUtf8(ex.what()));
		return false;
	}

	return load();
}

bool UserManager::appendSession(const QString &nickname,
								const SessionRecord &session,
								QString &errorMessage) {
	if (!navigation_.findUser(nickname)) {
		errorMessage = QObject::tr("El usuario no existe.");
		return false;
	}

	Session navSession(session.timestamp, session.hits, session.faults);

	try {
		navigation_.addSession(nickname, navSession);
	} catch (const std::exception &ex) {
		errorMessage = QObject::tr("No se pudo guardar la sesión: %1").arg(QString::fromUtf8(ex.what()));
		return false;
	}

	if (!storeSessionAttempts(nickname, session, errorMessage)) {
		return false;
	}

	return load();
}

std::optional<UserRecord> UserManager::getUser(const QString &nickname) const {
	const int index = findIndex(nickname);
	if (index == -1) {
		return std::nullopt;
	}
	return users_.at(index);
}

QVector<UserRecord> UserManager::allUsers() const {
	return users_;
}

QString UserManager::resolvedAvatarPath(const QString &storedPath) const {
	if (storedPath.startsWith(QLatin1String(":/"))) {
		return storedPath;
	}
	return avatarsDirectory_.isEmpty() ? storedPath : avatarsDirectory_ + QLatin1Char('/') + storedPath;
}

QString UserManager::hashPassword(const QString &password, const QString &salt) const {
	const auto salted = (salt + password).toUtf8();
	return QString::fromLatin1(QCryptographicHash::hash(salted, QCryptographicHash::Sha256).toHex());
}

QString UserManager::generateSalt() const {
	return QString::number(QRandomGenerator::global()->generate64(), 16);
}

QString UserManager::ensureAvatarStored(const QString &nickname,
										const QString &sourcePath,
										QString &errorMessage) const {
	if (sourcePath.startsWith(QLatin1String(":/")) || avatarsDirectory_.isEmpty()) {
		return sourcePath;
	}

	QFile sourceFile(sourcePath);
	if (!sourceFile.exists()) {
		errorMessage = QObject::tr("No se encuentra la imagen seleccionada.");
		return {};
	}

	const QString extension = QFileInfo(sourceFile).suffix();
	const QString targetName = nickname + QLatin1String("_") + QString::number(QDateTime::currentSecsSinceEpoch()) + QLatin1String(".") + extension;
	const QString targetPath = avatarsDirectory_ + QLatin1Char('/') + targetName;

	if (!sourceFile.copy(targetPath)) {
		errorMessage = QObject::tr("No se pudo copiar la imagen seleccionada.");
		return {};
	}

	QFile::setPermissions(targetPath, QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther | QFile::WriteUser);
	return targetName;
}

int UserManager::findIndex(const QString &nickname) const {
	for (int i = 0; i < users_.size(); ++i) {
		if (users_.at(i).nickname.compare(nickname, Qt::CaseInsensitive) == 0) {
			return i;
		}
	}
	return -1;
}

QString UserManager::encodePasswordPayload(const QString &salt, const QString &hash) const {
	return salt.isEmpty() ? hash : salt + QLatin1Char(':') + hash;
}

void UserManager::decodePasswordPayload(const QString &payload, QString &saltOut, QString &hashOut) const {
	const int separator = payload.indexOf(QLatin1Char(':'));
	if (separator < 0) {
		saltOut.clear();
		hashOut = payload;
		return;
	}
	saltOut = payload.left(separator);
	hashOut = payload.mid(separator + 1);
}

UserRecord UserManager::makeRecordFromNavUser(const User &navUser) const {
	UserRecord record;
	record.nickname = navUser.nickName();
	record.email = navUser.email();
	decodePasswordPayload(navUser.password(), record.salt, record.passwordHash);
	record.birthdate = navUser.birthdate();
	record.avatarPath = persistAvatarImage(record.nickname, navUser.avatar());

	QVector<Session> sessions = navUser.sessions();
	if (sessions.isEmpty()) {
		sessions = navigation_.dao().loadSessionsFor(navUser.nickName());
	}

	record.sessions.reserve(sessions.size());
	for (const auto &navSession : sessions) {
		record.sessions.push_back(makeRecordFromNavSession(record.nickname, navSession));
	}

	return record;
}

SessionRecord UserManager::makeRecordFromNavSession(const QString &nickname, const Session &navSession) const {
	SessionRecord session;
	session.timestamp = navSession.timeStamp();
	session.hits = navSession.hits();
	session.faults = navSession.faults();
	session.attempts = loadSessionAttempts(nickname, session.timestamp);
	return session;
}

QString UserManager::persistAvatarImage(const QString &nickname, const QImage &image) const {
	if (image.isNull() || avatarsDirectory_.isEmpty()) {
		return QString::fromLatin1(kDefaultAvatarResource);
	}

	const QString fileName = nickname + QLatin1String("_navdb.png");
	const QString absolutePath = avatarsDirectory_ + QLatin1Char('/') + fileName;
	if (image.save(absolutePath, "PNG")) {
		QFile::setPermissions(absolutePath, QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther | QFile::WriteUser);
		return fileName;
	}

	return QString::fromLatin1(kDefaultAvatarResource);
}

QImage UserManager::loadAvatarImage(const QString &path) const {
	QString effectivePath = path;
	if (!path.startsWith(QLatin1String(":/")) && QFileInfo::exists(resolvedAvatarPath(path))) {
		effectivePath = resolvedAvatarPath(path);
	}

	QImage image(effectivePath);
	if (!image.isNull()) {
		return image;
	}

	image.load(QString::fromLatin1(kDefaultAvatarResource));
	return image;
}

QString UserManager::resolveDatabasePath() const {
	QDir appDir(QCoreApplication::applicationDirPath());
	const QString localPath = appDir.absoluteFilePath(QStringLiteral("navdb.sqlite"));
	if (QFileInfo::exists(localPath)) {
		return localPath;
	}

	QStringList probes{
		QStringLiteral("navdb/navdb.sqlite"),
		QStringLiteral("../navdb/navdb.sqlite"),
		QStringLiteral("../Resources/navdb.sqlite")
	};

	for (const auto &probe : probes) {
		const QString candidate = appDir.absoluteFilePath(probe);
		if (QFileInfo::exists(candidate)) {
			return candidate;
		}
	}

	QDir walker(appDir);
	for (int depth = 0; depth < 5 && walker.cdUp(); ++depth) {
		const QString candidate = walker.absoluteFilePath(QStringLiteral("navdb/navdb.sqlite"));
		if (QFileInfo::exists(candidate)) {
			return candidate;
		}
	}

	return localPath;
}

bool UserManager::ensureHistoryStorage(QString &errorMessage) const {
	if (historyStorageReady_) {
		return true;
	}

	if (databasePath_.isEmpty()) {
		errorMessage = QObject::tr("No se encontró la base de datos de navdb.");
		return false;
	}

	bool ready = true;
	QString localError;
	const QString connection = historyConnectionName();
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
		db.setDatabaseName(databasePath_);
		if (!db.open()) {
			ready = false;
			localError = db.lastError().text();
		} else {
			QSqlQuery query(db);
			const QString createSql = QStringLiteral(
				"CREATE TABLE IF NOT EXISTS %1 ("
				"userNickName TEXT NOT NULL,"
				"sessionTimestamp TEXT NOT NULL,"
				"attemptTimestamp TEXT,"
				"problemId INTEGER,"
				"question TEXT,"
				"selectedAnswer TEXT,"
				"correctAnswer TEXT,"
				"wasCorrect INTEGER NOT NULL,"
				"optionsJson TEXT,"
				"selectedIndex INTEGER,"
				"PRIMARY KEY(userNickName, sessionTimestamp, attemptTimestamp, question))"
			).arg(QString::fromLatin1(kHistoryTableName));

			if (!query.exec(createSql)) {
				ready = false;
				localError = query.lastError().text();
			}
		}
		db.close();
	}
	QSqlDatabase::removeDatabase(connection);

	if (!ready) {
		errorMessage = localError;
		return false;
	}

	historyStorageReady_ = true;
	return true;
}

QVector<QuestionAttempt> UserManager::loadSessionAttempts(const QString &nickname, const QDateTime &sessionTimestamp) const {
	QVector<QuestionAttempt> attempts;
	QString error;
	if (!ensureHistoryStorage(error)) {
		return attempts;
	}

	const QString connection = historyConnectionName();
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
		db.setDatabaseName(databasePath_);
		if (db.open()) {
			QSqlQuery query(db);
			query.prepare(QStringLiteral(
				"SELECT attemptTimestamp, problemId, question, selectedAnswer, correctAnswer, wasCorrect, optionsJson, selectedIndex "
				"FROM %1 WHERE userNickName = ? AND sessionTimestamp = ? ORDER BY attemptTimestamp" ).arg(QString::fromLatin1(kHistoryTableName)));
			query.addBindValue(nickname);
			query.addBindValue(sessionKey(sessionTimestamp));

			if (query.exec()) {
				while (query.next()) {
					QuestionAttempt attempt;
					attempt.timestamp = QDateTime::fromString(query.value(0).toString(), Qt::ISODateWithMs);
					attempt.problemId = query.value(1).toInt();
					attempt.question = query.value(2).toString();
					attempt.selectedAnswer = query.value(3).toString();
					attempt.correctAnswer = query.value(4).toString();
					attempt.correct = query.value(5).toInt() == 1;
					attempt.selectedIndex = query.value(7).isNull() ? -1 : query.value(7).toInt();

					const auto optionsDoc = QJsonDocument::fromJson(query.value(6).toByteArray());
					if (optionsDoc.isArray()) {
						const auto optionsArray = optionsDoc.array();
						for (const auto &optionValue : optionsArray) {
							if (!optionValue.isObject()) {
								continue;
							}
							const auto optionObj = optionValue.toObject();
							AttemptOption option;
							option.text = optionObj.value("text").toString();
							option.correct = optionObj.value("correct").toBool();
							attempt.options.push_back(option);
						}
					}

					if (attempt.options.isEmpty()) {
						if (!attempt.selectedAnswer.isEmpty()) {
							AttemptOption option;
							option.text = attempt.selectedAnswer;
							option.correct = attempt.correct;
							attempt.options.push_back(option);
						}
						if (!attempt.correctAnswer.isEmpty() && attempt.correctAnswer != attempt.selectedAnswer) {
							AttemptOption option;
							option.text = attempt.correctAnswer;
							option.correct = true;
							attempt.options.push_back(option);
						}
					}

					attempts.push_back(std::move(attempt));
				}
			}
		}
		db.close();
	}
	QSqlDatabase::removeDatabase(connection);

	return attempts;
}

bool UserManager::storeSessionAttempts(const QString &nickname,
									   const SessionRecord &session,
									   QString &errorMessage) const {
	if (session.attempts.isEmpty()) {
		return true;
	}

	if (!ensureHistoryStorage(errorMessage)) {
		return false;
	}

	const QString connection = historyConnectionName();
	bool success = true;
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
		db.setDatabaseName(databasePath_);
		if (!db.open()) {
			errorMessage = db.lastError().text();
			success = false;
		} else {
			db.transaction();
			QSqlQuery deleteQuery(db);
			deleteQuery.prepare(QStringLiteral("DELETE FROM %1 WHERE userNickName = ? AND sessionTimestamp = ?")
									.arg(QString::fromLatin1(kHistoryTableName)));
			deleteQuery.addBindValue(nickname);
			deleteQuery.addBindValue(sessionKey(session.timestamp));
			if (!deleteQuery.exec()) {
				errorMessage = deleteQuery.lastError().text();
				db.rollback();
				success = false;
			} else {
				QSqlQuery insertQuery(db);
				insertQuery.prepare(QStringLiteral(
					"INSERT OR REPLACE INTO %1 "
					"(userNickName, sessionTimestamp, attemptTimestamp, problemId, question, selectedAnswer, correctAnswer, wasCorrect, optionsJson, selectedIndex) "
					"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
				).arg(QString::fromLatin1(kHistoryTableName)));

				for (const auto &attempt : session.attempts) {
					QJsonArray optionsArray;
					for (const auto &option : attempt.options) {
						optionsArray.push_back(QJsonObject{{"text", option.text}, {"correct", option.correct}});
					}

					insertQuery.bindValue(0, nickname);
					insertQuery.bindValue(1, sessionKey(session.timestamp));
					insertQuery.bindValue(2, attemptKey(attempt.timestamp));
					insertQuery.bindValue(3, attempt.problemId);
					insertQuery.bindValue(4, attempt.question);
					insertQuery.bindValue(5, attempt.selectedAnswer);
					insertQuery.bindValue(6, attempt.correctAnswer);
					insertQuery.bindValue(7, attempt.correct ? 1 : 0);
					insertQuery.bindValue(8, QJsonDocument(optionsArray).toJson(QJsonDocument::Compact));
					insertQuery.bindValue(9, attempt.selectedIndex);

					if (!insertQuery.exec()) {
						errorMessage = insertQuery.lastError().text();
						success = false;
						break;
					}
				}

				if (success) {
					db.commit();
				} else {
					db.rollback();
				}
			}
		}
		db.close();
	}
	QSqlDatabase::removeDatabase(connection);

	return success;
}

QString UserManager::historyConnectionName() const {
	return QStringLiteral("per_history_%1_%2")
		.arg(reinterpret_cast<quintptr>(this))
		.arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

