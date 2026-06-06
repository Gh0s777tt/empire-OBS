#include "EmpireVerticalDock.hpp"
#include "OBSQTDisplay.hpp"

#include <utility/display-helpers.hpp>

#include <obs-frontend-api.h>
#include <graphics/graphics.h>
#include <graphics/vec2.h>
#include <util/platform.h>
#include <util/config-file.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <cstring>
#include <string>

#include "moc_EmpireVerticalDock.cpp"

#define VERTICAL_W 1080
#define VERTICAL_H 1920

/* Prefer a hardware encoder (NVENC/QSV/AMF) for the vertical output to spare
 * CPU; fall back to x264. */
static const char *empire_pick_video_encoder()
{
	static const char *prefs[] = {"obs_nvenc", "jim_nvenc",        "ffmpeg_nvenc", "obs_qsv11_v2",
				      "obs_qsv11", "h264_texture_amf", "amd_amf_h264"};
	for (const char *id : prefs) {
		if (obs_encoder_get_display_name(id))
			return id;
	}
	return "obs_x264";
}

EmpireVerticalDock::EmpireVerticalDock(QWidget *parent) : QFrame(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	/* Row 1: caption + Fill/Fit toggle. */
	QHBoxLayout *bar = new QHBoxLayout();
	bar->setContentsMargins(6, 4, 6, 2);
	QLabel *caption = new QLabel(QStringLiteral("Vertical 9:16 — mirrors program"), this);
	caption->setStyleSheet("color:#B3B3B3;");
	bar->addWidget(caption);
	bar->addStretch();
	modeButton = new QPushButton(this);
	modeButton->setStyleSheet("background-color:#333; color:white; font-weight:bold; padding:2px 10px;");
	connect(modeButton, &QPushButton::clicked, this, [this]() {
		fillMode = !fillMode;
		ApplyFraming();
		UpdateModeButton();
	});
	bar->addWidget(modeButton);
	layout->addLayout(bar);

	/* Row 2: vertical RTMP URL + key + Go Live + Record. */
	QHBoxLayout *ctl = new QHBoxLayout();
	ctl->setContentsMargins(6, 0, 6, 4);
	ctl->setSpacing(6);
	urlEdit = new QLineEdit(this);
	urlEdit->setPlaceholderText(QStringLiteral("rtmp://…  vertical RTMP URL"));
	keyEdit = new QLineEdit(this);
	keyEdit->setEchoMode(QLineEdit::Password);
	keyEdit->setPlaceholderText(QStringLiteral("stream key"));
	connect(urlEdit, &QLineEdit::editingFinished, this, [this]() { SaveConfig(); });
	connect(keyEdit, &QLineEdit::editingFinished, this, [this]() { SaveConfig(); });
	streamButton = new QPushButton(this);
	connect(streamButton, &QPushButton::clicked, this, [this]() { ToggleStreaming(); });
	recordButton = new QPushButton(this);
	connect(recordButton, &QPushButton::clicked, this, [this]() { ToggleRecording(); });
	ctl->addWidget(urlEdit, 3);
	ctl->addWidget(keyEdit, 2);
	ctl->addWidget(streamButton);
	ctl->addWidget(recordButton);
	layout->addLayout(ctl);

	/* Private 9:16 canvas, inheriting the main video timing/format. */
	obs_video_info ovi = {};
	obs_get_video_info(&ovi);
	ovi.base_width = VERTICAL_W;
	ovi.base_height = VERTICAL_H;
	ovi.output_width = VERTICAL_W;
	ovi.output_height = VERTICAL_H;
	canvas = obs_canvas_create_private("Empire Vertical", &ovi, ACTIVATE | SCENE_REF);

	/* Dedicated vertical scene, set as the canvas program (channel 0). */
	if (canvas) {
		vScene = obs_canvas_scene_create(canvas, "Empire Vertical");
		obs_canvas_set_channel(canvas, 0, obs_scene_get_source(vScene));
	}

	display = new OBSQTDisplay(this);
	layout->addWidget(display, 1);

	auto addDraw = [this](OBSQTDisplay *d) {
		obs_display_add_draw_callback(d->GetDisplay(), EmpireVerticalDock::RenderVertical, this);
	};
	connect(display, &OBSQTDisplay::DisplayCreated, this, addDraw);

	LoadConfig();
	UpdateModeButton();
	UpdateRecordButton();
	UpdateStreamButton();
	SyncToCurrentScene();
	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	setObjectName(QStringLiteral("empireVerticalDock"));
}

