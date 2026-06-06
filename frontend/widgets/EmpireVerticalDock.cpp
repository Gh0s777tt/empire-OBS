#include "EmpireVerticalDock.hpp"
#include "OBSQTDisplay.hpp"

#include <utility/display-helpers.hpp>

#include <obs-frontend-api.h>
#include <graphics/graphics.h>
#include <graphics/vec2.h>
#include <graphics/vec3.h>
#include <graphics/vec4.h>
#include <graphics/matrix4.h>
#include <util/platform.h>
#include <util/config-file.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cstdio>
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

struct EmpireHit {
	float cx, cy;
	obs_sceneitem_t *hit;
};

static bool empire_hit_enum(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
	EmpireHit *hd = static_cast<EmpireHit *>(param);
	matrix4 transform, inv;
	obs_sceneitem_get_box_transform(item, &transform);
	if (!matrix4_inv(&inv, &transform))
		return true;
	vec3 p;
	vec3_set(&p, hd->cx, hd->cy, 0.0f);
	vec3_transform(&p, &p, &inv);
	/* enum is bottom-to-top, so keeping the last hit yields the topmost item. */
	if (p.x >= 0.0f && p.x <= 1.0f && p.y >= 0.0f && p.y <= 1.0f)
		hd->hit = item;
	return true;
}

