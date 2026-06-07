#include "EmpireControlsDock.hpp"

#include <obs-frontend-api.h>

#include <QMetaObject>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "moc_EmpireControlsDock.cpp"

static const char *kBtnIdle = "QPushButton { background:#1C1C1C; border:1px solid #2A2A2A; border-radius:9px;"
			      " padding:11px 12px; color:#CCCCCC; font-weight:600; }"
			      "QPushButton:hover { background:#262626; color:#FFFFFF; border-color:#3A3A3A; }"
			      "QPushButton:pressed { background:#161616; }";

static const char *kBtnPrimary = "QPushButton { background:#E50914; border:1px solid #E50914; border-radius:9px;"
				 " padding:11px 12px; color:#FFFFFF; font-weight:700; }"
				 "QPushButton:hover { background:#F6121D; }";

static const char *kBtnStop = "QPushButton { background:#7A0509; border:1px solid #B20710; border-radius:9px;"
			      " padding:11px 12px; color:#FFFFFF; font-weight:700; }"
			      "QPushButton:hover { background:#B20710; }";

static const char *kBtnActive = "QPushButton { background:#E50914; border:1px solid #E50914; border-radius:9px;"
				" padding:11px 12px; color:#FFFFFF; font-weight:700; }"
				"QPushButton:hover { background:#B20710; }";

static const char *kBtnExit = "QPushButton { background:#1C1C1C; border:1px solid #3A1416; border-radius:9px;"
			      " padding:11px 12px; color:#E06A6A; font-weight:600; }"
			      "QPushButton:hover { background:#2A1416; color:#FF8080; }";

EmpireControlsDock::EmpireControlsDock(QWidget *parent) : QFrame(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(8, 8, 8, 8);
	layout->setSpacing(7);

	streamBtn = new QPushButton(this);
	streamBtn->setMinimumHeight(38);
	streamBtn->setCursor(Qt::PointingHandCursor);
	connect(streamBtn, &QPushButton::clicked, this, []() {
		if (obs_frontend_streaming_active())
			obs_frontend_streaming_stop();
		else
			obs_frontend_streaming_start();
	});
	layout->addWidget(streamBtn);

	recordBtn = new QPushButton(this);
	recordBtn->setMinimumHeight(38);
	recordBtn->setCursor(Qt::PointingHandCursor);
	connect(recordBtn, &QPushButton::clicked, this, []() {
		if (obs_frontend_recording_active())
			obs_frontend_recording_stop();
		else
			obs_frontend_recording_start();
	});
	layout->addWidget(recordBtn);

	replayBtn = new QPushButton(this);
	replayBtn->setMinimumHeight(38);
	replayBtn->setCursor(Qt::PointingHandCursor);
	connect(replayBtn, &QPushButton::clicked, this, []() {
		if (obs_frontend_replay_buffer_active())
			obs_frontend_replay_buffer_stop();
		else
			obs_frontend_replay_buffer_start();
	});
	layout->addWidget(replayBtn);

	studioBtn = new QPushButton(this);
	studioBtn->setMinimumHeight(38);
	studioBtn->setCursor(Qt::PointingHandCursor);
	connect(studioBtn, &QPushButton::clicked, this,
		[]() { obs_frontend_set_preview_program_mode(!obs_frontend_preview_program_mode_active()); });
	layout->addWidget(studioBtn);

	settingsBtn = new QPushButton(QStringLiteral("USTAWIENIA"), this);
	settingsBtn->setMinimumHeight(38);
	settingsBtn->setCursor(Qt::PointingHandCursor);
	settingsBtn->setStyleSheet(kBtnIdle);
	connect(settingsBtn, &QPushButton::clicked, this, []() {
		QWidget *mw = static_cast<QWidget *>(obs_frontend_get_main_window());
		if (mw)
			QMetaObject::invokeMethod(mw, "on_action_Settings_triggered", Qt::QueuedConnection);
	});
	layout->addWidget(settingsBtn);

	exitBtn = new QPushButton(QStringLiteral("WYJŚCIE"), this);
	exitBtn->setMinimumHeight(38);
	exitBtn->setCursor(Qt::PointingHandCursor);
	exitBtn->setStyleSheet(kBtnExit);
	connect(exitBtn, &QPushButton::clicked, this, []() {
		QWidget *mw = static_cast<QWidget *>(obs_frontend_get_main_window());
		if (mw)
			mw->close();
	});
	layout->addWidget(exitBtn);

	layout->addStretch();

	UpdateStates();
	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	setObjectName(QStringLiteral("empireControlsDock"));
}

EmpireControlsDock::~EmpireControlsDock()
{
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);
}

void EmpireControlsDock::UpdateStates()
{
	const bool streaming = obs_frontend_streaming_active();
	const bool recording = obs_frontend_recording_active();
	const bool replay = obs_frontend_replay_buffer_active();
	const bool studio = obs_frontend_preview_program_mode_active();

	streamBtn->setText(streaming ? QStringLiteral("■  ZATRZYMAJ STREAM") : QStringLiteral("●  ROZPOCZNIJ STREAM"));
	streamBtn->setStyleSheet(streaming ? kBtnStop : kBtnPrimary);

	recordBtn->setText(recording ? QStringLiteral("■  ZATRZYMAJ NAGRYWANIE")
				     : QStringLiteral("●  ROZPOCZNIJ NAGRYWANIE"));
	recordBtn->setStyleSheet(recording ? kBtnActive : kBtnIdle);

	replayBtn->setText(replay ? QStringLiteral("REPLAY BUFFER  ·  WŁ.") : QStringLiteral("REPLAY BUFFER"));
	replayBtn->setStyleSheet(replay ? kBtnActive : kBtnIdle);

	studioBtn->setText(studio ? QStringLiteral("TRYB STUDIO  ·  WŁ.") : QStringLiteral("TRYB STUDIO"));
	studioBtn->setStyleSheet(studio ? kBtnActive : kBtnIdle);
}

void EmpireControlsDock::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	EmpireControlsDock *dock = static_cast<EmpireControlsDock *>(ptr);

	switch (event) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED:
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED:
	case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
	case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		dock->UpdateStates();
		break;
	default:
		break;
	}
}
