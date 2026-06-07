#include "EmpireCommandDock.hpp"

#include <obs-frontend-api.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "moc_EmpireCommandDock.cpp"

#define EMPIRE_CMD_INTERVAL 1000

static QString empire_fmt_time(uint64_t secs)
{
	const uint64_t h = secs / 3600;
	const uint64_t m = (secs % 3600) / 60;
	const uint64_t s = secs % 60;
	return QString("%1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
}

EmpireCommandDock::EmpireCommandDock(QWidget *parent) : QFrame(parent), cpu_info(os_cpu_usage_info_start()), timer(this)
{
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setContentsMargins(12, 8, 12, 8);
	layout->setSpacing(16);

	statusLabel = new QLabel(this);
	statusLabel->setTextFormat(Qt::RichText);
	statusLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
	layout->addWidget(statusLabel);

	statsLabel = new QLabel(this);
	statsLabel->setStyleSheet("color: #B3B3B3;");
	layout->addWidget(statsLabel);

	layout->addStretch();

	goLiveBtn = new QPushButton(this);
	goLiveBtn->setMinimumHeight(34);
	goLiveBtn->setMinimumWidth(96);
	connect(goLiveBtn, &QPushButton::clicked, this, []() {
		if (obs_frontend_streaming_active())
			obs_frontend_streaming_stop();
		else
			obs_frontend_streaming_start();
	});
	layout->addWidget(goLiveBtn);

	recordBtn = new QPushButton(this);
	recordBtn->setMinimumHeight(34);
	connect(recordBtn, &QPushButton::clicked, this, []() {
		if (obs_frontend_recording_active())
			obs_frontend_recording_stop();
		else
			obs_frontend_recording_start();
	});
	layout->addWidget(recordBtn);

	studioBtn = new QPushButton(this);
	studioBtn->setMinimumHeight(34);
	connect(studioBtn, &QPushButton::clicked, this,
		[]() { obs_frontend_set_preview_program_mode(!obs_frontend_preview_program_mode_active()); });
	layout->addWidget(studioBtn);

	connect(&timer, &QTimer::timeout, this, &EmpireCommandDock::Update);
	timer.setInterval(EMPIRE_CMD_INTERVAL);

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
	const bool streaming = obs_frontend_streaming_active();
	const bool recording = obs_frontend_recording_active();
	const uint64_t now = os_gettime_ns();

	QString status;
	if (streaming) {
		const uint64_t secs = streamStart ? (now - streamStart) / 1000000000ULL : 0;
		status = QString("<span style='color:#E50914;'>● LIVE</span>&nbsp;&nbsp;%1").arg(empire_fmt_time(secs));
	} else if (recording) {
		const uint64_t secs = recordStart ? (now - recordStart) / 1000000000ULL : 0;
		status = QString("<span style='color:#E50914;'>● REC</span>&nbsp;&nbsp;%1").arg(empire_fmt_time(secs));
	} else {
		status = QStringLiteral("<span style='color:#808080;'>○ Offline</span>");
	}
	statusLabel->setText(status);

	const double cpu = os_cpu_usage_info_query(cpu_info);
	const double fps = obs_get_active_fps();
	double droppedPct = 0.0;
	double kbps = 0.0;

	OBSOutputAutoRelease out = obs_frontend_get_streaming_output();
	if (out) {
		const int total = obs_output_get_total_frames(out);
		const int dropped = obs_output_get_frames_dropped(out);
		if (total < first_total || dropped < first_dropped) {
			first_total = total;
			first_dropped = dropped;
		}
		const int t = total - first_total;
		const int d = dropped - first_dropped;
		droppedPct = t > 0 ? (double)d / (double)t * 100.0 : 0.0;

		const uint64_t bytes = obs_output_get_total_bytes(out);
		if (lastBytesTime != 0 && now > lastBytesTime) {
			const double sec = (double)(now - lastBytesTime) / 1000000000.0;
			const uint64_t db = (bytes >= lastBytes) ? (bytes - lastBytes) : 0;
			kbps = sec > 0.0 ? (double)(db * 8) / sec / 1000.0 : 0.0;
		}
		lastBytes = bytes;
		lastBytesTime = now;
	} else {
		first_total = 0;
		first_dropped = 0;
		lastBytes = 0;
		lastBytesTime = 0;
	}

	statsLabel->setText(QString("CPU %1%%  ·  FPS %2  ·  Drop %3%%  ·  %4 kb/s")
				    .arg(QString::number(cpu, 'f', 0))
				    .arg(QString::number(fps, 'f', 0))
				    .arg(QString::number(droppedPct, 'f', 1))
				    .arg(QString::number(kbps, 'f', 0)));
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