EmpireVerticalDock::~EmpireVerticalDock()
{
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);

	/* Stop the recording + stream (they use the canvas video) before any teardown. */
	if (recordOutput) {
		obs_output_force_stop(recordOutput);
		recordOutput = nullptr;
		recordVEnc = nullptr;
		recordAEnc = nullptr;
	}
	if (streamOutput) {
		obs_output_force_stop(streamOutput);
		streamOutput = nullptr;
		streamVEnc = nullptr;
		streamAEnc = nullptr;
		streamService = nullptr;
	}

	/* Remove the draw callback BEFORE tearing down the canvas so the
	 * graphics thread can never touch a freed canvas. */
	if (display && display->GetDisplay())
		obs_display_remove_draw_callback(display->GetDisplay(), EmpireVerticalDock::RenderVertical, this);

	/* The canvas owns vScene + its items; removing/releasing it frees them. */
	if (canvas) {
		obs_canvas_set_channel(canvas, 0, nullptr);
		obs_canvas_remove(canvas);
		obs_canvas_release(canvas);
		canvas = nullptr;
	}
	mirrorItem = nullptr;
	vScene = nullptr;
}

void EmpireVerticalDock::SyncToCurrentScene()
{
	if (!vScene)
		return;

	/* Replace the mirror item with the current program scene. */
	if (mirrorItem) {
		obs_sceneitem_remove(mirrorItem);
		mirrorItem = nullptr;
	}

	obs_source_t *prog = obs_frontend_get_current_scene();
	if (!prog)
		return;

	mirrorItem = obs_scene_add(vScene, prog);
	obs_source_release(prog);

	ApplyFraming();
}

void EmpireVerticalDock::ApplyFraming()
{
	if (!mirrorItem)
		return;

	/* Scale the mirrored program to FILL (crop) or FIT (letterbox) the 9:16 frame. */
	struct vec2 bounds;
	vec2_set(&bounds, (float)VERTICAL_W, (float)VERTICAL_H);
	obs_sceneitem_set_bounds_type(mirrorItem, fillMode ? OBS_BOUNDS_SCALE_OUTER : OBS_BOUNDS_SCALE_INNER);
	obs_sceneitem_set_bounds_alignment(mirrorItem, OBS_ALIGN_CENTER);
	obs_sceneitem_set_bounds(mirrorItem, &bounds);

	struct vec2 pos;
	vec2_set(&pos, (float)VERTICAL_W / 2.0f, (float)VERTICAL_H / 2.0f);
	obs_sceneitem_set_alignment(mirrorItem, OBS_ALIGN_CENTER);
	obs_sceneitem_set_pos(mirrorItem, &pos);
}

void EmpireVerticalDock::UpdateModeButton()
{
	if (modeButton)
		modeButton->setText(fillMode ? QStringLiteral("Fill (crop)") : QStringLiteral("Fit (bars)"));
}

void EmpireVerticalDock::UpdateRecordButton()
{
	if (!recordButton)
		return;
	recordButton->setText(recording ? QStringLiteral("Stop ● REC") : QStringLiteral("Record 9:16"));
	recordButton->setStyleSheet(
		recording ? "background-color:#B20710; color:white; font-weight:bold; padding:2px 10px;"
			  : "background-color:#46D369; color:#141414; font-weight:bold; padding:2px 10px;");
}

void EmpireVerticalDock::UpdateStreamButton()
{
	if (!streamButton)
		return;
	streamButton->setText(streaming ? QStringLiteral("Stop ◉ LIVE") : QStringLiteral("Go Live 9:16"));
	streamButton->setStyleSheet(
		streaming ? "background-color:#B20710; color:white; font-weight:bold; padding:2px 10px;"
			  : "background-color:#E50914; color:white; font-weight:bold; padding:2px 10px;");
}

