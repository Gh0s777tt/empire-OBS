#include "EmpireVerticalDock.hpp"
#include "OBSQTDisplay.hpp"

#include <utility/display-helpers.hpp>

#include <obs-frontend-api.h>
#include <graphics/graphics.h>
#include <graphics/vec2.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "moc_EmpireVerticalDock.cpp"

#define VERTICAL_W 1080
#define VERTICAL_H 1920

EmpireVerticalDock::EmpireVerticalDock(QWidget *parent) : QFrame(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	/* Top toolbar: caption + Fill/Fit toggle. */
	QHBoxLayout *bar = new QHBoxLayout();
	bar->setContentsMargins(6, 4, 6, 4);
	QLabel *caption = new QLabel(QStringLiteral("Vertical 9:16 — mirrors program"), this);
	caption->setStyleSheet("color:#B3B3B3;");
	bar->addWidget(caption);
	bar->addStretch();
	modeButton = new QPushButton(this);
	modeButton->setStyleSheet("background-color:#E50914; color:white; font-weight:bold; padding:2px 10px;");
	connect(modeButton, &QPushButton::clicked, this, [this]() {
		fillMode = !fillMode;
		ApplyFraming();
		UpdateModeButton();
	});
	bar->addWidget(modeButton);
	layout->addLayout(bar);

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

	UpdateModeButton();
	SyncToCurrentScene();
	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	setObjectName(QStringLiteral("empireVerticalDock"));
}

EmpireVerticalDock::~EmpireVerticalDock()
{
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);

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
