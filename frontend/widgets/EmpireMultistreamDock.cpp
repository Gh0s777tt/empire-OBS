#include "EmpireMultistreamDock.hpp"

#include <widgets/OBSBasic.hpp>
#include <qt-wrappers.hpp>

#include <obs-frontend-api.h>

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <cstdio>
#include <string>

#include "moc_EmpireMultistreamDock.cpp"

#define MS_SECTION "EmpireMultistream"

EmpireMultistreamDock::EmpireMultistreamDock(QWidget *parent) : QFrame(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(8, 8, 8, 8);
	layout->setSpacing(6);

	QLabel *title = new QLabel(QStringLiteral("Empire Multistream — extra RTMP destinations"), this);
	title->setStyleSheet("font-weight: bold; color: #E50914;");
	layout->addWidget(title);

	QLabel *hint = new QLabel(QStringLiteral("Each enabled destination streams alongside your main stream, "
						 "sharing its encoders (no extra GPU/CPU). They start and stop "
						 "automatically with the main stream."),
				  this);
	hint->setWordWrap(true);
	hint->setStyleSheet("color: #B3B3B3;");
	layout->addWidget(hint);

	QGridLayout *grid = new QGridLayout();
	grid->setHorizontalSpacing(6);
	grid->setVerticalSpacing(6);
	grid->addWidget(new QLabel(QStringLiteral("On"), this), 0, 0);
	grid->addWidget(new QLabel(QStringLiteral("RTMP URL"), this), 0, 1);
	grid->addWidget(new QLabel(QStringLiteral("Stream key"), this), 0, 2);
	grid->addWidget(new QLabel(QStringLiteral("Status"), this), 0, 3);

	for (int i = 0; i < NUM_DESTS; i++) {
		rows[i].enable = new QCheckBox(this);
		rows[i].url = new QLineEdit(this);
		rows[i].url->setPlaceholderText(QStringLiteral("rtmp://live.example.com/app"));
		rows[i].key = new QLineEdit(this);
		rows[i].key->setEchoMode(QLineEdit::Password);
		rows[i].key->setPlaceholderText(QStringLiteral("stream key"));
		rows[i].status = new QLabel(QStringLiteral("idle"), this);
		rows[i].status->setStyleSheet("color: #808080;");

		grid->addWidget(rows[i].enable, i + 1, 0);
		grid->addWidget(rows[i].url, i + 1, 1);
		grid->addWidget(rows[i].key, i + 1, 2);
		grid->addWidget(rows[i].status, i + 1, 3);

		connect(rows[i].enable, &QCheckBox::toggled, this, [this]() { Save(); });
		connect(rows[i].url, &QLineEdit::editingFinished, this, [this]() { Save(); });
		connect(rows[i].key, &QLineEdit::editingFinished, this, [this]() { Save(); });
	}
	grid->setColumnStretch(1, 3);
	grid->setColumnStretch(2, 2);
	layout->addLayout(grid);

	mainStatus = new QLabel(this);
	mainStatus->setStyleSheet("color: #808080;");
	layout->addWidget(mainStatus);

	startAllButton = new QPushButton(this);
	startAllButton->setMinimumHeight(34);
	connect(startAllButton, &QPushButton::clicked, this, [this]() { ToggleStreaming(); });
	layout->addWidget(startAllButton);

	layout->addStretch();

	Load();
	UpdateControls();

	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	setObjectName(QStringLiteral("empireMultistreamDock"));
}

EmpireMultistreamDock::~EmpireMultistreamDock()
{
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);
	StopAll();
}

void EmpireMultistreamDock::SetStatus(int i, const QString &text, const char *cssColor)
{
	rows[i].status->setText(text);
	rows[i].status->setStyleSheet(QStringLiteral("color: %1;").arg(cssColor));
}

void EmpireMultistreamDock::UpdateControls()
{
	bool active = obs_frontend_streaming_active();

	startAllButton->setText(active ? QStringLiteral("Stop all streams") : QStringLiteral("Start all streams"));
	startAllButton->setStyleSheet(active ? "background-color: #B20710; color: white; font-weight: bold;"
					     : "background-color: #E50914; color: white; font-weight: bold;");

	mainStatus->setText(active ? QStringLiteral("Main stream: LIVE") : QStringLiteral("Main stream: idle"));
	mainStatus->setStyleSheet(active ? "color: #46D369;" : "color: #808080;");
}

void EmpireMultistreamDock::ToggleStreaming()
{
	if (obs_frontend_streaming_active())
		obs_frontend_streaming_stop();
	else
		obs_frontend_streaming_start();
}