void EmpireVerticalDock::ToggleRecording()
{
	if (recording && recordOutput) {
		obs_output_stop(recordOutput);
		recording = false;
	} else {
		StartRecording();
	}
	UpdateRecordButton();
}

void EmpireVerticalDock::StartRecording()
{
	if (!canvas)
		return;
	video_t *vid = obs_canvas_get_video(canvas);
	if (!vid)
		return;

	/* Release any previous (already-stopped) output/encoders. */
	recordOutput = nullptr;
	recordVEnc = nullptr;
	recordAEnc = nullptr;

	/* Video encoder bound to the vertical canvas video. */
	OBSDataAutoRelease vset = obs_data_create();
	obs_data_set_int(vset, "bitrate", 12000);
	obs_data_set_string(vset, "rate_control", "CBR");
	recordVEnc = obs_video_encoder_create(empire_pick_video_encoder(), "empire_vert_venc", vset, nullptr);
	if (!recordVEnc)
		recordVEnc = obs_video_encoder_create("obs_x264", "empire_vert_venc", vset, nullptr);
	obs_encoder_set_video(recordVEnc, vid);

	/* Audio encoder bound to the main audio mix. */
	OBSDataAutoRelease aset = obs_data_create();
	obs_data_set_int(aset, "bitrate", 160);
	recordAEnc = obs_audio_encoder_create("ffmpeg_aac", "empire_vert_aenc", aset, 0, nullptr);
	obs_encoder_set_audio(recordAEnc, obs_get_audio());

	/* Output file: the configured recording folder + a timestamped name. */
	config_t *cfg = obs_frontend_get_profile_config();
	const char *mode = cfg ? config_get_string(cfg, "Output", "Mode") : nullptr;
	const char *dir = nullptr;
	if (cfg)
		dir = (mode && strcmp(mode, "Advanced") == 0) ? config_get_string(cfg, "AdvOut", "RecFilePath")
							      : config_get_string(cfg, "SimpleOutput", "FilePath");
	std::string folder = (dir && *dir) ? dir : ".";

	char *fname = os_generate_formatted_filename("mp4", true, "empire-vertical-%CCYY-%MM-%DD_%hh-%mm-%ss");
	std::string fullPath = folder + "/" + (fname ? fname : "empire-vertical.mp4");
	bfree(fname);

	OBSDataAutoRelease oset = obs_data_create();
	obs_data_set_string(oset, "path", fullPath.c_str());
	recordOutput = obs_output_create("ffmpeg_muxer", "empire_vert_record", oset, nullptr);
	obs_output_set_video_encoder(recordOutput, recordVEnc);
	obs_output_set_audio_encoder(recordOutput, recordAEnc, 0);

	if (obs_output_start(recordOutput)) {
		recording = true;
		blog(LOG_INFO, "[Empire Vertical] recording to %s", fullPath.c_str());
	} else {
		const char *err = obs_output_get_last_error(recordOutput);
		blog(LOG_WARNING, "[Empire Vertical] recording failed to start: %s", (err && *err) ? err : "unknown");
		recordOutput = nullptr;
		recordVEnc = nullptr;
		recordAEnc = nullptr;
		recording = false;
	}
}

void EmpireVerticalDock::ToggleStreaming()
{
	if (streaming && streamOutput) {
		obs_output_stop(streamOutput);
		streaming = false;
	} else {
		StartStreaming();
	}
	UpdateStreamButton();
}

