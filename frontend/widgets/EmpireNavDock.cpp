#include "EmpireNavDock.hpp"

#include <obs-frontend-api.h>
#include <util/config-file.h>

#include <cstring>

#include <QDesktopServices>
#include <QDockWidget>
#include <QMetaObject>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "moc_EmpireNavDock.cpp"

static QWidget *empire_main_window()
{
	return static_cast<QWidget *>(obs_frontend_get_main_window());
}

EmpireNavDock::EmpireNavDock(QWidget *parent) : QFrame(parent)
{
	setStyleSheet("EmpireNavDock { background:#0F0F0F; border-left:1px solid #1E1E1E; }");

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 14, 0, 14);
	layout->setSpacing(4);

	QPushButton *streams = MakeNavButton(QStringLiteral("STREAMY"));
	connect(streams, &QPushButton::clicked, this, [this]() { ToggleDock("empire_multistream_dock"); });
	layout->addWidget(streams);

	QPushButton *recordings = MakeNavButton(QStringLiteral("NAGRANIA"));
	connect(recordings, &QPushButton::clicked, this, []() {
		config_t *cfg = obs_frontend_get_profile_config();
		if (!cfg)
			return;
		const char *mode = config_get_string(cfg, "Output", "Mode");
		const char *dir = (mode && strcmp(mode, "Advanced") == 0)
					  ? config_get_string(cfg, "AdvOut", "RecFilePath")
					  : config_get_string(cfg, "SimpleOutput", "FilePath");
		if (dir && *dir)
			QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromUtf8(dir)));
	});
	layout->addWidget(recordings);

	QPushButton *scenes = MakeNavButton(QStringLiteral("SCENY"));
	connect(scenes, &QPushButton::clicked, this, [this]() { ToggleDock("empire_scenes_dock"); });
	layout->addWidget(scenes);

	QPushButton *settings = MakeNavButton(QStringLiteral("USTAWIENIA"));
	connect(settings, &QPushButton::clicked, this, []() {
		QWidget *mw = empire_main_window();
		if (mw)
			QMetaObject::invokeMethod(mw, "on_action_Settings_triggered", Qt::QueuedConnection);
	});
	layout->addWidget(settings);

	layout->addStretch();

	setObjectName(QStringLiteral("empireNavDock"));
}

QPushButton *EmpireNavDock::MakeNavButton(const QString &label)
{
	QPushButton *b = new QPushButton(label, this);
	b->setMinimumHeight(48);
	b->setCursor(Qt::PointingHandCursor);
	b->setStyleSheet("QPushButton { background:transparent; border:none; border-left:3px solid transparent;"
			 " color:#AFAFAF; font-weight:700; font-size:12px; text-align:left; padding-left:16px; }"
			 "QPushButton:hover { background:#161616; color:#FFFFFF; border-left:3px solid #E50914; }"
			 "QPushButton:pressed { background:#1C1C1C; }");
	return b;
}

void EmpireNavDock::ToggleDock(const char *id)
{
	QWidget *mw = empire_main_window();
	QDockWidget *d = mw ? mw->findChild<QDockWidget *>(QString::fromUtf8(id)) : nullptr;
	if (!d)
		return;
	const bool show = !d->isVisible();
	d->setVisible(show);
	if (show) {
		d->setFloating(false);
		d->raise();
	}
}