EmpireVerticalDock::EmpireVerticalDock(QWidget *parent) : QFrame(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	/* Row 1: caption + Mirror/Custom + Fill/Fit. */
	QHBoxLayout *bar = new QHBoxLayout();
	bar->setContentsMargins(6, 4, 6, 2);
	QLabel *caption = new QLabel(QStringLiteral("Vertical 9:16"), this);
	caption->setStyleSheet("color:#B3B3B3;");
	bar->addWidget(caption);
	bar->addStretch();
	customButton = new QPushButton(this);
	customButton->setStyleSheet("background-color:#333; color:white; font-weight:bold; padding:2px 10px;");
	connect(customButton, &QPushButton::clicked, this, [this]() { ToggleCustomMode(); });
	bar->addWidget(customButton);
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

	if (canvas) {
		vScene = obs_canvas_scene_create(canvas, "Empire Vertical");
		obs_canvas_set_channel(canvas, 0, obs_scene_get_source(vScene));
	}

	display = new EmpireVerticalDisplay(this);
	layout->addWidget(display, 1);

	auto addDraw = [this](OBSQTDisplay *d) {
		obs_display_add_draw_callback(d->GetDisplay(), EmpireVerticalDock::RenderVertical, this);
	};
	connect(display, &OBSQTDisplay::DisplayCreated, this, addDraw);
	connect(display, &EmpireVerticalDisplay::mousePressedAt, this, [this](QPointF p) { OnMousePress(p); });
	connect(display, &EmpireVerticalDisplay::mouseDraggedAt, this, [this](QPointF p) { OnMouseDrag(p); });
	connect(display, &EmpireVerticalDisplay::mouseReleasedHere, this, [this]() { dragging = false; });
	connect(display, &EmpireVerticalDisplay::wheelScaled, this, [this](int d) { OnWheel(d); });
	connect(display, &EmpireVerticalDisplay::contextMenuRequested, this, [this](QPoint p) { ShowContextMenu(p); });

	LoadConfig();
	UpdateModeButton();
	UpdateCustomButton();
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

	if (display && display->GetDisplay())
		obs_display_remove_draw_callback(display->GetDisplay(), EmpireVerticalDock::RenderVertical, this);

	if (canvas) {
		obs_canvas_set_channel(canvas, 0, nullptr);
		obs_canvas_remove(canvas);
		obs_canvas_release(canvas);
		canvas = nullptr;
	}
	mirrorItem = nullptr;
	selectedItem = nullptr;
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
	selectedItem = nullptr;

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

void EmpireVerticalDock::ToggleCustomMode()
{
	customMode = !customMode;
	selectedItem = nullptr;
	dragging = false;

	if (customMode) {
		/* Freeze sync and convert the mirror to free pos+scale matching the
		 * current framing, so the user can pan/zoom it without a jump. */
		if (mirrorItem) {
			obs_sceneitem_set_bounds_type(mirrorItem, OBS_BOUNDS_NONE);
			obs_source_t *src = obs_sceneitem_get_source(mirrorItem);
			float sw = (float)obs_source_get_width(src);
			float sh = (float)obs_source_get_height(src);
			if (sw > 0.0f && sh > 0.0f) {
				float cover = std::max((float)VERTICAL_W / sw, (float)VERTICAL_H / sh);
				float contain = std::min((float)VERTICAL_W / sw, (float)VERTICAL_H / sh);
				struct vec2 scale;
				scale.x = scale.y = (fillMode ? cover : contain);
				obs_sceneitem_set_scale(mirrorItem, &scale);
			}
			obs_sceneitem_set_alignment(mirrorItem, OBS_ALIGN_CENTER);
			struct vec2 pos;
			vec2_set(&pos, (float)VERTICAL_W / 2.0f, (float)VERTICAL_H / 2.0f);
			obs_sceneitem_set_pos(mirrorItem, &pos);
		}
	} else {
		ApplyFraming();
		SyncToCurrentScene();
	}

	UpdateCustomButton();
}

void EmpireVerticalDock::UpdateModeButton()
{
	if (modeButton)
		modeButton->setText(fillMode ? QStringLiteral("Fill (crop)") : QStringLiteral("Fit (bars)"));
}

void EmpireVerticalDock::UpdateCustomButton()
{
	if (customButton)
		customButton->setText(customMode ? QStringLiteral("Custom (edit)") : QStringLiteral("Mirror"));
	if (modeButton)
		modeButton->setEnabled(!customMode);
}

bool EmpireVerticalDock::WidgetToCanvas(const QPointF &pos, float &cx, float &cy) const
{
	if (previewScale <= 0.0f)
		return false;
	const float dpr = (float)display->devicePixelRatioF();
	cx = ((float)pos.x() * dpr - previewX) / previewScale;
	cy = ((float)pos.y() * dpr - previewY) / previewScale;
	return cx >= 0.0f && cy >= 0.0f && cx <= (float)VERTICAL_W && cy <= (float)VERTICAL_H;
}

obs_sceneitem_t *EmpireVerticalDock::HitTest(float cx, float cy) const
{
	EmpireHit hd = {cx, cy, nullptr};
	if (vScene)
		obs_scene_enum_items(vScene, empire_hit_enum, &hd);
	return hd.hit;
}

void EmpireVerticalDock::OnMousePress(const QPointF &pos)
{
	if (!customMode)
		return;
	float cx, cy;
	if (!WidgetToCanvas(pos, cx, cy))
		return;

	selectedItem = HitTest(cx, cy);
	dragging = (selectedItem != nullptr);
	if (dragging) {
		dragStartCx = cx;
		dragStartCy = cy;
		struct vec2 p;
		obs_sceneitem_get_pos(selectedItem, &p);
		itemStartX = p.x;
		itemStartY = p.y;
	}
}

void EmpireVerticalDock::OnMouseDrag(const QPointF &pos)
{
	if (!customMode || !dragging || !selectedItem)
		return;
	float cx, cy;
	WidgetToCanvas(pos, cx, cy); /* keep dragging even slightly outside the frame */
	struct vec2 p;
	p.x = itemStartX + (cx - dragStartCx);
	p.y = itemStartY + (cy - dragStartCy);
	obs_sceneitem_set_pos(selectedItem, &p);
}

void EmpireVerticalDock::OnWheel(int delta)
{
	if (!customMode || !selectedItem)
		return;
	struct vec2 scale;
	obs_sceneitem_get_scale(selectedItem, &scale);
	const float factor = (delta > 0) ? 1.05f : (1.0f / 1.05f);
	scale.x *= factor;
	scale.y *= factor;
	if (scale.x < 0.02f || scale.x > 50.0f)
		return;
	obs_sceneitem_set_scale(selectedItem, &scale);
}

void EmpireVerticalDock::ShowContextMenu(QPoint globalPos)
{
	QMenu menu(this);
	if (customMode) {
		QMenu *addMenu = menu.addMenu(QStringLiteral("Add source"));
		static const char *ids[] = {"image_source",   "text_gdiplus", "color_source",    "ffmpeg_source",
					    "browser_source", "dshow_input",  "monitor_capture", "window_capture"};
		for (const char *id : ids) {
			const char *dn = obs_source_get_display_name(id);
			if (!dn)
				continue;
			const QString sid = QString::fromUtf8(id);
			addMenu->addAction(QString::fromUtf8(dn), this,
					   [this, sid]() { AddSource(sid.toUtf8().constData()); });
		}
		if (selectedItem) {
			menu.addSeparator();
			menu.addAction(QStringLiteral("Properties…"), this, [this]() {
				if (selectedItem)
					obs_frontend_open_source_properties(obs_sceneitem_get_source(selectedItem));
			});
			menu.addAction(QStringLiteral("Remove selected"), this, [this]() { RemoveSelected(); });
		}
	} else {
		menu.addAction(QStringLiteral("Switch to Custom (edit) to compose"), this,
			       [this]() { ToggleCustomMode(); });
	}
	menu.exec(globalPos);
}

void EmpireVerticalDock::AddSource(const char *id)
{
	if (!vScene)
		return;

	char name[160];
	snprintf(name, sizeof(name), "Empire Vertical: %s %d", id, ++addCounter);
	obs_source_t *source = obs_source_create(id, name, nullptr, nullptr);
	if (!source)
		return;

	obs_sceneitem_t *item = obs_scene_add(vScene, source);
	if (item) {
		obs_sceneitem_set_alignment(item, OBS_ALIGN_CENTER);
		struct vec2 pos;
		vec2_set(&pos, (float)VERTICAL_W / 2.0f, (float)VERTICAL_H / 2.0f);
		obs_sceneitem_set_pos(item, &pos);
		selectedItem = item;
	}

	obs_frontend_open_source_properties(source);
	obs_source_release(source);
}

void EmpireVerticalDock::RemoveSelected()
{
	if (!selectedItem)
		return;
	if (selectedItem == mirrorItem)
		mirrorItem = nullptr;
	obs_sceneitem_remove(selectedItem);
	selectedItem = nullptr;
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

	recordOutput = nullptr;
	recordVEnc = nullptr;
	recordAEnc = nullptr;

	OBSDataAutoRelease vset = obs_data_create();
	obs_data_set_int(vset, "bitrate", 12000);
	obs_data_set_string(vset, "rate_control", "CBR");
	recordVEnc = obs_video_encoder_create(empire_pick_video_encoder(), "empire_vert_venc", vset, nullptr);
	if (!recordVEnc)
		recordVEnc = obs_video_encoder_create("obs_x264", "empire_vert_venc", vset, nullptr);
	obs_encoder_set_video(recordVEnc, vid);

	OBSDataAutoRelease aset = obs_data_create();
	obs_data_set_int(aset, "bitrate", 160);
	recordAEnc = obs_audio_encoder_create("ffmpeg_aac", "empire_vert_aenc", aset, 0, nullptr);
	obs_encoder_set_audio(recordAEnc, obs_get_audio());

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

void EmpireVerticalDock::UpdateStreamButton()
{
	if (!streamButton)
		return;
	streamButton->setText(streaming ? QStringLiteral("Stop ◉ LIVE") : QStringLiteral("Go Live 9:16"));
	streamButton->setStyleSheet(
		streaming ? "background-color:#B20710; color:white; font-weight:bold; padding:2px 10px;"
			  : "background-color:#E50914; color:white; font-weight:bold; padding:2px 10px;");
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
		if (!dock->customMode)
			dock->SyncToCurrentScene();
		break;
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

	/* Clear to black first so an empty/partial vertical scene never shows
	 * uninitialised GPU memory (the "white bars"). */
	struct vec4 clearColor;
	vec4_set(&clearColor, 0.0f, 0.0f, 0.0f, 1.0f);
	gs_clear(GS_CLEAR_COLOR, &clearColor, 1.0f, 0);

	int x = 0, y = 0;
	float scale = 1.0f;
	GetScaleAndCenterPos((int)ovi.base_width, (int)ovi.base_height, (int)dw, (int)dh, x, y, scale);

	/* Remember the transform so mouse events can map widget px -> canvas space. */
	self->previewX = (float)x;
	self->previewY = (float)y;
	self->previewScale = scale;

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

	/* Empire-red outline around the selected item in Custom mode. */
	if (self->customMode && self->selectedItem) {
		matrix4 boxT;
		obs_sceneitem_get_box_transform(self->selectedItem, &boxT);
		vec3 c[5];
		vec3_set(&c[0], 0.0f, 0.0f, 0.0f);
		vec3_set(&c[1], 1.0f, 0.0f, 0.0f);
		vec3_set(&c[2], 1.0f, 1.0f, 0.0f);
		vec3_set(&c[3], 0.0f, 1.0f, 0.0f);
		c[4] = c[0];
		for (int i = 0; i < 5; i++)
			vec3_transform(&c[i], &c[i], &boxT);

		gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
		gs_eparam_t *colParam = gs_effect_get_param_by_name(solid, "color");
		struct vec4 col;
		vec4_set(&col, 0.898f, 0.035f, 0.078f, 1.0f);
		gs_effect_set_vec4(colParam, &col);
		while (gs_effect_loop(solid, "Solid")) {
			gs_render_start(false);
			for (int i = 0; i < 5; i++)
				gs_vertex2f(c[i].x, c[i].y);
			gs_render_stop(GS_LINESTRIP);
		}
	}

	gs_projection_pop();
	gs_viewport_pop();
}
