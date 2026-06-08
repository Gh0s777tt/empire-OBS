#include "EmpireCommandDock.hpp"

#include <obs-frontend-api.h>

#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTime>

#include "moc_EmpireCommandDock.cpp"

#define EMPIRE_CMD_INTERVAL 1000

static QString empire_fmt_time(uint64_t secs)
{
	const uint64_t h = secs / 3600;
	const uint64_t m = (secs % 3600) / 60;
	const uint64_t s = secs % 60;
	return QString("%1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}

static QDockWidget *empire_find_vertical_dock()
{
	QWidget *mw = static_cast<QWidget *>(obs_frontend_get_main_window());
	return mw ? mw->findChild<QDockWidget *>(QStringLiteral("empire_vertical_dock")) : nullptr;
}

EmpireCommandDock::EmpireCommandDock(QWidget *parent) : QFrame(parent), cpu_info(os_cpu_usage_info_start()), timer(this)
{
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setContentsMargins(14, 8, 14, 8);
	layout->setSpacing(18);

	brandLabel = new QLabel(this);
	brandLabel->setTextFormat(Qt::RichText);
	brandLabel->setText(QStringLiteral(
		"<span style='font-size:15px; font-weight:800; color:#FFFFFF;'>Empire<span style='color:#E50914;'>OBS</span></span>"));
	layout->addWidget(brandLabel);

	/* Quick Vertical 9:16 toggle — always on the bar so the 9:16 ↔ normal switch
	 * is one click away. Shows/hides the Empire Vertical dock; red when active. */
	verticalBtn = new QPushButton(QStringLiteral("9:16"), this);
	verticalBtn->setCheckable(true);
	verticalBtn->setMinimumHeight(32);
	verticalBtn->setCursor(Qt::PointingHandCursor);
	verticalBtn->setToolTip(QStringLiteral("Przełącz podgląd Vertical 9:16"));
	verticalBtn->setStyleSheet(
		"QPushButton { background:#232323; color:#B3B3B3; font-weight:700; border-radius:9px; padding:0 14px; }"
		"QPushButton:hover { background:#2C2C2C; color:#FFFFFF; }"
		"QPushButton:checked { background:#E50914; color:#FFFFFF; }");
	connect(verticalBtn, &QPushButton::clicked, this, []() {
		QDockWidget *vdock = empire_find_vertical_dock();
		if (!vdock)
			return;
		const bool show = !vdock->isVisible();
		vdock->setVisible(show);
		if (show) {
			vdock->setFloating(false);
			vdock->raise();
		}
	});
	layout->addWidget(verticalBtn);

	infoLabel = new QLabel(this);
	infoLabel->setTextFormat(Qt::RichText);
	infoLabel->setStyleSheet("color:#888888;");
	layout->addWidget(infoLabel);

	layout->addStretch();

	statsLabel = new QLabel(this);
	statsLabel->setStyleSheet("color:#B3B3B3;");
	layout->addWidget(statsLabel);

	onAirLabel = new QLabel(this);
	onAirLabel->setStyleSheet("color:#808080; font-weight:bold;");
	layout->addWidget(onAirLabel);

	clockLabel = new QLabel(this);
	clockLabel->setStyleSheet("color:#FFFFFF; font-weight:bold; font-size:14px;");
	layout->addWidget(clockLabel);

	goLiveBtn = new QPushButton(this);
	goLiveBtn->setMinimumHeight(32);
	goLiveBtn->setMinimumWidth(92);
	connect(goLiveBtn, &QPushButton::clicked, this, []() {
		if (obs_frontend_streaming_active())
			obs_frontend_streaming_stop();
		else
			obs_frontend_streaming_start();
	});
	layout->addWidget(goLiveBtn);

	recordBtn = new QPushButton(this);
	recordBtn->setMinimumHeight(32);
	connect(recordBtn, &QPushButton::clicked, this, []() {
		if (obs_frontend_recording_active())
			obs_frontend_recording_stop();
		else
			obs_frontend_recording_start();
	});
	layout->addWidget(recordBtn);

	studioBtn = new QPushButton(this);
	studioBtn->setMinimumHeight(32);
	connect(studioBtn, &QPushButton::clicked, this,
		[]() { obs_frontend_set_preview_program_mode(!obs_frontend_preview_program_mode_active()); });
	layout->addWidget(studioBtn);

	connect(&timer, &QTimer::timeout, this, &EmpireCommandDock::Update);
	timer.setInterval(EMPIRE_CMD_INTERVAL);

	UpdateInfo();
	UpdateButtons();
	Update();
	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	setObjectName(QStringLiteral("empireCommandDock"));
}

EmpireCommandDock::~EmpireCommandDock()
{
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);
	os_cpu_usage_info_destroy(cpu_info);
}

void EmpireCommandDock::UpdateInfo()
{
	char *profile = obs_frontend_get_current_profile();
	char *collection = obs_frontend_get_current_scene_collection();
	infoLabel->setText(QString("Profil: <b style='color:#DDDDDD;'>%1</b>&nbsp;&nbsp;·&nbsp;&nbsp;"
				   "Scene Collection: <b style='color:#DDDDDD;'>%2</b>")
				   .arg(profile ? QString::fromUtf8(profile) : QString())
				   .arg(collection ? QString::fromUtf8(collection) : QString()));
	bfree(profile);
	bfree(collection);
}

void EmpireCommandDock::UpdateButtons()
{
	const bool streaming = obs_frontend_streaming_active();
	const bool recording = obs_frontend_recording_active();
	const bool studio = obs_frontend_preview_program_mode_active();

	goLiveBtn->setText(streaming ? QStringLiteral("Stop") : QStringLiteral("● Go Live"));
	goLiveBtn->setStyleSheet(
		streaming ? "background:#B20710; color:white; font-weight:600; border-radius:9px; padding:0 16px;"
			  : "background:#E50914; color:white; font-weight:600; border-radius:9px; padding:0 16px;");

	recordBtn->setText(recording ? QStringLiteral("Stop Rec") : QStringLiteral("Record"));
	recordBtn->setStyleSheet(
		recording ? "background:#B20710; color:white; font-weight:600; border-radius:9px; padding:0 14px;"
			  : "background:#232323; color:white; font-weight:600; border-radius:9px; padding:0 14px;");

	studioBtn->setText(studio ? QStringLiteral("Studio ✓") : QStringLiteral("Studio"));
	studioBtn->setStyleSheet(
		studio ? "background:#E50914; color:white; font-weight:600; border-radius:9px; padding:0 14px;"
		       : "background:#232323; color:white; font-weight:600; border-radius:9px; padding:0 14px;");
}

void EmpireCommandDock::Update()
{
	clockLabel->setText(QTime::currentTime().toString(QStringLiteral("HH:mm")));

	const double cpu = os_cpu_usage_info_query(cpu_info);
	const double fps = obs_get_active_fps();
	statsLabel->setTextFormat(Qt::RichText);
	statsLabel->setText(
		QString("CPU <b style='color:#FFFFFF;'>%1%</b>&nbsp;&nbsp;·&nbsp;&nbsp;FPS <b style='color:#FFFFFF;'>%2</b>")
			.arg(QString::number(cpu, 'f', 1))
			.arg(QString::number(fps, 'f', 0)));

	const bool streaming = obs_frontend_streaming_active();
	const bool recording = obs_frontend_recording_active();
	const uint64_t now = os_gettime_ns();

	if (streaming) {
		const uint64_t secs = streamStart ? (now - streamStart) / 1000000000ULL : 0;
		onAirLabel->setText(QString("● ON AIR  %1").arg(empire_fmt_time(secs)));
		onAirLabel->setStyleSheet("color:#E50914; font-weight:bold;");
	} else if (recording) {
		const uint64_t secs = recordStart ? (now - recordStart) / 1000000000ULL : 0;
		onAirLabel->setText(QString("● REC  %1").arg(empire_fmt_time(secs)));
		onAirLabel->setStyleSheet("color:#E50914; font-weight:bold;");
	} else {
		onAirLabel->setText(QStringLiteral("○ Offline"));
		onAirLabel->setStyleSheet("color:#808080; font-weight:bold;");
	}

	if (verticalBtn) {
		QDockWidget *vdock = empire_find_vertical_dock();
		verticalBtn->setChecked(vdock && vdock->isVisible());
	}
}

void EmpireCommandDock::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	EmpireCommandDock *dock = static_cast<EmpireCommandDock *>(ptr);

	switch (event) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		dock->streamStart = os_gettime_ns();
		dock->UpdateButtons();
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		dock->recordStart = os_gettime_ns();
		dock->UpdateButtons();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
	case OBS_FRONTEND_EVENT_STREAMING_STARTING:
	case OBS_FRONTEND_EVENT_RECORDING_STARTING:
	case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
	case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
		dock->UpdateButtons();
		break;
	case OBS_FRONTEND_EVENT_PROFILE_CHANGED:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		dock->UpdateInfo();
		break;
	default:
		break;
	}
}

void EmpireCommandDock::showEvent(QShowEvent *)
{
	timer.start();
	Update();
}

void EmpireCommandDock::hideEvent(QHideEvent *)
{
	timer.stop();
}