void EmpireVerticalDock::StartStreaming()
{
	if (!canvas)
		return;
	video_t *vid = obs_canvas_get_video(canvas);
	if (!vid)
		return;

	const std::string url = urlEdit->text().trimmed().toUtf8().constData();
	const std::string key = keyEdit->text().trimmed().toUtf8().constData();
	if (url.empty() || key.empty()) {
		blog(LOG_WARNING, "[Empire Vertical] set an RTMP URL + stream key before going live");
		return;
	}

	/* Release any previous (already-stopped) output/encoders/service. */
	streamOutput = nullptr;
	streamVEnc = nullptr;
	streamAEnc = nullptr;
	streamService = nullptr;

	OBSDataAutoRelease vset = obs_data_create();
	obs_data_set_int(vset, "bitrate", 6000);
	obs_data_set_string(vset, "rate_control", "CBR");
	streamVEnc = obs_video_encoder_create(empire_pick_video_encoder(), "empire_vert_stream_venc", vset, nullptr);
	if (!streamVEnc)
		streamVEnc = obs_video_encoder_create("obs_x264", "empire_vert_stream_venc", vset, nullptr);
	obs_encoder_set_video(streamVEnc, vid);

	OBSDataAutoRelease aset = obs_data_create();
	obs_data_set_int(aset, "bitrate", 160);
	streamAEnc = obs_audio_encoder_create("ffmpeg_aac", "empire_vert_stream_aenc", aset, 0, nullptr);
	obs_encoder_set_audio(streamAEnc, obs_get_audio());

	OBSDataAutoRelease svc = obs_data_create();
	obs_data_set_string(svc, "server", url.c_str());
	obs_data_set_string(svc, "key", key.c_str());
	streamService = obs_service_create("rtmp_custom", "empire_vert_service", svc, nullptr);

	streamOutput = obs_output_create("rtmp_output", "empire_vert_stream", nullptr, nullptr);
	obs_output_set_video_encoder(streamOutput, streamVEnc);
	obs_output_set_audio_encoder(streamOutput, streamAEnc, 0);
	obs_output_set_service(streamOutput, streamService);
	obs_output_set_reconnect_settings(streamOutput, 20, 2);

	if (obs_output_start(streamOutput)) {
		streaming = true;
		blog(LOG_INFO, "[Empire Vertical] going live 9:16 to %s", url.c_str());
	} else {
		const char *err = obs_output_get_last_error(streamOutput);
		blog(LOG_WARNING, "[Empire Vertical] stream failed to start: %s", (err && *err) ? err : "unknown");
		streamOutput = nullptr;
		streamVEnc = nullptr;
		streamAEnc = nullptr;
		streamService = nullptr;
		streaming = false;
	}
}

void EmpireVerticalDock::LoadConfig()
{
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return;
	const char *url = config_get_string(cfg, "EmpireVertical", "StreamURL");
	const char *key = config_get_string(cfg, "EmpireVertical", "StreamKey");
	if (urlEdit)
		urlEdit->setText(url ? url : "");
	if (keyEdit)
		keyEdit->setText(key ? key : "");
}

void EmpireVerticalDock::SaveConfig()
{
	config_t *cfg = obs_frontend_get_profile_config();
	if (!cfg)
		return;
	config_set_string(cfg, "EmpireVertical", "StreamURL", urlEdit->text().toUtf8().constData());
	config_set_string(cfg, "EmpireVertical", "StreamKey", keyEdit->text().toUtf8().constData());
	config_save_safe(cfg, "tmp", nullptr);
}

void EmpireVerticalDock::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	EmpireVerticalDock *dock = static_cast<EmpireVerticalDock *>(ptr);

	switch (event) {
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		dock->SyncToCurrentScene();
		break;
	default:
		break;
	}
}

void EmpireVerticalDock::RenderVertical(void *data, uint32_t, uint32_t)
{
	EmpireVerticalDock *self = static_cast<EmpireVerticalDock *>(data);

	obs_canvas_t *canvas = self->canvas;
	if (!canvas)
		return;

	obs_video_info ovi;
	if (!obs_canvas_get_video_info(canvas, &ovi))
		return;

	uint32_t dw = 0, dh = 0;
	obs_display_size(self->display->GetDisplay(), &dw, &dh);

	int x = 0, y = 0;
	float scale = 1.0f;
	GetScaleAndCenterPos((int)ovi.base_width, (int)ovi.base_height, (int)dw, (int)dh, x, y, scale);

	const int cx = int(scale * float(ovi.base_width));
	const int cy = int(scale * float(ovi.base_height));

	gs_viewport_push();
	gs_projection_push();

	gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height), -100.0f, 100.0f);
	gs_set_viewport(x, y, cx, cy);

	/* Render the canvas's sources directly (obs_view_render) rather than its
	 * composed texture — a private canvas isn't composited by the core video
	 * loop, so its texture would be empty/garbage. */
	obs_canvas_render(canvas);

	gs_projection_pop();
	gs_viewport_pop();
}