void EmpireMultistreamDock::Load()
{
	config_t *cfg = OBSBasic::Get()->Config();
	for (int i = 0; i < NUM_DESTS; i++) {
		char k[64];
		snprintf(k, sizeof(k), "Dest%dEnabled", i + 1);
		rows[i].enable->setChecked(config_get_bool(cfg, MS_SECTION, k));
		snprintf(k, sizeof(k), "Dest%dURL", i + 1);
		const char *url = config_get_string(cfg, MS_SECTION, k);
		rows[i].url->setText(url ? url : "");
		snprintf(k, sizeof(k), "Dest%dKey", i + 1);
		const char *key = config_get_string(cfg, MS_SECTION, k);
		rows[i].key->setText(key ? key : "");
	}
}

void EmpireMultistreamDock::Save()
{
	config_t *cfg = OBSBasic::Get()->Config();
	for (int i = 0; i < NUM_DESTS; i++) {
		char k[64];
		snprintf(k, sizeof(k), "Dest%dEnabled", i + 1);
		config_set_bool(cfg, MS_SECTION, k, rows[i].enable->isChecked());
		snprintf(k, sizeof(k), "Dest%dURL", i + 1);
		config_set_string(cfg, MS_SECTION, k, QT_TO_UTF8(rows[i].url->text()));
		snprintf(k, sizeof(k), "Dest%dKey", i + 1);
		config_set_string(cfg, MS_SECTION, k, QT_TO_UTF8(rows[i].key->text()));
	}
	config_save_safe(cfg, "tmp", nullptr);
}

void EmpireMultistreamDock::StartAll()
{
	StopAll();

	OBSOutputAutoRelease mainOutput = obs_frontend_get_streaming_output();
	if (!mainOutput)
		return;

	obs_encoder_t *venc = obs_output_get_video_encoder(mainOutput);
	obs_encoder_t *aenc = obs_output_get_audio_encoder(mainOutput, 0);
	if (!venc || !aenc)
		return;

	for (int i = 0; i < NUM_DESTS; i++) {
		if (!rows[i].enable->isChecked())
			continue;

		std::string url = QT_TO_UTF8(rows[i].url->text());
		std::string key = QT_TO_UTF8(rows[i].key->text());
		if (url.empty() || key.empty()) {
			SetStatus(i, QStringLiteral("skipped (empty)"), "#EABC48");
			continue;
		}

		OBSDataAutoRelease svcSettings = obs_data_create();
		obs_data_set_string(svcSettings, "server", url.c_str());
		obs_data_set_string(svcSettings, "key", key.c_str());

		char svcName[64];
		snprintf(svcName, sizeof(svcName), "empire_dest_%d", i + 1);
		OBSServiceAutoRelease service = obs_service_create("rtmp_custom", svcName, svcSettings, nullptr);

		char outName[64];
		snprintf(outName, sizeof(outName), "empire_stream_%d", i + 1);
		OBSOutputAutoRelease output = obs_output_create("rtmp_output", outName, nullptr, nullptr);
		obs_output_set_video_encoder(output, venc);
		obs_output_set_audio_encoder(output, aenc, 0);
		obs_output_set_service(output, service);
		obs_output_set_reconnect_settings(output, 20, 2);

		if (obs_output_start(output)) {
			SetStatus(i, QStringLiteral("LIVE"), "#46D369");
			liveOutputs.push_back(std::move(output));
			liveServices.push_back(std::move(service));
		} else {
			const char *err = obs_output_get_last_error(output);
			SetStatus(i, QStringLiteral("failed"), "#E50914");
			blog(LOG_WARNING, "[Empire Multistream] destination %d failed to start: %s", i + 1,
			     (err && *err) ? err : "unknown error");
		}
	}
}

void EmpireMultistreamDock::StopAll()
{
	for (auto &o : liveOutputs)
		obs_output_stop(o);
	liveOutputs.clear();
	liveServices.clear();

	for (int i = 0; i < NUM_DESTS; i++)
		SetStatus(i, QStringLiteral("idle"), "#808080");
}

void EmpireMultistreamDock::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	EmpireMultistreamDock *dock = static_cast<EmpireMultistreamDock *>(ptr);

	switch (event) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		dock->StartAll();
		dock->UpdateControls();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
		dock->StopAll();
		dock->UpdateControls();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STARTING:
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		dock->UpdateControls();
		break;
	default:
		break;
	}
}
